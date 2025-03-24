#include <Augusta/ShaderFactory.h>
#include <Augusta/Context.h>
#include <stdexcept>
#include <fstream>

namespace aug
{
	ShaderModule::ShaderModule(const std::string& filepath, VkShaderStageFlagBits stageFlag)
	{
		std::string glsl_code = ReadFile(filepath);

		shaderc_shader_kind shaderKind;
		switch (stageFlag)
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

		std::vector<uint32_t> spirv_code = CompileFile(filepath, shaderKind, glsl_code, true);

		VkShaderModuleCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = spirv_code.size()*sizeof(uint32_t);
		createInfo.pCode = spirv_code.data();

		if (vkCreateShaderModule(aug::Context::m_VkDevice, &createInfo, nullptr, &m_VkShaderModule) != VK_SUCCESS)
			throw std::runtime_error("Failed to create shader module!");

		m_VkShaderStageFlag = stageFlag;	
		m_strEntryPointName = "main"; //forced by shaderc library

		m_VkPipelineShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		m_VkPipelineShaderStageCreateInfo.stage = m_VkShaderStageFlag;
		m_VkPipelineShaderStageCreateInfo.module = m_VkShaderModule;
		m_VkPipelineShaderStageCreateInfo.pName = m_strEntryPointName.c_str();
	}

	ShaderModule::~ShaderModule()
	{
		vkDestroyShaderModule(aug::Context::m_VkDevice, m_VkShaderModule, nullptr);
	}

	const VkPipelineShaderStageCreateInfo ShaderModule::GetPipelineShaderModuleCreateInfo()
	{		
		return m_VkPipelineShaderStageCreateInfo;
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


	std::string Shader::m_sPath = "";

	Shader::Shader(const char* szFileName, int32_t stageBitMask)
	{
		//TODO crade, à nettoyer
		if (stageBitMask & VK_SHADER_STAGE_VERTEX_BIT)
		{
			m_mModules[VK_SHADER_STAGE_VERTEX_BIT] = new ShaderModule(m_sPath + szFileName + ".vert", VK_SHADER_STAGE_VERTEX_BIT);
			m_vVkPipelineShaderStageCreateInfo.push_back(m_mModules[VK_SHADER_STAGE_VERTEX_BIT]->GetPipelineShaderModuleCreateInfo());
		}
		if (stageBitMask & VK_SHADER_STAGE_FRAGMENT_BIT)
		{
			m_mModules[VK_SHADER_STAGE_FRAGMENT_BIT] = new ShaderModule(m_sPath + szFileName + ".frag", VK_SHADER_STAGE_FRAGMENT_BIT);
			m_vVkPipelineShaderStageCreateInfo.push_back(m_mModules[VK_SHADER_STAGE_FRAGMENT_BIT]->GetPipelineShaderModuleCreateInfo());
		}
	}

	Shader::~Shader()
	{
		for (auto item : m_mModules)
			delete item.second;
	}
}
