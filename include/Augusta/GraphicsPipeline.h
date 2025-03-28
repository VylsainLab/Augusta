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
	struct SGraphicsPipelineDesc
	{
		Window* pWindow;
		Shader* pShader;
		VkPipelineVertexInputStateCreateInfo vertexInputInfo;
		uint32_t uiPushConstantSize; //TODO move to Uniforms
		std::vector<aug::Buffer*>* pvUniformBuffers;
	};

	class GraphicsPipeline
	{
	public:
		GraphicsPipeline(aug::Window* pWindow);
		virtual ~GraphicsPipeline();

		void CreateRenderPass(aug::Window* pWindow);
		void Init(const SGraphicsPipelineDesc& desc);

		void Bind(const VkCommandBuffer& commandBuffer, uint32_t descriptorSetCount, uint8_t uiCurrentFrame);

		const VkRenderPass& GetRenderPass() { return m_VkRenderPass; }
		const VkPipelineLayout& GetPipelineLayout() { return m_VkPipelineLayout; }

	protected:
		VkRenderPass m_VkRenderPass = VK_NULL_HANDLE;

		VkDescriptorSetLayout m_VkDescriptorSetLayout = VK_NULL_HANDLE;
		VkDescriptorPool m_VkDescriptorPool = VK_NULL_HANDLE;
		std::vector<VkDescriptorSet> m_vDescriptorSets;

		VkPipelineLayout m_VkPipelineLayout = VK_NULL_HANDLE;
		VkPipeline m_VkGraphicsPipeline = VK_NULL_HANDLE;
	};
}

#endif