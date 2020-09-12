#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <VulkanWrap/Window.h>
#include <VulkanWrap/Context.h>
#include <vector>
#include <memory>

#ifdef NDEBUG
#define ENABLE_VALIDATION_LAYERS	false
#else
#define ENABLE_VALIDATION_LAYERS	true
#endif

namespace vkw
{
	class Application
	{
	public:
		Application();
		virtual ~Application();
		
		vkw::Window* AddNewWindow(VkInstance &instance, const char* szName, uint16_t uiWidth, uint16_t uiHeight);

		void Run();

		virtual void Render() = 0;

	protected:
		void UpdateWindowsList();

		//Vulkan context
		bool CheckValidationLayers();
		void CreateInstance();
		void SetupDebugMessenger();
		void InitVulkan();
		void ReleaseVulkan();

		static bool m_bGLFWInitialized;
		VkInstance m_VkInstance;
		VkDebugUtilsMessengerEXT m_VkDebugMessenger;
		std::vector<std::shared_ptr<vkw::Window>> m_vWindows;

		std::vector<const char*> m_vValidationLayers = { "VK_LAYER_KHRONOS_validation" };
	};
}
