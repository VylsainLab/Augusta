#pragma once
#include <vulkan/vulkan.hpp>
#include <map>

namespace aug
{
	struct SDescriptorBinding
	{
		uint8_t _uiBinding;
		VkShaderStageFlags _shaderStage;
		VkDescriptorType _type;
	};

	struct SDescriptorSetDesc
	{
		void AddBinding(uint8_t uiBinding, VkShaderStageFlags stage, VkDescriptorType type)
		{
			this->_uiSet = _uiSet;
			_vBindings.push_back({ uiBinding,stage,type });
		}

		uint8_t _uiSet;
		std::vector<SDescriptorBinding> _vBindings;
	};

	typedef uint32_t DescriptorSetLayoutHandle;
	typedef uint32_t DescriptorSetHandle;
	class DescriptorTarget
	{
	public:
		void AllocateDescriptor(DescriptorSetLayoutHandle h);
		virtual void UpdateDescriptor(DescriptorSetLayoutHandle h) = 0;

		void FreeDescriptor();

		bool HasDescriptor(DescriptorSetLayoutHandle h){return m_mDescriptorHandles.find(h)!= m_mDescriptorHandles.end(); }

		DescriptorSetHandle GetDescriptorSetHandle(DescriptorSetLayoutHandle h) { return m_mDescriptorHandles[h]; }

	protected:
		std::map<DescriptorSetLayoutHandle, DescriptorSetHandle> m_mDescriptorHandles; //one set of descriptors per pipeline
	};
	
	class Buffer;
	class DescriptorFactory
	{
	public:		
		static DescriptorSetLayoutHandle AllocateDescriptorSetLayout(const SDescriptorSetDesc& desc);
		static DescriptorSetHandle AllocateDescriptor(DescriptorSetLayoutHandle h);
		static void UpdateDescriptors(DescriptorSetLayoutHandle h, uint8_t uiCount, DescriptorSetHandle* pHandles, Buffer** pBuffers);
		static void UpdateDescriptor(DescriptorSetHandle h, VkDescriptorBufferInfo* info);
		static void UpdateDescriptor(DescriptorSetHandle h, VkDescriptorImageInfo* info, uint8_t uiBinding=0);

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

