#include <Augusta/LightManager.h>
#include <Augusta/Buffer.h>
#include <imgui.h>
#include <Augusta/Utils.h>

namespace aug
{
    std::shared_ptr<SDirectionalLight> aug::LightManager::AddDirectionalLight(const std::string& strName)
    {
        std::shared_ptr<SDirectionalLight> pLight = std::make_shared<SDirectionalLight>();
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
			SDirectionalLight _aDirectionalLights[MAX_NB_LIGHTS];
		}lightUBO;

		lightUBO._uiNbDirectionalLights = m_mDirectionalLights.size();
		for (uint32_t i = 0; i < lightUBO._uiNbDirectionalLights; ++i)
		{
			auto it = std::next(m_mDirectionalLights.begin(), i);
			lightUBO._aDirectionalLights[i] = *it->second.get();
		}

		m_pUniformBufferObject = std::make_unique<Buffer>(sizeof(SLightUBO), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, &lightUBO);
	}

	void GetAzimuthElevationFromDirection(const glm::vec3 dir, float& fAzimuth, float& fElevation)
	{
		float distanceXZ = std::hypot(dir.x, dir.z);
		fElevation = std::atan2(dir.y, distanceXZ) * 180. / PI;
		fAzimuth = std::atan2(dir.x, dir.z) * 180 / PI;
	}

	void GetDirectionFromAzimuthElevation(const float& fAzimuth, const float& fElevation, glm::vec3 &dir)
	{
		float radElevation = fElevation * PI / 180.;
		float cosElevation = std::cos(radElevation);

		float radAzimuth = fAzimuth * PI / 180.;
		dir.x = cosElevation * std::cos(radAzimuth);
		dir.z = cosElevation * std::sin(radAzimuth);

		dir.y = std::sin(radElevation);
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
					if (ImGui::SliderFloat("Illuminance", &light.second->_fIlluminance, 0.0, 1000.0))
						bUpdate = true;

					float fAzimuth, fElevation;
					GetAzimuthElevationFromDirection(light.second->_vDirection, fAzimuth, fElevation);
					if (ImGui::SliderFloat("Azimuth", &fAzimuth, -180.0, 180.0))
						bUpdate = true;

					if(ImGui::SliderFloat("Elevation", &fElevation, -90.0, 90.0))
						bUpdate = true;

					if(bUpdate)
						GetDirectionFromAzimuthElevation(fAzimuth, fElevation, light.second->_vDirection);

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