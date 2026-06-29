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
		for (uint32_t i = 0; i < TEXTURE_CHANNEL_COUNT; i++)
		{
			if (m_aTextures[i] == nullptr)
				continue;
			
			VkDescriptorImageInfo imageInfo;
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
			imageInfo.imageView = m_aTextures[i]->GetImageView();
			imageInfo.sampler = m_aTextures[i]->GetSampler();

			DescriptorFactory::UpdateDescriptor(m_mDescriptorHandles[h], &imageInfo, i);
		}
		vkUpdateDescriptorSets(aug::Context::m_VkDevice, static_cast<uint32_t>(vWrites.size()), vWrites.data(), 0, nullptr);
		vWrites.clear();
		vImageInfos.clear();
	}

}