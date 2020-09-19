#include <VulkanWrap/Window.h>
#include <VulkanWrap/Context.h>
#include <VulkanWrap/SwapChain.h>
#include <cassert>
#include <stdexcept>
#include <memory>

namespace vkw
{
	Window::Window(const std::string& name, uint16_t width, uint16_t height)
	{					
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		m_pWindow = glfwCreateWindow(width, height, name.c_str(), nullptr, nullptr);

		//create surface
		if (glfwCreateWindowSurface(vkw::Context::m_VkInstance, m_pWindow, nullptr, &m_VkSurface) != VK_SUCCESS)
			throw std::runtime_error("Failed to create window surface!");		
	}

	Window::~Window()
	{
		for (auto framebuffer : m_vVkSwapChainFramebuffers)
			vkDestroyFramebuffer(vkw::Context::m_VkDevice, framebuffer, nullptr);

		m_pSwapChain.reset();

		vkDestroySurfaceKHR(vkw::Context::m_VkInstance, m_VkSurface, nullptr);

		glfwDestroyWindow(m_pWindow);
	}

	void Window::InitSwapChain()
	{
		m_pSwapChain = std::make_unique<SwapChain>(m_VkSurface);
	}

	void Window::InitFramebuffers(const VkRenderPass& renderPass)
	{
		m_vVkSwapChainFramebuffers.resize(m_pSwapChain->GetImageCount());

		for (uint32_t i = 0; i < m_vVkSwapChainFramebuffers.size(); i++)
		{
			VkImageView attachments[] = { m_pSwapChain->GetImageViewAtIndex(i) };

			VkFramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderPass;
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments = attachments;
			framebufferInfo.width = m_pSwapChain->GetExtent().width;
			framebufferInfo.height = m_pSwapChain->GetExtent().height;
			framebufferInfo.layers = 1;

			if (vkCreateFramebuffer(vkw::Context::m_VkDevice, &framebufferInfo, nullptr, &m_vVkSwapChainFramebuffers[i]) != VK_SUCCESS)
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

	VkFormat Window::GetSwapChainImageFormat() const
	{
		return m_pSwapChain->GetImageFormat();
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