#include <Augusta/SwapChain.h>
#include <Augusta/Context.h>
#include <stdexcept>

namespace aug
{
	SwapChainSupportDetails SwapChain::QuerySwapChainSupport(const VkPhysicalDevice& device, const VkSurfaceKHR& surface)
	{
		SwapChainSupportDetails details;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

		//formats
		uint32_t uiFormatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &uiFormatCount, nullptr);

		if (uiFormatCount != 0)
		{
			details.formats.resize(uiFormatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &uiFormatCount, details.formats.data());
		}

		//present modes
		uint32_t uiPresentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &uiPresentModeCount, nullptr);

		if (uiPresentModeCount != 0)
		{
			details.presentModes.resize(uiPresentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &uiPresentModeCount, details.presentModes.data());
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

	SwapChain::SwapChain(const VkSurfaceKHR& surface)
	{
		aug::SwapChainSupportDetails swapChainSupport = SwapChain::QuerySwapChainSupport(Context::m_VkPhysicalDevice, surface);

		VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
		VkExtent2D extent = swapChainSupport.capabilities.currentExtent;

		m_uiImageCount = swapChainSupport.capabilities.minImageCount + 1;

		if (swapChainSupport.capabilities.maxImageCount > 0 && m_uiImageCount > swapChainSupport.capabilities.maxImageCount)
			m_uiImageCount = swapChainSupport.capabilities.maxImageCount;

		VkSwapchainCreateInfoKHR createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = surface;
		createInfo.minImageCount = m_uiImageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		aug::QueueFamilyIndices indices = aug::Context::GetQueueFamilies(surface);
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

		if (vkCreateSwapchainKHR(aug::Context::m_VkDevice, &createInfo, nullptr, &m_VkSwapChain) != VK_SUCCESS)
			throw std::runtime_error("Failed to create swap chain!");

		m_VkSwapChainImageFormat = surfaceFormat.format;
		m_VkSwapChainExtent = extent;
		vkGetSwapchainImagesKHR(aug::Context::m_VkDevice, m_VkSwapChain, &m_uiImageCount, nullptr);
		m_vVkSwapChainImages.resize(m_uiImageCount);
		vkGetSwapchainImagesKHR(aug::Context::m_VkDevice, m_VkSwapChain, &m_uiImageCount, m_vVkSwapChainImages.data());

		//Image views
		m_vVkSwapChainImageViews.resize(m_vVkSwapChainImages.size());

		VkImageViewCreateInfo ivcreateInfo = {};
		ivcreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		ivcreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		ivcreateInfo.format = m_VkSwapChainImageFormat;
		ivcreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		ivcreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		ivcreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		ivcreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		ivcreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		ivcreateInfo.subresourceRange.baseMipLevel = 0;
		ivcreateInfo.subresourceRange.levelCount = 1;
		ivcreateInfo.subresourceRange.baseArrayLayer = 0;
		ivcreateInfo.subresourceRange.layerCount = 1;

		for (size_t i = 0; i < m_vVkSwapChainImages.size(); ++i)
		{
			ivcreateInfo.image = m_vVkSwapChainImages[i];

			if (vkCreateImageView(aug::Context::m_VkDevice, &ivcreateInfo, nullptr, &m_vVkSwapChainImageViews[i]) != VK_SUCCESS)
				throw std::runtime_error("Failed to create image views!");
		}
	}

	SwapChain::~SwapChain()
	{
		for (auto imageView : m_vVkSwapChainImageViews)
			vkDestroyImageView(aug::Context::m_VkDevice, imageView, nullptr);

		vkDestroySwapchainKHR(aug::Context::m_VkDevice, m_VkSwapChain, nullptr);
	}
}