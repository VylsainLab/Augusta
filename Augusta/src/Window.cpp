#include <Augusta/Window.h>
#include <Augusta/Context.h>
#include <Augusta/SwapChain.h>
#include <cassert>
#include <stdexcept>
#include <memory>

namespace aug
{
	SRenderTargetLayout Window::WINDOW_LAYOUT_ATTACHMENT = { VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
	SRenderTargetLayout Window::WINDOW_LAYOUT_PRESENT = { VK_IMAGE_LAYOUT_PRESENT_SRC_KHR };

	Window::Window(const std::string& name, uint16_t width, uint16_t height, bool bResizable, bool bVisible)
	{					
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, bResizable);
		glfwWindowHint(GLFW_VISIBLE, bVisible);

		m_pWindow = glfwCreateWindow(width, height, name.c_str(), nullptr, nullptr);

		//create surface
		if (glfwCreateWindowSurface(aug::Context::m_VkInstance, m_pWindow, nullptr, &m_VkSurface) != VK_SUCCESS)
			throw std::runtime_error("Failed to create window surface!");		
	}

	Window::~Window()
	{
#ifndef USE_DYNAMIC_RENDERING
		for (auto framebuffer : m_vVkSwapChainFramebuffers)
			vkDestroyFramebuffer(aug::Context::m_VkDevice, framebuffer, nullptr);
#endif

		m_pSwapChain.reset();

		vkDestroySurfaceKHR(aug::Context::m_VkInstance, m_VkSurface, nullptr);

		glfwDestroyWindow(m_pWindow);
	}

	void Window::InitAttachments()
	{
		m_pSwapChain = std::make_unique<SwapChain>(m_VkSurface);

		//Create depth resources
		STextureDesc desc;
		desc._strName = "WindowDepthBuffer";
		desc._width = m_pSwapChain->GetExtent().width;
		desc._height = m_pSwapChain->GetExtent().height;
		desc._aspect = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		desc._filtering = VK_FILTER_LINEAR;
		desc._format = VK_FORMAT_D32_SFLOAT_S8_UINT;
		desc._memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;
		desc._usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		desc._tiling = VK_IMAGE_TILING_OPTIMAL;
		desc._samplingMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		desc._layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		m_pDepthStencilTexture = TextureFactory::LoadTextureFromMemory(desc);// std::make_unique<Texture>(desc);
	}

#ifndef USE_DYNAMIC_RENDERING
	void Window::InitFramebuffers(const VkRenderPass& renderPass)
	{
		m_vVkSwapChainFramebuffers.resize(m_pSwapChain->GetImageCount());

		for (uint32_t i = 0; i < m_vVkSwapChainFramebuffers.size(); i++)
		{
			VkImageView attachments[] = { m_pSwapChain->GetImageViewAtIndex(i), m_pDepthStencilTexture->GetImageView() };

			VkFramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderPass;
			framebufferInfo.attachmentCount = 2;
			framebufferInfo.pAttachments = attachments;
			framebufferInfo.width = m_pSwapChain->GetExtent().width;
			framebufferInfo.height = m_pSwapChain->GetExtent().height;
			framebufferInfo.layers = 1;

			if (vkCreateFramebuffer(aug::Context::m_VkDevice, &framebufferInfo, nullptr, &m_vVkSwapChainFramebuffers[i]) != VK_SUCCESS)
				throw std::runtime_error("Failed to create framebuffer!");
		}
	}

    VkFramebuffer Window::GetSwapChainFramebuffer(uint32_t index) const
    {
        return m_vVkSwapChainFramebuffers.at(index);
    }
#endif

	bool Window::IsClosed()
	{
		return glfwWindowShouldClose(m_pWindow);
	}

	uint32_t Window::AcquireNextImage(VkSemaphore semaphore)
	{
		return m_pSwapChain->AcquireNextImage(semaphore);
	}

	uint32_t Window::GetSwapChainImageCount()
	{
		return m_pSwapChain->GetImageCount();
	}

	uint32_t Window::GetSwapChainCurrentImageIndex()
	{
		return m_pSwapChain->GetCurrentImageIndex();
	}

	VkSwapchainKHR Window::GetSwapChainHandle() const
	{
		return m_pSwapChain->GetSwapChainHandle();
	}

	VkExtent2D Window::GetExtent() const
	{
		return m_pSwapChain->GetExtent();
	}

	std::vector<VkFormat> Window::GetColorFormats() const
	{
		return std::vector<VkFormat>({ m_pSwapChain->GetImageFormat() });
	}

	VkFormat Window::GetDepthFormat() const
	{
		return m_pDepthStencilTexture->GetFormat();
	}

	std::vector<VkImageView> Window::GetColorImageViews() const
	{
		return std::vector<VkImageView>({ m_pSwapChain->GetImageViewAtIndex(m_pSwapChain->GetCurrentImageIndex()) });
	}

	VkImageView Window::GetDepthImageView() const
	{
		return m_pDepthStencilTexture->GetImageView();
	}

	void Window::TransitionToLayout(const VkCommandBuffer& cb, SRenderTargetLayout layout)
	{
		//depth transition unnecessary
		m_pSwapChain->TransitionCurrentImageToLayout(cb, layout._colorLayout);		
	}
}