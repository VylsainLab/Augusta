#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstdlib>
#include <optional>

#define WINDOW_WIDTH	800
#define WINDOW_HEIGHT	600

#ifdef NDEBUG
	#define ENABLE_VALIDATION_LAYERS	false
#else
	#define ENABLE_VALIDATION_LAYERS	true
#endif

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
	GLFWwindow* m_pWindow = nullptr;

	VkInstance m_VkInstance;
	VkDebugUtilsMessengerEXT m_VkDebugMessenger;
	VkPhysicalDevice m_VkPhysicalDevice = VK_NULL_HANDLE;
	VkDevice m_VkDevice;
	VkQueue m_VkGraphicsQueue;

	std::vector<const char*> m_vValidationLayers = { "VK_LAYER_KHRONOS_validation" };
	
	//******WINDOW*********
	void InitWindow()
	{
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		m_pWindow = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Vulkan", nullptr, nullptr);
	}

	//****INSTANCE*****
	bool CheckValidationLayers()
	{
		uint32_t uiLayerCount = 0;
		vkEnumerateInstanceLayerProperties(&uiLayerCount, nullptr);
		std::vector<VkLayerProperties> vAvailableLayers(uiLayerCount);
		vkEnumerateInstanceLayerProperties(&uiLayerCount, vAvailableLayers.data());

		for (const auto &layer : m_vValidationLayers)
		{
			bool bFound = false;
			for (const auto& layerProperties : vAvailableLayers)
			{
				if (strcmp(layer, layerProperties.layerName) == 0)
				{
					bFound = true;
					break;
				}
			}

			if (!bFound)
				return false;
		}

		return true;
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
	
	void CreateInstance()
	{
		ListExtensions();

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

		if(vkCreateInstance(&createInfo, nullptr, &m_VkInstance) != VK_SUCCESS)
			throw std::runtime_error("Failed to create instance !");
	}

	//****DEBUG CALLBACK*****
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

	void SetupDebugMessenger()
	{
		if (!ENABLE_VALIDATION_LAYERS)
			return;

		VkDebugUtilsMessengerCreateInfoEXT createInfo;
		PopulateDebugMessengerCreateInfo(createInfo);

		if (CreateDebugUtilsMessengerEXT(m_VkInstance, &createInfo, nullptr, &m_VkDebugMessenger) != VK_SUCCESS)
			throw std::runtime_error("Failed to set up debug messenger!");
	}

	//****PHYSICAL DEVICE*****
	struct QueueFamilyIndices
	{
		std::optional<uint32_t> uiGraphicsFamily;

		bool IsComplete() { return uiGraphicsFamily.has_value(); }
	};

	QueueFamilyIndices FindQueueFamilies(const VkPhysicalDevice &device)
	{
		QueueFamilyIndices indices;

		uint32_t uiQueueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &uiQueueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> vQueueFamilies(uiQueueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &uiQueueFamilyCount, vQueueFamilies.data());

		for (uint32_t i = 0; i < uiQueueFamilyCount; ++i)
		{
			if (vQueueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
				indices.uiGraphicsFamily = i;

			if (indices.IsComplete())
				break;
		}

		return indices;
	}

	bool IsDeviceSuitable(const VkPhysicalDevice &device)
	{
		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(device, &deviceProperties);

		VkPhysicalDeviceFeatures deviceFeatures;
		vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

		QueueFamilyIndices indices = FindQueueFamilies(device);
		return indices.IsComplete();
	}

	void PickPhysicalDevice() //select first found suitable device
	{
		uint32_t uiDeviceCount;
		vkEnumeratePhysicalDevices(m_VkInstance, &uiDeviceCount, nullptr);
		if (uiDeviceCount == 0)
			throw std::runtime_error("Faild to find GPUs with Vulkan support!");

		std::vector<VkPhysicalDevice> vDevices(uiDeviceCount);
		vkEnumeratePhysicalDevices(m_VkInstance, &uiDeviceCount, vDevices.data());

		for (const auto& device : vDevices)
		{
			if (IsDeviceSuitable(device))
			{
				m_VkPhysicalDevice = device;
				break;
			}
		}

		if (m_VkPhysicalDevice == VK_NULL_HANDLE)
			throw std::runtime_error("Failed to find a suitable GPU!");
	}


	//****LOGICAL DEVICE*****
	void CreateLogicalDevice()
	{
		QueueFamilyIndices indices = FindQueueFamilies(m_VkPhysicalDevice);

		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = indices.uiGraphicsFamily.value();
		queueCreateInfo.queueCount = 1;
		float fQueuePriority = 1.0f;
		queueCreateInfo.pQueuePriorities = &fQueuePriority;

		VkPhysicalDeviceFeatures deviceFeatures = {};

		VkDeviceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.pQueueCreateInfos = &queueCreateInfo;
		createInfo.queueCreateInfoCount = 1;
		createInfo.pEnabledFeatures = &deviceFeatures;
		createInfo.enabledExtensionCount = 0;

		//needed for older Vulkan versions compatibility 
		if (ENABLE_VALIDATION_LAYERS) 
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(m_vValidationLayers.size());
			createInfo.ppEnabledLayerNames = m_vValidationLayers.data();
		}
		else
			createInfo.enabledLayerCount = 0;

		if (vkCreateDevice(m_VkPhysicalDevice, &createInfo, nullptr, &m_VkDevice) != VK_SUCCESS)
			throw std::runtime_error("Failed to create logical device!");

		vkGetDeviceQueue(m_VkDevice, indices.uiGraphicsFamily.value(), 0, &m_VkGraphicsQueue);
	}

	void InitVulkan() 
	{
		CreateInstance();
		SetupDebugMessenger();
		PickPhysicalDevice();
		CreateLogicalDevice();
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
		vkDestroyDevice(m_VkDevice, nullptr);

		if (ENABLE_VALIDATION_LAYERS)
			DestroyDebugUtilsMessengerEXT(m_VkInstance, m_VkDebugMessenger, nullptr);
		
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