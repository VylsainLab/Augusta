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

	void RunDemo() 
	{
		Init();		
		Run();
		Cleanup();
	}

private:	
	
	aug::VertexFormat m_VertexFormat; //move to render subpass

	aug::Shader* m_pShader = nullptr;
	std::vector<aug::Buffer*> m_vUniformBuffers; //One per swap chain image

	VkCommandBuffer m_ActiveCommandBuffer = VK_NULL_HANDLE;

	//Scene	
	std::shared_ptr<aug::Scene> m_pScene;

	aug::Camera m_Camera;

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
		m_AssimpParser.LoadSceneFromFile(m_pScene, "../../Assets/KV2/kv2.FBX", "../../Assets/KV2/textures/");
		m_pScene->GetRootNode()->Scale(glm::dvec3(0.01));
	
		aug::Shader::SetPath("shaders/");
		m_pShader = new aug::Shader("shader", VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

		CreateUniformBuffers();

		aug::SPipelineDesc desc;
		desc.pWindow = m_pWindow.get();
		desc.pShader = m_pShader;
		desc.vertexInputInfo = m_VertexFormat.GetPipelineVertexInputStateCreateInfo();
		desc.uiPushConstantSize = sizeof(PushConstantData);
		desc.pvUniformBuffers = &m_vUniformBuffers;

		m_pPipeline->Init(desc);

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

		for (uint32_t i = 0; i < pNode->GetNbMeshes(); ++i)
		{
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

		delete m_pShader;
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
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}