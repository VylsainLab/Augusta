#include <VulkanWrap/Context.h>
#include <VulkanWrap/Window.h>
#include <VulkanWrap/Context.h>
#include <vector>
#include <memory>

namespace vkw
{
	class Application
	{
	public:
		Application(const std::string& name, uint16_t width, uint16_t height);
		virtual ~Application();

		void Setup(const VkRenderPass& renderPass, const VkPipeline& graphicsPipeline);

		void Run();

		virtual void Render();

	protected:

		void CreateCommandPool();
		void CreateSwapChainCommandBuffers(const VkRenderPass& renderPass, const VkPipeline& graphicsPipeline);
		void CreateSyncObjects();

		static bool m_bGLFWInitialized;
		std::unique_ptr<vkw::Window> m_pWindow;

		std::vector<VkCommandBuffer> m_vVkSwapChainCommandBuffers;
		VkCommandPool m_VkCommandPool;
		std::vector<VkSemaphore> m_vVkImageAvailableSemaphores;
		std::vector<VkSemaphore> m_vVkRenderFinishedSemaphores;
		std::vector<VkFence> m_vVkInFlightFences;
		std::vector<VkFence> m_vVkImagesInFlightFences;
		uint32_t m_uiCurrentFrame = 0;
	};
}
