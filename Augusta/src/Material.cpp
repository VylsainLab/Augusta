#include <Augusta/Material.h>
#include <Augusta/Context.h>
#include <Augusta/Pipeline.h>
#include <stdexcept>
#include <deque>

namespace aug
{
	Material::Material()
	{
	}

	Material::~Material()
	{
	}

	void Material::UpdateDescriptor(DescriptorSetLayoutHandle h)
	{
		VkDescriptorBufferInfo bufferInfo;
		bufferInfo.buffer = m_pMaterialUniformBuffer->GetBufferHandle();
		bufferInfo.offset = 0;
		bufferInfo.range = m_pMaterialUniformBuffer->GetBufferSize();
		DescriptorFactory::UpdateDescriptor(m_mDescriptorHandles[h], &bufferInfo, 0);

		for (uint32_t i = 0; i < TEXTURE_CHANNEL_COUNT; i++)
		{
			if (m_aTextures[i] == nullptr)
				continue;
			
			VkDescriptorImageInfo imageInfo;
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
			imageInfo.imageView = m_aTextures[i]->GetImageView();
			imageInfo.sampler = m_aTextures[i]->GetSampler();

			DescriptorFactory::UpdateDescriptor(m_mDescriptorHandles[h], &imageInfo, i+1);
		}
	}

	void Material::BuildUniformBuffer()
	{
		m_pMaterialUniformBuffer = std::make_unique<Buffer>(sizeof(m_Desc), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, &m_Desc);
	}

}