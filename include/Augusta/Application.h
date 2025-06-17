#ifndef AUG_APPLICATION_H
#define AUG_APPLICATION_H

#include <Augusta/Context.h>
#include <Augusta/Window.h>
#include <Augusta/Pipeline.h>
#include <Augusta/Context.h>
#include <Augusta/Buffer.h>
#include <Augusta/Scene.h>
#include <vector>
#include <memory>
#include <imgui-docking/misc/freetype/imgui_freetype.h>
#include <imgui-docking/imgui.h>

namespace aug
{
	class IGLFWEventObserver
	{
	public:
		virtual void ProcessEvents(GLFWwindow* pWindow) = 0;
	};

	class Application : public ISceneRenderer
	{
	public:
		Application(const std::string& name, uint16_t width, uint16_t height, bool bResizable=true, bool bVisible=true);
		virtual ~Application();

		void Run();		
		
		virtual void Render(VkCommandBuffer commandBuffer)=0;		

		void AddEventObserver(IGLFWEventObserver* pObserver);

	protected:
		void ProcessEvents();
		void BeginRender();
		void EndRender();

		void CreateSwapChainCommandBuffers();
		void CreateSyncObjects();
		void InitImGui();

		static bool m_bGLFWInitialized;
		std::unique_ptr<Window> m_pWindow;
		std::unique_ptr<Pipeline> m_pPipeline;
		std::vector<VkCommandBuffer> m_vVkSwapChainCommandBuffers;
		
		std::vector<VkSemaphore> m_vVkImageAvailableSemaphores;
		std::vector<VkSemaphore> m_vVkRenderFinishedSemaphores;
		std::vector<VkFence> m_vVkInFlightFences;
		std::vector<VkFence> m_vVkImagesInFlightFences;
		uint32_t m_uiCurrentFrame = 0;

		std::vector<IGLFWEventObserver*> m_vEventObservers;
	};
}

#endif