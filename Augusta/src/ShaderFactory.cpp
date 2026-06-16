#include <Augusta/ShaderFactory.h>
#include <Augusta/Context.h>
#include <stdexcept>
#include <fstream>

namespace aug
{
	ShaderModule::ShaderModule(const std::string& strName, const std::string& filepath, VkShaderStageFlagBits stageFlag)
	{
		m_strName = strName;
		m_strFilePath = filepath;
		m_VkShaderStageFlag = stageFlag;
		ReadAndCompileModule();
	}

	ShaderModule::~ShaderModule()
	{
		CleanModule();
	}

	void ShaderModule::ReadAndCompileModule()
	{
		CleanModule();

		m_LastModificationTime = std::filesystem::last_write_time(m_strFilePath);

		std::string glsl_code = ReadFile(m_strFilePath);

		shaderc_shader_kind shaderKind;
		switch (m_VkShaderStageFlag)
		{
		case VK_SHADER_STAGE_VERTEX_BIT:
			shaderKind = shaderc_vertex_shader;
			break;
		case VK_SHADER_STAGE_FRAGMENT_BIT:
			shaderKind = shaderc_fragment_shader;
			break;
		case VK_SHADER_STAGE_COMPUTE_BIT:
			shaderKind = shaderc_compute_shader;
			break;
		case VK_SHADER_STAGE_GEOMETRY_BIT:
			shaderKind = shaderc_geometry_shader;
			break;
		case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
			shaderKind = shaderc_tess_control_shader;
			break;
		case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
			shaderKind = shaderc_tess_evaluation_shader;
			break;
		default:
			throw std::runtime_error("Unsuported shader kind.");
			break;
		}

		std::vector<uint32_t> spirv_code = CompileFile(m_strFilePath, shaderKind, glsl_code, true);

		VkShaderModuleCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = spirv_code.size() * sizeof(uint32_t);
		createInfo.pCode = spirv_code.data();

		if (vkCreateShaderModule(aug::Context::m_VkDevice, &createInfo, nullptr, &m_VkShaderModule) != VK_SUCCESS)
			throw std::runtime_error("Failed to create shader module!");

		m_strEntryPointName = "main"; //forced by shaderc library

		m_VkPipelineShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		m_VkPipelineShaderStageCreateInfo.stage = m_VkShaderStageFlag;
		m_VkPipelineShaderStageCreateInfo.module = m_VkShaderModule;
		m_VkPipelineShaderStageCreateInfo.pName = m_strEntryPointName.c_str();
	}

	void ShaderModule::CleanModule()
	{
		if(m_VkShaderModule)
			vkDestroyShaderModule(aug::Context::m_VkDevice, m_VkShaderModule, nullptr);
	}

	const VkPipelineShaderStageCreateInfo ShaderModule::GetPipelineShaderModuleCreateInfo()
	{		
		return m_VkPipelineShaderStageCreateInfo;
	}

	bool ShaderModule::CheckForModifications()
	{
		if (std::filesystem::exists(m_strFilePath))
		{
			std::filesystem::file_time_type t = std::filesystem::last_write_time(m_strFilePath);
			if (t > m_LastModificationTime)
				return true;
		}

		return false;
	}

	std::string ShaderModule::ReadFile(const std::string& filepath)
	{
		std::ifstream file(filepath, std::ios::ate | std::ios::binary);

		if (!file.is_open())
			throw std::runtime_error("failed to open file!");

		size_t fileSize = (size_t)file.tellg();
		std::vector<char> buffer(fileSize);
		file.seekg(0);
		file.read(buffer.data(), fileSize);

		file.close();

		return std::string(buffer.begin(),buffer.end());
	}

	// Compiles a shader to a SPIR-V binary. Returns the binary as
	// a vector of 32-bit words.
	std::vector<uint32_t> ShaderModule::CompileFile(const std::string& source_name,
		shaderc_shader_kind kind,
		const std::string& source,
		bool optimize) 
	{
		shaderc::Compiler compiler;
		shaderc::CompileOptions options;

		if (optimize) 
			options.SetOptimizationLevel(shaderc_optimization_level_size);

		shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(source, kind, source_name.c_str(), options);

		if (module.GetCompilationStatus() != shaderc_compilation_status_success) 
			throw std::runtime_error(module.GetErrorMessage());

		return { module.cbegin(), module.cend() };
	}

	std::string Shader::m_sDirectory = "";

	Shader::Shader(const SShaderDesc& desc)
	{
		m_Desc = desc;

		for (auto& stage : desc.vShaderStages)
		{
			std::string strExtension;
			switch (stage.first)
			{
			case VK_SHADER_STAGE_VERTEX_BIT:
				strExtension = ".vert";
				break;

			case VK_SHADER_STAGE_FRAGMENT_BIT:
				strExtension = ".frag";
				break;

			default:
				std::runtime_error("Unsupported shader stage!");
				break;
			}

			std::string strName = stage.second + strExtension;
			m_mModules[stage.first] = std::make_unique<ShaderModule>(strName, m_sDirectory + strName, stage.first);
			m_vVkPipelineShaderStageCreateInfo.push_back(m_mModules[stage.first]->GetPipelineShaderModuleCreateInfo());
		}
	}

	Shader::~Shader()
	{
	}

	bool Shader::CheckForModifications()
	{
		bool bRet = false;

		for (auto& it : m_mModules)
		{
			if (it.second->CheckForModifications()) //TODO extend to all asset types
			{
				printf("\nShader module has changed: %s", it.second->GetName());
				it.second->ReadAndCompileModule();
				bRet = true;
			}
		}

		m_vVkPipelineShaderStageCreateInfo.clear();
		for (auto& it : m_mModules)
			m_vVkPipelineShaderStageCreateInfo.push_back(it.second->GetPipelineShaderModuleCreateInfo());

		return bRet;
	}
}
