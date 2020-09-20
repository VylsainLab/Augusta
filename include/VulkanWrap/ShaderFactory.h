#ifndef VKW_SHADERFACTORY_H
#define VKW_SHADERFACTORY_H

#include <vulkan/vulkan.h>
#include <vector>
#include <string>

namespace vkw
{
	class ShaderModule
	{
	public:
		ShaderModule(const std::string& filepath, VkShaderStageFlagBits stage, const std::string& entryPointName);
		~ShaderModule();

		//VkShaderModule GetShaderModuleHandle() const { return m_VkShaderModule; }
		const VkPipelineShaderStageCreateInfo& GetPipelineShaderModuleCreateInfo();

	protected:
		std::vector<char> ReadFile(const std::string& filepath);
		VkShaderModule m_VkShaderModule;
		VkShaderStageFlagBits m_VkShaderStageFlag;
		std::string m_strEntryPointName;
	};
}

#endif