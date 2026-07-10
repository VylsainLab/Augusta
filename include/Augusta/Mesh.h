#ifndef AUG_MESH_H
#define AUG_MESH_H

#include <Augusta/Buffer.h>
#include <Augusta/VertexFormat.h>
#include <Augusta/Material.h>

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
		MeshUsage _usage;
		VertexFormat* _pFormat;
		uint32_t _vertexCount = 0;
		void* _vertexData = nullptr;
		uint32_t _indexCount = 0;
		const uint32_t* _indexData = nullptr;
		std::shared_ptr<Material> _pMaterial = nullptr;
	};

	class Mesh
	{
	public:
		Mesh(SMeshDesc desc);
		~Mesh();

		void Draw(const VkCommandBuffer &commandBuffer);

		std::shared_ptr<Material> m_pMaterial = nullptr;
	private:
		uint32_t m_uiVertexCount = 0;
		std::unique_ptr<Buffer> m_pVertexBuffer = nullptr;
		uint32_t m_uiIndexCount = 0;
		std::unique_ptr<Buffer> m_pIndexBuffer = nullptr;		
	};
}

#endif