#include <Augusta/Texture.h>
#include <Augusta/Buffer.h>
#include <Augusta/Context.h>
#include <Augusta/MemoryAllocator.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>
#include <stdexcept>

namespace aug
{
	Texture::Texture(const std::string& strPath)
	{
		LoadFromFile(strPath);
	}

	Texture::Texture(STextureDesc& desc)
	{
		m_TextureDesc = desc;
		CreateImage();
		CreateImageView();
		CreateSampler();
		TransitionImageToLayout(desc.layout);
	}

	Texture::~Texture()
	{
		vkDestroySampler(Context::m_VkDevice, m_VkSampler, nullptr);

		vkDestroyImageView(Context::m_VkDevice, m_VkImageView, nullptr);

		vmaDestroyImage(MemoryAllocator::m_VmaAllocator, m_VkImage, m_VmaAllocation);
	}

	void Texture::LoadFromFile(const std::string& strPath)
	{
		//Load file and create staging buffer
		int32_t w, h, c;
		stbi_uc* pData = stbi_load(strPath.c_str(), &w, &h, &c, STBI_rgb_alpha);
		if (pData == nullptr)
			throw std::runtime_error(std::string(std::string("Failed to load image ") + strPath));

		uint64_t uiSize = w * h * 4;
		Buffer stagingBuffer(uiSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, pData);

		stbi_image_free(pData);

		//Create and allocate image
		m_TextureDesc.aspect = VK_IMAGE_ASPECT_COLOR_BIT;
		m_TextureDesc.format = VK_FORMAT_R8G8B8A8_SRGB;
		m_TextureDesc.width = w;
		m_TextureDesc.height = h;
		m_TextureDesc.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		m_TextureDesc.memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;
		m_TextureDesc.tiling = VK_IMAGE_TILING_OPTIMAL;
		m_TextureDesc.filtering = VK_FILTER_LINEAR;
		m_TextureDesc.samplingMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		m_TextureDesc.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		CreateImage();
		
		//Transition image layout for copy
		TransitionImageToLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		//Copy buffer to image
		VkCommandBuffer commandBuffer = Context::BuildSingleTimeCommandBuffer();

		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0; // tightly packed
		region.bufferImageHeight = 0; //same
		region.imageSubresource.aspectMask = m_TextureDesc.aspect;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;
		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = { static_cast<uint32_t>(w), static_cast<uint32_t>(h), 1 };
		
		vkCmdCopyBufferToImage(	commandBuffer, stagingBuffer.GetBufferHandle(), m_VkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region );

		Context::SubmitAndFreeCommandBuffer(commandBuffer);

		//Transition image layout for sampling
		TransitionImageToLayout( m_TextureDesc.layout);

		CreateImageView();
		CreateSampler();
	}

	void Texture::CreateImage()
	{
		//Create and allocate image
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = static_cast<uint32_t>(m_TextureDesc.width);
		imageInfo.extent.height = static_cast<uint32_t>(m_TextureDesc.height);
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = m_TextureDesc.format;
		imageInfo.tiling = m_TextureDesc.tiling;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = m_TextureDesc.usage;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.flags = 0; // Optional

		VmaAllocationCreateInfo allocCreateInfo = {};
		allocCreateInfo.usage = m_TextureDesc.memoryUsage;

		if (vmaCreateImage(MemoryAllocator::m_VmaAllocator, &imageInfo, &allocCreateInfo, &m_VkImage, &m_VmaAllocation, &m_VmaAllocationInfo) != VK_SUCCESS)
			throw std::runtime_error("Failed to create image!");
	}

	void Texture::CreateImageView()
	{
		//Image view
		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = m_VkImage;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = m_TextureDesc.format;
		viewInfo.subresourceRange.aspectMask = m_TextureDesc.aspect;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(Context::m_VkDevice, &viewInfo, nullptr, &m_VkImageView) != VK_SUCCESS)
			throw std::runtime_error("Failed to create texture image view!");
	}

	void Texture::CreateSampler()
	{
		//Sampler
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = m_TextureDesc.filtering;
		samplerInfo.minFilter = m_TextureDesc.filtering;
		samplerInfo.addressModeU = m_TextureDesc.samplingMode;
		samplerInfo.addressModeV = m_TextureDesc.samplingMode;
		samplerInfo.addressModeW = m_TextureDesc.samplingMode;
		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.maxAnisotropy = 16.0f;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 0.0f;
		if (vkCreateSampler(Context::m_VkDevice, &samplerInfo, nullptr, &m_VkSampler) != VK_SUCCESS)
			throw std::runtime_error("Failed to create texture sampler!");
	}

	void Texture::TransitionImageToLayout(VkImageLayout newLayout)
	{
		VkCommandBuffer commandBuffer = Context::BuildSingleTimeCommandBuffer();	

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = m_CurrentImageLayout;
		barrier.newLayout = newLayout;

		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

		barrier.image = m_VkImage;
		barrier.subresourceRange.aspectMask = m_TextureDesc.aspect;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;
		if (m_CurrentImageLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			//We can start the transition asap but we have to wait until transition is done before we transfer data
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (m_CurrentImageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			//Wait until transfer is complete before transition and then allow shader read
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if (m_CurrentImageLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		}
		else
			throw std::invalid_argument("Unsupported layout transition!");

		vkCmdPipelineBarrier( commandBuffer, sourceStage, destinationStage, 0, 0, nullptr,	0, nullptr, 1, &barrier	);

		Context::SubmitAndFreeCommandBuffer(commandBuffer);

		m_CurrentImageLayout = newLayout;
	}
}