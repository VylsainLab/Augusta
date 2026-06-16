#ifndef AUG_SHADERFACTORY_H
#define AUG_SHADERFACTORY_H

#include <vulkan/vulkan.h>
#include <shaderc/shaderc.hpp>
#include <vector>
#include <map>
#include <string>
#include <filesystem>

namespace aug
{
	struct SShaderDesc
	{
		std::vector< std::pair<VkShaderStageFlagBits, std::string> > vShaderStages;
	};

	//Object designed to load a GLSL file, compile it into a Spir-V code and create the related VkShaderModule
	class ShaderModule
	{
	public:
		ShaderModule(const std::string& strName, const std::string& filepath, VkShaderStageFlagBits stage);
		~ShaderModule();

		void ReadAndCompileModule();		

		const VkPipelineShaderStageCreateInfo GetPipelineShaderModuleCreateInfo();
		const VkShaderStageFlagBits GetShaderStageFlagBits();

		bool CheckForModifications();

		const char* GetName() { return m_strName.c_str(); }

	protected:
		void CleanModule();

		std::string ReadFile(const std::string& filepath);
		std::vector<uint32_t> CompileFile(const std::string& source_name,
			shaderc_shader_kind kind,
			const std::string& source,
			bool optimize = false);
		
		std::string m_strName;
		VkShaderModule m_VkShaderModule;
		VkShaderStageFlagBits m_VkShaderStageFlag;
		VkPipelineShaderStageCreateInfo m_VkPipelineShaderStageCreateInfo;
		std::string m_strEntryPointName;
		std::string m_strFilePath;
		std::filesystem::file_time_type m_LastModificationTime;
	};


	//Object containing all shader stages, with their constants and descriptors
	class Shader
	{
	public:
		Shader(const SShaderDesc& desc);
		~Shader();

		uint32_t GetStageCount() { return static_cast<uint32_t>(m_vVkPipelineShaderStageCreateInfo.size()); }
		const VkPipelineShaderStageCreateInfo* GetPipelineShaderStagesCreateInfo() { return m_vVkPipelineShaderStageCreateInfo.data(); }

		bool CheckForModifications();

		static void SetDirectory(const char* szPath) { m_sDirectory = szPath; }

	protected:
		static std::string m_sDirectory;

		SShaderDesc m_Desc;
		std::map<int32_t, std::unique_ptr<ShaderModule>> m_mModules;
		std::vector<VkPipelineShaderStageCreateInfo> m_vVkPipelineShaderStageCreateInfo;
	};
}

#endif