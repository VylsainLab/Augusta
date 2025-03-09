#ifndef AUG_MESH_H
#define AUG_MESH_H

#include <Augusta/Buffer.h>
#include <Augusta/VertexFormat.h>

namespace aug
{
	enum MeshUsage
	{
		MESH_USAGE_STATIC,
		MESH_USAGE_DYNAMIC
	};

	//* MeshUsage parameter will define how often mesh data will be modified and therefore if allocated memory is on CPU or GPU side
	//* Index data is optional and draw command will depend on it
	struct SMeshDesc
	{
		MeshUsage usage;
		VertexFormat* pFormat;
		uint32_t vertexCount = 0;
		void* vertexData = nullptr;
		uint32_t indexCount = 0;
		const uint32_t* indexData = nullptr;
	};

	class Mesh
	{
	public:
		Mesh(SMeshDesc desc);
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