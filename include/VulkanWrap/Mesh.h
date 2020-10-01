#ifndef VKW_MESH_H
#define VKW_MESH_H

#include <VulkanWrap/Buffer.h>
#include <VulkanWrap/VertexFormat.h>

namespace vkw
{
	enum MeshUsage
	{
		MESH_USAGE_STATIC,
		MESH_USAGE_DYNAMIC
	};

	class Mesh
	{
	public:
		Mesh(MeshUsage usage, VertexFormat vertexFormat, uint32_t vertexCount, void* vertexData, uint32_t indexCount=0, const uint32_t* indexData=nullptr);
		~Mesh();

		void Draw(const VkCommandBuffer &commandBuffer);

	private:
		uint32_t m_uiVertexCount = 0;
		std::unique_ptr<Buffer> m_pVertexBuffer = nullptr;
		uint32_t m_uiIndexCount = 0;
		std::unique_ptr<Buffer> m_pIndexBuffer = nullptr;
	};
}

#endif