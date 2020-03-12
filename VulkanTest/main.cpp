#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstdlib>
#include <optional>
#include <set>
#include <algorithm>
#include <fstream>

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
	std::vector<VkImageView> m_vVkSwapChainImageViews;
	std::vector<VkFramebuffer> m_vVkSwapChainFramebuffers;
	std::vector<VkCommandBuffer> m_vVkSwapChainCommandBuffers;
	VkRenderPass m_VkRenderPass;
	VkPipelineLayout m_VkPipelineLayout;
	VkPipeline m_VkGraphicsPipeline;

	VkCommandPool m_VkCommandPool;

	VkSemaphore m_VkImageAvailableSemaphore;
	VkSemaphore m_VkRenderFinishedSemaphore;

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
			createInfo.queueFamilyIndexCount = 0; 
			createInfo.pQueueFamilyIndices = nullptr; 
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

	//************IMAGE VIEWS****************

	void CreateSwapChainImageViews()
	{
		m_vVkSwapChainImageViews.resize(m_vVkSwapChainImages.size());

		VkImageViewCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = m_VkSwapChainImageFormat;
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		for (size_t i = 0; i < m_vVkSwapChainImages.size(); ++i)
		{
			createInfo.image = m_vVkSwapChainImages[i];

			if (vkCreateImageView(m_VkDevice, &createInfo, nullptr, &m_vVkSwapChainImageViews[i]) != VK_SUCCESS)
				throw std::runtime_error("Failed to create image views!");
		}
	}

	//**************SHADERS******************

	//TODO use libshaderc
	static std::vector<char> readFile(const std::string& filename) 
	{
		std::ifstream file(filename, std::ios::ate | std::ios::binary);

		if (!file.is_open())
			throw std::runtime_error("failed to open file!");

		size_t fileSize = (size_t)file.tellg();
		std::vector<char> buffer(fileSize);
		file.seekg(0);
		file.read(buffer.data(), fileSize);
		file.close();

		return buffer;
	}

	VkShaderModule CreateShaderModule(const std::vector<char>& code) 
	{
		VkShaderModuleCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(m_VkDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
			throw std::runtime_error("Failed to create shader module!");

		return shaderModule;
	}

	//************RENDER PASS****************

	void CreateRenderPass()
	{
		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format = m_VkSwapChainImageFormat;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRef = {};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;

		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAttachment;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;

		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		if (vkCreateRenderPass(m_VkDevice, &renderPassInfo, nullptr, &m_VkRenderPass) != VK_SUCCESS)
			throw std::runtime_error("failed to create render pass!");
	}

	//************GRAPHICS PIPELINE**********

	void CreateGraphicsPipeline()
	{
		//Shaders
		auto vertShaderCode = readFile("shaders/vert.spv");
		auto fragShaderCode = readFile("shaders/frag.spv");

		VkShaderModule vertShaderModule = CreateShaderModule(vertShaderCode);
		VkShaderModule fragShaderModule = CreateShaderModule(fragShaderCode);

		VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertShaderModule;
		vertShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShaderModule;
		fragShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

		//Vertex input : hard coded in vertex shader for now
		VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 0;
		vertexInputInfo.pVertexBindingDescriptions = nullptr; 
		vertexInputInfo.vertexAttributeDescriptionCount = 0;
		vertexInputInfo.pVertexAttributeDescriptions = nullptr; 

		//Input assembly
		VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		//Viewport
		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)m_VkSwapChainExtent.width;
		viewport.height = (float)m_VkSwapChainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = m_VkSwapChainExtent;

		VkPipelineViewportStateCreateInfo viewportState = {};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		//Rasterizer
		VkPipelineRasterizationStateCreateInfo rasterizer = {};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE; //TODO enable the needed GPU feature
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f; 
		rasterizer.depthBiasClamp = 0.0f; 
		rasterizer.depthBiasSlopeFactor = 0.0f; 

		//Multisampling : disabled for now
		VkPipelineMultisampleStateCreateInfo multisampling = {};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampling.minSampleShading = 1.0f; 
		multisampling.pSampleMask = nullptr; 
		multisampling.alphaToCoverageEnable = VK_FALSE; 
		multisampling.alphaToOneEnable = VK_FALSE; 

		//TODO : depth stencil

		//Blending
		VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; 
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; 
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; 
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; 
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; 
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; 

		VkPipelineColorBlendStateCreateInfo colorBlending = {};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY; 
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f; 
		colorBlending.blendConstants[1] = 0.0f; 
		colorBlending.blendConstants[2] = 0.0f; 
		colorBlending.blendConstants[3] = 0.0f; 

		//TODO : eventual dynamic states

		//Pipeline layout (uniforms specification)
		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 0; 
		pipelineLayoutInfo.pSetLayouts = nullptr; 
		pipelineLayoutInfo.pushConstantRangeCount = 0; 
		pipelineLayoutInfo.pPushConstantRanges = nullptr; 

		if (vkCreatePipelineLayout(m_VkDevice, &pipelineLayoutInfo, nullptr, &m_VkPipelineLayout) != VK_SUCCESS)
			throw std::runtime_error("failed to create pipeline layout!");

		//Graphics pipeline
		VkGraphicsPipelineCreateInfo pipelineInfo = {};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = nullptr; 
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = nullptr; 
		pipelineInfo.layout = m_VkPipelineLayout;
		pipelineInfo.renderPass = m_VkRenderPass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; 
		pipelineInfo.basePipelineIndex = -1; 

		if (vkCreateGraphicsPipelines(m_VkDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_VkGraphicsPipeline) != VK_SUCCESS)
			throw std::runtime_error("failed to create graphics pipeline!");

		//cleanup
		vkDestroyShaderModule(m_VkDevice, fragShaderModule, nullptr);
		vkDestroyShaderModule(m_VkDevice, vertShaderModule, nullptr);
	}

	//**************FRAMEBUFFERS*************

	void CreateSwapChainFramebuffers()
	{
		m_vVkSwapChainFramebuffers.resize(m_vVkSwapChainImageViews.size());

		for (size_t i = 0; i < m_vVkSwapChainFramebuffers.size(); i++)
		{
			VkImageView attachments[] = { m_vVkSwapChainImageViews[i] };

			VkFramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = m_VkRenderPass;
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments = attachments;
			framebufferInfo.width = m_VkSwapChainExtent.width;
			framebufferInfo.height = m_VkSwapChainExtent.height;
			framebufferInfo.layers = 1;

			if (vkCreateFramebuffer(m_VkDevice, &framebufferInfo, nullptr, &m_vVkSwapChainFramebuffers[i]) != VK_SUCCESS)
				throw std::runtime_error("Failed to create framebuffer!");
		}
	}

	//*************COMMAND POOL**************

	void CreateCommandPool()
	{
		QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(m_VkPhysicalDevice);

		VkCommandPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = queueFamilyIndices.uiGraphicsFamily.value();
		poolInfo.flags = 0;

		if (vkCreateCommandPool(m_VkDevice, &poolInfo, nullptr, &m_VkCommandPool) != VK_SUCCESS)
			throw std::runtime_error("Failed to create command pool!");
	}

	//************COMMAND BUFFERS************

	void CreateSwapChainCommandBuffers()
	{
		m_vVkSwapChainCommandBuffers.resize(m_vVkSwapChainFramebuffers.size());

		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = m_VkCommandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = (uint32_t)m_vVkSwapChainCommandBuffers.size();

		if (vkAllocateCommandBuffers(m_VkDevice, &allocInfo, m_vVkSwapChainCommandBuffers.data()) != VK_SUCCESS)
			throw std::runtime_error("Failed to allocate command buffers!");
				
		for (size_t i = 0; i < m_vVkSwapChainCommandBuffers.size(); i++)
		{
			//Begin command buffer recording
			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = 0;
			beginInfo.pInheritanceInfo = nullptr;

			if (vkBeginCommandBuffer(m_vVkSwapChainCommandBuffers[i], &beginInfo) != VK_SUCCESS)
				throw std::runtime_error("Failed to begin recording command buffer!");

			//Render pass
			VkRenderPassBeginInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = m_VkRenderPass;
			renderPassInfo.framebuffer = m_vVkSwapChainFramebuffers[i];
			renderPassInfo.renderArea.offset = { 0, 0 };
			renderPassInfo.renderArea.extent = m_VkSwapChainExtent;
			VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
			renderPassInfo.clearValueCount = 1;
			renderPassInfo.pClearValues = &clearColor;
			vkCmdBeginRenderPass(m_vVkSwapChainCommandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

			vkCmdBindPipeline(m_vVkSwapChainCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_VkGraphicsPipeline);

			vkCmdDraw(m_vVkSwapChainCommandBuffers[i], 3, 1, 0, 0);

			vkCmdEndRenderPass(m_vVkSwapChainCommandBuffers[i]);

			if (vkEndCommandBuffer(m_vVkSwapChainCommandBuffers[i]) != VK_SUCCESS)
				throw std::runtime_error("Failed to record command buffer!");
		}

		
	}

	//************SEMAPHORES*****************

	void CreateSemaphores()
	{
		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		if (vkCreateSemaphore(m_VkDevice, &semaphoreInfo, nullptr, &m_VkImageAvailableSemaphore) != VK_SUCCESS ||
			vkCreateSemaphore(m_VkDevice, &semaphoreInfo, nullptr, &m_VkRenderFinishedSemaphore) != VK_SUCCESS)
			throw std::runtime_error("Failed to create semaphores!");
	}

	//************DRAW FRAME*****************

	void DrawFrame()
	{
		uint32_t uiImageIndex = 0;
		vkAcquireNextImageKHR(m_VkDevice, m_VkSwapChain, UINT64_MAX, m_VkImageAvailableSemaphore, VK_NULL_HANDLE, &uiImageIndex);

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		VkSemaphore waitSemaphores[] = { m_VkImageAvailableSemaphore };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_vVkSwapChainCommandBuffers[uiImageIndex];
		VkSemaphore signalSemaphores[] = { m_VkRenderFinishedSemaphore };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		if (vkQueueSubmit(m_VkGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
			throw std::runtime_error("Failed to submit draw command buffer!");


		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;
		VkSwapchainKHR swapChains[] = { m_VkSwapChain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &uiImageIndex;
		presentInfo.pResults = nullptr;
		vkQueuePresentKHR(m_VkPresentQueue, &presentInfo);

		vkQueueWaitIdle(m_VkPresentQueue);
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
		CreateSwapChainImageViews();
		CreateRenderPass();
		CreateGraphicsPipeline();
		CreateSwapChainFramebuffers();
		CreateCommandPool();
		CreateSwapChainCommandBuffers();
		CreateSemaphores();
	}

	void MainLoop() 
	{
		while (!glfwWindowShouldClose(m_pWindow))
		{
			glfwPollEvents();
			DrawFrame();
		}

		vkDeviceWaitIdle(m_VkDevice);
	}

	void Cleanup() 
	{
		vkDestroySemaphore(m_VkDevice, m_VkRenderFinishedSemaphore, nullptr);
		vkDestroySemaphore(m_VkDevice, m_VkImageAvailableSemaphore, nullptr);

		vkDestroyCommandPool(m_VkDevice, m_VkCommandPool, nullptr);

		for (auto framebuffer : m_vVkSwapChainFramebuffers)
			vkDestroyFramebuffer(m_VkDevice, framebuffer, nullptr);
		
		vkDestroyPipeline(m_VkDevice, m_VkGraphicsPipeline, nullptr);

		vkDestroyPipelineLayout(m_VkDevice, m_VkPipelineLayout, nullptr);

		vkDestroyRenderPass(m_VkDevice, m_VkRenderPass, nullptr);

		for (auto imageView : m_vVkSwapChainImageViews)
			vkDestroyImageView(m_VkDevice, imageView, nullptr);
		
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