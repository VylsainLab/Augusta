#include <Augusta/Mesh.h>
#include <Augusta/Utils.h>

namespace aug
{
	Mesh::Mesh(SMeshDesc desc)
	{
		VmaMemoryUsage memoryUsage;
		switch (desc._usage)
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

		m_uiVertexCount = desc._vertexCount;
		m_pVertexBuffer = std::make_unique<Buffer>(
			static_cast<uint64_t>(desc._vertexCount* desc._pFormat->GetStride()),
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
			memoryUsage,
			desc._vertexData);
		
		if (desc._indexCount >= 0 && desc._indexData != nullptr)
		{
			m_uiIndexCount = desc._indexCount;
			m_pIndexBuffer = std::make_unique<Buffer>((uint64_t)(m_uiIndexCount * sizeof(uint32_t)), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY, (void*)desc._indexData);
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

	std::shared_ptr<Mesh> CreateSphereMesh(uint32_t uiVertexFormatMask, uint32_t uiSubdivision, float fRadius, bool bInwardNormals)
	{
		VertexFormat format(uiVertexFormatMask);

		uint32_t uiNbVertices = uiSubdivision * uiSubdivision;
		uint32_t uiNbIndices = (uiNbVertices-1)*6;
		//face geometry patch
		std::vector<std::byte> vVertices;
		vVertices.reserve(uiNbVertices * format.GetStride());
		std::vector<uint32_t> vIndices;
		vIndices.reserve(uiNbIndices);
		for (uint32_t i = 0; i < uiSubdivision; ++i)
		{
			for (uint32_t j = 0; j < uiSubdivision; ++j)
			{
				glm::vec2 uv(float(j) / (uiSubdivision - 1), float(i) / (uiSubdivision - 1));
				float alpha = 2.f * PI * uv.x;
				float beta = PI * (-0.5f + uv.y);
				glm::vec3 pos = fRadius * glm::vec3(-cos(beta) * cos(alpha), sin(beta), cos(beta) * sin(alpha));
				glm::vec3 normal = (bInwardNormals ? -1.f : 1.f) * glm::normalize(pos);
				uv *= -1;

				if ((uiVertexFormatMask & VERTEX_FORMAT_POS_VEC3F32) == VERTEX_FORMAT_POS_VEC3F32)
				{
					const std::byte* pByte = reinterpret_cast<std::byte*>(&pos);
					vVertices.insert(vVertices.end(), pByte, pByte+sizeof(glm::vec3));
				}
				
				if ((uiVertexFormatMask & VERTEX_FORMAT_NORMAL_VEC3F32) == VERTEX_FORMAT_NORMAL_VEC3F32)
				{
					const std::byte* pByte = reinterpret_cast<std::byte*>(&normal);
					vVertices.insert(vVertices.end(), pByte, pByte + sizeof(glm::vec3));
				}

				if ((uiVertexFormatMask & VERTEX_FORMAT_UV_VEC2F32) == VERTEX_FORMAT_UV_VEC2F32)
				{
					const std::byte* pByte = reinterpret_cast<std::byte*>(&uv);
					vVertices.insert(vVertices.end(), pByte, pByte + sizeof(glm::vec2));
				}				

				if (i < uiSubdivision - 1 && j < uiSubdivision - 1)
				{
					if (i != 0)//a single triangle is enough at poles
					{
						uint32_t i0 = i * uiSubdivision + j;
						uint32_t i1 = (i + 1) * uiSubdivision + j;
						uint32_t i2 = i * uiSubdivision + j + 1;

						if (bInwardNormals)
						{
							vIndices.push_back(i0);	vIndices.push_back(i1);	vIndices.push_back(i2);
						}
						else
						{
							vIndices.push_back(i0);	vIndices.push_back(i2);	vIndices.push_back(i1);
						}
					}

					if (i != uiSubdivision - 1)
					{
						uint32_t i0 = (i + 1) * uiSubdivision + j;
						uint32_t i1 = (i + 1) * uiSubdivision + j + 1;
						uint32_t i2 = i * uiSubdivision + j + 1;

						if (bInwardNormals)
						{
							vIndices.push_back(i0);	vIndices.push_back(i1);	vIndices.push_back(i2);
						}
						else
						{
							vIndices.push_back(i0);	vIndices.push_back(i2);	vIndices.push_back(i1);
						}
					}
				}
			}
		}


		aug::SMeshDesc desc;
		desc._usage = aug::MESH_USAGE_STATIC;
		desc._pFormat = &format;
		desc._vertexCount = uiNbVertices;
		desc._vertexData = vVertices.data();
		desc._indexCount = static_cast<uint32_t>(vIndices.size());
		desc._indexData = vIndices.data();
		return std::make_shared<Mesh>(desc);
	}
}