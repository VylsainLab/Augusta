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
	/*VkDescriptorSetLayout m_VkDescriptorSetLayout = VK_NULL_HANDLE;
	VkDescriptorPool m_VkDescriptorPool = VK_NULL_HANDLE;
	std::vector<VkDescriptorSet> m_vDescriptorSets;*/

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
		for (uint32_t i = 0; i < m_pWindow->GetSwapChainImageCount(); ++i)
			m_vUniformBuffers.push_back(new aug::Buffer(sizeof(UniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, nullptr));
	}

	//***************************************

	void Init()
	{
		m_pScene = std::make_shared<aug::Scene>();
		m_AssimpParser.LoadSceneFromFile(m_pScene, "../../Assets/KV2/kv2.FBX", "../../Assets/KV2/textures/");
		m_pScene->GetRootNode()->Scale(glm::dvec3(0.01));
		
		aug::ShaderModule vertShaderModule("shaders/shader.vert", VK_SHADER_STAGE_VERTEX_BIT);
		aug::ShaderModule fragShaderModule("shaders/shader.frag", VK_SHADER_STAGE_FRAGMENT_BIT);

		CreateUniformBuffers();

		aug::SGraphicsPipelineDesc desc;
		desc.pWindow = m_pWindow.get();
		desc.vShaderStages = { vertShaderModule.GetPipelineShaderModuleCreateInfo(), fragShaderModule.GetPipelineShaderModuleCreateInfo() };
		desc.vertexInputInfo = m_VertexFormat.GetPipelineVertexInputStateCreateInfo();
		desc.uiPushConstantSize = sizeof(PushConstantData);
		desc.pvUniformBuffers = &m_vUniformBuffers;

		m_pGraphicsPipeline->Init(desc);
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

		m_pGraphicsPipeline->Bind(commandBuffer, 1, m_uiCurrentImageIndex);

		RecursiveRender(m_pScene->GetRootNode(), glm::dmat4(1.));
	}

	void Cleanup() 
	{		
		for (uint32_t i = 0; i < m_pWindow->GetSwapChainImageCount(); ++i)
			delete m_vUniformBuffers[i];
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