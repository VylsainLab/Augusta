#pragma once

#include <vulkan/vulkan.hpp>
#include <vector>

namespace aug
{
	//usual vertex attributes
	#define VERTEX_FORMAT_POS_VEC3F32		1
	#define VERTEX_FORMAT_NORMAL_VEC3F32	2
	#define VERTEX_FORMAT_UV_VEC2F32		4	

	//more attributes for custom types
	enum class VertexFormatComponents
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
		VertexFormat(const VertexFormat& copy) = delete;
		VertexFormat(uint32_t uiComponentMask);

		VkPipelineVertexInputStateCreateInfo GetPipelineVertexInputStateCreateInfo() const;

		uint32_t GetStride() const { return m_Binding.stride; }

	protected:
		VkVertexInputBindingDescription m_Binding;
		std::vector<VkVertexInputAttributeDescription> m_vAttributes;
	};
}