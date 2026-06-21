#pragma once
#include <vulkan/vulkan.hpp>
#include <map>

namespace aug
{
	struct SDescriptorBinding
	{
		uint8_t uiBinding;
		VkShaderStageFlags shaderStage;
		VkDescriptorType type;
	};

	struct SDescriptorSetDesc
	{
		void AddBinding(uint8_t uiBinding, VkShaderStageFlags stage, VkDescriptorType type)
		{
			this->uiSet = uiSet;
			vBindings.push_back({ uiBinding,stage,type });
		}

		uint8_t uiSet;
		std::vector<SDescriptorBinding> vBindings;
	};

	typedef uint32_t DescriptorSetLayoutHandle;
	typedef uint32_t DescriptorSetHandle;
	class DescriptorTarget
	{
		virtual void AllocateDescriptors(DescriptorSetLayoutHandle h)=0;

	protected:
		std::map<DescriptorSetLayoutHandle, std::vector<DescriptorSetHandle>> m_mDescriptorHandles; //one set of descriptors per pipeline
	};
	
	class Buffer;
	class DescriptorFactory
	{
	public:
		static DescriptorSetLayoutHandle AllocateDescriptorSetLayout(const SDescriptorSetDesc& desc);
		static void AllocateDescriptors(DescriptorSetLayoutHandle h, uint8_t uiCount, DescriptorSetHandle* pHandles);
		static void UpdateDescriptors(DescriptorSetLayoutHandle h, uint8_t uiCount, DescriptorSetHandle* pHandles, Buffer** pBuffers);

		static VkDescriptorSetLayout GetDescriptorSetLayout(DescriptorSetLayoutHandle h) { return m_mDescriptors[h].layout; }
		static VkDescriptorSet GetDescriptorSet(DescriptorSetLayoutHandle hL, DescriptorSetHandle hS) { return m_mDescriptors[hL].vDescriptotSets[hS]; }
	protected:
		struct SDescriptorSet
		{
			VkDescriptorSetLayout layout;
			std::vector<VkDescriptorSet> vDescriptotSets;
		};
		static std::map<DescriptorSetLayoutHandle, SDescriptorSet> m_mDescriptors;
		static uint32_t m_uiLayoutCount;
	};
}

