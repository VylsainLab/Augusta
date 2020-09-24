#include <VulkanWrap/Application.h>
#include <VulkanWrap/SwapChain.h>
#include <VulkanWrap/ShaderFactory.h>
#include <VulkanWrap/VertexFormat.h>
#include <VulkanWrap/Buffer.h>
#include <iostream>
#include <stdexcept>

#define WINDOW_WIDTH	800
#define WINDOW_HEIGHT	600

class HelloTriangleApplication : public vkw::Application
{
public:
	HelloTriangleApplication(const std::string& name, uint16_t width, uint16_t height)
		: vkw::Application(name, width, height)
	{}

	void RunTriangleApp() 
	{
		Init();		
		Run();
		Cleanup();
	}

private:	
	VkPipelineLayout m_VkPipelineLayout;
	VkPipeline m_VkGraphicsPipeline;
	uint32_t m_uiIndexCount = 0;
	vkw::Buffer* m_pVertexBuffer = nullptr;
	vkw::Buffer* m_pIndexBuffer = nullptr;

	//************GRAPHICS PIPELINE**********

	void CreateGraphicsPipeline()
	{
		vkw::ShaderModule vertShaderModule("shaders/shader.vert", VK_SHADER_STAGE_VERTEX_BIT);
		vkw::ShaderModule fragShaderModule("shaders/shader.frag", VK_SHADER_STAGE_FRAGMENT_BIT);

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderModule.GetPipelineShaderModuleCreateInfo(), fragShaderModule.GetPipelineShaderModuleCreateInfo() };

		std::vector<vkw::VertexFormatComponents> vComponents = { vkw::VERTEX_FORMAT_VEC2F32,vkw::VERTEX_FORMAT_VEC3F32 }; //pos 2D, color
		vkw::VertexFormat vertexFormat(vComponents);		
		VkPipelineVertexInputStateCreateInfo vertexInputInfo = vertexFormat.GetPipelineVertexInputStateCreateInfo();

		//Input assembly
		VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		//Viewport
		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)m_pWindow->GetSwapChainExtent().width;
		viewport.height = (float)m_pWindow->GetSwapChainExtent().height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = m_pWindow->GetSwapChainExtent();

		VkPipelineViewportStateCreateInfo viewportState = {};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		//Rasterizer
		VkPipelineRasterizationStateCreateInfo rasterizer = {};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE; //TODO enable the needed GPU feature
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f; 
		rasterizer.depthBiasClamp = 0.0f; 
		rasterizer.depthBiasSlopeFactor = 0.0f; 

		//Multisampling : disabled for now
		VkPipelineMultisampleStateCreateInfo multisampling = {};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampling.minSampleShading = 1.0f; 
		multisampling.pSampleMask = nullptr; 
		multisampling.alphaToCoverageEnable = VK_FALSE; 
		multisampling.alphaToOneEnable = VK_FALSE; 

		//TODO : depth stencil

		//Blending
		VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; 
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; 
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; 
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; 
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; 
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; 

		VkPipelineColorBlendStateCreateInfo colorBlending = {};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY; 
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f; 
		colorBlending.blendConstants[1] = 0.0f; 
		colorBlending.blendConstants[2] = 0.0f; 
		colorBlending.blendConstants[3] = 0.0f; 

		//TODO : eventual dynamic states

		//Pipeline layout (uniforms specification)
		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 0; 
		pipelineLayoutInfo.pSetLayouts = nullptr; 
		pipelineLayoutInfo.pushConstantRangeCount = 0; 
		pipelineLayoutInfo.pPushConstantRanges = nullptr; 

		if (vkCreatePipelineLayout(vkw::Context::m_VkDevice, &pipelineLayoutInfo, nullptr, &m_VkPipelineLayout) != VK_SUCCESS)
			throw std::runtime_error("failed to create pipeline layout!");

		//Graphics pipeline
		VkGraphicsPipelineCreateInfo pipelineInfo = {};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = nullptr; 
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = nullptr; 
		pipelineInfo.layout = m_VkPipelineLayout;
		pipelineInfo.renderPass = m_VkRenderPass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; 
		pipelineInfo.basePipelineIndex = -1; 

		if (vkCreateGraphicsPipelines(vkw::Context::m_VkDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_VkGraphicsPipeline) != VK_SUCCESS)
			throw std::runtime_error("failed to create graphics pipeline!");
	}

	void CreateVertexBuffer()
	{		
		const std::vector<float> vertices = {
			-0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
			0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
			0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
			-0.5f, 0.5f, 1.0f, 1.0f, 1.0f
		};
		
		m_pVertexBuffer = new vkw::Buffer((uint64_t)vertices.size()*sizeof(float), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT , VMA_MEMORY_USAGE_GPU_ONLY, (void*)vertices.data());

		const std::vector<uint16_t> indices = { 0, 1, 2, 2, 3, 0 };
		m_uiIndexCount = (uint32_t)indices.size();
		m_pIndexBuffer = new vkw::Buffer((uint64_t)(m_uiIndexCount * sizeof(uint16_t)), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY, (void*)indices.data());
	}

	//***************************************

	void Init() 
	{		
		CreateGraphicsPipeline();
		CreateVertexBuffer();		
	}

	virtual void Render(VkCommandBuffer& commandBuffer) override
	{
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_VkGraphicsPipeline);

		VkBuffer vertexBuffers[] = { m_pVertexBuffer->GetBufferHandle() };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

		vkCmdBindIndexBuffer(commandBuffer, m_pIndexBuffer->GetBufferHandle(), 0, VK_INDEX_TYPE_UINT16);

		vkCmdDrawIndexed(commandBuffer, m_uiIndexCount, 1, 0, 0, 0);
	}

	void Cleanup() 
	{		
		delete m_pVertexBuffer;
		delete m_pIndexBuffer;

		vkDestroyPipeline(vkw::Context::m_VkDevice, m_VkGraphicsPipeline, nullptr);

		vkDestroyPipelineLayout(vkw::Context::m_VkDevice, m_VkPipelineLayout, nullptr);
	}
};

int main() 
{
	HelloTriangleApplication app("Vulkan", WINDOW_WIDTH, WINDOW_HEIGHT);

	try 
	{
		app.RunTriangleApp();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}