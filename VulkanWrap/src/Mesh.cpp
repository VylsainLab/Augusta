#include <VulkanWrap/Mesh.h>

namespace vkw
{
	//* MeshUsage parameter will define how often mesh data will be modified and therefore if allocated memory is on CPU or GPU side
	//* Index data is optional and draw command will depend on it
	Mesh::Mesh(MeshUsage usage, VertexFormat vertexFormat, uint32_t vertexCount, void* vertexData, uint32_t indexCount, const uint32_t* indexData)
	{
		VmaMemoryUsage memoryUsage;
		switch (usage)
		{
		case vkw::VKW_MESH_USAGE_STATIC:
			memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;
			break;
		case vkw::VKW_MESH_USAGE_DYNAMIC:
			memoryUsage = VMA_MEMORY_USAGE_CPU_ONLY;
			break;
		default:
			break;
		}

		m_uiVertexCount = vertexCount;
		m_pVertexBuffer = std::make_unique<Buffer>(
			static_cast<uint64_t>(vertexCount*vertexFormat.GetStride()), 
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
			memoryUsage,
			vertexData);
		
		if (indexCount >= 0 && indexData != nullptr)
		{
			m_uiIndexCount = indexCount;
			m_pIndexBuffer = std::make_unique<Buffer>((uint64_t)(m_uiIndexCount * sizeof(uint32_t)), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY, (void*)indexData);
		}
	}

	Mesh::~Mesh()
	{
	}

	void Mesh::Draw(const VkCommandBuffer& commandBuffer)
	{
		VkBuffer vertexBuffers[] = { m_pVertexBuffer->GetBufferHandle() };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

		if (m_pIndexBuffer != nullptr)
		{
			vkCmdBindIndexBuffer(commandBuffer, m_pIndexBuffer->GetBufferHandle(), 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(commandBuffer, m_uiIndexCount, 1, 0, 0, 0);
		}
		else
		{
			vkCmdDraw(commandBuffer, m_uiVertexCount, 1, 0, 0);
		}
	}
}