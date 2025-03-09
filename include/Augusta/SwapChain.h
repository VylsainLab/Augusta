#ifndef AUG_SWAPCHAIN_H
#define AUG_SWAPCHAIN_H

#include <vulkan/vulkan.h>
#include <vector>

namespace aug
{
	struct SwapChainSupportDetails
	{
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	class SwapChain
	{
	public:
		static SwapChainSupportDetails QuerySwapChainSupport(const VkPhysicalDevice& device, const VkSurfaceKHR& surface);

		SwapChain(const VkSurfaceKHR& surface);
		virtual ~SwapChain();

		VkSwapchainKHR GetSwapChainHandle() const { return m_VkSwapChain; }
		VkFormat GetImageFormat() const { return m_VkSwapChainImageFormat; }
		VkExtent2D GetExtent() const { return m_VkSwapChainExtent; }
		uint32_t GetImageCount() const { return m_uiImageCount; }
		VkImageView GetImageViewAtIndex(uint32_t index) const { return m_vVkSwapChainImageViews.at(index); }

	protected:
		VkSwapchainKHR m_VkSwapChain;
		VkFormat m_VkSwapChainImageFormat;
		VkExtent2D m_VkSwapChainExtent;
		std::vector<VkImage> m_vVkSwapChainImages;
		std::vector<VkImageView> m_vVkSwapChainImageViews;
		uint32_t m_uiImageCount = 0;
	};
}

#endif