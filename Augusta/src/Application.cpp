#include <Augusta/Application.h>
#include <Augusta/SwapChain.h>
#include <cassert>
#include <string>
#include <set>
#include <array>
#include <stdexcept>
#include <iostream>

#define MAX_FRAMES_IN_FLIGHT	2

namespace aug
{
	bool Application::m_bGLFWInitialized = false;

	Application::Application(const std::string& name, uint16_t width, uint16_t height)
	{
		if (m_bGLFWInitialized == false)
		{
			bool ret = glfwInit();
			assert(ret == GLFW_TRUE);
		}

		aug::Context::InitInstance();

		m_pWindow = std::make_unique<Window>(name,width,height);

		aug::Context::Init(m_pWindow->GetSurface());
		m_pWindow->InitAttachments();

		m_pGraphicsPipeline = std::make_unique<GraphicsPipeline>(m_pWindow.get());
		
		m_pWindow->InitFramebuffers(m_pGraphicsPipeline->GetRenderPass()); //TODO move to pipeline
		CreateSwapChainCommandBuffers();
		CreateSyncObjects();
	}

	Application::~Application()
	{
		for (uint8_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
		{
			vkDestroySemaphore(Context::m_VkDevice, m_vVkRenderFinishedSemaphores[i], nullptr);
			vkDestroySemaphore(Context::m_VkDevice, m_vVkImageAvailableSemaphores[i], nullptr);
			vkDestroyFence(Context::m_VkDevice, m_vVkInFlightFences[i], nullptr);
		}

		m_pGraphicsPipeline.reset();
		m_pWindow.reset();

		Context::Release();

		if (m_bGLFWInitialized == true)
		{
			glfwTerminate();
		}
	}

	void Application::Run()
	{
		while (!m_pWindow->IsClosed())
		{
			ProcessEvents();
			BeginRender();
			Render(m_vVkSwapChainCommandBuffers[m_uiCurrentImageIndex]);
			EndRender();
		}

		vkDeviceWaitIdle(aug::Context::m_VkDevice);
	}

	void Application::AddEventObserver(IGLFWEventObserver* pObserver)
	{
		m_vEventObservers.push_back(pObserver);
	}

	void Application::ProcessEvents()
	{
		glfwPollEvents();

		if (glfwGetKey(m_pWindow->GetGLFWWindow(), GLFW_KEY_ESCAPE) == GLFW_PRESS)
			glfwSetWindowShouldClose(m_pWindow->GetGLFWWindow(), GLFW_TRUE);

		for (auto observer : m_vEventObservers)
			observer->ProcessEvents(m_pWindow->GetGLFWWindow());
	}

	void Application::BeginRender()
	{
		vkWaitForFences(aug::Context::m_VkDevice, 1, &m_vVkInFlightFences[m_uiCurrentFrame], VK_TRUE, UINT64_MAX);

		vkAcquireNextImageKHR(aug::Context::m_VkDevice, m_pWindow->GetSwapChainHandle(), UINT64_MAX, m_vVkImageAvailableSemaphores[m_uiCurrentFrame], VK_NULL_HANDLE, &m_uiCurrentImageIndex);

		// Check if a previous frame is using this image (i.e. there is its fence to wait on)
		if (m_vVkImagesInFlightFences[m_uiCurrentImageIndex] != VK_NULL_HANDLE)
			vkWaitForFences(aug::Context::m_VkDevice, 1, &m_vVkImagesInFlightFences[m_uiCurrentImageIndex], VK_TRUE, UINT64_MAX);

		// Mark the image as now being in use by this frame
		m_vVkImagesInFlightFences[m_uiCurrentImageIndex] = m_vVkInFlightFences[m_uiCurrentFrame];

		vkResetCommandBuffer(m_vVkSwapChainCommandBuffers[m_uiCurrentFrame], VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);

		//Begin command buffer recording
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		beginInfo.pInheritanceInfo = nullptr;

		if (vkBeginCommandBuffer(m_vVkSwapChainCommandBuffers[m_uiCurrentImageIndex], &beginInfo) != VK_SUCCESS)
			throw std::runtime_error("Failed to begin recording command buffer!");

		//Render pass
		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_pGraphicsPipeline->GetRenderPass();
		renderPassInfo.framebuffer = m_pWindow->GetSwapChainFramebuffer(m_uiCurrentImageIndex);
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = m_pWindow->GetSwapChainExtent();
		VkClearValue clearColors[] = { { 0.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 0} };
		renderPassInfo.clearValueCount = 2;
		renderPassInfo.pClearValues = clearColors;
		vkCmdBeginRenderPass(m_vVkSwapChainCommandBuffers[m_uiCurrentImageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	}

	void Application::EndRender()
	{
		vkCmdEndRenderPass(m_vVkSwapChainCommandBuffers[m_uiCurrentImageIndex]);

		if (vkEndCommandBuffer(m_vVkSwapChainCommandBuffers[m_uiCurrentImageIndex]) != VK_SUCCESS)
			throw std::runtime_error("Failed to record command buffer!");

		//Submit command buffer to graphics queue
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		VkSemaphore waitSemaphores[] = { m_vVkImageAvailableSemaphores[m_uiCurrentFrame] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_vVkSwapChainCommandBuffers[m_uiCurrentImageIndex];
		VkSemaphore signalSemaphores[] = { m_vVkRenderFinishedSemaphores[m_uiCurrentFrame] };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		vkResetFences(aug::Context::m_VkDevice, 1, &m_vVkInFlightFences[m_uiCurrentFrame]);

		if (vkQueueSubmit(aug::Context::m_VkGraphicsQueue, 1, &submitInfo, m_vVkInFlightFences[m_uiCurrentFrame]) != VK_SUCCESS)
			throw std::runtime_error("Failed to submit draw command buffer!");

		//Present to window
		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;
		VkSwapchainKHR swapChains[] = { m_pWindow->GetSwapChainHandle() };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &m_uiCurrentImageIndex;
		presentInfo.pResults = nullptr;
		vkQueuePresentKHR(aug::Context::m_VkPresentQueue, &presentInfo);

		m_uiCurrentFrame = (m_uiCurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	void Application::CreateSwapChainCommandBuffers()
	{
		m_vVkSwapChainCommandBuffers.resize(m_pWindow->GetSwapChainImageCount());

		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = Context::m_VkCommandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = (uint32_t)m_vVkSwapChainCommandBuffers.size();

		if (vkAllocateCommandBuffers(aug::Context::m_VkDevice, &allocInfo, m_vVkSwapChainCommandBuffers.data()) != VK_SUCCESS)
			throw std::runtime_error("Failed to allocate command buffers!");
	}

	void Application::CreateSyncObjects()
	{
		m_vVkImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		m_vVkRenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		m_vVkInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
		m_vVkImagesInFlightFences.resize(m_pWindow->GetSwapChainImageCount(), VK_NULL_HANDLE);

		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo = {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (uint8_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
		{
			if (vkCreateSemaphore(aug::Context::m_VkDevice, &semaphoreInfo, nullptr, &m_vVkImageAvailableSemaphores[i]) != VK_SUCCESS ||
				vkCreateSemaphore(aug::Context::m_VkDevice, &semaphoreInfo, nullptr, &m_vVkRenderFinishedSemaphores[i]) != VK_SUCCESS ||
				vkCreateFence(aug::Context::m_VkDevice, &fenceInfo, nullptr, &m_vVkInFlightFences[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to create sync objects for a frame!");
			}
		}
	}
}