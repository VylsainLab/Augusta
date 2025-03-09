#include <Augusta/Window.h>
#include <Augusta/Context.h>
#include <Augusta/SwapChain.h>
#include <cassert>
#include <stdexcept>
#include <memory>

namespace aug
{
	Window::Window(const std::string& name, uint16_t width, uint16_t height)
	{					
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		m_pWindow = glfwCreateWindow(width, height, name.c_str(), nullptr, nullptr);

		//create surface
		if (glfwCreateWindowSurface(aug::Context::m_VkInstance, m_pWindow, nullptr, &m_VkSurface) != VK_SUCCESS)
			throw std::runtime_error("Failed to create window surface!");		
	}

	Window::~Window()
	{
		for (auto framebuffer : m_vVkSwapChainFramebuffers)
			vkDestroyFramebuffer(aug::Context::m_VkDevice, framebuffer, nullptr);

		m_pSwapChain.reset();

		vkDestroySurfaceKHR(aug::Context::m_VkInstance, m_VkSurface, nullptr);

		glfwDestroyWindow(m_pWindow);
	}

	void Window::InitAttachments()
	{
		m_pSwapChain = std::make_unique<SwapChain>(m_VkSurface);

		//Create depth resources
		STextureDesc desc;
		desc.width = m_pSwapChain->GetExtent().width;
		desc.height = m_pSwapChain->GetExtent().height;
		desc.aspect = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		desc.filtering = VK_FILTER_LINEAR;
		desc.format = VK_FORMAT_D32_SFLOAT_S8_UINT;
		desc.memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;
		desc.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		desc.tiling = VK_IMAGE_TILING_OPTIMAL;
		desc.samplingMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		desc.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		m_pDepthStencilTexture = std::make_unique<Texture>(desc);
	}

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

	bool Window::IsClosed()
	{
		return glfwWindowShouldClose(m_pWindow);
	}

	uint32_t Window::GetSwapChainImageCount()
	{
		return m_pSwapChain->GetImageCount();
	}

	VkSwapchainKHR Window::GetSwapChainHandle() const
	{
		return m_pSwapChain->GetSwapChainHandle();
	}

	VkFormat Window::GetColorFormat() const
	{
		return m_pSwapChain->GetImageFormat();
	}

	VkFormat Window::GetDepthStencilFormat() const
	{
		return m_pDepthStencilTexture->GetFormat();
	}

	VkExtent2D Window::GetSwapChainExtent() const
	{
		return m_pSwapChain->GetExtent();
	}

	VkFramebuffer Window::GetSwapChainFramebuffer(uint32_t index) const
	{
		return m_vVkSwapChainFramebuffers.at(index);
	}
}