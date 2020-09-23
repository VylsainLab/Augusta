#ifndef VKW_MEMORYALLOCATOR_H
#define VKW_MEMORYALLOCATOR_H

#include <VulkanMemoryAllocator/vk_mem_alloc.h>

namespace vkw
{
	class MemoryAllocator
	{
	public:
		static void Init();
		static void Release();

		static VmaAllocator m_VmaAllocator;
	};
}

#endif