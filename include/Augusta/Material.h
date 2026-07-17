#ifndef AUG_MATERIAL_H
#define AUG_MATERIAL_H

#include <Augusta/Texture.h>
#include <memory>
#include <vector>

namespace aug
{
	#define TEXTURE_CHANNEL_ALBEDO_BIT		1
	#define TEXTURE_CHANNEL_NORMAL_BIT		2
	#define TEXTURE_CHANNEL_AO_BIT			4
	#define TEXTURE_CHANNEL_ROUGHNESS_BIT	8
	#define TEXTURE_CHANNEL_METALNESS_BIT	16
	#define TEXTURE_CHANNEL_EMISSIVE_BIT	32

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
	
	struct SMaterialDesc
	{
		float _fOpacity;
		float _fRoughness=0.5;
		float _fMetalness=0.;
		int32_t _iTexMask;
	};

	class Material : public DescriptorTarget
	{
	public:
		Material();
		~Material();

		void UpdateDescriptor(DescriptorSetLayoutHandle h) override;

		void BuildUniformBuffer();

		std::string m_sName = "";
		uint32_t m_uiIndex = 0;
		std::shared_ptr<Texture> m_aTextures[ETextureChannel::TEXTURE_CHANNEL_COUNT] = { nullptr };
		SMaterialDesc m_Desc;
		std::unique_ptr<Buffer> m_pMaterialUniformBuffer;
	private:		
		
	};
}

#endif