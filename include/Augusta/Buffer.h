#ifndef AUG_BUFFER_H
#define AUG_BUFFER_H

#include <vma/vk_mem_alloc.h>

namespace aug
{
	// Vulkan buffer and memory allocation wrapping object
	// Memory allocation is done via VulkanMemoryAllocator library
	class Buffer
	{
	public:
		Buffer(uint64_t size, VkBufferUsageFlags bufferUsage, VmaMemoryUsage memoryUsage, void* pData);
		~Buffer();

		void CopyData(uint64_t size, void* pData);

		VkBuffer GetBufferHandle() const { return m_VkBuffer; }
		uint64_t GetBufferSize() const { return m_uiSize; }

	private:
		uint64_t m_uiSize = 0;
		VkBuffer m_VkBuffer = VK_NULL_HANDLE;
		VmaAllocation m_VmaAllocation = VK_NULL_HANDLE;
		VmaAllocationInfo m_VmaAllocationInfo;
	};
}

#endif