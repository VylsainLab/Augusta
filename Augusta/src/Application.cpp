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

		m_pWindow = std::make_unique<Window>(name, width, height, bResizable, bVisible);

		Context::Init(m_pWindow->GetSurface());
		DescriptorFactory::Init();
		m_pWindow->InitAttachments();
		InitImGui();

		m_pMainPipeline = std::make_unique<Pipeline>(m_pWindow.get());
		
#ifndef USE_DYNAMIC_RENDERING
		m_pWindow->InitFramebuffers(m_pMainPipeline->GetRenderPass());
#endif
		CreateSwapChainCommandBuffers();
		CreateSyncObjects();
		CreateQueryPool();

		aug::MaterialFactory::Init();

		Debug::RegisterDebugee("Debug", "Materials", std::bind(&MaterialFactory::DrawDebug));
		Debug::RegisterDebugee("Debug", "Textures", std::bind(&TextureFactory::DrawDebug));
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

		m_pMainPipeline.reset();
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
			std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
			static std::chrono::system_clock::time_point last = now;
			int64_t iNbNanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(now - last).count();
			m_fDeltaT = static_cast<float>(iNbNanoseconds) / 1000000000.0f;			
			last = now;

			StartFrameTiming();

			ProcessEvents();

			for (auto& pass : m_vRenderPasses)
				pass._RenderFunc();

			BeginRender();
			MainRenderPass(m_vVkSwapChainCommandBuffers[m_pWindow->GetSwapChainCurrentImageIndex()]);
			RenderImGui();
			EndRender();

			EndFrameTiming();
			GetTimestamps();
		}

		vkDeviceWaitIdle(aug::Context::m_VkDevice);
	}

	void Application::RenderImGui()
	{
		if (m_bDisplayImGuiMenu)
		{
			Debug::DrawDebugees();
			/*if (ImGui::BeginMainMenuBar())
			{
				if (ImGui::BeginMenu("Debug"))
				{
					if (ImGui::MenuItem("Textures"))
						m_bDisplayDebugTextures = true;

					if (ImGui::MenuItem("Materials"))
						m_bDisplayDebugMaterials = true;

					if (ImGui::MenuItem("ImGui demo"))
						m_bDisplayDebugImGui = true;

					ImGui::EndMenu();
				}
				ImGui::EndMainMenuBar();
			}

			if (m_bDisplayDebugTextures)
			{
				ImGui::Begin("Textures",&m_bDisplayDebugTextures);
				TextureFactory::DrawDebug();
				ImGui::End();
			}

			if (m_bDisplayDebugMaterials)
			{
				ImGui::Begin("Materials", &m_bDisplayDebugMaterials);
				MaterialFactory::DrawDebug();
				ImGui::End();
			}*/
			if (m_bDisplayConsole)
				Debug::DrawConsole();

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

		if (glfwGetKey(m_pWindow->GetGLFWWindow(), GLFW_KEY_GRAVE_ACCENT) == GLFW_PRESS)
			m_bDisplayConsole = !m_bDisplayConsole;

		for (auto observer : m_vEventObservers)
			observer->ProcessEvents(m_pWindow->GetGLFWWindow(), m_fDeltaT);
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
     
		WriteTimestamp(m_vVkSwapChainCommandBuffers[currentImage], VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);

		if (m_pMainPipeline)
		{
#ifdef USE_DYNAMIC_RENDERING   
			m_pMainPipeline->BeginRendering(m_vVkSwapChainCommandBuffers[currentImage],m_pWindow.get(),Window::WINDOW_LAYOUT_ATTACHMENT);
			m_pMainPipeline->Bind(m_vVkSwapChainCommandBuffers[currentImage]);
#else
			m_pMainPipeline->Bind(m_vVkSwapChainCommandBuffers[currentImage], m_pWindow->GetSwapChainFramebuffer(currentImage), m_pWindow->GetSwapChainExtent(), 1, m_uiCurrentFrame);
#endif
		}	
	}

	void Application::EndRender()
	{
		ImGui::Render();

		uint32_t uiCurrentImage = m_pWindow->GetSwapChainCurrentImageIndex();

		ImDrawData* main_draw_data = ImGui::GetDrawData();
		ImGui_ImplVulkan_RenderDrawData(main_draw_data, m_vVkSwapChainCommandBuffers[uiCurrentImage]);		

		if (m_pMainPipeline)
			m_pMainPipeline->EndRendering(m_vVkSwapChainCommandBuffers[uiCurrentImage],m_pWindow.get(),Window::WINDOW_LAYOUT_PRESENT);

		WriteTimestamp(m_vVkSwapChainCommandBuffers[uiCurrentImage], VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
		
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

	void Application::CreateQueryPool()
	{
		VkQueryPoolCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
		createInfo.pNext = nullptr; // Optional
		createInfo.flags = 0; // Reserved for future use, must be 0!

		createInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
		createInfo.queryCount = NB_QUERIES;

		VkResult result = vkCreateQueryPool(Context::m_VkDevice, &createInfo, nullptr, &m_TimingQueryPool);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create time query pool!");
		}

		vkResetQueryPool(Context::m_VkDevice, m_TimingQueryPool, 0, NB_QUERIES);
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
		std::vector<VkFormat> vColorFormats = m_pWindow->GetColorFormats();
		renderingInfo.colorAttachmentCount = static_cast<uint32_t>(vColorFormats.size());
		renderingInfo.pColorAttachmentFormats = vColorFormats.data();
		renderingInfo.depthAttachmentFormat = m_pWindow->GetDepthFormat();

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

	void Application::WriteTimestamp(const VkCommandBuffer& cb, VkPipelineStageFlagBits stage)
	{
		vkResetQueryPool(Context::m_VkDevice, m_TimingQueryPool, m_uiCurrentQuery, 1);
		vkCmdWriteTimestamp(cb, stage, m_TimingQueryPool, m_uiCurrentQuery);
		m_uiCurrentQuery = (m_uiCurrentQuery + 1) % NB_QUERIES;
	}

	void Application::GetTimestamps()
	{
		static uint32_t prevStartFrameQuery=0, prevEndFrameQuery=0;
		if (prevStartFrameQuery != prevEndFrameQuery)
		{
			uint32_t queryCount = prevEndFrameQuery - prevStartFrameQuery;
			std::vector<uint64_t> vQueries;
			vQueries.resize(queryCount);
			VkResult result = vkGetQueryPoolResults(Context::m_VkDevice, m_TimingQueryPool, prevStartFrameQuery, queryCount, queryCount * sizeof(uint64_t), vQueries.data(), sizeof(uint64_t), VK_QUERY_RESULT_64_BIT);

			if (result == VK_SUCCESS)
			{
				for (int i = 1; i < vQueries.size(); ++i)
				{
					uint64_t nbNano = vQueries[i] - vQueries[i - 1];
					//printf("\nDelta %d-%d: %f ms", i - 1, i, static_cast<float>(nbNano) / 1000000.0f);
				}
				uint64_t nbNano = vQueries.back() - vQueries[0];
				//printf("\nFrame : %f ms", static_cast<float>(nbNano) / 1000000.0f);
			}
		}
		prevStartFrameQuery = m_uiStartFrameQuery;
		prevEndFrameQuery = m_uiEndFrameQuery;
	}
}