#ifndef AUG_TEXTURE_H
#define AUG_TEXTURE_H

#include <Augusta/Buffer.h>
#include <vulkan/vulkan.h>
#include <string>
#include <vma/vk_mem_alloc.h>
#include <memory>
#include <vector>
#include <unordered_map>

namespace aug
{
	struct STextureDesc
	{
		uint32_t width = 0;
		uint32_t height = 0;
		uint32_t levels = 0;
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
		Texture(STextureDesc& desc, Buffer* pBuffer = nullptr);
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
		VkSampler m_VkSampler = VK_NULL_HANDLE; //TODO decouple samplers from textures

		VkImageLayout m_CurrentImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		void CreateImage();
		void CreateImageView();
		void CreateSampler();
	};

	class TextureFactory
	{
	public:
		static void AddTexturePath(const std::string& strPath);
		static void SetTextureExtension(const std::string& strExt);

		static std::shared_ptr<Texture> LoadTextureFromFile(const std::string& strPath);

	protected:
		static std::string FindTexture(const std::string& strDirPath, const std::string& strFilename);

		static std::shared_ptr<Texture> LoadTextureFromDDS(const std::string& strPath);
		static std::shared_ptr<Texture> LoadTextureFromSTBI(const std::string& strPath);

		static std::vector<std::string> m_vPaths;
		static std::string m_sForcedExtension;
		static std::unordered_map<std::string, std::weak_ptr<Texture>> m_mTextureDictionary;
	};
}

#endif
