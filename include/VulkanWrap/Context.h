#ifndef VKW_CONTEXT_H
#define VKW_CONTEXT_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include <optional>

#ifdef NDEBUG
#define ENABLE_VALIDATION_LAYERS	false
#else
#define ENABLE_VALIDATION_LAYERS	true
#endif

namespace vkw
{
	struct QueueFamilyIndices
	{
		std::optional<uint32_t> uiGraphicsFamily;
		std::optional<uint32_t> uiPresentFamily;

		bool IsComplete() { return uiGraphicsFamily.has_value() && uiPresentFamily.has_value(); }
	};
	

	class Context
	{
	public:
		static void InitInstance();
		static void Init(const VkSurfaceKHR& reference_surface);
		static void Release();

		static QueueFamilyIndices GetQueueFamilies(const VkSurfaceKHR& surface);
		
		static VkInstance m_VkInstance;
		static std::vector<const char*> m_vValidationLayers;

		static VkSurfaceKHR m_ReferenceSurface;
		
		static VkDevice m_VkDevice;
		static VkPhysicalDevice m_VkPhysicalDevice;
		static VkQueue m_VkGraphicsQueue;
		static VkQueue m_VkPresentQueue;
		static std::vector<const char*> m_vDeviceExtensions;

	protected:
		static bool CheckValidationLayers();
		static void CreateInstance();
		static void SetupDebugMessenger();		

		static bool CheckDeviceExtensionSupport(const VkPhysicalDevice& device);		
		static bool IsDeviceSuitable(const VkPhysicalDevice& device, const VkSurfaceKHR& surface);
		static void PickPhysicalDevice();
		static void CreateLogicalDevice();
 
		static VkDebugUtilsMessengerEXT m_VkDebugMessenger;		
	};
}
#endif