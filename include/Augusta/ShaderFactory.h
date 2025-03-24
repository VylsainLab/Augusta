#ifndef AUG_SHADERFACTORY_H
#define AUG_SHADERFACTORY_H

#include <vulkan/vulkan.h>
#include <shaderc/shaderc.hpp>
#include <vector>
#include <map>
#include <string>

namespace aug
{
	//Object designed to load a GLSL file, compile it into a Spir-V code and create the related VkShaderModule
	class ShaderModule
	{
	public:
		ShaderModule(const std::string& filepath, VkShaderStageFlagBits stage);
		~ShaderModule();

		const VkPipelineShaderStageCreateInfo GetPipelineShaderModuleCreateInfo();
		const VkShaderStageFlagBits GetShaderStageFlagBits();

	protected:
		std::string ReadFile(const std::string& filepath);
		std::vector<uint32_t> CompileFile(const std::string& source_name,
			shaderc_shader_kind kind,
			const std::string& source,
			bool optimize = false);

		VkShaderModule m_VkShaderModule;
		VkShaderStageFlagBits m_VkShaderStageFlag;
		VkPipelineShaderStageCreateInfo m_VkPipelineShaderStageCreateInfo;
		std::string m_strEntryPointName;
	};


	//Object containing all shader stages, with their constants and descriptors
	class Shader
	{
	public:

		Shader(const char* szFileName, int32_t stageBitMask);
		~Shader();

		uint32_t GetStageCount() { return m_vVkPipelineShaderStageCreateInfo.size(); }
		const VkPipelineShaderStageCreateInfo* GetPipelineShaderStagesCreateInfo() { return m_vVkPipelineShaderStageCreateInfo.data(); }

		static void SetPath(const char* szPath) { m_sPath = szPath; }

	protected:
		static std::string m_sPath;
		std::map<int32_t, ShaderModule*> m_mModules;
		std::vector<VkPipelineShaderStageCreateInfo> m_vVkPipelineShaderStageCreateInfo;
	};
}

#endif