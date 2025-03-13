#include <Augusta/Application.h>
#include <Augusta/ShaderFactory.h>
#include <Augusta/Camera.h>
#include <Augusta/AssimpParser.h>
#include <iostream>
#include <stdexcept>

#define WINDOW_WIDTH	1920
#define WINDOW_HEIGHT	1080

class AugustaDemo : public aug::Application
{
public:
	AugustaDemo(const std::string& name, uint16_t width, uint16_t height)
		:	aug::Application(name, width, height),
			m_VertexFormat({ aug::VERTEX_FORMAT_VEC3F32,aug::VERTEX_FORMAT_VEC3F32, aug::VERTEX_FORMAT_VEC2F32 }),
			m_AssimpParser(aug::VERTEX_COMPONENT_NORMAL|aug::VERTEX_COMPONENT_TEXCOORD, true, aiProcess_Triangulate|aiProcess_PreTransformVertices)
	{
		aug::SCameraDesc desc;
		desc.speed = 0.001f;
		desc.sensitivity = 0.1f;
		desc.position = glm::vec3(0., 0., 1.);
		desc.aspect = float(width) / float(height);
		m_Camera = aug::Camera(desc);
		AddEventObserver(&m_Camera);
	}

	void RunTriangleApp() 
	{
		Init();		
		Run();
		Cleanup();
	}

private:	
	
	aug::VertexFormat m_VertexFormat; //move to render subpass
	std::vector<aug::Buffer*> m_vUniformBuffers; //One per swap chain image
	VkDescriptorSetLayout m_VkDescriptorSetLayout = VK_NULL_HANDLE;
	VkDescriptorPool m_VkDescriptorPool = VK_NULL_HANDLE;
	std::vector<VkDescriptorSet> m_vDescriptorSets;

	VkCommandBuffer m_ActiveCommandBuffer = VK_NULL_HANDLE;

	//Scene	
	std::shared_ptr<aug::Scene> m_pScene;

	aug::Camera m_Camera;

	//Utils
	aug::AssimpParser m_AssimpParser;

	void CreateDescriptorSetLayout()
	{
		VkDescriptorSetLayoutBinding uboLayoutBinding{};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		uboLayoutBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding samplerLayoutBinding{};
		samplerLayoutBinding.binding = 1;
		samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerLayoutBinding.descriptorCount = 1;
		samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		samplerLayoutBinding.pImmutableSamplers = nullptr;

		std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();

		if (vkCreateDescriptorSetLayout(aug::Context::m_VkDevice, &layoutInfo, nullptr, &m_VkDescriptorSetLayout) != VK_SUCCESS)
			throw std::runtime_error("Failed to create descriptor set layout!");
	}

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
		for (uint32_t i = 0; i < m_pWindow->GetSwapChainImageCount(); ++i)
			m_vUniformBuffers.push_back(new aug::Buffer(sizeof(UniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, nullptr));
	}

	void CreateDescriptorPool()
	{
		std::array<VkDescriptorPoolSize,2> poolSizes{};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = m_pWindow->GetSwapChainImageCount();
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount = m_pWindow->GetSwapChainImageCount();

		VkDescriptorPoolCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		createInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		createInfo.pPoolSizes = poolSizes.data();
		createInfo.maxSets = m_pWindow->GetSwapChainImageCount();
		if (vkCreateDescriptorPool(aug::Context::m_VkDevice, &createInfo, nullptr, &m_VkDescriptorPool) != VK_SUCCESS)
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
		if (vkAllocateDescriptorSets(aug::Context::m_VkDevice, &allocInfo, m_vDescriptorSets.data()) != VK_SUCCESS)
			throw std::runtime_error("Failed to allocate descriptor sets!");

		for (size_t i = 0; i < m_pWindow->GetSwapChainImageCount(); i++)
		{
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = m_vUniformBuffers[i]->GetBufferHandle();
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(UniformBufferObject);

			std::array<VkWriteDescriptorSet,2> descriptorWrites{};
			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = m_vDescriptorSets[i];
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pBufferInfo = &bufferInfo;

			/*descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[1].dstSet = m_vDescriptorSets[i];
			descriptorWrites[1].dstBinding = 1;
			descriptorWrites[1].dstArrayElement = 0;
			descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[1].descriptorCount = 1;
			descriptorWrites[1].pImageInfo = &imageInfo;

			vkUpdateDescriptorSets(aug::Context::m_VkDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);*/
			vkUpdateDescriptorSets(aug::Context::m_VkDevice, 1, &descriptorWrites[0], 0, nullptr);
		}
	}

	//***************************************

	void Init()
	{
		m_pScene = std::make_shared<aug::Scene>();
		m_AssimpParser.LoadSceneFromFile(m_pScene, "../../Assets/KV2/kv2.FBX", "../../Assets/KV2/textures/");
		m_pScene->GetRootNode()->Scale(glm::dvec3(0.01));

		CreateDescriptorSetLayout();
		
		aug::ShaderModule vertShaderModule("shaders/shader.vert", VK_SHADER_STAGE_VERTEX_BIT);
		aug::ShaderModule fragShaderModule("shaders/shader.frag", VK_SHADER_STAGE_FRAGMENT_BIT);

		aug::SGraphicsPipelineDesc desc;
		desc.pWindow = m_pWindow.get();
		desc.vShaderStages = { vertShaderModule.GetPipelineShaderModuleCreateInfo(), fragShaderModule.GetPipelineShaderModuleCreateInfo() };
		desc.vertexInputInfo = m_VertexFormat.GetPipelineVertexInputStateCreateInfo();
		desc.uiPushConstantSize = sizeof(PushConstantData);
		desc.pDescriptorSetLayout = &m_VkDescriptorSetLayout;

		m_pGraphicsPipeline->Init(desc);

		CreateUniformBuffers();
		CreateDescriptorPool();
		CreateDescriptorSets();		
	}

	void Update()
	{
		m_Camera.ComputeCamera();

		//update uniform buffers
		UniformBufferObject ubo;
		ubo.view = glm::mat4(m_Camera.GetViewMatrix());
		ubo.proj = glm::mat4(m_Camera.GetProjectionMatrix());
		m_vUniformBuffers[m_uiCurrentImageIndex]->CopyData(sizeof(UniformBufferObject), &ubo);
	}

	virtual void RenderNode(std::shared_ptr<aug::Node> pNode, glm::dmat4 trans) override
	{
		glm::mat4 ftrans = static_cast<glm::mat4>(trans);
		vkCmdPushConstants(
			m_ActiveCommandBuffer,
			m_pGraphicsPipeline->GetPipelineLayout(),
			VK_SHADER_STAGE_VERTEX_BIT,
			0,
			sizeof(PushConstantData),
			&ftrans);

		for (uint32_t i = 0; i < pNode->GetNbMeshes(); ++i)
		{
			pNode->GetMesh(i)->Draw(m_ActiveCommandBuffer);
		}
	}

	virtual void Render(VkCommandBuffer commandBuffer) override
	{
		m_ActiveCommandBuffer = commandBuffer;

		Update();

		m_pGraphicsPipeline->Bind(commandBuffer, 1, &m_vDescriptorSets[m_uiCurrentImageIndex]);

		RecursiveRender(m_pScene->GetRootNode(), glm::dmat4(1.));
	}

	void Cleanup() 
	{		
		for (uint32_t i = 0; i < m_pWindow->GetSwapChainImageCount(); ++i)
			delete m_vUniformBuffers[i];

		vkDestroyDescriptorPool(aug::Context::m_VkDevice, m_VkDescriptorPool, nullptr);

		vkDestroyDescriptorSetLayout(aug::Context::m_VkDevice, m_VkDescriptorSetLayout, nullptr);
	}
};

int main() 
{
	AugustaDemo app("Augusta", WINDOW_WIDTH, WINDOW_HEIGHT);

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