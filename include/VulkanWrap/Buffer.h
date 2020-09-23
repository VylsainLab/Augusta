#ifndef VKW_BUFFER_H
#define VKW_BUFFER_H

#include <VulkanMemoryAllocator/vk_mem_alloc.h>

namespace vkw
{
	class Buffer
	{
	public:
		Buffer(uint32_t size, VkBufferUsageFlags bufferUsage, VmaMemoryUsage memoryUsage, void* pData);
		~Buffer();

		VkBuffer GetBufferHandle() const { return m_VkBuffer; }

	private:
		VkBuffer m_VkBuffer = VK_NULL_HANDLE;
		VmaAllocation m_VmaAllocation = VK_NULL_HANDLE;
	};
}

#endif