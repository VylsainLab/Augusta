#include <Augusta/Pipeline.h>
#include <Augusta/Context.h>
#include <Augusta/DescriptorFactory.h>
#include <array>
#include <stdexcept>

namespace aug
{
	aug::Pipeline::Pipeline(IRenderTarget* pRT)
	{
#ifndef USE_DYNAMIC_RENDERING
		CreateRenderPass(pRT);
#endif
	}

	aug::Pipeline::~Pipeline()
	{
		CleanPipeline();

		for (auto& layout : m_vDescriptorSetHandles)
		{
			DescriptorFactory::DestroyDescriptorSetLayout(layout);
		}
	}

#ifndef USE_DYNAMIC_RENDERING
	void aug::Pipeline::CreateRenderPass(IRenderTarget* pRT)
	{
		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format = pWindow->GetColorFormat();
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRef = {};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription depthAttachment = {};
		depthAttachment.format = pWindow->GetDepthStencilFormat();
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRef = {};
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		subpass.pDepthStencilAttachment = &depthAttachmentRef;

		std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;

		// Make sure we reach the color attachment output stage before writing to attachments 
		// since the framebuffer image is not garanteed to be available before that
		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		if (vkCreateRenderPass(aug::Context::m_VkDevice, &renderPassInfo, nullptr, &m_VkRenderPass) != VK_SUCCESS)
			throw std::runtime_error("failed to create render pass!");
	}
#endif

	void aug::Pipeline::Init(const SPipelineDesc& desc)
	{
		m_Desc = desc;

		m_pShader = std::make_unique<Shader>(desc._shaderDesc);

		BuildPipeline();
	}

	void Pipeline::BeginRendering(const VkCommandBuffer& commandBuffer, IRenderTarget* pRT, SRenderTargetLayout layout)
	{
		//Swapchain image transition
		pRT->TransitionToLayout(commandBuffer, layout);

#ifdef USE_DYNAMIC_RENDERING
		//Dynamic rendering
		VkClearValue clearColors{};
		clearColors.color = { 0.1f, 0.1f, 0.1f, 1.0f };		

		std::vector<VkRenderingAttachmentInfo> vColorAttachmentInfo{};
		std::vector<VkImageView> vColorImageViews = pRT->GetColorImageViews();
		vColorAttachmentInfo.resize(vColorImageViews.size());
		for (int i = 0; i < vColorAttachmentInfo.size(); ++i)
		{
			vColorAttachmentInfo[i].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
			vColorAttachmentInfo[i].imageView = vColorImageViews[i];
			vColorAttachmentInfo[i].imageLayout = layout._colorLayout;
			vColorAttachmentInfo[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			vColorAttachmentInfo[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			vColorAttachmentInfo[i].clearValue = clearColors;
		}

		clearColors.depthStencil = { 1.0f, 0 };

		VkRenderingAttachmentInfo depthAttachmentInfo{};
		depthAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		depthAttachmentInfo.imageView = pRT->GetDepthImageView();
		depthAttachmentInfo.imageLayout = layout._depthStencilLayout;
		depthAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachmentInfo.clearValue = clearColors;

		VkRenderingInfo renderingInfo{};
		renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		VkRect2D renderArea{};
		renderArea.extent = pRT->GetExtent();
		renderingInfo.renderArea = renderArea;
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = static_cast<uint32_t>(vColorAttachmentInfo.size());
		renderingInfo.pColorAttachments = vColorAttachmentInfo.data();
		renderingInfo.pDepthAttachment = &depthAttachmentInfo;

		vkCmdBeginRendering(commandBuffer, &renderingInfo);
#endif
	}

#ifdef USE_DYNAMIC_RENDERING
	void aug::Pipeline::Bind(const VkCommandBuffer& commandBuffer)
#else
	void aug::Pipeline::Bind(const VkCommandBuffer& commandBuffer, const VkFramebuffer& framebuffer, const VkExtent2D& extent)
#endif
	{
#ifndef USE_DYNAMIC_RENDERING
		//Render pass
		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_VkRenderPass;
		renderPassInfo.framebuffer = framebuffer;
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = extent;
		VkClearValue clearColors[] = { { 0.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 0} }; //TODO configurable
		renderPassInfo.clearValueCount = 2;
		renderPassInfo.pClearValues = clearColors;
		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
#endif

		if (m_VkGraphicsPipeline == nullptr)
			return;

		if (m_pShader->CheckForModifications())
		{
			BuildPipeline();
		}

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_VkGraphicsPipeline);
	}

	void Pipeline::EndRendering(const VkCommandBuffer& commandBuffer, IRenderTarget* pRT, SRenderTargetLayout layout)
	{
#ifdef USE_DYNAMIC_RENDERING
		vkCmdEndRendering(commandBuffer);
#else
		vkCmdEndRenderPass(commandBuffer);
#endif
		pRT->TransitionToLayout(commandBuffer, layout);
	}

	void aug::Pipeline::PushConstants(const VkCommandBuffer& commandBuffer, void* pData)
	{
		vkCmdPushConstants(
			commandBuffer,
			m_VkPipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT,
			0,
			m_Desc._uiPushConstantSize,
			pData);
	}

	DescriptorSetLayoutHandle aug::Pipeline::DeclareResourceLayout(const SDescriptorSetDesc& desc)
	{
		DescriptorSetLayoutHandle h = DescriptorFactory::AllocateDescriptorSetLayout(desc);
		m_vDescriptorSetHandles.push_back(h);
		return h;
	}

	void aug::Pipeline::RegisterResource(DescriptorSetLayoutHandle h, DescriptorTarget* pResource)
	{
		pResource->AllocateDescriptor(h);
		pResource->UpdateDescriptor(h);
	}

	void aug::Pipeline::UpdateResource(DescriptorSetLayoutHandle h, DescriptorTarget* pResource)
	{
		pResource->UpdateDescriptor(h);
	}

	void aug::Pipeline::BindResource(const VkCommandBuffer& cb, DescriptorSetLayoutHandle h, uint8_t uiSet, DescriptorTarget* pResource)
	{
		VkDescriptorSet s = DescriptorFactory::GetDescriptorSet(pResource->GetDescriptorSetHandle(h));
		vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, m_VkPipelineLayout, uiSet, 1, &s, 0, nullptr);
	}

	void aug::Pipeline::BuildPipeline()
	{
		CleanPipeline();
		
		//Input assembly
		VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		//Viewport
		VkExtent2D extent = m_Desc._pRenderTarget->GetExtent();
		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)extent.width;
		viewport.height = (float)extent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = extent;

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
		std::vector<VkFormat> vColorFormats = m_Desc._pRenderTarget->GetColorFormats();

		std::vector<VkPipelineColorBlendAttachmentState> vColorBlendAttachment(vColorFormats.size(),
			{
				VK_FALSE,
				VK_BLEND_FACTOR_ONE,
				VK_BLEND_FACTOR_ZERO,
				VK_BLEND_OP_ADD,
				VK_BLEND_FACTOR_ONE,
				VK_BLEND_FACTOR_ZERO,
				VK_BLEND_OP_ADD,
				VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
			});		

		VkPipelineColorBlendStateCreateInfo colorBlending = {};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount = vColorBlendAttachment.size();
		colorBlending.pAttachments = vColorBlendAttachment.data();
		colorBlending.blendConstants[0] = 0.0f;
		colorBlending.blendConstants[1] = 0.0f;
		colorBlending.blendConstants[2] = 0.0f;
		colorBlending.blendConstants[3] = 0.0f;

		VkPipelineDepthStencilStateCreateInfo depthStencil = {};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = VK_TRUE;
		depthStencil.depthWriteEnable = VK_TRUE;
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.minDepthBounds = 0.0f; // Optional
		depthStencil.maxDepthBounds = 1.0f; // Optional
		depthStencil.stencilTestEnable = VK_FALSE;

		//PushConstants //TODO move to shader
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = m_Desc._uiPushConstantSize;

		//Pipeline layout
		std::vector<VkDescriptorSetLayout> descriptorLayouts;
		for (auto& h : m_Desc._vLayoutHandles)
			descriptorLayouts.push_back(DescriptorFactory::GetDescriptorSetLayout(h));
		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorLayouts.size());
		pipelineLayoutInfo.pSetLayouts = descriptorLayouts.data();
		if (m_Desc._uiPushConstantSize > 0)
		{
			pipelineLayoutInfo.pushConstantRangeCount = 1;
			pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
		}

		if (vkCreatePipelineLayout(aug::Context::m_VkDevice, &pipelineLayoutInfo, nullptr, &m_VkPipelineLayout) != VK_SUCCESS)
			throw std::runtime_error("Failed to create pipeline layout!");

		//Dynamic rendering (replaces render passes)
		VkPipelineRenderingCreateInfo renderingInfo = {};
		renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
		renderingInfo.colorAttachmentCount = static_cast<uint32_t>(vColorFormats.size());
		renderingInfo.pColorAttachmentFormats = vColorFormats.data();
		renderingInfo.depthAttachmentFormat = m_Desc._pRenderTarget->GetDepthFormat();

		//Graphics pipeline
		VkGraphicsPipelineCreateInfo pipelineInfo = {};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = m_pShader->GetStageCount();
		pipelineInfo.pStages = m_pShader->GetPipelineShaderStagesCreateInfo();
		pipelineInfo.pVertexInputState = &m_Desc._vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = &depthStencil;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = nullptr;
		pipelineInfo.layout = m_VkPipelineLayout;
#ifdef USE_DYNAMIC_RENDERING
		pipelineInfo.pNext = &renderingInfo;
#else
		pipelineInfo.renderPass = m_VkRenderPass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineInfo.basePipelineIndex = -1;
#endif	

		if (vkCreateGraphicsPipelines(aug::Context::m_VkDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_VkGraphicsPipeline) != VK_SUCCESS)
			throw std::runtime_error("failed to create graphics pipeline!");
	}

	void aug::Pipeline::CleanPipeline()
	{
		if (m_VkGraphicsPipeline)
		{
			vkDestroyPipeline(aug::Context::m_VkDevice, m_VkGraphicsPipeline, nullptr);
			m_VkGraphicsPipeline = VK_NULL_HANDLE;
		}

		if (m_VkPipelineLayout)
			vkDestroyPipelineLayout(aug::Context::m_VkDevice, m_VkPipelineLayout, nullptr);

#ifndef USE_DYNAMIC_RENDERING
		if (m_VkRenderPass)
			vkDestroyRenderPass(Context::m_VkDevice, m_VkRenderPass, nullptr);
#endif
	}
}