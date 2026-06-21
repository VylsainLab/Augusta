#include <Augusta/DescriptorFactory.h>
#include <Augusta/Context.h>
#include <Augusta/Buffer.h>

namespace aug
{
	std::map<DescriptorSetLayoutHandle, DescriptorFactory::SDescriptorSet> DescriptorFactory::m_mDescriptors;
	uint32_t DescriptorFactory::m_uiLayoutCount = 0;

	DescriptorSetLayoutHandle DescriptorFactory::AllocateDescriptorSetLayout(const SDescriptorSetDesc& desc)
	{
		SDescriptorSet& set = m_mDescriptors[m_uiLayoutCount];
		VkDescriptorSetLayout& layout = set.layout;

		std::vector<VkDescriptorSetLayoutBinding> vLayoutBindings{};
		vLayoutBindings.resize(desc.vBindings.size());
		for (int i=0; i<vLayoutBindings.size(); ++i)
		{
			vLayoutBindings[i].binding = desc.vBindings[i].uiBinding;
			vLayoutBindings[i].descriptorType = desc.vBindings[i].type;
			vLayoutBindings[i].descriptorCount = 1;
			vLayoutBindings[i].stageFlags = desc.vBindings[i].shaderStage;
			vLayoutBindings[i].pImmutableSamplers = nullptr;
		}

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = vLayoutBindings.size();
		layoutInfo.pBindings = vLayoutBindings.data();

		if (vkCreateDescriptorSetLayout(aug::Context::m_VkDevice, &layoutInfo, nullptr, &set.layout) != VK_SUCCESS)
			throw std::runtime_error("Failed to create descriptor set layout!");

		DescriptorSetLayoutHandle res = m_uiLayoutCount;
		m_uiLayoutCount++;
		return res;
	}

	void DescriptorFactory::AllocateDescriptors(DescriptorSetLayoutHandle h, uint8_t uiCount, DescriptorSetHandle* pHandles)
	{
		std::vector<VkDescriptorSetLayout> layouts(uiCount, m_mDescriptors[h].layout);

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = aug::Context::m_VkDescriptorPool;
		allocInfo.descriptorSetCount = uiCount;
		allocInfo.pSetLayouts = layouts.data();
		
		std::vector<VkDescriptorSet> vDescriptorSets;
		vDescriptorSets.resize(uiCount);
		if (vkAllocateDescriptorSets(aug::Context::m_VkDevice, &allocInfo, vDescriptorSets.data()) != VK_SUCCESS)
			throw std::runtime_error("Failed to allocate descriptor sets!");

		for (int i = 0; i < uiCount; ++i)
			pHandles[i] = m_mDescriptors[h].vDescriptotSets.size() + i;

		m_mDescriptors[h].vDescriptotSets.insert(m_mDescriptors[h].vDescriptotSets.end(), vDescriptorSets.begin(), vDescriptorSets.end());
	}

	void DescriptorFactory::UpdateDescriptors(DescriptorSetLayoutHandle h, uint8_t uiCount, DescriptorSetHandle* pHandles, Buffer** pBuffers)
	{
		for (size_t i = 0; i < uiCount; i++)
		{
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = pBuffers[i]->GetBufferHandle();
			bufferInfo.offset = 0;
			bufferInfo.range = pBuffers[i]->GetBufferSize();

			VkWriteDescriptorSet descriptorWrite{};
			descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite.dstSet = m_mDescriptors[h].vDescriptotSets[pHandles[i]];
			descriptorWrite.dstBinding = 0;
			descriptorWrite.dstArrayElement = 0;
			descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrite.descriptorCount = 1;
			descriptorWrite.pBufferInfo = &bufferInfo;

			vkUpdateDescriptorSets(aug::Context::m_VkDevice, 1, &descriptorWrite, 0, nullptr);
		}
	}
}
