#ifndef VKW_VERTEXFORMAT_H
#define VKW_VERTEXFORMAT_H

#include <vulkan/vulkan.hpp>
#include <vector>

namespace vkw
{
	enum VertexFormatComponents
	{
		VERTEX_FORMAT_INT32,
		VERTEX_FORMAT_UINT32,
		VERTEX_FORMAT_FLOAT32,
		VERTEX_FORMAT_VEC2F32,
		VERTEX_FORMAT_VEC3F32
	};

	// Class used to define vertex format and build appropriate creation structure for Vulkan
	// For now, single binding point for single interleaved vertex data buffer
	class VertexFormat
	{
	public:
		VertexFormat(const std::vector<VertexFormatComponents>& vComponents);

		VkPipelineVertexInputStateCreateInfo GetPipelineVertexInputStateCreateInfo() const;

		uint32_t GetStride() const { return m_Binding.stride; }

	protected:
		VkVertexInputBindingDescription m_Binding;
		std::vector<VkVertexInputAttributeDescription> m_vAttributes;
	};
}

#endif