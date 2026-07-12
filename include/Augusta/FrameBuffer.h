#pragma once
#include <stdint.h>
#include <vector>
#include <optional>
#include <memory>
#include <vulkan/vulkan.h>
#include <Augusta/Texture.h>
#include <Augusta/IRenderTarget.h>

namespace aug
{
	struct SFramebufferDesc
	{
		std::string _strName;
		uint16_t _uiWidth = 0;
		uint16_t _uiHeight = 0;
		std::vector<VkFormat> _vColorAttachmentsFormats;
		VkFormat _DepthFormat = VK_FORMAT_UNDEFINED;
	};	

	class Framebuffer : public IRenderTarget
	{
	public:
		Framebuffer(const SFramebufferDesc& desc);
		~Framebuffer();

		const SFramebufferDesc& GetDesc() { return m_Desc; }

		//IRenderTarget
		VkExtent2D GetExtent() const override;
		std::vector<VkFormat> GetColorFormats() const override;
		VkFormat GetDepthFormat() const override;
		std::vector<VkImageView> GetColorImageViews() const override;
		VkImageView GetDepthImageView() const override;
		void TransitionToLayout(const VkCommandBuffer& cb, SRenderTargetLayout layout) override;

		void BlitToRenderTarget(const VkCommandBuffer& cb);

		static SRenderTargetLayout FRAMEBUFFER_LAYOUT_ATTACHMENT;
		static SRenderTargetLayout FRAMEBUFFER_LAYOUT_SAMPLING;

	protected:
		SFramebufferDesc m_Desc;

		struct SAttachment
		{
			std::shared_ptr<Texture> _pAttachmentTexture = nullptr; //framebuffer attachment
			std::shared_ptr<Texture> _pRenderTargetTexture = nullptr; //texture to blit to and use in shader
		};
		std::vector<SAttachment> m_vColorAttachments;
		std::optional<SAttachment> m_DepthAttachment;

#ifndef USE_DYNAMIC_RENDERING
		VkFramebuffer m_VkFramebuffer;
#endif
	};
}

