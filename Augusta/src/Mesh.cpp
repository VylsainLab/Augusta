#include <Augusta/Mesh.h>

namespace aug
{
	Mesh::Mesh(SMeshDesc desc)
	{
		VmaMemoryUsage memoryUsage;
		switch (desc.usage)
		{
		case aug::MESH_USAGE_STATIC:
			memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;
			break;
		case aug::MESH_USAGE_DYNAMIC:
			memoryUsage = VMA_MEMORY_USAGE_CPU_ONLY;
			break;
		default:
			break;
		}

		m_uiVertexCount = desc.vertexCount;
		m_pVertexBuffer = std::make_unique<Buffer>(
			static_cast<uint64_t>(desc.vertexCount* desc.pFormat->GetStride()),
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
			memoryUsage,
			desc.vertexData);
		
		if (desc.indexCount >= 0 && desc.indexData != nullptr)
		{
			m_uiIndexCount = desc.indexCount;
			m_pIndexBuffer = std::make_unique<Buffer>((uint64_t)(m_uiIndexCount * sizeof(uint32_t)), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY, (void*)desc.indexData);
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