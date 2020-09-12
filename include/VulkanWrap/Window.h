#ifndef VKW_WINDOW_H
#define VKW_WINDOW_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>

namespace vkw
{
	class Window
	{
	public:
		Window(const char* szName, uint16_t uiWidth, uint16_t uiHeight);
		virtual ~Window();

		VkSurfaceKHR GetSurface() { return m_VkSurface; }

		bool IsClosed();

	protected:
		GLFWwindow* m_pWindow = nullptr;
		static bool m_bGLFWInitialized;
		VkSurfaceKHR m_VkSurface;
	};
}
#endif