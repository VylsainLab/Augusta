#include <VulkanWrap/Context.h>
#include <VulkanWrap/Window.h>
#include <VulkanWrap/Context.h>
#include <vector>
#include <memory>

namespace vkw
{
	class Application
	{
	public:
		Application(std::vector<SWindowDesc> vWindowList);
		virtual ~Application();

		void Run();

		virtual void Render() = 0;

	protected:
		void UpdateWindowsList();

		static bool m_bGLFWInitialized;
		std::vector<std::shared_ptr<vkw::Window>> m_vWindows;
	};
}
