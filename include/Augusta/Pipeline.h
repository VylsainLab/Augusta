#ifndef AUG_GRAPHICSPIPELINE_H
#define AUG_GRAPHICSPIPELINE_H

#include <Augusta/Window.h>
#include <Augusta/Buffer.h>
#include <Augusta/ShaderFactory.h>
#include <Augusta/Material.h>
#include <vector>

#ifndef MAX_FRAMES_IN_FLIGHT
#define MAX_FRAMES_IN_FLIGHT	2
#endif

namespace aug
{
	struct SPipelineDesc
	{
		Window* pWindow; //TODO replace with framebuffer/render target
		Shader* pShader;
		VkPipelineVertexInputStateCreateInfo vertexInputInfo;
		uint32_t uiPushConstantSize;
		std::vector<aug::Buffer*>* pvUniformBuffers;
		//TODO include shaders, materials, descriptor set layout, etc...
	};

	class Pipeline
	{
	public:
		Pipeline(aug::Window* pWindow);
		virtual ~Pipeline();

#ifndef USE_DYNAMIC_RENDERING
		void CreateRenderPass(aug::Window* pWindow); //TODO configurable
        const VkRenderPass& GetRenderPass() { return m_VkRenderPass; }
#endif
		void Init(const SPipelineDesc& desc);

#ifdef USE_DYNAMIC_RENDERING
        void Bind(const VkCommandBuffer& commandBuffer, uint32_t descriptorSetCount, uint8_t uiCurrentFrame);
#else
        void Bind(const VkCommandBuffer& commandBuffer, const VkFramebuffer& framebuffer, const VkExtent2D& extent, uint32_t descriptorSetCount, uint8_t uiCurrentFrame);
#endif

		void PushConstants(const VkCommandBuffer& commandBuffer, void* pData);

		void UpdateDescriptors(const VkCommandBuffer &cb, std::shared_ptr<Material> pMat, uint8_t uiCurrentFrame);

	protected:

		void BuildPipeline();
		void CleanPipeline();

		VkPipelineLayout m_VkPipelineLayout = VK_NULL_HANDLE;

		VkDescriptorSetLayout m_VkDescriptorSetLayoutMaterial = VK_NULL_HANDLE;
	
		SPipelineDesc m_Desc;
#ifndef USE_DYNAMIC_RENDERING
		VkRenderPass m_VkRenderPass = VK_NULL_HANDLE;
#endif
		VkDescriptorSetLayout m_VkDescriptorSetLayout = VK_NULL_HANDLE;
		std::vector<VkDescriptorSet> m_vDescriptorSets;
		
		VkPipeline m_VkGraphicsPipeline = VK_NULL_HANDLE;
	};
}

#endif