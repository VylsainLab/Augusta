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
			case VertexFormatComponents::VERTEX_FORMAT_INT32:
				format = VK_FORMAT_R32_SINT;
				size = sizeof(int32_t);				
				break;
			case VertexFormatComponents::VERTEX_FORMAT_UINT32:
				format = VK_FORMAT_R32_UINT;
				size = sizeof(uint32_t);
				break;
			case VertexFormatComponents::VERTEX_FORMAT_FLOAT32:
				format = VK_FORMAT_R32_SFLOAT;
				size = sizeof(float);
				break;
			case VertexFormatComponents::VERTEX_FORMAT_VEC2F32:
				format = VK_FORMAT_R32G32_SFLOAT;
				size = 2*sizeof(float);
				break;
			case VertexFormatComponents::VERTEX_FORMAT_VEC3F32:
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

	VertexFormat::VertexFormat(uint32_t uiComponentMask)
	{
		m_Binding = VkVertexInputBindingDescription{};
		m_Binding.binding = 0;
		m_Binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		uint32_t uiLocation = 0;
		VkVertexInputAttributeDescription desc{};
		if ((uiComponentMask & VERTEX_FORMAT_POS_VEC3F32) == VERTEX_FORMAT_POS_VEC3F32)
		{			
			desc.location = uiLocation;
			desc.format = VK_FORMAT_R32G32B32_SFLOAT;
			desc.offset = m_Binding.stride; 	
			m_vAttributes.push_back(desc);
			m_Binding.stride += 3 * sizeof(float);
			uiLocation++;
		}

		if ((uiComponentMask & VERTEX_FORMAT_NORMAL_VEC3F32) == VERTEX_FORMAT_NORMAL_VEC3F32)
		{
			desc.location = uiLocation;
			desc.format = VK_FORMAT_R32G32B32_SFLOAT;
			desc.offset = m_Binding.stride; 		
			m_vAttributes.push_back(desc);
			m_Binding.stride += 3 * sizeof(float);
			uiLocation++;
		}

		if ((uiComponentMask & VERTEX_FORMAT_UV_VEC2F32) == VERTEX_FORMAT_UV_VEC2F32)
		{
			desc.location = uiLocation;
			desc.format = VK_FORMAT_R32G32_SFLOAT;
			desc.offset = m_Binding.stride; 	
			m_vAttributes.push_back(desc);
			m_Binding.stride += 2 * sizeof(float);
			uiLocation++;
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
