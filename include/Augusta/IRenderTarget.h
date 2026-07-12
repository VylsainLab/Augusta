#pragma once
#include <vulkan/vulkan.h>
#include <vector>

namespace aug
{
	struct SRenderTargetLayout
	{
		VkImageLayout _colorLayout;
		VkImageLayout _depthStencilLayout;
	};

	class IRenderTarget
	{
	public:
		virtual ~IRenderTarget() = default;

		virtual VkExtent2D GetExtent() const = 0;

		virtual std::vector<VkFormat> GetColorFormats() const = 0;

		virtual VkFormat GetDepthFormat() const = 0;

		virtual std::vector<VkImageView> GetColorImageViews() const = 0;
		virtual VkImageView GetDepthImageView() const = 0;

		virtual void TransitionToLayout(const VkCommandBuffer& cb, SRenderTargetLayout layout) = 0;
	};
}

