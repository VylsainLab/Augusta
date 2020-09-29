#ifndef VKW_TEXTURE_H
#define VKW_TEXTURE_H

#include <vulkan/vulkan.h>
#include <string>
#include <VulkanMemoryAllocator/vk_mem_alloc.h>

namespace vkw
{
	class Texture
	{
	public:
		virtual ~Texture();

		void LoadFromFile(const std::string& strPath);

	protected:
		VkImage m_VkImage = VK_NULL_HANDLE;
		VmaAllocation m_VmaAllocation = VK_NULL_HANDLE;
		VmaAllocationInfo m_VmaAllocationInfo;

		void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
	};
}

#endif
