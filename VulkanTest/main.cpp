#include <VulkanWrap/Application.h>
#include <VulkanWrap/SwapChain.h>
#include <VulkanWrap/ShaderFactory.h>
#include <VulkanWrap/VertexFormat.h>
#include <VulkanWrap/Buffer.h>
#include <VulkanWrap/Camera.h>
#include <iostream>
#include <stdexcept>

#define WINDOW_WIDTH	800
#define WINDOW_HEIGHT	600

class HelloTriangleApplication : public vkw::Application
{
public:
	HelloTriangleApplication(const std::string& name, uint16_t width, uint16_t height)
		:	vkw::Application(name, width, height)
	{
		vkw::SCameraDesc desc;
		desc.speed = 0.001f;
		desc.sensitivity = 0.1f;
		desc.position = glm::vec3(0., 0., 1.);
		m_Camera = vkw::Camera(desc);
		AddEventObserver(&m_Camera);
	}

	void RunTriangleApp() 
	{
		Init();		
		Run();
		Cleanup();
	}

private:	
	VkDescriptorSetLayout m_VkDescriptorSetLayout = VK_NULL_HANDLE;
	VkDescriptorPool m_VkDescriptorPool = VK_NULL_HANDLE;
	VkPipelineLayout m_VkPipelineLayout = VK_NULL_HANDLE;
	VkPipeline m_VkGraphicsPipeline = VK_NULL_HANDLE;
	uint32_t m_uiIndexCount = 0;
	vkw::Buffer* m_pVertexBuffer = nullptr;
	vkw::Buffer* m_pIndexBuffer = nullptr;
	std::vector<vkw::Buffer*> m_vUniformBuffers; //One per swap chain image
	std::vector<VkDescriptorSet> m_vDescriptorSets;

	//Game objects
	vkw::Camera m_Camera;

	void CreateDescriptorSetLayout()
	{
		VkDescriptorSetLayoutBinding uboLayoutBinding{};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		uboLayoutBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = 1;
		layoutInfo.pBindings = &uboLayoutBinding;

		if (vkCreateDescriptorSetLayout(vkw::Context::m_VkDevice, &layoutInfo, nullptr, &m_VkDescriptorSetLayout) != VK_SUCCESS)
			throw std::runtime_error("Failed to create descriptor set layout!");
	}

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
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
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
		pipelineLayoutInfo.setLayoutCount = 1; 
		pipelineLayoutInfo.pSetLayouts = &m_VkDescriptorSetLayout; 
		pipelineLayoutInfo.pushConstantRangeCount = 0; 
		pipelineLayoutInfo.pPushConstantRanges = nullptr; 

		if (vkCreatePipelineLayout(vkw::Context::m_VkDevice, &pipelineLayoutInfo, nullptr, &m_VkPipelineLayout) != VK_SUCCESS)
			throw std::runtime_error("Failed to create pipeline layout!");

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

	struct UniformBufferObject 
	{
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;
	};

	void CreateUniformBuffers()
	{
		for (uint32_t i = 0; i < m_pWindow->GetSwapChainImageCount(); ++i)
			m_vUniformBuffers.push_back(new vkw::Buffer(sizeof(UniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, nullptr));
	}

	void CreateDescriptorPool()
	{
		VkDescriptorPoolSize poolSize{};
		poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSize.descriptorCount = m_pWindow->GetSwapChainImageCount();

		VkDescriptorPoolCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		createInfo.poolSizeCount = 1;
		createInfo.pPoolSizes = &poolSize;
		createInfo.maxSets = m_pWindow->GetSwapChainImageCount();
		if (vkCreateDescriptorPool(vkw::Context::m_VkDevice, &createInfo, nullptr, &m_VkDescriptorPool) != VK_SUCCESS)
			throw std::runtime_error("Failed to create descriptor pool!");
	}

	void CreateDescriptorSets()
	{
		std::vector<VkDescriptorSetLayout> layouts(m_pWindow->GetSwapChainImageCount(), m_VkDescriptorSetLayout);

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = m_VkDescriptorPool;
		allocInfo.descriptorSetCount = m_pWindow->GetSwapChainImageCount();
		allocInfo.pSetLayouts = layouts.data();

		m_vDescriptorSets.resize(m_pWindow->GetSwapChainImageCount());
		if (vkAllocateDescriptorSets(vkw::Context::m_VkDevice, &allocInfo, m_vDescriptorSets.data()) != VK_SUCCESS)
			throw std::runtime_error("Failed to allocate descriptor sets!");

		for (size_t i = 0; i < m_pWindow->GetSwapChainImageCount(); i++)
		{
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = m_vUniformBuffers[i]->GetBufferHandle();
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(UniformBufferObject);

			VkWriteDescriptorSet descriptorWrite{};
			descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite.dstSet = m_vDescriptorSets[i];
			descriptorWrite.dstBinding = 0;
			descriptorWrite.dstArrayElement = 0;			
			descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrite.descriptorCount = 1;
			descriptorWrite.pBufferInfo = &bufferInfo;
			descriptorWrite.pImageInfo = nullptr; // Optional
			descriptorWrite.pTexelBufferView = nullptr; // Optional
			vkUpdateDescriptorSets(vkw::Context::m_VkDevice, 1, &descriptorWrite, 0, nullptr);
		}
	}

	//***************************************

	void Init() 
	{		
		CreateDescriptorSetLayout();
		CreateGraphicsPipeline();
		CreateVertexBuffer();	
		CreateUniformBuffers();
		CreateDescriptorPool();
		CreateDescriptorSets();
	}

	void Update()
	{
		m_Camera.ComputeCamera();

		//update uniform buffers
		UniformBufferObject ubo;
		ubo.model = glm::mat4(1.f);
		ubo.view = glm::mat4(m_Camera.GetViewMatrix());
		ubo.proj = glm::mat4(m_Camera.GetProjectionMatrix());
		m_vUniformBuffers[m_uiCurrentImageIndex]->CopyData(sizeof(UniformBufferObject), &ubo);
	}

	virtual void Render(VkCommandBuffer& commandBuffer) override
	{
		Update();

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_VkGraphicsPipeline);

		VkBuffer vertexBuffers[] = { m_pVertexBuffer->GetBufferHandle() };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

		vkCmdBindIndexBuffer(commandBuffer, m_pIndexBuffer->GetBufferHandle(), 0, VK_INDEX_TYPE_UINT16);

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_VkPipelineLayout, 0, 1, &m_vDescriptorSets[m_uiCurrentImageIndex], 0, nullptr);

		vkCmdDrawIndexed(commandBuffer, m_uiIndexCount, 1, 0, 0, 0);
	}

	void Cleanup() 
	{		
		delete m_pVertexBuffer;
		delete m_pIndexBuffer;

		for (uint32_t i = 0; i < m_pWindow->GetSwapChainImageCount(); ++i)
			delete m_vUniformBuffers[i];

		vkDestroyDescriptorPool(vkw::Context::m_VkDevice, m_VkDescriptorPool, nullptr);

		vkDestroyPipeline(vkw::Context::m_VkDevice, m_VkGraphicsPipeline, nullptr);

		vkDestroyPipelineLayout(vkw::Context::m_VkDevice, m_VkPipelineLayout, nullptr);

		vkDestroyDescriptorSetLayout(vkw::Context::m_VkDevice, m_VkDescriptorSetLayout, nullptr);
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