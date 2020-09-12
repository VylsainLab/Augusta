#ifndef VKW_CONTEXT_H
#define VKW_CONTEXT_H

#include <vulkan/vulkan.h>

namespace vkw
{
	class Context
	{
	public:
		static void Init();

		static VkInstance GetInstance() { return m_VkInstance; }

	protected:
		static VkInstance m_VkInstance;
	};
}
#endif