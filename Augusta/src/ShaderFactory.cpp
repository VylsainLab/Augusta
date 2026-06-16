#include <Augusta/ShaderFactory.h>
#include <Augusta/Context.h>
#include <stdexcept>
#include <fstream>
#include <array>

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

		std::string glsl_code = ReadFile(m_strFilePath, m_mLastModificationTimes[m_strFilePath]);

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
		for (auto& entry : m_mLastModificationTimes)
		{
			if (std::filesystem::exists(entry.first))
			{
				std::filesystem::file_time_type t = std::filesystem::last_write_time(entry.first);
				if (t > entry.second)
					return true;
			}
		}

		return false;
	}

	std::string ShaderModule::ReadFile(const std::string& filepath, std::filesystem::file_time_type& t)
	{
		std::ifstream file(filepath, std::ios::ate | std::ios::binary);

		if (!file.is_open())
			throw std::runtime_error("failed to open file!");

		t = std::filesystem::last_write_time(filepath);

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
			options.SetOptimizationLevel(shaderc_optimization_level_performance);

		options.SetIncluder(std::make_unique<ShaderIncluder>(this));

		shaderc::PreprocessedSourceCompilationResult res = compiler.PreprocessGlsl(source, kind, source_name.c_str(), options);
		shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(res.begin(), kind, source_name.c_str(), options);

		if (module.GetCompilationStatus() != shaderc_compilation_status_success) 
			throw std::runtime_error(module.GetErrorMessage());

		return { module.cbegin(), module.cend() };
	}

	ShaderIncluder::ShaderIncluder(ShaderModule* pShader)
	{
		m_pShaderModule = pShader;
	}

	shaderc_include_result* ShaderIncluder::GetInclude(const char* requested_source, shaderc_include_type type, const char* requesting_source, size_t include_depth)
	{
		std::array<std::string, 2>* aContent = new std::array<std::string, 2>();
		(*aContent)[0] = Shader::GetDirectory() + "/" + requested_source;
		(*aContent)[1] = ShaderModule::ReadFile((*aContent)[0], m_pShaderModule->GetLastModificationTime((*aContent)[0]));

		shaderc_include_result* pRes = new shaderc_include_result();
		pRes->source_name = (*aContent)[0].data();
		pRes->source_name_length = (*aContent)[0].size();

		pRes->content = (*aContent)[1].data();
		pRes->content_length = (*aContent)[1].size();

		pRes->user_data = aContent;

		return pRes;
	}

	void ShaderIncluder::ReleaseInclude(shaderc_include_result* data)
	{
		delete data->user_data;
		delete data;
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
				throw std::runtime_error("Unsupported shader stage!");
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
