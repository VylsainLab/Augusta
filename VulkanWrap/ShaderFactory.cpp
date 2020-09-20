#include <VulkanWrap/ShaderFactory.h>
#include <VulkanWrap/Context.h>
#include <stdexcept>
#include <fstream>

namespace vkw
{
	ShaderModule::ShaderModule(const std::string& filepath, VkShaderStageFlagBits stageFlag, const std::string& entryPointName)
	{
		std::vector<char> code = ReadFile(filepath);

		VkShaderModuleCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		if (vkCreateShaderModule(vkw::Context::m_VkDevice, &createInfo, nullptr, &m_VkShaderModule) != VK_SUCCESS)
			throw std::runtime_error("Failed to create shader module!");

		m_VkShaderStageFlag = stageFlag;
		m_strEntryPointName = entryPointName;

		
	}

	ShaderModule::~ShaderModule()
	{
		vkDestroyShaderModule(vkw::Context::m_VkDevice, m_VkShaderModule, nullptr);
	}

	const VkPipelineShaderStageCreateInfo& ShaderModule::GetPipelineShaderModuleCreateInfo()
	{
		VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo = {};
		pipelineShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		pipelineShaderStageCreateInfo.stage = m_VkShaderStageFlag;
		pipelineShaderStageCreateInfo.module = m_VkShaderModule;
		pipelineShaderStageCreateInfo.pName = m_strEntryPointName.c_str();
		return pipelineShaderStageCreateInfo;
	}

	std::vector<char> ShaderModule::ReadFile(const std::string& filepath)
	{
		std::ifstream file(filepath, std::ios::ate | std::ios::binary);

		if (!file.is_open())
			throw std::runtime_error("failed to open file!");

		size_t fileSize = (size_t)file.tellg();
		std::vector<char> buffer(fileSize);
		file.seekg(0);
		file.read(buffer.data(), fileSize);
		file.close();

		return buffer;
	}
}
