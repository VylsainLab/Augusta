#ifndef AUG_APPLICATION_H
#define AUG_APPLICATION_H

#include <Augusta/Context.h>
#include <Augusta/Window.h>
#include <Augusta/Context.h>
#include <Augusta/Buffer.h>
#include <vector>
#include <memory>

namespace aug
{
	class IGLFWEventObserver
	{
	public:
		virtual void ProcessEvents(GLFWwindow* pWindow) = 0;
	};

	class Application
	{
	public:
		Application(const std::string& name, uint16_t width, uint16_t height);
		virtual ~Application();

		void Run();		
		
		virtual void Render(VkCommandBuffer commandBuffer)=0;		

		void AddEventObserver(IGLFWEventObserver* pObserver);

	protected:
		void ProcessEvents();
		void BeginRender();
		void EndRender();

		void CreateRenderPass();
		void CreateSwapChainCommandBuffers();
		void CreateSyncObjects();

		static bool m_bGLFWInitialized;
		std::unique_ptr<aug::Window> m_pWindow;

		VkRenderPass m_VkRenderPass;
		std::vector<VkCommandBuffer> m_vVkSwapChainCommandBuffers;
		
		std::vector<VkSemaphore> m_vVkImageAvailableSemaphores;
		std::vector<VkSemaphore> m_vVkRenderFinishedSemaphores;
		std::vector<VkFence> m_vVkInFlightFences;
		std::vector<VkFence> m_vVkImagesInFlightFences;
		uint32_t m_uiCurrentFrame = 0;
		uint32_t m_uiCurrentImageIndex = 0;

		std::vector<IGLFWEventObserver*> m_vEventObservers;
	};
}

#endif