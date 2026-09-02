#include <Augusta/LightManager.h>
#include <Augusta/Buffer.h>
#include <imgui.h>
#include <Augusta/Utils.h>

namespace aug
{
	void DirectionalLight::Update()
	{
		GetDirectionFromAzimuthElevation(m_fDegAzimuth, m_fDegElevation, m_UB._vDirection);
	}

    std::shared_ptr<DirectionalLight> aug::LightManager::AddDirectionalLight(const std::string& strName)
    {
        std::shared_ptr<DirectionalLight> pLight = std::make_shared<DirectionalLight>();
        m_mDirectionalLights[strName] = pLight;
        return pLight;
    }

    void LightManager::UpdateDescriptor(DescriptorSetLayoutHandle h)
    {
		if (std::find(m_vDescriptorSetLayoutHandles.begin(), m_vDescriptorSetLayoutHandles.end(), h) == m_vDescriptorSetLayoutHandles.end())
			m_vDescriptorSetLayoutHandles.push_back(h);

		VkDescriptorSet s = DescriptorFactory::GetDescriptorSet(m_mDescriptorHandles[h]);

		BuildUniformBuffer();

		VkDescriptorBufferInfo bufferInfo;
		bufferInfo.buffer = m_pUniformBufferObject->GetBufferHandle();
		bufferInfo.offset = 0;
		bufferInfo.range = m_pUniformBufferObject->GetBufferSize();
		DescriptorFactory::UpdateDescriptor(m_mDescriptorHandles[h], &bufferInfo, 0);
    }

	void LightManager::BuildUniformBuffer()
	{
		struct SLightUBO
		{
			uint32_t _uiNbDirectionalLights;
			DirectionalLight::SDirectionalLight _aDirectionalLights[MAX_NB_LIGHTS];
		}lightUBO;

		lightUBO._uiNbDirectionalLights = static_cast<uint32_t>(m_mDirectionalLights.size());
		for (uint32_t i = 0; i < lightUBO._uiNbDirectionalLights; ++i)
		{
			auto it = std::next(m_mDirectionalLights.begin(), i);
			lightUBO._aDirectionalLights[i] = it->second->m_UB;
		}

		m_pUniformBufferObject = std::make_shared<Buffer>(sizeof(SLightUBO), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, &lightUBO);
	}

	void LightManager::DrawDebug()
	{
		bool bUpdate = false;

		if (ImGui::CollapsingHeader("Directional lights", ImGuiTreeNodeFlags_None))
		{
			for (auto& light : m_mDirectionalLights)
			{
				if (ImGui::TreeNode(light.first.c_str()))
				{
					if (ImGui::ColorEdit3("Color", &light.second->m_UB._vColor.r, ImGuiColorEditFlags_NoInputs))
						bUpdate = true;

					if (ImGui::SliderFloat("Illuminance", &light.second->m_UB._fIlluminance, 0.0, 1000.0))
						bUpdate = true;

					if (ImGui::SliderFloat("Azimuth", &light.second->m_fDegAzimuth, -180.0, 180.0))
						bUpdate = true;

					if(ImGui::SliderFloat("Elevation", &light.second->m_fDegElevation, -90.0, 90.0))
						bUpdate = true;

					if (bUpdate)
						light.second->Update();

					ImGui::TreePop();
				}				
			}			
		}

		if (bUpdate)
		{
			for (auto& handle : m_vDescriptorSetLayoutHandles)
				UpdateDescriptor(handle);
		}
	}
}