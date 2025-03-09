#include <VulkanWrap/MemoryAllocator.h>
#include <VulkanWrap/Context.h>
#include <stdexcept>

#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

namespace vkw
{
	VmaAllocator MemoryAllocator::m_VmaAllocator;

	void MemoryAllocator::Init()
	{
		VmaAllocatorCreateInfo createInfo = {};
		createInfo.physicalDevice = Context::m_VkPhysicalDevice;
		createInfo.device = Context::m_VkDevice;
		createInfo.instance = Context::m_VkInstance;
		if (vmaCreateAllocator(&createInfo, &m_VmaAllocator) != VK_SUCCESS)
			throw std::runtime_error("Unable to create memory allocator!");
	}

	void MemoryAllocator::Release()
	{
		vmaDestroyAllocator(m_VmaAllocator);
	}
}
