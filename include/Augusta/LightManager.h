#pragma once
#include <Augusta/DescriptorFactory.h>
#include <glm/glm.hpp>
#include <unordered_map>
#include <memory>
#include <string>

#define MAX_NB_LIGHTS 8

namespace aug
{
	//Ensure alignment is ok for uniform buffer serializing
	struct SDirectionalLight
	{
		alignas(16) glm::vec3 _vDirection = glm::vec3(0.f, 1.f, 0.f);
		alignas(4) float _fIlluminance = 1.f;		
	};

	/*struct SImageBasedLight
	{

	};*/

	class LightManager : public DescriptorTarget
	{
	public:
		std::shared_ptr<SDirectionalLight> AddDirectionalLight(const std::string& strName);
		//std::shared_ptr<SImageBasedLight> AddImageBasedLight(const std::string& strName);

		void UpdateDescriptor(DescriptorSetLayoutHandle h) override;

		std::weak_ptr<Buffer> GetUniformBuffer() { return m_pUniformBufferObject; }

		void DrawDebug();

	protected:

		void BuildUniformBuffer();

		std::unordered_map<std::string, std::shared_ptr<SDirectionalLight>> m_mDirectionalLights;
		//std::unordered_map<std::string, std::shared_ptr<SImageBasedLight>> m_mImageBasedLights;

		std::shared_ptr<Buffer> m_pUniformBufferObject = nullptr;

		//keep track of layout handles to update descriptors on light change
		std::vector<DescriptorSetLayoutHandle> m_vDescriptorSetLayoutHandles;
	};
}