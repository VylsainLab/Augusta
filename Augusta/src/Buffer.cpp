#include <Augusta/Buffer.h>
#include <Augusta/Context.h>
#include <Augusta/MemoryAllocator.h>
#include <cassert>

namespace aug
{
	Buffer::Buffer(uint64_t size, VkBufferUsageFlags bufferUsage, VmaMemoryUsage memoryUsage, void* pData)
	{
		assert(size != 0);

		VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		bufferInfo.size = size;
		bufferInfo.usage = bufferUsage;

		//if memory is GPU only, set the buffer as the destination of the staging buffer transfer
		if ((memoryUsage & VMA_MEMORY_USAGE_GPU_ONLY) != 0) 
			bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;

		VmaAllocationCreateInfo allocCreateInfo = {};
		allocCreateInfo.usage = memoryUsage;

		vmaCreateBuffer(MemoryAllocator::m_VmaAllocator, &bufferInfo, &allocCreateInfo, &m_VkBuffer, &m_VmaAllocation, &m_VmaAllocationInfo);

		m_uiSize = size;// m_VmaAllocationInfo.size;

		if (pData != nullptr)
			CopyData(m_uiSize, pData);
	}

	Buffer::~Buffer()
	{
		vmaDestroyBuffer(MemoryAllocator::m_VmaAllocator, m_VkBuffer, m_VmaAllocation);
	}

	void Buffer::CopyData(uint64_t size, void* pData)
	{
		assert(size <= m_uiSize);

		VkMemoryPropertyFlags memFlags;
		vmaGetMemoryTypeProperties(MemoryAllocator::m_VmaAllocator, m_VmaAllocationInfo.memoryType, &memFlags);
		if ((memFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0) //CPU side memory, map it directly
		{
			void* data;
			vmaMapMemory(MemoryAllocator::m_VmaAllocator, m_VmaAllocation, &data);
			memcpy(data, pData, size);
			vmaUnmapMemory(MemoryAllocator::m_VmaAllocator, m_VmaAllocation);
		}
		else //create a cpu side buffer to map data and transfer it to final buffer
		{
			Buffer stagingBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, pData);
			
			VkCommandBuffer commandBuffer = Context::BuildSingleTimeCommandBuffer();

			VkBufferCopy copyRegion{};
			copyRegion.srcOffset = 0; // Optional
			copyRegion.dstOffset = 0; // Optional
			copyRegion.size = size;
			vkCmdCopyBuffer(commandBuffer, stagingBuffer.GetBufferHandle(), this->GetBufferHandle(), 1, &copyRegion);

			Context::SubmitAndFreeCommandBuffer(commandBuffer);
		}
	}
}