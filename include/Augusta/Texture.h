#ifndef AUG_TEXTURE_H
#define AUG_TEXTURE_H

#include <vulkan/vulkan.h>
#include <string>
#include <vma/vk_mem_alloc.h>

namespace aug
{
	struct STextureDesc
	{
		uint32_t width = 0;
		uint32_t height = 0;
		VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
		VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
		VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;
		VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;
		VkFilter filtering = VK_FILTER_LINEAR;
		VkSamplerAddressMode samplingMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
	};

	class Texture
	{
	public:
		Texture(const std::string& strPath);
		Texture(STextureDesc& desc);
		virtual ~Texture();		

		void TransitionImageToLayout(VkImageLayout newLayout);

		VkImageView GetImageView() const { return m_VkImageView; }
		VkSampler GetSampler() const { return m_VkSampler; }
		VkFormat GetFormat() const { return m_TextureDesc.format; }

	protected:
		STextureDesc m_TextureDesc;

		VkImage m_VkImage = VK_NULL_HANDLE;
		VmaAllocation m_VmaAllocation = VK_NULL_HANDLE;
		VmaAllocationInfo m_VmaAllocationInfo;
		VkImageView m_VkImageView = VK_NULL_HANDLE;
		VkSampler m_VkSampler = VK_NULL_HANDLE;

		VkImageLayout m_CurrentImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		void LoadFromFile(const std::string& strPath);
		void CreateImage();
		void CreateImageView();
		void CreateSampler();
	};
}

#endif
