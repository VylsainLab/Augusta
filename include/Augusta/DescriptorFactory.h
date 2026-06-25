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
	public:
		virtual void AllocateDescriptor(DescriptorSetLayoutHandle h)=0;
		virtual void UpdateDescriptor(DescriptorSetLayoutHandle h) = 0;

		void FreeDescriptor();

		DescriptorSetHandle GetDescriptorSetHandle(DescriptorSetLayoutHandle h) { return m_mDescriptorHandles[h]; }

	protected:
		std::map<DescriptorSetLayoutHandle, DescriptorSetHandle> m_mDescriptorHandles; //one set of descriptors per pipeline
	};
	
	class Buffer;
	class DescriptorFactory
	{
	public:		
		static DescriptorSetLayoutHandle AllocateDescriptorSetLayout(const SDescriptorSetDesc& desc);
		static void AllocateDescriptors(DescriptorSetLayoutHandle h, uint8_t uiCount, std::vector<DescriptorSetHandle>& vHandles);
		static void UpdateDescriptors(DescriptorSetLayoutHandle h, uint8_t uiCount, DescriptorSetHandle* pHandles, Buffer** pBuffers);
		static void UpdateDescriptor(DescriptorSetHandle h, VkDescriptorBufferInfo* info);

		static void DestroyDescriptorSetLayout(DescriptorSetLayoutHandle h);
		static void FreeDescriptors(DescriptorSetLayoutHandle h, uint32_t uiCount, DescriptorSetHandle* pHandles);

		static VkDescriptorSetLayout GetDescriptorSetLayout(DescriptorSetLayoutHandle h) { return m_vLayouts[h]; }
		static VkDescriptorSet GetDescriptorSet(DescriptorSetHandle h) { return m_vSets[h]; }
		static VkDescriptorPool m_VkDescriptorPool;
	protected:
		friend class Application;
		static void Init();
		static void Release();
		static void CreateDescriptorPool();

		
		static std::map<DescriptorSetLayoutHandle, std::vector<DescriptorSetLayoutHandle>> m_mDescriptorMapping;
		static std::vector<VkDescriptorSetLayout> m_vLayouts;
		static std::vector<VkDescriptorSet> m_vSets;
	};
}

