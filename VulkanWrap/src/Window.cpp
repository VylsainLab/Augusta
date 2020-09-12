#include <VulkanWrap/Window.h>
#include <cassert>
#include <stdexcept>

namespace vkw
{
	Window::Window(VkInstance &instance, const char* szName, uint16_t uiWidth, uint16_t uiHeight)
	{					
		m_pVkInstance = &instance;

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		m_pWindow = glfwCreateWindow(uiWidth, uiHeight, szName, nullptr, nullptr);

		//create surface
		if (glfwCreateWindowSurface(instance, m_pWindow, nullptr, &m_VkSurface) != VK_SUCCESS)
			throw std::runtime_error("Failed to create window surface!");
	}

	Window::~Window()
	{
		vkDestroySurfaceKHR(*m_pVkInstance, m_VkSurface, nullptr);

		glfwDestroyWindow(m_pWindow);
	}

	bool Window::IsClosed()
	{
		return glfwWindowShouldClose(m_pWindow);
	}
}