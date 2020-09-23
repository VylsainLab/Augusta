#include <VulkanWrap/Buffer.h>
#include <VulkanWrap/Context.h>
#include <VulkanWrap/MemoryAllocator.h>

namespace vkw
{
	Buffer::Buffer(uint32_t size, VkBufferUsageFlags bufferUsage, VmaMemoryUsage memoryUsage, void* pData)
	{
		VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		bufferInfo.size = size;
		bufferInfo.usage = bufferUsage;

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = memoryUsage;

		vmaCreateBuffer(MemoryAllocator::m_VmaAllocator, &bufferInfo, &allocInfo, &m_VkBuffer, &m_VmaAllocation, nullptr);

		void* data;
		vmaMapMemory(MemoryAllocator::m_VmaAllocator, m_VmaAllocation, &data);
		memcpy(data, pData, size);
		vmaUnmapMemory(MemoryAllocator::m_VmaAllocator, m_VmaAllocation);
	}

	Buffer::~Buffer()
	{
		vmaDestroyBuffer(MemoryAllocator::m_VmaAllocator, m_VkBuffer, m_VmaAllocation);
	}
}