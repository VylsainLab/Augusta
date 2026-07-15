#ifndef AUG_GRAPHICSPIPELINE_H
#define AUG_GRAPHICSPIPELINE_H

#include <Augusta/Window.h>
#include <Augusta/Buffer.h>
#include <Augusta/ShaderFactory.h>
#include <Augusta/Material.h>
#include <Augusta/FrameBuffer.h>
#include <Augusta/IRenderTarget.h>
#include <vector>

#ifndef MAX_FRAMES_IN_FLIGHT
#define MAX_FRAMES_IN_FLIGHT	2
#endif

namespace aug
{
	struct SPipelineDesc
	{
		IRenderTarget* _pRenderTarget;		
		SShaderDesc _shaderDesc;
		VkPipelineVertexInputStateCreateInfo _vertexInputInfo;
		uint32_t _uiPushConstantSize = 0;
		std::vector<DescriptorSetLayoutHandle> _vLayoutHandles;
	};

	class Pipeline
	{
	public:
		Pipeline(IRenderTarget* pRT);
		virtual ~Pipeline();

#ifndef USE_DYNAMIC_RENDERING
		void CreateRenderPass(IRenderTarget* pRT); //TODO configurable
        const VkRenderPass& GetRenderPass() { return m_VkRenderPass; }
#endif
		void Init(const SPipelineDesc& desc);

#ifdef USE_DYNAMIC_RENDERING
		void BeginRendering(const VkCommandBuffer& commandBuffer, IRenderTarget* pRT, SRenderTargetLayout layout);
        void Bind(const VkCommandBuffer& commandBuffer);
#else
        void Bind(const VkCommandBuffer& commandBuffer, const VkFramebuffer& framebuffer, const VkExtent2D& extent);
#endif
		void EndRendering(const VkCommandBuffer& commandBuffer, IRenderTarget* pRT, SRenderTargetLayout layout);

		void PushConstants(const VkCommandBuffer& commandBuffer, void* pData);

		DescriptorSetLayoutHandle DeclareResourceLayout(const SDescriptorSetDesc &desc);
		void RegisterResource(DescriptorSetLayoutHandle h, DescriptorTarget* pResource);
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