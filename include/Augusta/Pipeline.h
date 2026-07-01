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
		SShaderDesc shaderDesc;
		VkPipelineVertexInputStateCreateInfo vertexInputInfo;
		uint32_t uiPushConstantSize;
		std::vector<DescriptorSetLayoutHandle> vLayoutHandles;
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

		DescriptorSetLayoutHandle DeclareResourceLayout(const SDescriptorSetDesc &desc);
		void RegisterResource(DescriptorSetLayoutHandle h, uint8_t uiBinding, DescriptorTarget* pResource);
		void UpdateResource(DescriptorSetLayoutHandle h, DescriptorTarget* pResource);
		void BindResource(const VkCommandBuffer& cb, DescriptorSetLayoutHandle h, uint8_t uiSet, DescriptorTarget* pResource);
	protected:

		void BuildPipeline();
		void CleanPipeline();

		VkPipelineLayout m_VkPipelineLayout = VK_NULL_HANDLE;
	
		SPipelineDesc m_Desc;
#ifndef USE_DYNAMIC_RENDERING
		VkRenderPass m_VkRenderPass = VK_NULL_HANDLE;
#endif
		
		VkPipeline m_VkGraphicsPipeline = VK_NULL_HANDLE;

		std::unique_ptr<Shader> m_pShader = nullptr;

		std::vector<DescriptorSetHandle> m_vDescriptorSetHandles;
	};
}

#endif