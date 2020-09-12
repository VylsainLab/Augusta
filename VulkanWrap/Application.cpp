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

		InitVulkan();
	}

	Application::~Application()
	{
		ReleaseVulkan();

		if (m_bGLFWInitialized == true)
		{
			glfwTerminate();
		}
	}

	vkw::Window* Application::AddNewWindow(VkInstance &instance, const char* szName, uint16_t uiWidth, uint16_t uiHeight)
	{
		std::shared_ptr<vkw::Window> pWindow(new vkw::Window(instance, szName, uiWidth, uiHeight));
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

	bool Application::CheckValidationLayers()
	{
		uint32_t uiLayerCount = 0;
		vkEnumerateInstanceLayerProperties(&uiLayerCount, nullptr);
		std::vector<VkLayerProperties> vAvailableLayers(uiLayerCount);
		vkEnumerateInstanceLayerProperties(&uiLayerCount, vAvailableLayers.data());

		std::set<std::string> requiredLayers(m_vValidationLayers.begin(), m_vValidationLayers.end());
		for (const auto& layer : vAvailableLayers)
			requiredLayers.erase(layer.layerName);

		return requiredLayers.empty();
	}

	void PrintExtensions()
	{
		uint32_t uiExtensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &uiExtensionCount, nullptr);
		std::vector<VkExtensionProperties> vExtensions(uiExtensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &uiExtensionCount, vExtensions.data());

		for (const auto& extension : vExtensions)
			std::cout << "\t" << extension.extensionName << std::endl;
	}

	std::vector<const char*> GetRequiredExtensions()
	{
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

		if (ENABLE_VALIDATION_LAYERS)
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

		return extensions;
	}

	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData)
	{
		std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;

		return VK_FALSE;
	}

	void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
	{
		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = DebugCallback;
	}

	VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
	{
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
		if (func != nullptr)
			return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
		else
			return VK_ERROR_EXTENSION_NOT_PRESENT;
	}

	void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
	{
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
		if (func != nullptr)
			func(instance, debugMessenger, pAllocator);
	}

	void Application::SetupDebugMessenger()
	{
		if (!ENABLE_VALIDATION_LAYERS)
			return;

		VkDebugUtilsMessengerCreateInfoEXT createInfo;
		PopulateDebugMessengerCreateInfo(createInfo);

		if (CreateDebugUtilsMessengerEXT(m_VkInstance, &createInfo, nullptr, &m_VkDebugMessenger) != VK_SUCCESS)
			throw std::runtime_error("Failed to set up debug messenger!");
	}

	void Application::CreateInstance()
	{
		PrintExtensions();

		if (ENABLE_VALIDATION_LAYERS && !CheckValidationLayers())
			throw std::runtime_error("Unavailable validation layer !");

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

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
		auto extensions = GetRequiredExtensions();
		createInfo.enabledExtensionCount = (uint32_t)extensions.size();
		createInfo.ppEnabledExtensionNames = extensions.data();
		if (ENABLE_VALIDATION_LAYERS)
		{
			createInfo.enabledLayerCount = (uint32_t)m_vValidationLayers.size();
			createInfo.ppEnabledLayerNames = m_vValidationLayers.data();

			PopulateDebugMessengerCreateInfo(debugCreateInfo);
			createInfo.pNext = &debugCreateInfo;
		}
		else
		{
			createInfo.enabledLayerCount = 0;
			createInfo.pNext = nullptr;
		}

		if (vkCreateInstance(&createInfo, nullptr, &m_VkInstance) != VK_SUCCESS)
			throw std::runtime_error("Failed to create instance !");
	}

	void Application::InitVulkan()
	{
		CreateInstance();
		SetupDebugMessenger();
	}

	void Application::ReleaseVulkan()
	{
		if (ENABLE_VALIDATION_LAYERS)
			DestroyDebugUtilsMessengerEXT(m_VkInstance, m_VkDebugMessenger, nullptr);

		vkDestroyInstance(m_VkInstance, nullptr);
	}
}