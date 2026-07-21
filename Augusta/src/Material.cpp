#include <Augusta/Material.h>
#include <Augusta/Context.h>
#include <Augusta/Pipeline.h>
#include <stdexcept>
#include <deque>
#include <glm/glm.hpp>

namespace aug
{
	Material::Material()
	{
	}

	Material::~Material()
	{
	}
		
	std::shared_ptr<Material> Material::MakeShared()
	{
		//Trick to be able to create shared pointers while keeping Texture constructor protected
		//so textures can only be allocated by TextureFactory
		struct SMaterial : public Material
		{
			SMaterial() : Material() {}
		};

		return std::make_shared<SMaterial>();
	}

	void Material::UpdateDescriptor(DescriptorSetLayoutHandle h)
	{
		VkDescriptorSet s = DescriptorFactory::GetDescriptorSet(m_mDescriptorHandles[h]);
		m_Desc._uAddress = reinterpret_cast<uint64_t>(s);
		printf("\nMaterial %s: %lx", m_sName.c_str(), s);

		BuildUniformBuffer();

		VkDescriptorBufferInfo bufferInfo;
		bufferInfo.buffer = m_pMaterialUniformBuffer->GetBufferHandle();
		bufferInfo.offset = 0;
		bufferInfo.range = m_pMaterialUniformBuffer->GetBufferSize();
		DescriptorFactory::UpdateDescriptor(m_mDescriptorHandles[h], &bufferInfo, 0);

		for (uint32_t i = 0; i < TEXTURE_CHANNEL_COUNT; i++)
		{
			if (m_aTextures[i] == nullptr)
				continue;
			
			VkDescriptorImageInfo imageInfo;
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
			imageInfo.imageView = m_aTextures[i]->GetImageView();
			imageInfo.sampler = m_aTextures[i]->GetSampler();

			DescriptorFactory::UpdateDescriptor(m_mDescriptorHandles[h], &imageInfo, i+1);
		}
	}

	void Material::BuildUniformBuffer()
	{
		m_pMaterialUniformBuffer = std::make_unique<Buffer>(sizeof(m_Desc), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, &m_Desc);
	}

	void Material::DrawDebug()
	{
		ImGui::Begin("Material Inspector");
		if (!m_aTextures[TEXTURE_CHANNEL_ALBEDO])
		{
			ImGui::SliderFloat("Roughness", &m_Desc._fOpacity, 0.0, 1.0);
		}
		if (!m_aTextures[TEXTURE_CHANNEL_ROUGHNESS])
			ImGui::SliderFloat("Roughness", &m_Desc._fRoughness, 0.0, 1.0);
		if (!m_aTextures[TEXTURE_CHANNEL_METALNESS])
			ImGui::SliderFloat("Metalness", &m_Desc._fMetalness, 0.0, 1.0);

		static int iSelected = 0;
		Texture* pSelectedTexture = nullptr;
		for (int i = 0; i < TEXTURE_CHANNEL_COUNT; ++i)
		{
			if (m_aTextures[i] == nullptr)
				continue;

			const bool bIsSelected = (iSelected == i);
			if (ImGui::Selectable(m_aTextures[i]->GetDesc()._strName.c_str(), bIsSelected))
				iSelected = i;

			if (bIsSelected)
			{
				pSelectedTexture = m_aTextures[i].get();
				ImGui::SetItemDefaultFocus();
			}
		}

		if(pSelectedTexture)
			pSelectedTexture->ImGuiDrawDebug();

		ImGui::End();
	}

	std::map<std::string, std::shared_ptr<Material>> MaterialFactory::m_mMaterials;
	std::unique_ptr<Buffer> MaterialFactory::m_pDefaultTextureBuffer;
	void MaterialFactory::Init()
	{
		//create placeholder material
		m_mMaterials["Default"] = Material::MakeShared();
		m_mMaterials["Default"]->m_sName = "Default";

		STextureDesc desc;
		desc._strName = "Default";
		desc._width = 2;
		desc._height = 2;
		desc._filtering = VK_FILTER_NEAREST;
		desc._layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		desc._usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		desc._format = VK_FORMAT_R8G8B8A8_SRGB;
		uint32_t uiSize = desc._width * desc._height * 4;
		uint8_t pixels[] =
		{
			255,0,255,255,
			255,255,255,255,
			255,255,255,255,
			255,0,255,255		
		};
		m_pDefaultTextureBuffer = std::make_unique<Buffer>(uiSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, pixels);
		m_mMaterials["Default"]->m_aTextures[TEXTURE_CHANNEL_ALBEDO] = TextureFactory::LoadTextureFromMemory(desc,m_pDefaultTextureBuffer.get());
	}

	std::shared_ptr<Material> MaterialFactory::CreateMaterial(const std::string& strName)
	{
		if (m_mMaterials[strName] != nullptr)
			return m_mMaterials[strName];

		m_mMaterials[strName] = Material::MakeShared();
		m_mMaterials[strName]->m_sName = strName;
		//m_mMaterials[strName]->m_uiIndex = m_mMaterials.size();
		return m_mMaterials[strName];
	}

	std::shared_ptr<Material> MaterialFactory::GetMaterialByName(const std::string& strName)
	{
		if(m_mMaterials[strName]!=nullptr)
			return m_mMaterials[strName];
		
		return m_mMaterials["Default"];
	}

	void MaterialFactory::DrawDebug()
	{
		static int32_t iSelected = -1;
		Material* pSelectedMaterial = nullptr;
		static ImGuiTextFilter filter;
		filter.Draw();

		if (ImGui::BeginListBox("Texture list"))
		{
			int iCount = 0;
			for (auto& mat : m_mMaterials)
			{
				const bool bIsSelected = (iSelected == iCount);
				if (ImGui::Selectable(mat.first.c_str(), bIsSelected))
					iSelected = iCount;

				if (bIsSelected)
				{
					ImGui::SetItemDefaultFocus();
					pSelectedMaterial = mat.second.get();
				}

				iCount++;
			}

			ImGui::EndListBox();
		}

		if (pSelectedMaterial != nullptr)
			pSelectedMaterial->DrawDebug();
	}
}