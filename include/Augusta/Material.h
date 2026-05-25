#ifndef AUG_MATERIAL_H
#define AUG_MATERIAL_H

#include <Augusta/Texture.h>
#include <memory>
#include <vector>

namespace aug
{
	class Material
	{
	public:
		Material();
		~Material();

		void CreateDescriptorSets(const VkDescriptorSetLayout* pSetLayouts);

		std::string m_sName = "";
		uint32_t m_uiIndex = 0;
		std::unique_ptr<Texture> m_pTexture = nullptr;

		//VkDescriptorSetLayout m_VkDescriptorSetLayout = VK_NULL_HANDLE;
		std::vector<VkDescriptorSet> m_vDescriptorSets;
	private:		
		
	};
}

#endif