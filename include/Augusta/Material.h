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
		uint64_t _uAddress = 0;
		float _fOpacity;
		float _fRoughness=0.5;
		float _fMetalness=0.;
		uint32_t _iTexMask=0;
	};

	class Material : public DescriptorTarget
	{
	public:
		friend class MaterialFactory;

		static std::shared_ptr<Material> MakeShared();

		void UpdateDescriptor(DescriptorSetLayoutHandle h) override;

		void BuildUniformBuffer();

		void DrawDebug();

		std::string m_sName = "";
		//uint32_t m_uiIndex = 0;
		std::shared_ptr<Texture> m_aTextures[ETextureChannel::TEXTURE_CHANNEL_COUNT] = { nullptr };
		SMaterialDesc m_Desc;
		std::unique_ptr<Buffer> m_pMaterialUniformBuffer;

	protected:		
		Material();
		~Material();
		
	};

	class MaterialFactory
	{
	public:
		static void Init();

		static std::shared_ptr<Material> CreateMaterial(const std::string& strName);
		static std::shared_ptr<Material> GetMaterialByName(const std::string& strName);

		static void DrawDebug();

	protected:
		static std::map<std::string, std::shared_ptr<Material>> m_mMaterials; //use weak pointer instead
		static std::unique_ptr<Buffer> m_pDefaultTextureBuffer;
	};
}

#endif