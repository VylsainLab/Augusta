#ifndef AUG_WINDOW_H
#define AUG_WINDOW_H

#include <Augusta/IRenderTarget.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>
#include <memory>
#include <vector>
#include <Augusta/Texture.h>

namespace aug
{
	class SwapChain;

	class Window : public IRenderTarget
	{		
	public:
		Window(const std::string &name, uint16_t width, uint16_t height, bool bResizable, bool bVisible);
		virtual ~Window();

		void InitAttachments();
#ifndef USE_DYNAMIC_RENDERING
		void InitFramebuffers(const VkRenderPass& renderPass);
        VkFramebuffer GetSwapChainFramebuffer(uint32_t index) const;
#endif	

		VkSurfaceKHR GetSurface() { return m_VkSurface; }
		bool IsClosed();

		uint32_t AcquireNextImage(VkSemaphore semaphore);

		uint32_t GetSwapChainImageCount();
		uint32_t GetSwapChainCurrentImageIndex();
		VkSwapchainKHR GetSwapChainHandle() const;		
		
		GLFWwindow* GetGLFWWindow() const { return m_pWindow; }

		//IRenderTarget		
		VkExtent2D GetExtent() const override;
		std::vector<VkFormat> GetColorFormats() const override;
		VkFormat GetDepthFormat() const override;
		std::vector<VkImageView> GetColorImageViews() const override;
		VkImageView GetDepthImageView() const override;
		void TransitionToLayout(const VkCommandBuffer& cb, SRenderTargetLayout layout) override;

		static SRenderTargetLayout WINDOW_LAYOUT_ATTACHMENT;
		static SRenderTargetLayout WINDOW_LAYOUT_PRESENT;

	protected:
		GLFWwindow* m_pWindow = nullptr;
		static bool m_bGLFWInitialized;
		VkSurfaceKHR m_VkSurface;
		std::unique_ptr<SwapChain> m_pSwapChain = nullptr;
#ifndef USE_DYNAMIC_RENDERING
		std::vector<VkFramebuffer> m_vVkSwapChainFramebuffers;
#endif
		std::shared_ptr<Texture> m_pDepthStencilTexture;
	};
}
#endif