#include <VulkanWrap/Context.h>
#include <VulkanWrap/SwapChain.h>
#include <VulkanWrap/MemoryAllocator.h>
#include <set>
#include <string>
#include <iostream>
#include <optional>

namespace vkw
{
	VkInstance Context::m_VkInstance = VK_NULL_HANDLE;
	VkDebugUtilsMessengerEXT Context::m_VkDebugMessenger;
	std::vector<const char*> Context::m_vValidationLayers = { "VK_LAYER_KHRONOS_validation" };
	VkSurfaceKHR Context::m_ReferenceSurface = VK_NULL_HANDLE;
	VkPhysicalDevice Context::m_VkPhysicalDevice = VK_NULL_HANDLE;
	VkDevice Context::m_VkDevice = VK_NULL_HANDLE;
	VkQueue Context::m_VkGraphicsQueue = VK_NULL_HANDLE;
	VkQueue Context::m_VkPresentQueue = VK_NULL_HANDLE;
	VkCommandPool Context::m_VkCommandPool = VK_NULL_HANDLE;
	std::vector<const char*> Context::m_vDeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

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

	void Context::InitInstance()
	{
		CreateInstance();
		SetupDebugMessenger();
	}

	void Context::Init(const VkSurfaceKHR& reference_surface)
	{
		m_ReferenceSurface = reference_surface;
		PickPhysicalDevice();
		CreateLogicalDevice();
		CreateCommandPool();
		MemoryAllocator::Init();
	}

	void Context::Release()
	{
		MemoryAllocator::Release();

		vkDestroyCommandPool(m_VkDevice, m_VkCommandPool, nullptr);

		vkDestroyDevice(m_VkDevice, nullptr);

		if (ENABLE_VALIDATION_LAYERS)
			DestroyDebugUtilsMessengerEXT(m_VkInstance, m_VkDebugMessenger, nullptr);

		vkDestroyInstance(m_VkInstance, nullptr);
	}

	bool Context::CheckValidationLayers()
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

	void Context::SetupDebugMessenger()
	{
		if (!ENABLE_VALIDATION_LAYERS)
			return;

		VkDebugUtilsMessengerCreateInfoEXT createInfo;
		PopulateDebugMessengerCreateInfo(createInfo);

		if (CreateDebugUtilsMessengerEXT(m_VkInstance, &createInfo, nullptr, &m_VkDebugMessenger) != VK_SUCCESS)
			throw std::runtime_error("Failed to set up debug messenger!");
	}

	void Context::CreateInstance()
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

	//Physical device
	bool Context::CheckDeviceExtensionSupport(const VkPhysicalDevice& device)
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

	QueueFamilyIndices FindQueueFamilies(const VkPhysicalDevice& device, const VkSurfaceKHR& surface)
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
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &bIsPresentSupported);
			if (bIsPresentSupported)
				indices.uiPresentFamily = i;

			if (indices.IsComplete())
				break;
		}

		return indices;
	}

	QueueFamilyIndices Context::GetQueueFamilies(const VkSurfaceKHR& surface)
	{
		if (m_VkPhysicalDevice == VK_NULL_HANDLE)
			throw std::runtime_error("Physical device has not been initialized");

		return FindQueueFamilies(m_VkPhysicalDevice, surface);
	}

	VkCommandBuffer Context::BuildSingleTimeCommandBuffer()
	{
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = m_VkCommandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(m_VkDevice, &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		return commandBuffer;
	}

	void Context::SubmitAndFreeCommandBuffer(VkCommandBuffer commandBuffer)
	{
		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(m_VkGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(m_VkGraphicsQueue);

		vkFreeCommandBuffers(m_VkDevice, m_VkCommandPool, 1, &commandBuffer);
	}

	bool Context::IsDeviceSuitable(const VkPhysicalDevice& device)
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
			SwapChainSupportDetails details = SwapChain::QuerySwapChainSupport(device, m_ReferenceSurface);
			bSwapChainAdequate = !details.formats.empty() && !details.presentModes.empty();
		}

		//queue families
		QueueFamilyIndices indices = FindQueueFamilies(device, m_ReferenceSurface);

		return indices.IsComplete() && bExtensionsSupported && bSwapChainAdequate;
	}

	void Context::PickPhysicalDevice() //select first found suitable device
	{
		uint32_t uiDeviceCount;
		vkEnumeratePhysicalDevices(vkw::Context::m_VkInstance, &uiDeviceCount, nullptr);
		if (uiDeviceCount == 0)
			throw std::runtime_error("Failed to find GPUs with Vulkan support!");

		std::vector<VkPhysicalDevice> vDevices(uiDeviceCount);
		vkEnumeratePhysicalDevices(vkw::Context::m_VkInstance, &uiDeviceCount, vDevices.data());

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

	void Context::CreateLogicalDevice()
	{
		QueueFamilyIndices indices = FindQueueFamilies(m_VkPhysicalDevice, m_ReferenceSurface);

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
			createInfo.enabledLayerCount = static_cast<uint32_t>(vkw::Context::m_vValidationLayers.size());
			createInfo.ppEnabledLayerNames = vkw::Context::m_vValidationLayers.data();
		}
		else
			createInfo.enabledLayerCount = 0;

		if (vkCreateDevice(m_VkPhysicalDevice, &createInfo, nullptr, &m_VkDevice) != VK_SUCCESS)
			throw std::runtime_error("Failed to create logical device!");

		vkGetDeviceQueue(m_VkDevice, indices.uiGraphicsFamily.value(), 0, &m_VkGraphicsQueue);
		vkGetDeviceQueue(m_VkDevice, indices.uiPresentFamily.value(), 0, &m_VkPresentQueue);
	}

	void Context::CreateCommandPool()
	{
		vkw::QueueFamilyIndices queueFamilyIndices = GetQueueFamilies(m_ReferenceSurface);

		VkCommandPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = queueFamilyIndices.uiGraphicsFamily.value();
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		if (vkCreateCommandPool(m_VkDevice, &poolInfo, nullptr, &m_VkCommandPool) != VK_SUCCESS)
			throw std::runtime_error("Failed to create command pool!");
	}
}