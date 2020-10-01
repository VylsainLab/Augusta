#ifndef VKW_SHADERFACTORY_H
#define VKW_SHADERFACTORY_H

#include <vulkan/vulkan.h>
#include <shaderc/shaderc.hpp>
#include <vector>
#include <string>

namespace vkw
{
	//Object designed to load a GLSL file, compile it into a Spir-V code and create the related VkShaderModule
	class ShaderModule
	{
	public:
		ShaderModule(const std::string& filepath, VkShaderStageFlagBits stage);
		~ShaderModule();

		const VkPipelineShaderStageCreateInfo GetPipelineShaderModuleCreateInfo();

	protected:
		std::string ReadFile(const std::string& filepath);
		std::vector<uint32_t> CompileFile(const std::string& source_name,
			shaderc_shader_kind kind,
			const std::string& source,
			bool optimize = false);

		VkShaderModule m_VkShaderModule;
		VkShaderStageFlagBits m_VkShaderStageFlag;
		std::string m_strEntryPointName;
	};
}

#endif