#include <Augusta/Application.h>
#include <Augusta/ShaderFactory.h>
#include <Augusta/DescriptorFactory.h>
#include <Augusta/Camera.h>
#include <Augusta/AssimpParser.h>
#include <Augusta/Material.h>
#include <Augusta/Utils.h>
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
		desc._speed = 10.0f;
		desc._sensitivity = 0.1f;
		desc._position = glm::vec3(0., 5., 10.);
		desc._aspect = float(width) / float(height);
		desc._yaw = 0.;
		desc._pitch = 30.;
		desc._roll = 0.;
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
	aug::DescriptorSetLayoutHandle m_hGBufferUBOSet;
	aug::DescriptorSetLayoutHandle m_hMaterialSet;

	//Deferred pass
	std::unique_ptr<aug::Pipeline> m_pDeferredPipeline;
	std::array<std::shared_ptr<aug::Framebuffer>, MAX_FRAMES_IN_FLIGHT> m_aDeferredFBs;
	std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> m_aDeferredCBs;
	VkFence m_DeferredFence;
	aug::DescriptorSetLayoutHandle m_hDeferredSet;
	aug::DescriptorSetLayoutHandle m_hDeferredUBOSet;

	//Main pass
	std::unique_ptr<aug::Buffer> m_pScreenTriangleVB = nullptr;
	aug::VertexFormat m_MainVertexFormat; //move to render subpass	

	//Scene	
	std::vector<std::shared_ptr<aug::Scene>> m_vScenes;

	//Cubemap
	std::shared_ptr<aug::Mesh> m_pSphere = nullptr;
	std::shared_ptr<aug::Texture> m_pHDRCubemap = nullptr;
	aug::DescriptorSetLayoutHandle m_hHDRCubemapSet;

	aug::Camera m_Camera;
	
	std::vector<aug::Buffer*> m_vMainUBOs; //One per swap chain image	

	//Utils
	aug::AssimpParser m_AssimpParser;	

	//************UNIFORMS**********
	struct UniformBufferObject 
	{
		glm::mat4 _view;
		glm::mat4 _proj;
		glm::vec3 _camPos;
	};

	struct PushConstantData
	{
		glm::mat4 _model;
	};

	void CreateUniformBuffers()
	{
		for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
			m_vMainUBOs.push_back(new aug::Buffer(sizeof(UniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, nullptr));
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

		for(auto& scene : m_vScenes)
			RecursiveRender(cb, scene->GetRootNode(), glm::dmat4(1.));

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

	void RenderDeferred()
	{
		VkCommandBuffer& cb = m_aDeferredCBs[m_uiCurrentFrame];
		vkResetCommandBuffer(cb, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);

		vkWaitForFences(aug::Context::m_VkDevice, 1, &m_DeferredFence, VK_TRUE, UINT64_MAX);

		//Begin command buffer recording
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		beginInfo.pInheritanceInfo = nullptr;

		if (vkBeginCommandBuffer(cb, &beginInfo) != VK_SUCCESS)
			throw std::runtime_error("Failed to begin recording command buffer!");

		WriteTimestamp(cb, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);

		m_pDeferredPipeline->Bind(cb);
		m_pDeferredPipeline->BeginRendering(cb, m_aDeferredFBs[m_uiCurrentFrame].get(), aug::Framebuffer::FRAMEBUFFER_LAYOUT_ATTACHMENT);

		m_pDeferredPipeline->BindResource(cb, m_hDeferredUBOSet, 0, m_vMainUBOs[m_uiCurrentFrame]);

		m_pDeferredPipeline->BindResource(cb, m_hGBufferSet, 1, m_aGBufferFBs[m_uiCurrentFrame].get());

		m_pDeferredPipeline->BindResource(cb, m_hHDRCubemapSet, 2, m_pHDRCubemap.get());

		//Draw screen triangle
		VkBuffer vertexBuffers[] = { m_pScreenTriangleVB->GetBufferHandle() };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(cb, 0, 1, vertexBuffers, offsets);
		vkCmdDraw(cb, 3, 1, 0, 0);

		m_pDeferredPipeline->EndRendering(cb, m_aDeferredFBs[m_uiCurrentFrame].get(), aug::Framebuffer::FRAMEBUFFER_LAYOUT_SAMPLING);

		m_aDeferredFBs[m_uiCurrentFrame]->BlitToRenderTarget(cb);

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

		vkResetFences(aug::Context::m_VkDevice, 1, &m_DeferredFence);

		if (vkQueueSubmit(aug::Context::m_VkGraphicsQueue, 1, &submitInfo, m_DeferredFence) != VK_SUCCESS)
			throw std::runtime_error("Failed to submit draw command buffer!");
	}

	void InitGBuffer()
	{
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
			fbDesc._strName = "00_GBuffer" + std::to_string(uiCount);
			fbDesc._uiWidth = WINDOW_WIDTH;
			fbDesc._uiHeight = WINDOW_HEIGHT;
			fbDesc._vColorAttachmentsFormats.push_back(VK_FORMAT_B8G8R8A8_UNORM);//albedo-roughness
			fbDesc._vColorAttachmentsFormats.push_back(VK_FORMAT_R16G16B16A16_SFLOAT);//normals-metalness
			fbDesc._vColorAttachmentsFormats.push_back(VK_FORMAT_R16G16B16A16_SFLOAT);//emissive-ao
			fbDesc._DepthFormat = VK_FORMAT_D24_UNORM_S8_UINT;
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
		m_hGBufferUBOSet = m_pGBufferPipeline->DeclareResourceLayout(descUB);
		gbufferPipelineDesc._vLayoutHandles.push_back(m_hGBufferUBOSet);

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

		m_pGBufferPipeline->RegisterResource(m_hGBufferUBOSet, m_vMainUBOs[0]);
		m_pGBufferPipeline->RegisterResource(m_hGBufferUBOSet, m_vMainUBOs[1]);

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
	}

	void InitDeferred()
	{
		VkFenceCreateInfo fenceInfo = {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		vkCreateFence(aug::Context::m_VkDevice, &fenceInfo, nullptr, &m_DeferredFence);

		uint8_t uiCount = 0;
		for (auto& fb : m_aDeferredFBs)
		{
			aug::SFramebufferDesc fbDesc;
			fbDesc._strName = "00_Deferred" + std::to_string(uiCount);
			fbDesc._uiWidth = WINDOW_WIDTH;
			fbDesc._uiHeight = WINDOW_HEIGHT;
			fbDesc._vColorAttachmentsFormats.push_back(VK_FORMAT_R16G16B16A16_SFLOAT);//HDR
			fb = std::make_shared<aug::Framebuffer>(fbDesc);
			uiCount++;
		}

		aug::SPipelineDesc deferredPipelineDesc;
		deferredPipelineDesc._pRenderTarget = m_aDeferredFBs[0].get();
		deferredPipelineDesc._shaderDesc._vShaderStages =
		{
			{VK_SHADER_STAGE_VERTEX_BIT, "deferred"},
			{VK_SHADER_STAGE_FRAGMENT_BIT, "deferred"}
		};
		deferredPipelineDesc._vertexInputInfo = m_MainVertexFormat.GetPipelineVertexInputStateCreateInfo();
		deferredPipelineDesc._uiPushConstantSize = sizeof(PushConstantData);
		m_pDeferredPipeline = std::make_unique<aug::Pipeline>(m_aDeferredFBs[0].get());

		aug::SDescriptorSetDesc descUB;
		descUB._uiSet = 0;
		descUB.AddBinding(0, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER); //model UB
		m_hDeferredUBOSet = m_pDeferredPipeline->DeclareResourceLayout(descUB);
		deferredPipelineDesc._vLayoutHandles.push_back(m_hDeferredUBOSet);

		aug::SDescriptorSetDesc GBufferTexturesSetDesc;
		GBufferTexturesSetDesc._uiSet = 1;
		GBufferTexturesSetDesc.AddBinding(0, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER); //albedo
		GBufferTexturesSetDesc.AddBinding(1, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER); //normals
		GBufferTexturesSetDesc.AddBinding(2, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER); //roughness-metalness-ao
		GBufferTexturesSetDesc.AddBinding(3, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER); //depth
		m_hGBufferSet = m_pDeferredPipeline->DeclareResourceLayout(GBufferTexturesSetDesc);

		aug::SDescriptorSetDesc hdrCubemapSetDesc;
		hdrCubemapSetDesc._uiSet = 2;
		hdrCubemapSetDesc.AddBinding(0, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		m_hHDRCubemapSet = m_pDeferredPipeline->DeclareResourceLayout(hdrCubemapSetDesc);
		
		deferredPipelineDesc._vLayoutHandles.push_back(m_hGBufferSet);
		deferredPipelineDesc._vLayoutHandles.push_back(m_hHDRCubemapSet);
		m_pDeferredPipeline->Init(deferredPipelineDesc);

		m_pDeferredPipeline->RegisterResource(m_hDeferredUBOSet, m_vMainUBOs[0]);
		m_pDeferredPipeline->RegisterResource(m_hDeferredUBOSet, m_vMainUBOs[1]);

		m_pDeferredPipeline->RegisterResource(m_hGBufferSet, m_aGBufferFBs[0].get());
		m_pDeferredPipeline->RegisterResource(m_hGBufferSet, m_aGBufferFBs[1].get());

		m_pDeferredPipeline->RegisterResource(m_hHDRCubemapSet, m_pHDRCubemap.get());

		aug::SRenderPass pass;
		pass._RenderFunc = std::bind(&AugustaDemo::RenderDeferred, this);
		AddRenderPass(pass);

		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = aug::Context::m_VkCommandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = (uint32_t)m_aDeferredCBs.size();

		if (vkAllocateCommandBuffers(aug::Context::m_VkDevice, &allocInfo, m_aDeferredCBs.data()) != VK_SUCCESS)
			throw std::runtime_error("Failed to allocate command buffers!");
	}

	void InitGround()
	{
		std::shared_ptr<aug::Scene> pScene = std::make_shared<aug::Scene>();
		m_vScenes.emplace_back(pScene);

		float half = 5;
		float vertices[32] = {
				-half,0.,-half,		0.,1.,0.,    0.,0.,
				-half,0., half, 	0., 1., 0.,	 0., half,
				half,0., half, 		0., 1., 0.,	 half, half,
				half,0., -half,		0., 1., 0.,	 half, 0.
		};

		aug::SMeshDesc groundMeshDesc;
		groundMeshDesc._usage = aug::MESH_USAGE_STATIC;
		groundMeshDesc._pFormat = &m_GBufferVertexFormat;
		groundMeshDesc._vertexCount = 4;
		groundMeshDesc._vertexData = vertices;
		groundMeshDesc._indexCount = 6;
		uint32_t aIndices[6] = { 0, 1, 2, 0, 2, 3 };
		groundMeshDesc._indexData = aIndices;

		std::shared_ptr<aug::Material> pGroundMat = aug::MaterialFactory::CreateMaterial("Ground");
		aug::TextureFactory::AddTexturePath("../../Assets/PBR/WetConcrete/");
		pGroundMat->m_aTextures[aug::TEXTURE_CHANNEL_ALBEDO] = aug::TextureFactory::LoadTextureFromFile("Albedo.dds");
		pGroundMat->m_aTextures[aug::TEXTURE_CHANNEL_NORMAL] = aug::TextureFactory::LoadTextureFromFile("Normal.dds");
		pGroundMat->m_aTextures[aug::TEXTURE_CHANNEL_ROUGHNESS] = aug::TextureFactory::LoadTextureFromFile("Roughness.dds");
		pGroundMat->m_Desc._iTexMask = TEXTURE_CHANNEL_ALBEDO_BIT | TEXTURE_CHANNEL_NORMAL_BIT | TEXTURE_CHANNEL_ROUGHNESS_BIT;
		groundMeshDesc._pMaterial = pGroundMat;
		pScene->CreateMesh(groundMeshDesc, pScene->GetRootNode());
	}

	void InitCubemap()
	{
		std::shared_ptr<aug::Scene> pScene = std::make_shared<aug::Scene>();
		m_vScenes.emplace_back(pScene);

		aug::TextureFactory::AddTexturePath("../../Assets/HDR");
		m_pHDRCubemap = aug::TextureFactory::LoadTextureFromFile("03_hangar.hdr");

		std::shared_ptr<aug::Material> pCubemapMat = aug::MaterialFactory::CreateMaterial("Cubemap");
		pCubemapMat->m_aTextures[aug::TEXTURE_CHANNEL_EMISSIVE] = m_pHDRCubemap;
		pCubemapMat->m_Desc._iTexMask = TEXTURE_CHANNEL_EMISSIVE_BIT;		

		struct SVertex
		{
			glm::vec3 pos;
			glm::vec3 normal;
			glm::vec2 uv;
		};
		
		uint32_t iRes = 100;
		double dRadius = 1000;

		//face geometry patch
		std::vector<SVertex> vVertices;
		std::vector<uint32_t> vIndices;
		for (uint32_t i = 0; i < iRes; ++i)
		{
			for (uint32_t j = 0; j < iRes; ++j)
			{
				SVertex vertex;
				vertex.uv = glm::vec2(float(j) / (iRes - 1), float(i) / (iRes - 1));
				float alpha = 2. * PI * vertex.uv.x;
				float beta = PI * (-0.5 + vertex.uv.y);
				vertex.pos = float(dRadius) * glm::vec3(-cos(beta) * cos(alpha), sin(beta), cos(beta) * sin(alpha));
				vertex.normal = -glm::normalize(vertex.pos);
				vertex.uv *= -1;
				vVertices.push_back(vertex);

				if (i < iRes - 1 && j < iRes - 1)
				{
					vIndices.push_back(i * iRes + j);
					vIndices.push_back((i + 1)* iRes + j);
					vIndices.push_back(i * iRes + j + 1);					

					vIndices.push_back((i + 1) * iRes + j);
					vIndices.push_back((i + 1)* iRes + j + 1);
					vIndices.push_back(i * iRes + j + 1);					
				}
			}
		}


		aug::SMeshDesc cubeMeshDesc;
		cubeMeshDesc._usage = aug::MESH_USAGE_STATIC;
		cubeMeshDesc._pFormat = &m_GBufferVertexFormat;
		cubeMeshDesc._vertexCount = vVertices.size();
		cubeMeshDesc._vertexData = vVertices.data();
		cubeMeshDesc._indexCount = vIndices.size();
		cubeMeshDesc._indexData = vIndices.data();
		cubeMeshDesc._pMaterial = pCubemapMat;
		m_pSphere = pScene->CreateMesh(cubeMeshDesc, pScene->GetRootNode());
	}

	void InitTestSphere()
	{
		std::shared_ptr<aug::Scene> pScene = std::make_shared<aug::Scene>();
		m_vScenes.emplace_back(pScene);

		std::shared_ptr<aug::Material> pMat = aug::MaterialFactory::CreateMaterial("Sphere");
		pMat->m_Desc._Albedo = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);

		struct SVertex
		{
			glm::vec3 pos;
			glm::vec3 normal;
			glm::vec2 uv;
		};

		uint32_t iRes = 100;
		double dRadius = 1;

		//face geometry patch
		std::vector<SVertex> vVertices;
		std::vector<uint32_t> vIndices;
		for (uint32_t i = 0; i < iRes; ++i)
		{
			for (uint32_t j = 0; j < iRes; ++j)
			{
				SVertex vertex;
				vertex.uv = glm::vec2(float(j) / (iRes - 1), float(i) / (iRes - 1));
				float alpha = 2. * PI * vertex.uv.x;
				float beta = PI * (-0.5 + vertex.uv.y);
				vertex.pos = float(dRadius) * glm::vec3(-cos(beta) * cos(alpha), sin(beta), cos(beta) * sin(alpha));
				vertex.normal = glm::normalize(vertex.pos);
				vertex.uv *= -1;
				vVertices.push_back(vertex);

				if (i < iRes - 1 && j < iRes - 1)
				{
					vIndices.push_back(i * iRes + j);					
					vIndices.push_back(i * iRes + j + 1);
					vIndices.push_back((i + 1)* iRes + j);

					vIndices.push_back((i + 1) * iRes + j);					
					vIndices.push_back(i * iRes + j + 1);
					vIndices.push_back((i + 1)* iRes + j + 1);
				}
			}
		}


		aug::SMeshDesc cubeMeshDesc;
		cubeMeshDesc._usage = aug::MESH_USAGE_STATIC;
		cubeMeshDesc._pFormat = &m_GBufferVertexFormat;
		cubeMeshDesc._vertexCount = vVertices.size();
		cubeMeshDesc._vertexData = vVertices.data();
		cubeMeshDesc._indexCount = vIndices.size();
		cubeMeshDesc._indexData = vIndices.data();
		cubeMeshDesc._pMaterial = pMat;
		pScene->CreateMesh(cubeMeshDesc, pScene->GetRootNode());

		pScene->GetRootNode()->Translate(glm::dvec3(0., 1., 0.));
	}

	void Init()
	{
		std::shared_ptr<aug::Scene> pScene = std::make_shared<aug::Scene>();
		m_vScenes.emplace_back(pScene);
		//m_AssimpParser.LoadSceneFromFile(pScene, "../../Assets/KV2/kv2.FBX", "../../Assets/KV2/textures/","dds"); pScene->GetRootNode()->Scale(glm::dvec3(0.05));
		//m_AssimpParser.LoadSceneFromFile(m_pScene, "../../Assets/F18/F18_opaque.FBX", "../../Assets/F18/", "dds"); m_pScene->GetRootNode()->Scale(glm::dvec3(0.001));
		//m_AssimpParser.LoadSceneFromFile(m_pScene, "../../Assets/Cottage/Cottage.FBX", "../../Assets/Cottage/", "dds");
		//m_AssimpParser.LoadSceneFromFile(m_pScene, "../../Assets/Lighthouse/lighthouse.FBX", "../../Assets/Lighthouse/Textures/", "dds");
		//m_AssimpParser.LoadSceneFromFile(m_pScene, "../../Assets/Sponza/untitled.FBX", "../../Assets/Sponza/", "dds");
		//m_AssimpParser.LoadSceneFromFile(m_pScene, "../../Assets/Voskhod/Voskhod.FBX", "../../Assets/Voskhod/", "dds"); m_pScene->GetRootNode()->Scale(glm::dvec3(0.01));
		m_AssimpParser.LoadSceneFromFile(pScene, "../../Assets/Viper/FINAL_MODEL_96.FBX", "../../Assets/Viper/"); pScene->GetRootNode()->Scale(glm::dvec3(0.01));
		//m_AssimpParser.LoadSceneFromFile(m_pScene, "../../Assets/Bistro_v5_2/BistroExterior.FBX", "../../Assets/Bistro_v5_2/Textures");
		
		InitGround();

		InitCubemap();

		InitTestSphere();

		aug::Shader::SetDirectory("shaders/");

		CreateUniformBuffers();

		InitGBuffer();

		InitDeferred();

		//Main pass (composition)
		aug::SPipelineDesc mainPipelineDesc;
		mainPipelineDesc._pRenderTarget = m_pWindow.get();
		mainPipelineDesc._shaderDesc._vShaderStages =
		{
			{VK_SHADER_STAGE_VERTEX_BIT, "composition"},
			{VK_SHADER_STAGE_FRAGMENT_BIT, "composition"}
		};
		mainPipelineDesc._vertexInputInfo = m_MainVertexFormat.GetPipelineVertexInputStateCreateInfo();	

		aug::SDescriptorSetDesc descCompositionTextures;
		descCompositionTextures._uiSet = 0;
		descCompositionTextures.AddBinding(0, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER); //HDR texture
		
		m_hDeferredSet = m_pMainPipeline->DeclareResourceLayout(descCompositionTextures);
		mainPipelineDesc._vLayoutHandles.push_back(m_hDeferredSet);
		m_pMainPipeline->Init(mainPipelineDesc);

		m_pMainPipeline->RegisterResource(m_hDeferredSet, m_aDeferredFBs[0].get());
		m_pMainPipeline->RegisterResource(m_hDeferredSet, m_aDeferredFBs[1].get());
	}	

	void Update()
	{
		m_Camera.ComputeCamera();

		//update uniform buffers
		UniformBufferObject ubo;
		ubo._view = glm::mat4(m_Camera.GetViewMatrix());
		ubo._proj = glm::mat4(m_Camera.GetProjectionMatrix());
		ubo._camPos = m_Camera.GetPosition();
		m_vMainUBOs[m_uiCurrentFrame]->CopyData(sizeof(UniformBufferObject), &ubo);
	}

	virtual void RenderNode(const VkCommandBuffer& commandBuffer, std::shared_ptr<aug::Node> pNode, glm::dmat4 trans) override
	{
		glm::mat4 ftrans = static_cast<glm::mat4>(trans);
		m_pGBufferPipeline->PushConstants(commandBuffer,&ftrans);

		m_pGBufferPipeline->BindResource(commandBuffer, m_hGBufferUBOSet, 0, m_vMainUBOs[m_uiCurrentFrame]);

		for (uint32_t i = 0; i < pNode->GetNbMeshes(); ++i)
		{
			std::shared_ptr<aug::Material> pMat = pNode->GetMesh(i)->m_pMaterial;
			if (pMat)
			{
				//Init material descriptors if not done already
				if (!pMat->HasDescriptor(m_hMaterialSet))
				{
					pMat->BuildUniformBuffer();
					m_pGBufferPipeline->RegisterResource(m_hMaterialSet, pNode->GetMesh(i)->m_pMaterial.get());
				}

				m_pGBufferPipeline->BindResource(commandBuffer, m_hMaterialSet, 1, pNode->GetMesh(i)->m_pMaterial.get());
			}

			pNode->GetMesh(i)->Draw(commandBuffer);
		}
	}

	virtual void MainRenderPass(const VkCommandBuffer& cb) override
	{
		m_pMainPipeline->BindResource(cb, m_hDeferredSet, 0, m_aDeferredFBs[m_uiCurrentFrame].get());

		//Draw screen triangle
		VkBuffer vertexBuffers[] = { m_pScreenTriangleVB->GetBufferHandle() };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(cb, 0, 1, vertexBuffers, offsets);
		vkCmdDraw(cb, 3, 1, 0, 0);
	}

	void Cleanup() 
	{		
		vkDestroyFence(aug::Context::m_VkDevice, m_GBufferFence, nullptr);

		for (auto elem : m_vMainUBOs)
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
		aug::Debug::Log(aug::LOG_TYPE_ERROR, std::format("Exception thrown: {}", e.what()));
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}