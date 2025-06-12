#ifndef AUG_CONTEXT_H
#define AUG_CONTEXT_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include <optional>

#ifdef NDEBUG
#define ENABLE_VALIDATION_LAYERS	false
#else
#define ENABLE_VALIDATION_LAYERS	true
#endif

namespace aug
{
	struct QueueFamilyIndices
	{
		std::optional<uint32_t> uiGraphicsFamily;
		std::optional<uint32_t> uiPresentFamily;

		bool IsComplete() { return uiGraphicsFamily.has_value() && uiPresentFamily.has_value(); }
	};
	
	//Class containing all the resources shared accross the whole application like Instance, Device, etc...
	//For now, Context also contains some resources and methods related to the main rendering thread (queues and command pools),
	//this will probably change as we move towards a multithreaded rendering engine
	class Context
	{
	public:
		static void InitInstance();
		static void Init(const VkSurfaceKHR& reference_surface);
		static void Release();

		static QueueFamilyIndices GetQueueFamilies(const VkSurfaceKHR& surface);

		static VkCommandBuffer BuildSingleTimeCommandBuffer();
		static void SubmitAndFreeCommandBuffer(VkCommandBuffer commandBuffer);
		
		static VkInstance m_VkInstance;
		static std::vector<const char*> m_vValidationLayers;

		static VkSurfaceKHR m_ReferenceSurface;
		
		static VkDevice m_VkDevice;
		static VkPhysicalDevice m_VkPhysicalDevice;
		static VkQueue m_VkGraphicsQueue;
		static VkQueue m_VkPresentQueue;
		static VkCommandPool m_VkCommandPool;
		static std::vector<const char*> m_vDeviceExtensions;
		static QueueFamilyIndices m_QueueFamilies;

	protected:
		static bool CheckValidationLayers();
		static void CreateInstance();
		static void SetupDebugMessenger();		

		static bool CheckDeviceExtensionSupport(const VkPhysicalDevice& device);		
		static bool IsDeviceSuitable(const VkPhysicalDevice& device);
		static void PickPhysicalDevice();
		static void CreateLogicalDevice();
		static void CreateCommandPool();
 
		static VkDebugUtilsMessengerEXT m_VkDebugMessenger;		
	};
}
#endif