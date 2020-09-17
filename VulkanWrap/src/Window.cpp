#include <VulkanWrap/Window.h>
#include <VulkanWrap/Context.h>
#include <cassert>
#include <stdexcept>

namespace vkw
{
	Window::Window(SWindowDesc desc)
	{					
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		m_pWindow = glfwCreateWindow(desc.uiWidth, desc.uiHeight, desc.strName.c_str(), nullptr, nullptr);

		//create surface
		if (glfwCreateWindowSurface(vkw::Context::m_VkInstance, m_pWindow, nullptr, &m_VkSurface) != VK_SUCCESS)
			throw std::runtime_error("Failed to create window surface!");
	}

	Window::~Window()
	{
		vkDestroySurfaceKHR(vkw::Context::m_VkInstance, m_VkSurface, nullptr);

		glfwDestroyWindow(m_pWindow);
	}

	bool Window::IsClosed()
	{
		return glfwWindowShouldClose(m_pWindow);
	}
}