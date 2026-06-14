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

	void Material::CreateDescriptorSets(const VkDescriptorSetLayout* pSetLayouts)
	{
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = aug::Context::m_VkDescriptorPool;
		allocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
		allocInfo.pSetLayouts = pSetLayouts;

		m_vDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
		if (vkAllocateDescriptorSets(aug::Context::m_VkDevice, &allocInfo, m_vDescriptorSets.data()) != VK_SUCCESS)
			throw std::runtime_error("Failed to allocate descriptor sets!");

		for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			std::vector<VkWriteDescriptorSet> vWrites;
			std::deque<VkDescriptorImageInfo> vImageInfos;
			for (uint32_t j = 0; j < TEXTURE_CHANNEL_COUNT; j++)
			{
				if (m_aTextures[j] == nullptr)
					continue;

				VkDescriptorImageInfo &descriptorImageInfo = vImageInfos.emplace_back(VkDescriptorImageInfo{});
				descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
				descriptorImageInfo.imageView = m_aTextures[j]->GetImageView();
				descriptorImageInfo.sampler = m_aTextures[j]->GetSampler();

				VkWriteDescriptorSet &descriptorWrite = vWrites.emplace_back(VkWriteDescriptorSet{});
				descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrite.dstSet = m_vDescriptorSets[i];
				descriptorWrite.dstBinding = j;
				descriptorWrite.dstArrayElement = 0;
				descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				descriptorWrite.descriptorCount = 1;
				descriptorWrite.pImageInfo = &descriptorImageInfo;
			}
			vkUpdateDescriptorSets(aug::Context::m_VkDevice, vWrites.size(), vWrites.data(), 0, nullptr);
			vWrites.clear();
			vImageInfos.clear();
		}
	}
}