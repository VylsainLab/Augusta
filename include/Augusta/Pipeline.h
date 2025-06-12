#ifndef AUG_GRAPHICSPIPELINE_H
#define AUG_GRAPHICSPIPELINE_H

#include <Augusta/Window.h>
#include <Augusta/Buffer.h>
#include <Augusta/ShaderFactory.h>
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
	};

	class Pipeline
	{
	public:
		Pipeline(aug::Window* pWindow);
		virtual ~Pipeline();

		void CreateRenderPass(aug::Window* pWindow); //TODO configurable
		void Init(const SPipelineDesc& desc);

		void Bind(const VkCommandBuffer& commandBuffer, const VkFramebuffer& framebuffer, const VkExtent2D& extent, uint32_t descriptorSetCount, uint8_t uiCurrentFrame);

		void PushConstants(const VkCommandBuffer& commandBuffer, void* pData);

		const VkRenderPass& GetRenderPass() { return m_VkRenderPass; }
		const VkPipelineLayout& GetPipelineLayout() { return m_VkPipelineLayout; }
		const VkDescriptorPool& GetPipelineDescriptorPool() {return m_VkDescriptorPool;	}

	protected:
		SPipelineDesc m_Desc;
		VkRenderPass m_VkRenderPass = VK_NULL_HANDLE;

		VkDescriptorSetLayout m_VkDescriptorSetLayout = VK_NULL_HANDLE;
		VkDescriptorPool m_VkDescriptorPool = VK_NULL_HANDLE;
		std::vector<VkDescriptorSet> m_vDescriptorSets;

		VkPipelineLayout m_VkPipelineLayout = VK_NULL_HANDLE;
		VkPipeline m_VkGraphicsPipeline = VK_NULL_HANDLE;
	};
}

#endif