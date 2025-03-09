#include <Augusta/VertexFormat.h>

namespace aug
{
	VertexFormat::VertexFormat(const std::vector<VertexFormatComponents>& vComponents)
	{
		m_Binding = VkVertexInputBindingDescription{};
		m_Binding.binding = 0;
		m_Binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		for (uint32_t i=0; i<vComponents.size(); ++i)
		{
			VkFormat format;
			uint32_t size;
			switch (vComponents[i])
			{
			case VERTEX_FORMAT_INT32:
				format = VK_FORMAT_R32_SINT;
				size = sizeof(int32_t);				
				break;
			case VERTEX_FORMAT_UINT32:
				format = VK_FORMAT_R32_UINT;
				size = sizeof(uint32_t);
				break;
			case VERTEX_FORMAT_FLOAT32:
				format = VK_FORMAT_R32_SFLOAT;
				size = sizeof(float);
				break;
			case VERTEX_FORMAT_VEC2F32:
				format = VK_FORMAT_R32G32_SFLOAT;
				size = 2*sizeof(float);
				break;
			case VERTEX_FORMAT_VEC3F32:
				format = VK_FORMAT_R32G32B32_SFLOAT;
				size = 3*sizeof(float);
				break;
			default:
				throw std::runtime_error("Unsupported vertex format component!");
				break;
			}

			m_vAttributes.push_back(VkVertexInputAttributeDescription());
			m_vAttributes.back().binding = 0;
			m_vAttributes.back().location = i;
			m_vAttributes.back().format = format;
			m_vAttributes.back().offset = m_Binding.stride; //current stride is offset			

			m_Binding.stride += size;
		}
	}

	VkPipelineVertexInputStateCreateInfo VertexFormat::GetPipelineVertexInputStateCreateInfo() const
	{
		VkPipelineVertexInputStateCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		createInfo.vertexBindingDescriptionCount = 1;
		createInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(m_vAttributes.size());
		createInfo.pVertexBindingDescriptions = &m_Binding;
		createInfo.pVertexAttributeDescriptions = m_vAttributes.data();
		return createInfo;
	}
}
