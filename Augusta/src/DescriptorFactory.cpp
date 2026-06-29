#include <Augusta/DescriptorFactory.h>
#include <Augusta/Context.h>
#include <Augusta/Buffer.h>
#include <Augusta/Utils.h>

namespace aug
{
	void DescriptorTarget::AllocateDescriptor(DescriptorSetLayoutHandle h)
	{
		m_mDescriptorHandles[h] = DescriptorFactory::AllocateDescriptor(h);
	}

	void DescriptorTarget::FreeDescriptor()
	{
		for (auto& layout : m_mDescriptorHandles)
		{
			if (!layout.second)
				DescriptorFactory::FreeDescriptors(layout.first,1,&layout.second);
		}
	}

	VkDescriptorPool DescriptorFactory::m_VkDescriptorPool = VK_NULL_HANDLE;
	std::map<DescriptorSetLayoutHandle, std::vector<DescriptorSetHandle>> DescriptorFactory::m_mDescriptorMapping;
	std::vector<VkDescriptorSetLayout> DescriptorFactory::m_vLayouts;
	std::vector<VkDescriptorSet> DescriptorFactory::m_vSets;

	void DescriptorFactory::Init()
	{
		CreateDescriptorPool();
	}

	void DescriptorFactory::Release()
	{
		vkDestroyDescriptorPool(Context::m_VkDevice, m_VkDescriptorPool, nullptr);
	}

	void DescriptorFactory::CreateDescriptorPool()
	{
		//Descriptor pool
		VkDescriptorPoolSize poolSizes[] =
		{
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, AUG_DESCRIPTOR_POOL_SIZE},
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, AUG_DESCRIPTOR_POOL_SIZE}
		};

		VkDescriptorPoolCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		createInfo.poolSizeCount = COUNT_OF(poolSizes);
		createInfo.pPoolSizes = poolSizes;
		createInfo.maxSets = AUG_DESCRIPTOR_POOL_SIZE * createInfo.poolSizeCount;
		if (vkCreateDescriptorPool(aug::Context::m_VkDevice, &createInfo, nullptr, &m_VkDescriptorPool) != VK_SUCCESS)
			throw std::runtime_error("Failed to create descriptor pool!");
	}

	DescriptorSetLayoutHandle DescriptorFactory::AllocateDescriptorSetLayout(const SDescriptorSetDesc& desc)
	{
		VkDescriptorSetLayout& layout = m_vLayouts.emplace_back();

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

		if (vkCreateDescriptorSetLayout(aug::Context::m_VkDevice, &layoutInfo, nullptr, &layout) != VK_SUCCESS)
			throw std::runtime_error("Failed to create descriptor set layout!");

		DescriptorSetLayoutHandle res = m_vLayouts.size()-1;
		return res;
	}

	DescriptorSetHandle DescriptorFactory::AllocateDescriptor(DescriptorSetLayoutHandle h)
	{
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = m_VkDescriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &m_vLayouts[h];
		
		VkDescriptorSet descriptorSet;
		VkResult res = vkAllocateDescriptorSets(aug::Context::m_VkDevice, &allocInfo, &descriptorSet);
		if (res != VK_SUCCESS)
			throw std::runtime_error("Failed to allocate descriptor sets!");

		DescriptorSetHandle sh = m_vSets.size();
		m_mDescriptorMapping[h].push_back(sh);
		m_vSets.push_back(descriptorSet);	
		return sh;
	}
	
	void DescriptorFactory::UpdateDescriptor(DescriptorSetHandle h, VkDescriptorBufferInfo* info)
	{
		VkWriteDescriptorSet descriptorWrite{};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = m_vSets[h];
		descriptorWrite.dstBinding = 0;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pBufferInfo = info;

		vkUpdateDescriptorSets(aug::Context::m_VkDevice, 1, &descriptorWrite, 0, nullptr);
	}

	void DescriptorFactory::UpdateDescriptor(DescriptorSetHandle h, VkDescriptorImageInfo* info, uint8_t uiBinding)
	{
		VkWriteDescriptorSet descriptorWrite{};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = m_vSets[h];
		descriptorWrite.dstBinding = uiBinding;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pImageInfo = info;

		vkUpdateDescriptorSets(aug::Context::m_VkDevice, 1, &descriptorWrite, 0, nullptr);
	}

	void DescriptorFactory::DestroyDescriptorSetLayout(DescriptorSetLayoutHandle h)
	{		
		if (m_mDescriptorMapping.find(h)!=m_mDescriptorMapping.end())
		{
			//free all related sets
			FreeDescriptors(h,m_mDescriptorMapping[h].size(), m_mDescriptorMapping[h].data());

			//destroy layout
			vkDestroyDescriptorSetLayout(aug::Context::m_VkDevice, m_vLayouts[h], nullptr);

			m_mDescriptorMapping[h].clear();
		}
	}

	void DescriptorFactory::FreeDescriptors(DescriptorSetLayoutHandle h, uint32_t uiCount, DescriptorSetHandle* pHandles)
	{
		std::vector<VkDescriptorSet> vSets;
		for (int i = 0; i < uiCount; ++i)
			vSets.push_back(m_vSets[pHandles[i]]);
		vkFreeDescriptorSets(Context::m_VkDevice, m_VkDescriptorPool, uiCount, vSets.data());
	}
}
