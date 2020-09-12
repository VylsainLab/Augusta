#include <VulkanWrap/Application.h>
#include <cassert>
#include <string>
#include <set>
#include <stdexcept>
#include <iostream>

namespace vkw
{
	bool Application::m_bGLFWInitialized = false;

	Application::Application()
	{
		if (m_bGLFWInitialized == false)
		{
			bool ret = glfwInit();
			assert(ret = GLFW_TRUE);
		}

		vkw::Context::Init();
	}

	Application::~Application()
	{
		vkw::Context::Release();

		if (m_bGLFWInitialized == true)
		{
			glfwTerminate();
		}
	}

	vkw::Window* Application::AddNewWindow(const char* szName, uint16_t uiWidth, uint16_t uiHeight)
	{
		std::shared_ptr<vkw::Window> pWindow(new vkw::Window(szName, uiWidth, uiHeight));
		m_vWindows.push_back(pWindow);
		return pWindow.get();
	}

	void Application::Run()
	{
		while (!m_vWindows.empty())
		{
			UpdateWindowsList();
			glfwPollEvents();
			Render();
		}
	}

	void Application::UpdateWindowsList()
	{
		for (auto it = m_vWindows.begin(); it != m_vWindows.end(); )
		{
			if ((*it)->IsClosed())
				m_vWindows.erase(it);
			else
				it++;
		}
	}
}