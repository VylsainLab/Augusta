#include <Augusta/FrameBuffer.h>
#include <Augusta/Context.h>
#include <windows.h>

namespace aug
{
	SRenderTargetLayout Framebuffer::FRAMEBUFFER_LAYOUT_ATTACHMENT = { VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
	SRenderTargetLayout Framebuffer::FRAMEBUFFER_LAYOUT_SAMPLING = { VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL };

	Framebuffer::Framebuffer(const SFramebufferDesc& desc)
	{
		m_Desc = desc;

		std::vector<VkImageView> vImageViews;

		for (int i=0; i<desc._vColorAttachmentsFormats.size(); ++i)
		{
			STextureDesc texDesc;
			texDesc._strName = desc._strName + "_color_attachment_" + std::to_string(i);
			texDesc._width = desc._uiWidth;
			texDesc._height = desc._uiHeight;
			texDesc._format = desc._vColorAttachmentsFormats[i];
			texDesc._aspect = VK_IMAGE_ASPECT_COLOR_BIT;
			texDesc._filtering = VK_FILTER_LINEAR;
			texDesc._memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;
			texDesc._usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
			texDesc._tiling = VK_IMAGE_TILING_OPTIMAL;
			texDesc._samplingMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
			texDesc._layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			SAttachment att;
			att._pAttachmentTexture = TextureFactory::LoadTextureFromMemory(texDesc);
			vImageViews.push_back(att._pAttachmentTexture->GetImageView());

			texDesc._strName = desc._strName + "_color_target_" + std::to_string(i);
			texDesc._usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			texDesc._layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			att._pRenderTargetTexture = TextureFactory::LoadTextureFromMemory(texDesc);

			m_vColorAttachments.push_back(att);
		}

		if (desc._DepthFormat != VK_FORMAT_UNDEFINED)
		{
			STextureDesc texDesc;
			texDesc._strName = desc._strName + "_depth";
			texDesc._width = desc._uiWidth;
			texDesc._height = desc._uiHeight;
			texDesc._format = desc._DepthFormat;
			texDesc._aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
			texDesc._filtering = VK_FILTER_NEAREST;
			texDesc._memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;
			texDesc._usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
			texDesc._tiling = VK_IMAGE_TILING_OPTIMAL;
			texDesc._samplingMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
			texDesc._layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			SAttachment att;
			att._pAttachmentTexture = TextureFactory::LoadTextureFromMemory(texDesc);
			vImageViews.push_back(att._pAttachmentTexture->GetImageView());

			texDesc._strName = desc._strName + "_depth_target";
			texDesc._usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			texDesc._layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			att._pRenderTargetTexture = TextureFactory::LoadTextureFromMemory(texDesc);

			m_DepthAttachment = att;
		}

#ifndef USE_DYNAMIC_RENDERING
		VkFramebufferCreateInfo fbufCreateInfo = {};
		//fbufCreateInfo.renderPass = renderPass;
		fbufCreateInfo.attachmentCount = vImageViews.size();
		fbufCreateInfo.pAttachments = vImageViews.data();
		fbufCreateInfo.width = desc._uiWidth;
		fbufCreateInfo.height = desc._uiHeight;
		fbufCreateInfo.layers = 1;

		vkCreateFramebuffer(Context::m_VkDevice, &fbufCreateInfo, nullptr, &m_VkFramebuffer);
#endif
	}

	Framebuffer::~Framebuffer()
	{
	}

	VkExtent2D Framebuffer::GetExtent() const
	{
		return VkExtent2D({ m_Desc._uiWidth, m_Desc._uiHeight });
	}

	std::vector<VkFormat> Framebuffer::GetColorFormats() const
	{
		return m_Desc._vColorAttachmentsFormats;
	}

	VkFormat Framebuffer::GetDepthFormat() const
	{
		return m_Desc._DepthFormat;
	}

	std::vector<VkImageView> Framebuffer::GetColorImageViews() const
	{
		std::vector<VkImageView> vColorImageViews;
		for (auto& att : m_vColorAttachments)
		{
			vColorImageViews.push_back(att._pAttachmentTexture->GetImageView());
		}
		return vColorImageViews;
	}

	VkImageView Framebuffer::GetDepthImageView() const
	{
		if (m_DepthAttachment.has_value())
			return m_DepthAttachment->_pAttachmentTexture->GetImageView();
		return VK_NULL_HANDLE;
	}

	void Framebuffer::TransitionToLayout(const VkCommandBuffer& cb, SRenderTargetLayout layout)
	{
		for (auto& att : m_vColorAttachments)
			att._pAttachmentTexture->TransitionImageToLayout(layout._colorLayout,cb);

		m_DepthAttachment->_pAttachmentTexture->TransitionImageToLayout(layout._depthStencilLayout,cb);
	}


	void Framebuffer::BlitToRenderTarget(const VkCommandBuffer& cb)
	{
		for (auto& att : m_vColorAttachments)
		{
			att._pAttachmentTexture->TransitionImageToLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, cb);
			att._pRenderTargetTexture->TransitionImageToLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, cb);

			VkImageBlit imgBlit;

			imgBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imgBlit.srcSubresource.mipLevel = 0;
			imgBlit.srcSubresource.baseArrayLayer = 0;
			imgBlit.srcSubresource.layerCount = 1;

			imgBlit.srcOffsets[0] = { 0, 0, 0 };
			imgBlit.srcOffsets[1].x = m_Desc._uiWidth;
			imgBlit.srcOffsets[1].y = m_Desc._uiHeight;
			imgBlit.srcOffsets[1].z = 1;

			imgBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imgBlit.dstSubresource.mipLevel = 0;
			imgBlit.dstSubresource.baseArrayLayer = 0;
			imgBlit.dstSubresource.layerCount = 1;

			imgBlit.dstOffsets[0] = { 0, 0, 0 };
			imgBlit.dstOffsets[1].x = m_Desc._uiWidth;
			imgBlit.dstOffsets[1].y = m_Desc._uiHeight;
			imgBlit.dstOffsets[1].z = 1;

			vkCmdBlitImage(
				cb,
				att._pAttachmentTexture->GetImage(),
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				att._pRenderTargetTexture->GetImage(),
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1,
				&imgBlit,
				att._pAttachmentTexture->GetDesc()._filtering
			);

			att._pAttachmentTexture->TransitionImageToLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, cb);
			att._pRenderTargetTexture->TransitionImageToLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, cb);
		}

		//Depth blit
		m_DepthAttachment->_pAttachmentTexture->TransitionImageToLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, cb);
		m_DepthAttachment->_pRenderTargetTexture->TransitionImageToLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, cb);

		VkImageBlit imgBlit;

		imgBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		imgBlit.srcSubresource.mipLevel = 0;
		imgBlit.srcSubresource.baseArrayLayer = 0;
		imgBlit.srcSubresource.layerCount = 1;

		imgBlit.srcOffsets[0] = { 0, 0, 0 };
		imgBlit.srcOffsets[1].x = m_Desc._uiWidth;
		imgBlit.srcOffsets[1].y = m_Desc._uiHeight;
		imgBlit.srcOffsets[1].z = 1;

		imgBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		imgBlit.dstSubresource.mipLevel = 0;
		imgBlit.dstSubresource.baseArrayLayer = 0;
		imgBlit.dstSubresource.layerCount = 1;

		imgBlit.dstOffsets[0] = { 0, 0, 0 };
		imgBlit.dstOffsets[1].x = m_Desc._uiWidth;
		imgBlit.dstOffsets[1].y = m_Desc._uiHeight;
		imgBlit.dstOffsets[1].z = 1;

		vkCmdBlitImage(
			cb,
			m_DepthAttachment->_pAttachmentTexture->GetImage(),
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			m_DepthAttachment->_pRenderTargetTexture->GetImage(),
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&imgBlit,
			m_DepthAttachment->_pAttachmentTexture->GetDesc()._filtering
		);

		m_DepthAttachment->_pAttachmentTexture->TransitionImageToLayout(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, cb);
		m_DepthAttachment->_pRenderTargetTexture->TransitionImageToLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, cb);
	}

	void Framebuffer::UpdateDescriptor(DescriptorSetLayoutHandle h)
	{
		for (uint32_t i = 0; i < m_vColorAttachments.size(); i++)
		{
			VkDescriptorImageInfo imageInfo;
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
			imageInfo.imageView = m_vColorAttachments[i]._pRenderTargetTexture->GetImageView();
			imageInfo.sampler = m_vColorAttachments[i]._pRenderTargetTexture->GetSampler();

			DescriptorFactory::UpdateDescriptor(m_mDescriptorHandles[h], &imageInfo, i);
		}

		VkDescriptorImageInfo imageInfo;
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
		imageInfo.imageView = m_DepthAttachment->_pRenderTargetTexture->GetImageView();
		imageInfo.sampler = m_DepthAttachment->_pRenderTargetTexture->GetSampler();

		DescriptorFactory::UpdateDescriptor(m_mDescriptorHandles[h], &imageInfo, m_vColorAttachments.size());
	}
}