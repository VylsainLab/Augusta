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

	void RunDemo() 
	{
		Init();		
		Run();
		Cleanup();
	}

private:	
	
	aug::VertexFormat m_VertexFormat; //move to render subpass

	std::vector<aug::Buffer*> m_vUniformBuffers; //One per swap chain image

	VkCommandBuffer m_ActiveCommandBuffer = VK_NULL_HANDLE;

	//Scene	
	std::shared_ptr<aug::Scene> m_pScene;

	aug::Camera m_Camera;

	aug::DescriptorSetLayoutHandle m_hModelMatrixUniformSet;
	std::array<aug::DescriptorSetHandle, MAX_FRAMES_IN_FLIGHT> m_aModelMatrixDescriptors;
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

	void Init()
	{
		m_pScene = std::make_shared<aug::Scene>();
		m_AssimpParser.LoadSceneFromFile(m_pScene, "../../Assets/KV2/kv2.FBX", "../../Assets/KV2/textures/","dds");
		m_pScene->GetRootNode()->Scale(glm::dvec3(0.01));
	
		aug::Shader::SetDirectory("shaders/");

		CreateUniformBuffers();

		aug::SPipelineDesc desc;
		desc.pWindow = m_pWindow.get();
		desc.shaderDesc.vShaderStages =
		{
			{VK_SHADER_STAGE_VERTEX_BIT, "shader"},
			{VK_SHADER_STAGE_FRAGMENT_BIT, "shader"}
		};
		desc.vertexInputInfo = m_VertexFormat.GetPipelineVertexInputStateCreateInfo();
		desc.uiPushConstantSize = sizeof(PushConstantData);
		//desc.pvUniformBuffers = &m_vUniformBuffers;

		aug::SDescriptorSetDesc descUB;
		descUB.uiSet = 0;
		descUB.AddBinding(0, VK_SHADER_STAGE_VERTEX_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER); //model UB
		m_hModelMatrixUniformSet = aug::DescriptorFactory::AllocateDescriptorSetLayout(descUB);
		desc.vLayoutHandles.push_back(m_hModelMatrixUniformSet);

		aug::SDescriptorSetDesc descMat;	
		descMat.uiSet = 1;
		descMat.AddBinding(0, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER); //albedo
		descMat.AddBinding(1, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER); //normals
		descMat.AddBinding(2, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER); //ao
		descMat.AddBinding(3, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER); //roughness
		descMat.AddBinding(4, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER); //metalness
		descMat.AddBinding(5, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER); //emissive
		m_hMaterialSet = aug::DescriptorFactory::AllocateDescriptorSetLayout(descMat);
		desc.vLayoutHandles.push_back(m_hMaterialSet);

		m_pPipeline->Init(desc);

		aug::DescriptorFactory::AllocateDescriptors(m_hModelMatrixUniformSet, MAX_FRAMES_IN_FLIGHT, m_aModelMatrixDescriptors.data());
		aug::DescriptorFactory::UpdateDescriptors(m_hModelMatrixUniformSet, MAX_FRAMES_IN_FLIGHT, m_aModelMatrixDescriptors.data(), m_vUniformBuffers.data());


		InitImGui();
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

	virtual void RenderNode(std::shared_ptr<aug::Node> pNode, glm::dmat4 trans) override
	{
		glm::mat4 ftrans = static_cast<glm::mat4>(trans);
		m_pPipeline->PushConstants(m_ActiveCommandBuffer ,&ftrans);

		m_pPipeline->BindDescriptorSets(m_ActiveCommandBuffer, m_hModelMatrixUniformSet, 1, &m_aModelMatrixDescriptors[m_uiCurrentFrame]);

		for (uint32_t i = 0; i < pNode->GetNbMeshes(); ++i)
		{
			m_pPipeline->UpdateDescriptorSets(m_ActiveCommandBuffer, pNode->GetMesh(i)->m_pMaterial, m_uiCurrentFrame);

			pNode->GetMesh(i)->Draw(m_ActiveCommandBuffer);
		}
	}

	virtual void Render(VkCommandBuffer commandBuffer) override
	{
		m_ActiveCommandBuffer = commandBuffer;

		Update();

		RecursiveRender(m_pScene->GetRootNode(), glm::dmat4(1.));

		ImGui::ShowDemoWindow();
	}

	void Cleanup() 
	{		
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