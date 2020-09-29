#ifndef VKW_WINDOW_H
#define VKW_WINDOW_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>
#include <memory>
#include <vector>
//#include <VulkanWrap/SwapChain.h>

namespace vkw
{
	class SwapChain;

	class Window
	{		
	public:
		Window(const std::string &name, uint16_t width, uint16_t height);
		virtual ~Window();

		void InitSwapChain();
		void InitFramebuffers(const VkRenderPass& renderPass);

		VkSurfaceKHR GetSurface() { return m_VkSurface; }
		bool IsClosed();

		uint32_t GetSwapChainImageCount();
		VkSwapchainKHR GetSwapChainHandle() const;
		VkFormat GetSwapChainImageFormat() const;
		VkExtent2D GetSwapChainExtent() const;
		VkFramebuffer GetSwapChainFramebuffer(uint32_t index) const;
		GLFWwindow* GetGLFWWindow() const { return m_pWindow; }

	protected:
		GLFWwindow* m_pWindow = nullptr;
		static bool m_bGLFWInitialized;
		VkSurfaceKHR m_VkSurface;
		std::unique_ptr<SwapChain> m_pSwapChain = nullptr;
		std::vector<VkFramebuffer> m_vVkSwapChainFramebuffers;
	};
}
#endif