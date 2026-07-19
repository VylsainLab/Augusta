#include <Augusta/Application.h>
#include <Augusta/ShaderFactory.h>
#include <Augusta/DescriptorFactory.h>
#include <Augusta/Camera.h>
#include <Augusta/AssimpParser.h>
#include <Augusta/Material.h>
#include <iostream>
#include <stdexcept>

#define WINDOW_WIDTH	1920
#define WINDOW_HEIGHT	1080

class AugustaDemo : public aug::Application
{
public:
	AugustaDemo(const std::string& name, uint16_t width, uint16_t height)
		:	aug::Application(name, width, height),
			m_GBufferVertexFormat({ aug::VERTEX_FORMAT_VEC3F32,aug::VERTEX_FORMAT_VEC3F32, aug::VERTEX_FORMAT_VEC2F32 }),
			m_MainVertexFormat({ aug::VERTEX_FORMAT_VEC2F32 }),
			m_AssimpParser(aug::VERTEX_COMPONENT_NORMAL|aug::VERTEX_COMPONENT_TEXCOORD, true, aiProcess_Triangulate|aiProcess_PreTransformVertices)
	{
		aug::SCameraDesc desc;
		desc._speed = 1.0f;
		desc._sensitivity = 0.1f;
		desc._position = glm::vec3(0., 0., 1.);
		desc._aspect = float(width) / float(height);
		m_Camera = aug::Camera(desc);
		AddEventObserver(&m_Camera);
	}

	void RunDemo() 
	{
		Init();		
		Run();
		Cleanup();
	}

private:	
	
	//GBuffer pass
	aug::VertexFormat m_GBufferVertexFormat;
	std::unique_ptr<aug::Pipeline> m_pGBufferPipeline;
	std::array<std::shared_ptr<aug::Framebuffer>, MAX_FRAMES_IN_FLIGHT> m_aGBufferFBs;
	std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> m_aGBufferCBs;	
	VkFence m_GBufferFence;
	aug::DescriptorSetLayoutHandle m_hGBufferSet;

	//Main pass
	std::unique_ptr<aug::Buffer> m_pScreenTriangleVB = nullptr;
	aug::VertexFormat m_MainVertexFormat; //move to render subpass
	std::vector<aug::Buffer*> m_vUniformBuffers; //One per swap chain image

	//Scene	
	std::shared_ptr<aug::Scene> m_pScene;

	aug::Camera m_Camera;

	aug::DescriptorSetLayoutHandle m_hModelMatrixUniformSet;
	aug::DescriptorSetLayoutHandle m_hMaterialSet;

	//Utils
	aug::AssimpParser m_AssimpParser;	

	//************UNIFORMS**********
	struct UniformBufferObject 
	{
		glm::mat4 view;
		glm::mat4 proj;
	};

	struct PushConstantData
	{
		glm::mat4 model;
	};

	void CreateUniformBuffers()
	{
		for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
			m_vUniformBuffers.push_back(new aug::Buffer(sizeof(UniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, nullptr));
	}

	//***************************************
	void RenderGBuffer()
	{
		Update();

		VkCommandBuffer& cb = m_aGBufferCBs[m_uiCurrentFrame];
		vkResetCommandBuffer(cb, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);

		vkWaitForFences(aug::Context::m_VkDevice, 1, &m_GBufferFence, VK_TRUE, UINT64_MAX);

		//Begin command buffer recording
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		beginInfo.pInheritanceInfo = nullptr;

		if (vkBeginCommandBuffer(cb, &beginInfo) != VK_SUCCESS)
			throw std::runtime_error("Failed to begin recording command buffer!");

		WriteTimestamp(cb, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);

		m_pGBufferPipeline->Bind(cb);
		m_pGBufferPipeline->BeginRendering(cb, m_aGBufferFBs[m_uiCurrentFrame].get(), aug::Framebuffer::FRAMEBUFFER_LAYOUT_ATTACHMENT);		

		RecursiveRender(cb, m_pScene->GetRootNode(), glm::dmat4(1.));

		m_pGBufferPipeline->EndRendering(cb, m_aGBufferFBs[m_uiCurrentFrame].get(), aug::Framebuffer::FRAMEBUFFER_LAYOUT_SAMPLING);

		m_aGBufferFBs[m_uiCurrentFrame]->BlitToRenderTarget(cb);

		WriteTimestamp(cb, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);

		if (vkEndCommandBuffer(cb) != VK_SUCCESS)
			throw std::runtime_error("Failed to record command buffer!");

		//Submit command buffer to graphics queue
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.waitSemaphoreCount = 0;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &cb;
		submitInfo.signalSemaphoreCount = 0;

		vkResetFences(aug::Context::m_VkDevice, 1, &m_GBufferFence);

		if (vkQueueSubmit(aug::Context::m_VkGraphicsQueue, 1, &submitInfo, m_GBufferFence) != VK_SUCCESS)
			throw std::runtime_error("Failed to submit draw command buffer!");
	}

	void Init()
	{
		m_pScene = std::make_shared<aug::Scene>();
		//m_AssimpParser.LoadSceneFromFile(m_pScene, "../../Assets/KV2/kv2.FBX", "../../Assets/KV2/textures/","dds");
		//m_AssimpParser.LoadSceneFromFile(m_pScene, "../../Assets/F18/F18_opaque.FBX", "../../Assets/F18/", "dds");
		//m_AssimpParser.LoadSceneFromFile(m_pScene, "../../Assets/Cottage/Cottage.FBX", "../../Assets/Cottage/", "dds");
		//m_AssimpParser.LoadSceneFromFile(m_pScene, "../../Assets/Lighthouse/lighthouse.FBX", "../../Assets/Lighthouse/Textures/", "dds");
		//m_AssimpParser.LoadSceneFromFile(m_pScene, "../../Assets/Sponza/untitled.FBX", "../../Assets/Sponza/", "dds");
		m_AssimpParser.LoadSceneFromFile(m_pScene, "../../Assets/Voskhod/Voskhod.FBX", "../../Assets/Voskhod/", "dds");
		m_pScene->GetRootNode()->Scale(glm::dvec3(0.01));
	
		aug::Shader::SetDirectory("shaders/");

		CreateUniformBuffers();

		//Gbuffer pass
		VkFenceCreateInfo fenceInfo = {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		vkCreateFence(aug::Context::m_VkDevice, &fenceInfo, nullptr, &m_GBufferFence);
		glm::vec2 vertexData[] = 
		{ 
			{3.0,-1.0},
			{-1.0,-1.0},			
			{-1.0,3.0} 
		};
		m_pScreenTriangleVB = std::make_unique<aug::Buffer>(static_cast<uint64_t>(3 * m_GBufferVertexFormat.GetStride()),
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VMA_MEMORY_USAGE_GPU_ONLY,
			vertexData);

		uint8_t uiCount = 0;
		for (auto& fb : m_aGBufferFBs)
		{
			aug::SFramebufferDesc fbDesc;
			fbDesc._strName = "GBuffer" + std::to_string(uiCount);
			fbDesc._uiWidth = WINDOW_WIDTH;
			fbDesc._uiHeight = WINDOW_HEIGHT;
			fbDesc._vColorAttachmentsFormats.push_back(VK_FORMAT_B8G8R8A8_UNORM);//albedo
			fbDesc._vColorAttachmentsFormats.push_back(VK_FORMAT_B8G8R8A8_UNORM);//normals
			fbDesc._vColorAttachmentsFormats.push_back(VK_FORMAT_B8G8R8A8_UNORM);//roughness-metalness-ao
			fbDesc._DepthFormat = VK_FORMAT_D32_SFLOAT;
			fb = std::make_shared<aug::Framebuffer>(fbDesc);
			uiCount++;
		}

		aug::SPipelineDesc gbufferPipelineDesc;
		gbufferPipelineDesc._pRenderTarget = m_aGBufferFBs[0].get();
		gbufferPipelineDesc._shaderDesc._vShaderStages =
		{
			{VK_SHADER_STAGE_VERTEX_BIT, "gbuffer"},
			{VK_SHADER_STAGE_FRAGMENT_BIT, "gbuffer"}
		};
		gbufferPipelineDesc._vertexInputInfo = m_GBufferVertexFormat.GetPipelineVertexInputStateCreateInfo();
		gbufferPipelineDesc._uiPushConstantSize = sizeof(PushConstantData);
		m_pGBufferPipeline = std::make_unique<aug::Pipeline>(m_aGBufferFBs[0].get());
		
		aug::SDescriptorSetDesc descUB;
		descUB._uiSet = 0;
		descUB.AddBinding(0, VK_SHADER_STAGE_VERTEX_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER); //model UB
		m_hModelMatrixUniformSet = m_pGBufferPipeline->DeclareResourceLayout(descUB);
		gbufferPipelineDesc._vLayoutHandles.push_back(m_hModelMatrixUniformSet);

		aug::SDescriptorSetDesc descMat;
		descMat._uiSet = 1;
		descMat.AddBinding(0, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		descMat.AddBinding(1, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER); //albedo
		descMat.AddBinding(2, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER); //normals
		descMat.AddBinding(3, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER); //ao
		descMat.AddBinding(4, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER); //roughness
		descMat.AddBinding(5, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER); //metalness
		descMat.AddBinding(6, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER); //emissive
		m_hMaterialSet = m_pGBufferPipeline->DeclareResourceLayout(descMat);
		gbufferPipelineDesc._vLayoutHandles.push_back(m_hMaterialSet);
		m_pGBufferPipeline->Init(gbufferPipelineDesc);

		m_pGBufferPipeline->RegisterResource(m_hModelMatrixUniformSet, m_vUniformBuffers[0]);
		m_pGBufferPipeline->RegisterResource(m_hModelMatrixUniformSet, m_vUniformBuffers[1]);

		aug::SRenderPass pass;
		pass._RenderFunc = std::bind(&AugustaDemo::RenderGBuffer, this);
		AddRenderPass(pass);

		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = aug::Context::m_VkCommandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = (uint32_t)m_aGBufferCBs.size();

		if (vkAllocateCommandBuffers(aug::Context::m_VkDevice, &allocInfo, m_aGBufferCBs.data()) != VK_SUCCESS)
			throw std::runtime_error("Failed to allocate command buffers!");

		//Main pass
		aug::SPipelineDesc mainPipelineDesc;
		mainPipelineDesc._pRenderTarget = m_pWindow.get();
		mainPipelineDesc._shaderDesc._vShaderStages =
		{
			{VK_SHADER_STAGE_VERTEX_BIT, "composition"},
			{VK_SHADER_STAGE_FRAGMENT_BIT, "composition"}
		};
		mainPipelineDesc._vertexInputInfo = m_MainVertexFormat.GetPipelineVertexInputStateCreateInfo();	

		aug::SDescriptorSetDesc descGBufferTextures;
		descGBufferTextures._uiSet = 0;
		descGBufferTextures.AddBinding(0, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER); //albedo
		descGBufferTextures.AddBinding(1, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER); //normals
		descGBufferTextures.AddBinding(2, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER); //roughness-metalness-ao
		descGBufferTextures.AddBinding(3, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER); //depth
		
		m_hGBufferSet = m_pMainPipeline->DeclareResourceLayout(descGBufferTextures);
		mainPipelineDesc._vLayoutHandles.push_back(m_hGBufferSet);
		m_pMainPipeline->Init(mainPipelineDesc);

		m_pMainPipeline->RegisterResource(m_hGBufferSet, m_aGBufferFBs[0].get());
		m_pMainPipeline->RegisterResource(m_hGBufferSet, m_aGBufferFBs[1].get());
	}	

	void Update()
	{
		m_Camera.ComputeCamera();

		//update uniform buffers
		UniformBufferObject ubo;
		ubo.view = glm::mat4(m_Camera.GetViewMatrix());
		ubo.proj = glm::mat4(m_Camera.GetProjectionMatrix());
		m_vUniformBuffers[m_uiCurrentFrame]->CopyData(sizeof(UniformBufferObject), &ubo);
	}

	virtual void RenderNode(const VkCommandBuffer& commandBuffer, std::shared_ptr<aug::Node> pNode, glm::dmat4 trans) override
	{
		glm::mat4 ftrans = static_cast<glm::mat4>(trans);
		m_pGBufferPipeline->PushConstants(commandBuffer,&ftrans);

		m_pGBufferPipeline->BindResource(commandBuffer, m_hModelMatrixUniformSet, 0, m_vUniformBuffers[m_uiCurrentFrame]);

		for (uint32_t i = 0; i < pNode->GetNbMeshes(); ++i)
		{
			//Init material descriptors if not done already
			if (!pNode->GetMesh(i)->m_pMaterial->HasDescriptor(m_hMaterialSet))
			{
				pNode->GetMesh(i)->m_pMaterial->BuildUniformBuffer();
				m_pGBufferPipeline->RegisterResource(m_hMaterialSet, pNode->GetMesh(i)->m_pMaterial.get());
			}

			m_pGBufferPipeline->BindResource(commandBuffer, m_hMaterialSet, 1, pNode->GetMesh(i)->m_pMaterial.get());

			pNode->GetMesh(i)->Draw(commandBuffer);
		}
	}

	virtual void MainRenderPass(const VkCommandBuffer& cb) override
	{
		m_pMainPipeline->BindResource(cb, m_hGBufferSet, 0, m_aGBufferFBs[m_uiCurrentFrame].get());

		//Draw screen triangle
		VkBuffer vertexBuffers[] = { m_pScreenTriangleVB->GetBufferHandle() };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(cb, 0, 1, vertexBuffers, offsets);
		vkCmdDraw(cb, 3, 1, 0, 0);
	}

	void Cleanup() 
	{		
		vkDestroyFence(aug::Context::m_VkDevice, m_GBufferFence, nullptr);

		for (auto elem : m_vUniformBuffers)
			delete elem;
	}
};

int main() 
{
	AugustaDemo app("Augusta", WINDOW_WIDTH, WINDOW_HEIGHT);

	try 
	{
		app.RunDemo();
	}
	catch (const std::exception& e)
	{
		std::cerr << "Exception thrown: " <<e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}