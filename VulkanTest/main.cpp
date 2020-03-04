#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstdlib>

class HelloTriangleApplication 
{
public:
	void Run() 
	{
		InitWindow();
		InitVulkan();
		MainLoop();
		Cleanup();
	}

private:
	const int WIDTH = 800;
	const int HEIGHT = 600;
	GLFWwindow* m_pWindow = nullptr;

	VkInstance m_VkInstance;

	//**************************

	void InitWindow()
	{
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		m_pWindow = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
	}

	void ListExtensions()
	{
		uint32_t uiExtensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &uiExtensionCount, nullptr);
		std::vector<VkExtensionProperties> vExtensions(uiExtensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &uiExtensionCount, vExtensions.data());

		for (const auto& extension : vExtensions)
			std::cout << "\t" << extension.extensionName << std::endl;
	}
	
	void CreateInstance()
	{
		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Hello Triangle";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;

		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		createInfo.enabledExtensionCount = glfwExtensionCount;
		createInfo.ppEnabledExtensionNames = glfwExtensions;
		createInfo.enabledLayerCount = 0;

		if(vkCreateInstance(&createInfo, nullptr, &m_VkInstance) != VK_SUCCESS)
			throw std::runtime_error("Failed to create instance !");

		ListExtensions();
	}

	void InitVulkan() 
	{
		CreateInstance();
	}

	void MainLoop() 
	{
		while (!glfwWindowShouldClose(m_pWindow))
		{
			glfwPollEvents();
		}
	}

	void Cleanup() 
	{
		vkDestroyInstance(m_VkInstance, nullptr);

		glfwDestroyWindow(m_pWindow);
		glfwTerminate();
	}
};

int main() 
{
	HelloTriangleApplication app;

	try 
	{
		app.Run();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}