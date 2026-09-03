#pragma once

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
		std::vector< std::pair<VkShaderStageFlagBits, std::string> > _vShaderStages;
	};

	//Object designed to load a GLSL file, compile it into a Spir-V code and create the related VkShaderModule
	class ShaderModule
	{
	public:
		ShaderModule(const std::string& strName, const std::string& filepath, VkShaderStageFlagBits stage);
		ShaderModule(const ShaderModule& copy) = delete;
		virtual ~ShaderModule();

		[[nodiscard]] bool ReadAndCompileModule();

		const VkPipelineShaderStageCreateInfo GetPipelineShaderModuleCreateInfo();
		const VkShaderStageFlagBits GetShaderStageFlagBits();

		bool CheckForModifications();

		const char* GetName() { return m_strName.c_str(); }
		const VkShaderModule GetModule() { return m_VkShaderModule; }

		std::filesystem::file_time_type& GetLastModificationTime(const std::string& strPath) { return m_mLastModificationTimes[strPath]; }

		static [[nodiscard]] bool ReadFile(const std::string& filepath, std::string& strDst, std::filesystem::file_time_type& t);

	protected:
		void CleanModule();		
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
		std::map<std::string,std::filesystem::file_time_type> m_mLastModificationTimes;
	};

	class ShaderIncluder : public shaderc::CompileOptions::IncluderInterface
	{
	public:
		ShaderIncluder(ShaderModule* pShader);

		shaderc_include_result* GetInclude(
			const char* requested_source,
			shaderc_include_type type,
			const char* requesting_source,
			size_t include_depth);

		void ReleaseInclude(shaderc_include_result* data);

		ShaderModule* m_pShaderModule = nullptr;
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

		static void AddDirectory(const char* szPath) { m_vDirectories.push_back(szPath); }
		//static const std::vector<std::string>& GetDirectories() { return m_vDirectories; }

		static std::string FindShader(const std::string& strFilename);

	protected:
		static std::vector<std::string> m_vDirectories;

		SShaderDesc m_Desc;
		std::map<int32_t, std::unique_ptr<ShaderModule>> m_mModules;
		std::vector<VkPipelineShaderStageCreateInfo> m_vVkPipelineShaderStageCreateInfo;
	};
}