#include <Augusta/Application.h>
#include <Augusta/SwapChain.h>
#include <cassert>
#include <string>
#include <set>
#include <array>
#include <stdexcept>
#include <iostream>
#include <imgui-docking/backends/imgui_impl_vulkan.h>
#include <imgui-docking/backends/imgui_impl_glfw.h>

namespace aug
{
	bool Application::m_bGLFWInitialized = false;

	Application::Application(const std::string& name, uint16_t width, uint16_t height, bool bResizable, bool bVisible)
	{
		if (m_bGLFWInitialized == false)
		{
			bool ret = glfwInit();
			assert(ret == GLFW_TRUE);
		}

		Context::InitInstance();

		m_pWindow = std::make_unique<Window>(name,width,height, bResizable, bVisible);

		Context::Init(m_pWindow->GetSurface());
		DescriptorFactory::Init();		
		m_pWindow->InitAttachments();
		InitImGui();

		m_pPipeline = std::make_unique<Pipeline>(m_pWindow.get());
		
#ifndef USE_DYNAMIC_RENDERING
		m_pWindow->InitFramebuffers(m_pPipeline->GetRenderPass());
#endif
		CreateSwapChainCommandBuffers();
		CreateSyncObjects();
	}

	Application::~Application()
	{
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		for (uint8_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
		{
			vkDestroySemaphore(Context::m_VkDevice, m_vVkRenderFinishedSemaphores[i], nullptr);
			vkDestroySemaphore(Context::m_VkDevice, m_vVkImageAvailableSemaphores[i], nullptr);
			vkDestroyFence(Context::m_VkDevice, m_vVkInFlightFences[i], nullptr);
		}

		DescriptorFactory::Release();

		m_pPipeline.reset();
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
			Render(m_vVkSwapChainCommandBuffers[m_pWindow->GetSwapChainCurrentImageIndex()]);
			RenderImGui();
			EndRender();
		}

		vkDeviceWaitIdle(aug::Context::m_VkDevice);
	}

	void Application::RenderImGui()
	{
		if (m_bDisplayImGuiMenu)
		{
			if (ImGui::BeginMainMenuBar())
			{
				if (ImGui::BeginMenu("Debug"))
				{
					if (ImGui::MenuItem("Textures"))
						m_bDisplayDebugTextures = true;

					if (ImGui::MenuItem("ImGui demo"))
						m_bDisplayDebugImGui = true;

					ImGui::EndMenu();
				}
				ImGui::EndMainMenuBar();
			}

			if (m_bDisplayDebugTextures)
			{
				ImGui::Begin("Textures",&m_bDisplayDebugTextures);
				TextureFactory::ImGuiDrawTextureDebug();
				ImGui::End();
			}

			if(m_bDisplayDebugImGui)
				ImGui::ShowDemoWindow(&m_bDisplayDebugImGui);
		}
	}

	void Application::AddEventObserver(IGLFWEventObserver* pObserver)
	{
		m_vEventObservers.push_back(pObserver);
	}

	void Application::ProcessEvents()
	{
		glfwPollEvents();

		//TODO: Move to input management class
		if (glfwGetKey(m_pWindow->GetGLFWWindow(), GLFW_KEY_ESCAPE) == GLFW_PRESS)
			glfwSetWindowShouldClose(m_pWindow->GetGLFWWindow(), GLFW_TRUE);

		if (glfwGetKey(m_pWindow->GetGLFWWindow(), GLFW_KEY_F1) == GLFW_PRESS)
			m_bDisplayImGuiMenu = !m_bDisplayImGuiMenu;

		for (auto observer : m_vEventObservers)
			observer->ProcessEvents(m_pWindow->GetGLFWWindow());
	}

	void Application::BeginRender()
	{
		// Start the Dear ImGui frame
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		vkWaitForFences(aug::Context::m_VkDevice, 1, &m_vVkInFlightFences[m_uiCurrentFrame], VK_TRUE, UINT64_MAX);

		uint32_t currentImage = m_pWindow->AcquireNextImage(m_vVkImageAvailableSemaphores[m_uiCurrentFrame]);

		// Check if a previous frame is using this image (i.e. there is its fence to wait on)
		if (m_vVkImagesInFlightFences[currentImage] != VK_NULL_HANDLE)
			vkWaitForFences(aug::Context::m_VkDevice, 1, &m_vVkImagesInFlightFences[currentImage], VK_TRUE, UINT64_MAX);

		// Mark the image as now being in use by this frame
		m_vVkImagesInFlightFences[currentImage] = m_vVkInFlightFences[m_uiCurrentFrame];

		vkResetCommandBuffer(m_vVkSwapChainCommandBuffers[m_uiCurrentFrame], VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);		

		//Begin command buffer recording
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		beginInfo.pInheritanceInfo = nullptr;

		if (vkBeginCommandBuffer(m_vVkSwapChainCommandBuffers[currentImage], &beginInfo) != VK_SUCCESS)
			throw std::runtime_error("Failed to begin recording command buffer!");

		//Swapchain image transition
		m_pWindow->TransitionCurrentSwapChainImageToLayout(m_vVkSwapChainCommandBuffers[currentImage], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

#ifdef USE_DYNAMIC_RENDERING
		//Dynamic rendering
		VkClearValue clearColors{};
		clearColors.color = { 0.0f, 0.0f, 0.0f, 0.0f };
		

		VkRenderingAttachmentInfo colorAttachmentInfo{};
		colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		colorAttachmentInfo.imageView = m_pWindow->GetCurrentColorImageView();
		colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
		colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachmentInfo.clearValue = clearColors;

		clearColors.depthStencil = { 1.0f, 0 };

		VkRenderingAttachmentInfo depthAttachmentInfo{};
		depthAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		depthAttachmentInfo.imageView = m_pWindow->GetDepthImageView();
		depthAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
		depthAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachmentInfo.clearValue = clearColors;

		VkRenderingInfo renderingInfo{};
		renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		VkRect2D renderArea{};
		renderArea.extent = m_pWindow->GetSwapChainExtent();
		renderingInfo.renderArea = renderArea;
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = 1;
		renderingInfo.pColorAttachments = &colorAttachmentInfo;
		renderingInfo.pDepthAttachment = &depthAttachmentInfo;

		vkCmdBeginRendering(m_vVkSwapChainCommandBuffers[currentImage], &renderingInfo);
        
        if (m_pPipeline)
            m_pPipeline->Bind(m_vVkSwapChainCommandBuffers[currentImage], 1, m_uiCurrentFrame);
#else
        if (m_pPipeline)
            m_pPipeline->Bind(m_vVkSwapChainCommandBuffers[currentImage], m_pWindow->GetSwapChainFramebuffer(currentImage), m_pWindow->GetSwapChainExtent(), 1, m_uiCurrentFrame);
#endif

		
	}

	void Application::EndRender()
	{
		ImGui::Render();

		uint32_t uiCurrentImage = m_pWindow->GetSwapChainCurrentImageIndex();

		ImDrawData* main_draw_data = ImGui::GetDrawData();
		ImGui_ImplVulkan_RenderDrawData(main_draw_data, m_vVkSwapChainCommandBuffers[uiCurrentImage]);		

#ifdef USE_DYNAMIC_RENDERING
		vkCmdEndRendering(m_vVkSwapChainCommandBuffers[uiCurrentImage]);
#else
		vkCmdEndRenderPass(m_vVkSwapChainCommandBuffers[uiCurrentImage]);
#endif

		//Transition swap chain images for presentation
		m_pWindow->TransitionCurrentSwapChainImageToLayout(m_vVkSwapChainCommandBuffers[uiCurrentImage], VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

		if (vkEndCommandBuffer(m_vVkSwapChainCommandBuffers[uiCurrentImage]) != VK_SUCCESS)
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
		submitInfo.pCommandBuffers = &m_vVkSwapChainCommandBuffers[uiCurrentImage];
		VkSemaphore signalSemaphores[] = { m_vVkRenderFinishedSemaphores[m_uiCurrentFrame] };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		vkResetFences(aug::Context::m_VkDevice, 1, &m_vVkInFlightFences[m_uiCurrentFrame]);

		if (vkQueueSubmit(aug::Context::m_VkGraphicsQueue, 1, &submitInfo, m_vVkInFlightFences[m_uiCurrentFrame]) != VK_SUCCESS)
			throw std::runtime_error("Failed to submit draw command buffer!");

		ImGuiIO& io = ImGui::GetIO();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}

		//Present to window		
		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;
		VkSwapchainKHR swapChains[] = { m_pWindow->GetSwapChainHandle() };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &uiCurrentImage;
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

	void Application::InitImGui()
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); 
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows

		ImGui::StyleColorsDark();

		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		ImGui_ImplGlfw_InitForVulkan(m_pWindow->GetGLFWWindow(), true);

		VkPipelineRenderingCreateInfo renderingInfo = {};
		renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
		renderingInfo.colorAttachmentCount = 1;
		VkFormat colorFormat = m_pWindow->GetColorFormat();
		renderingInfo.pColorAttachmentFormats = &colorFormat;
		renderingInfo.depthAttachmentFormat = m_pWindow->GetDepthStencilFormat();

		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = aug::Context::m_VkInstance;
		init_info.PhysicalDevice = aug::Context::m_VkPhysicalDevice;
		init_info.Device = aug::Context::m_VkDevice;
		init_info.QueueFamily = aug::Context::m_QueueFamilies.uiGraphicsFamily.value();
		init_info.Queue = aug::Context::m_VkGraphicsQueue;
		init_info.DescriptorPoolSize = IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE + 1000;
#ifdef USE_DYNAMIC_RENDERING
		init_info.UseDynamicRendering = true;
		init_info.PipelineRenderingCreateInfo = renderingInfo;
#else
		init_info.RenderPass = m_pPipeline->GetRenderPass();
#endif
		init_info.Subpass = 0;
		init_info.MinImageCount = 3;
		init_info.ImageCount = 3;
		init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		ImGui_ImplVulkan_Init(&init_info);

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		beginInfo.pInheritanceInfo = nullptr;
	}
}