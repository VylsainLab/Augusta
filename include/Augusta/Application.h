#ifndef AUG_APPLICATION_H
#define AUG_APPLICATION_H

#include <Augusta/Context.h>
#include <Augusta/Window.h>
#include <Augusta/Pipeline.h>
#include <Augusta/Context.h>
#include <Augusta/Buffer.h>
#include <Augusta/Scene.h>
#include <Augusta/FrameBuffer.h>
#include <vector>
#include <memory>
#include <imgui-docking/misc/freetype/imgui_freetype.h>
#include <imgui-docking/imgui.h>
#include <functional>

#define NB_QUERIES 100

namespace aug
{
	class IGLFWEventObserver
	{
	public:
		virtual void ProcessEvents(GLFWwindow* pWindow, float fDeltaT) = 0;
	};

	struct SRenderPass
	{
		std::function<void(void)> _RenderFunc;
	};

	class Application : public ISceneRenderer
	{
	public:
		Application(const std::string& name, uint16_t width, uint16_t height, bool bResizable=true, bool bVisible=true);
		virtual ~Application();

		void Run();		
		
		virtual void MainRenderPass(const VkCommandBuffer& commandBuffer)=0;		
		void RenderImGui();

		void AddEventObserver(IGLFWEventObserver* pObserver);

		void AddRenderPass(SRenderPass& pass){ m_vRenderPasses.push_back(pass);	}

		void WriteTimestamp(const VkCommandBuffer& cb, VkPipelineStageFlagBits stage);

	protected:
		void ProcessEvents();
		void BeginRender();
		void EndRender();

		void CreateSwapChainCommandBuffers();
		void CreateSyncObjects();
		void CreateQueryPool();
		void InitImGui();

		void StartFrameTiming() { m_uiStartFrameQuery = m_uiCurrentQuery; }
		void EndFrameTiming() { m_uiEndFrameQuery = m_uiCurrentQuery == 0 ? NB_QUERIES : m_uiCurrentQuery; }
		
		void GetTimestamps();

		static bool m_bGLFWInitialized;
		std::unique_ptr<Window> m_pWindow;
		std::unique_ptr<Pipeline> m_pMainPipeline;
		std::vector<VkCommandBuffer> m_vVkSwapChainCommandBuffers;
		
		
		std::vector<VkSemaphore> m_vVkImageAvailableSemaphores;
		std::vector<VkSemaphore> m_vVkRenderFinishedSemaphores;
		std::vector<VkFence> m_vVkInFlightFences;
		std::vector<VkFence> m_vVkImagesInFlightFences;
		uint32_t m_uiCurrentFrame = 0;

		VkQueryPool m_TimingQueryPool;
		uint32_t m_uiCurrentQuery = 0;
		uint32_t m_uiStartFrameQuery = 0;
		uint32_t m_uiEndFrameQuery = 0;

		std::vector<IGLFWEventObserver*> m_vEventObservers;

		bool m_bDisplayImGuiMenu = true;
		bool m_bDisplayDebugTextures = false;
		bool m_bDisplayDebugMaterials = false;
		bool m_bDisplayDebugImGui = false;

		std::vector<SRenderPass> m_vRenderPasses;

		float m_fDeltaT = 0;
	};
}

#endif