#ifndef AUG_MEMORYALLOCATOR_H
#define AUG_MEMORYALLOCATOR_H

#include <vma/vk_mem_alloc.h>

namespace aug
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