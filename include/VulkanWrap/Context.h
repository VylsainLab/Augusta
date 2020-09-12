#ifndef VKW_CONTEXT_H
#define VKW_CONTEXT_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>

#ifdef NDEBUG
#define ENABLE_VALIDATION_LAYERS	false
#else
#define ENABLE_VALIDATION_LAYERS	true
#endif

namespace vkw
{
	class Context
	{
	public:
		static void Init();
		static void Release();

		static VkInstance m_VkInstance;
		static std::vector<const char*> m_vValidationLayers;

	protected:
		static bool CheckValidationLayers();
		static void CreateInstance();
		static void SetupDebugMessenger();		
 
		static VkDebugUtilsMessengerEXT m_VkDebugMessenger;		
	};
}
#endif