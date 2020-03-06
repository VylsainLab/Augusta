#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstdlib>
#include <optional>
#include <set>
#include <algorithm>

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
	VkSurfaceKHR m_VkSurface;
	VkPhysicalDevice m_VkPhysicalDevice = VK_NULL_HANDLE;
	VkDevice m_VkDevice;
	VkQueue m_VkGraphicsQueue;
	VkQueue m_VkPresentQueue;
	VkSwapchainKHR m_VkSwapChain;
	VkFormat m_VkSwapChainImageFormat;
	VkExtent2D m_VkSwapChainExtent;
	std::vector<VkImage> m_vVkSwapChainImages;

	std::vector<const char*> m_vValidationLayers = { "VK_LAYER_KHRONOS_validation" };
	std::vector<const char*> m_vDeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	
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

		std::set<std::string> requiredLayers(m_vValidationLayers.begin(), m_vValidationLayers.end());
		for (const auto& layer : vAvailableLayers)
			requiredLayers.erase(layer.layerName);

		return requiredLayers.empty();
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

	//****SURFACE*****

	void CreateSurface()
	{
		if(glfwCreateWindowSurface(m_VkInstance, m_pWindow, nullptr, &m_VkSurface) != VK_SUCCESS)
			throw std::runtime_error("Failed to create window surface!");
	}

	//**************SWAP CHAIN***************

	struct SwapChainSupportDetails
	{
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	SwapChainSupportDetails QuerySwapChainSupport(const VkPhysicalDevice& device)
	{
		SwapChainSupportDetails details;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_VkSurface, &details.capabilities);

		//formats
		uint32_t uiFormatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_VkSurface, &uiFormatCount, nullptr);

		if (uiFormatCount != 0)
		{
			details.formats.resize(uiFormatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_VkSurface, &uiFormatCount, details.formats.data());
		}

		//present modes
		uint32_t uiPresentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_VkSurface, &uiPresentModeCount, nullptr);

		if (uiPresentModeCount != 0)
		{
			details.presentModes.resize(uiPresentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_VkSurface, &uiPresentModeCount, details.presentModes.data());
		}

		return details;
	}

	VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& vAvailableFormats)
	{
		for (const auto& availableFormat : vAvailableFormats) 
		{
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
				return availableFormat;
		}

		return vAvailableFormats[0];
	}

	VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& vAvailablePresentModes) 
	{
		for (const auto& availablePresentMode : vAvailablePresentModes) 
		{
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
				return availablePresentMode;
		}

		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) 
	{
		if (capabilities.currentExtent.width != UINT32_MAX) 
		{
			return capabilities.currentExtent;
		}
		else 
		{
			VkExtent2D actualExtent = { WINDOW_WIDTH, WINDOW_HEIGHT };

			actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
			actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

			return actualExtent;
		}
	}

	void CreateSwapChain()
	{
		SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(m_VkPhysicalDevice);

		VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
		VkExtent2D extent = ChooseSwapExtent(swapChainSupport.capabilities);

		uint32_t uiImageCount = swapChainSupport.capabilities.minImageCount + 1;

		if (swapChainSupport.capabilities.maxImageCount > 0 && uiImageCount > swapChainSupport.capabilities.maxImageCount)
			uiImageCount = swapChainSupport.capabilities.maxImageCount;
		
		VkSwapchainCreateInfoKHR createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = m_VkSurface;
		createInfo.minImageCount = uiImageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		QueueFamilyIndices indices = FindQueueFamilies(m_VkPhysicalDevice);
		uint32_t queueFamilyIndices[] = { indices.uiGraphicsFamily.value(), indices.uiPresentFamily.value() };

		if (indices.uiGraphicsFamily != indices.uiPresentFamily) 
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else //most of the time, graphics and present queues are the same :)
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0; // Optional
			createInfo.pQueueFamilyIndices = nullptr; // Optional
		}

		createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = VK_NULL_HANDLE;

		if (vkCreateSwapchainKHR(m_VkDevice, &createInfo, nullptr, &m_VkSwapChain) != VK_SUCCESS)
			throw std::runtime_error("Failed to create swap chain!");

		m_VkSwapChainImageFormat = surfaceFormat.format;
		m_VkSwapChainExtent = extent;
		vkGetSwapchainImagesKHR(m_VkDevice, m_VkSwapChain, &uiImageCount, nullptr);
		m_vVkSwapChainImages.resize(uiImageCount);
		vkGetSwapchainImagesKHR(m_VkDevice, m_VkSwapChain, &uiImageCount, m_vVkSwapChainImages.data());
	}

	//****PHYSICAL DEVICE*****

	bool CheckDeviceExtensionSupport(const VkPhysicalDevice& device)
	{
		uint32_t uiExtensionCount = 0;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &uiExtensionCount, nullptr);

		std::vector<VkExtensionProperties> vAvailableExtensions(uiExtensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &uiExtensionCount, vAvailableExtensions.data());

		std::set<std::string> requiredExtensions(m_vDeviceExtensions.begin(), m_vDeviceExtensions.end());
		for (const auto& extension : vAvailableExtensions)
			requiredExtensions.erase(extension.extensionName);

		return requiredExtensions.empty();
	}

	struct QueueFamilyIndices
	{
		std::optional<uint32_t> uiGraphicsFamily;
		std::optional<uint32_t> uiPresentFamily;

		bool IsComplete() { return uiGraphicsFamily.has_value() && uiPresentFamily.has_value(); }
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

			VkBool32 bIsPresentSupported = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_VkSurface, &bIsPresentSupported);
			if (bIsPresentSupported)
				indices.uiPresentFamily = i;

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

		//extensions
		bool bExtensionsSupported = CheckDeviceExtensionSupport(device);

		//swap chain
		bool bSwapChainAdequate = false;
		if (bExtensionsSupported)
		{
			SwapChainSupportDetails details = QuerySwapChainSupport(device);
			bSwapChainAdequate = !details.formats.empty() && !details.presentModes.empty();
		}

		//queue families
		QueueFamilyIndices indices = FindQueueFamilies(device);

		return indices.IsComplete() && bExtensionsSupported && bSwapChainAdequate;
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

		std::set<uint32_t> sUniqueQueueFamilies = { indices.uiGraphicsFamily.value(), indices.uiPresentFamily.value() };
		std::vector<VkDeviceQueueCreateInfo> vQueueCreateInfos;
		float fQueuePriority = 1.0f;
		for (uint32_t uiQueueFamily : sUniqueQueueFamilies)
		{
			VkDeviceQueueCreateInfo queueCreateInfo = {};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = uiQueueFamily;
			queueCreateInfo.queueCount = 1;			
			queueCreateInfo.pQueuePriorities = &fQueuePriority;
			vQueueCreateInfos.push_back(queueCreateInfo);
		}

		VkPhysicalDeviceFeatures deviceFeatures = {};

		VkDeviceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.queueCreateInfoCount = (uint32_t)vQueueCreateInfos.size();
		createInfo.pQueueCreateInfos = vQueueCreateInfos.data();
		createInfo.pEnabledFeatures = &deviceFeatures;
		createInfo.enabledExtensionCount = (uint32_t)m_vDeviceExtensions.size();
		createInfo.ppEnabledExtensionNames = m_vDeviceExtensions.data();

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
		vkGetDeviceQueue(m_VkDevice, indices.uiPresentFamily.value(), 0, &m_VkPresentQueue);
	}

	//***************************************
	void InitVulkan() 
	{
		CreateInstance();
		SetupDebugMessenger();
		CreateSurface();
		PickPhysicalDevice();
		CreateLogicalDevice();
		CreateSwapChain();
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
		vkDestroySwapchainKHR(m_VkDevice, m_VkSwapChain, nullptr);

		vkDestroyDevice(m_VkDevice, nullptr);

		vkDestroySurfaceKHR(m_VkInstance, m_VkSurface, nullptr);

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