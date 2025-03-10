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

		m_pWindow = std::make_unique<aug::Window>(name,width,height);

		aug::Context::Init(m_pWindow->GetSurface());

		m_pWindow->InitAttachments();
		CreateRenderPass();
		m_pWindow->InitFramebuffers(m_VkRenderPass);
		CreateSwapChainCommandBuffers();
		CreateSyncObjects();
	}

	Application::~Application()
	{
		vkDestroyRenderPass(Context::m_VkDevice, m_VkRenderPass, nullptr);

		for (uint8_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
		{
			vkDestroySemaphore(Context::m_VkDevice, m_vVkRenderFinishedSemaphores[i], nullptr);
			vkDestroySemaphore(Context::m_VkDevice, m_vVkImageAvailableSemaphores[i], nullptr);
			vkDestroyFence(Context::m_VkDevice, m_vVkInFlightFences[i], nullptr);
		}

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
		renderPassInfo.renderPass = m_VkRenderPass;
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

	void Application::CreateRenderPass()
	{
		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format = m_pWindow->GetColorFormat();
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRef = {};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription depthAttachment = {};
		depthAttachment.format = m_pWindow->GetDepthStencilFormat();
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRef = {};
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		subpass.pDepthStencilAttachment = &depthAttachmentRef;

		std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;

		// Make sure we reach the color attachment output stage before writing to attachments 
		// since the framebuffer image is not garanteed to be available before that
		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		if (vkCreateRenderPass(aug::Context::m_VkDevice, &renderPassInfo, nullptr, &m_VkRenderPass) != VK_SUCCESS)
			throw std::runtime_error("failed to create render pass!");
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