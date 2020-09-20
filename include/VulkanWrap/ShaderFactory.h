#ifndef VKW_SHADERFACTORY_H
#define VKW_SHADERFACTORY_H

#include <vulkan/vulkan.h>
#include <vector>
#include <string>

namespace vkw
{
	class ShaderFactory
	{
	public:
		static VkShaderModule CreateShaderModule(const std::string& filepath);
	protected:
		static std::vector<char> ReadFile(const std::string& filepath);
	};
}

#endif