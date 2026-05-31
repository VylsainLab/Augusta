#ifndef AUG_MATERIAL_H
#define AUG_MATERIAL_H

#include <Augusta/Texture.h>
#include <memory>
#include <vector>

namespace aug
{
	enum ETextureChannel
	{
		TEXTURE_CHANNEL_ALBEDO,
		TEXTURE_CHANNEL_NORMAL,
		TEXTURE_CHANNEL_AO,
		TEXTURE_CHANNEL_ROUGHNESS,
		TEXTURE_CHANNEL_METALNESS,
		TEXTURE_CHANNEL_EMISSVE,
		TEXTURE_CHANNEL_COUNT
	};

	class Material
	{
	public:
		Material();
		~Material();

		void CreateDescriptorSets(const VkDescriptorSetLayout* pSetLayouts);

		std::string m_sName = "";
		uint32_t m_uiIndex = 0;
		std::shared_ptr<Texture> m_aTextures[ETextureChannel::TEXTURE_CHANNEL_COUNT] = { nullptr };

		std::vector<VkDescriptorSet> m_vDescriptorSets;
	private:		
		
	};
}

#endif