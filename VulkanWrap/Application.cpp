#include <VulkanWrap/Application.h>
#include <cassert>
#include <string>
#include <set>
#include <stdexcept>
#include <iostream>

namespace vkw
{
	bool Application::m_bGLFWInitialized = false;

	Application::Application(std::vector<SWindowDesc> vWindowList)
	{
		if (m_bGLFWInitialized == false)
		{
			bool ret = glfwInit();
			assert(ret = GLFW_TRUE);
		}

		vkw::Context::InitInstance();
		if (vWindowList.empty())
			throw std::runtime_error("Application needs at least one window.");
		for (auto desc : vWindowList)
		{
			std::shared_ptr<vkw::Window> pWindow(new vkw::Window(desc));
			m_vWindows.push_back(pWindow);
		}
		vkw::Context::Init(m_vWindows.at(0)->GetSurface());
	}

	Application::~Application()
	{
		vkw::Context::Release();

		if (m_bGLFWInitialized == true)
		{
			glfwTerminate();
		}
	}

	void Application::Run()
	{
		while (!m_vWindows.empty())
		{
			UpdateWindowsList();
			glfwPollEvents();
			Render();
		}

		vkDeviceWaitIdle(vkw::Context::m_VkDevice);
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