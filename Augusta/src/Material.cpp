#include <Augusta/Material.h>
#include <Augusta/Context.h>
#include <Augusta/Pipeline.h>
#include <stdexcept>

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
		//Descriptor layout
		/*VkDescriptorSetLayoutBinding samplerLayoutBinding{};
		samplerLayoutBinding.binding = 1;
		samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerLayoutBinding.descriptorCount = 1;
		samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		samplerLayoutBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = 1;
		layoutInfo.pBindings = &samplerLayoutBinding;

		if (vkCreateDescriptorSetLayout(aug::Context::m_VkDevice, &layoutInfo, nullptr, &m_VkDescriptorSetLayout) != VK_SUCCESS)
			throw std::runtime_error("Failed to create descriptor set layout!");

		std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_VkDescriptorSetLayout);*/

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = aug::Context::m_VkDescriptorPool;
		allocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
		allocInfo.pSetLayouts = pSetLayouts;

		m_vDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
		if (vkAllocateDescriptorSets(aug::Context::m_VkDevice, &allocInfo, m_vDescriptorSets.data()) != VK_SUCCESS)
			throw std::runtime_error("Failed to allocate descriptor sets!");

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			VkDescriptorImageInfo descriptorImageInfo{};
			descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
			descriptorImageInfo.imageView = m_pTexture->GetImageView();
			descriptorImageInfo.sampler = m_pTexture->GetSampler();

			VkWriteDescriptorSet descriptorWrite{};
			descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite.dstSet = m_vDescriptorSets[i];
			descriptorWrite.dstBinding = 0;
			descriptorWrite.dstArrayElement = 0;
			descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrite.descriptorCount = 1;
			descriptorWrite.pImageInfo = &descriptorImageInfo;

			vkUpdateDescriptorSets(aug::Context::m_VkDevice, 1, &descriptorWrite, 0, nullptr);
		}
	}
}