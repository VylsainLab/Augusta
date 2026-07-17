#include <gli/gli.hpp>

#include <Augusta/Texture.h>
#include <Augusta/Buffer.h>
#include <Augusta/Context.h>
#include <Augusta/MemoryAllocator.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>
#include <iostream>
#include <filesystem>

#include <imgui.h>
#include <imgui-docking\backends\imgui_impl_vulkan.h>
#include <algorithm>

namespace aug
{
	Texture::Texture(STextureDesc& desc, Buffer* pBuffer)
	{
		static uint32_t iCount = 0;		
		if (desc._strName.empty())
		{
			desc._strName = "texture_" + std::to_string(iCount);
		}
		iCount++;

		m_TextureDesc = desc;

		CreateImage();
		CreateImageView();

		if (pBuffer)
		{
			//Transition image layout for copy
			TransitionImageToLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

			//Copy buffer to image
			VkCommandBuffer commandBuffer = Context::BuildSingleTimeCommandBuffer();

			VkBufferImageCopy region{};
			region.bufferOffset = 0;
			region.bufferRowLength = 0; // tightly packed
			region.bufferImageHeight = 0; //same
			region.imageSubresource.aspectMask = m_TextureDesc._aspect;
			region.imageSubresource.mipLevel = 0;
			region.imageSubresource.baseArrayLayer = 0;
			region.imageSubresource.layerCount = 1;
			region.imageOffset = { 0, 0, 0 };
			region.imageExtent = { static_cast<uint32_t>(m_TextureDesc._width), static_cast<uint32_t>(m_TextureDesc._height), 1 };

			vkCmdCopyBufferToImage(commandBuffer, pBuffer->GetBufferHandle(), m_VkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

			Context::SubmitAndFreeCommandBuffer(commandBuffer);
		}

		CreateSampler();

		TransitionImageToLayout(desc._layout);
	}

	Texture::~Texture()
	{
		ImGui_ImplVulkan_RemoveTexture(m_ImGuiDescriptorSet);

		vkDestroySampler(Context::m_VkDevice, m_VkSampler, nullptr);

		vkDestroyImageView(Context::m_VkDevice, m_VkImageView, nullptr);

		vmaDestroyImage(MemoryAllocator::m_VmaAllocator, m_VkImage, m_VmaAllocation);
	}

	void Texture::CreateImage()
	{
		//Create and allocate image
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = static_cast<uint32_t>(m_TextureDesc._width);
		imageInfo.extent.height = static_cast<uint32_t>(m_TextureDesc._height);
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = m_TextureDesc._format;
		imageInfo.tiling = m_TextureDesc._tiling;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = m_TextureDesc._usage;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.flags = 0; // Optional

		VmaAllocationCreateInfo allocCreateInfo = {};
		allocCreateInfo.usage = m_TextureDesc._memoryUsage;

		if (vmaCreateImage(MemoryAllocator::m_VmaAllocator, &imageInfo, &allocCreateInfo, &m_VkImage, &m_VmaAllocation, &m_VmaAllocationInfo) != VK_SUCCESS)
			throw std::runtime_error("Failed to create image!");
	}

	void Texture::CreateImageView()
	{
		//Image view
		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = m_VkImage;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = m_TextureDesc._format;
		viewInfo.subresourceRange.aspectMask = m_TextureDesc._aspect;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(Context::m_VkDevice, &viewInfo, nullptr, &m_VkImageView) != VK_SUCCESS)
			throw std::runtime_error("Failed to create texture image view!");
	}

	void Texture::CreateSampler()
	{
		//Sampler
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = m_TextureDesc._filtering;
		samplerInfo.minFilter = m_TextureDesc._filtering;
		samplerInfo.addressModeU = m_TextureDesc._samplingMode;
		samplerInfo.addressModeV = m_TextureDesc._samplingMode;
		samplerInfo.addressModeW = m_TextureDesc._samplingMode;
		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.maxAnisotropy = 16.0f; //TODO expose in graphics settings
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 0.0f;
		if (vkCreateSampler(Context::m_VkDevice, &samplerInfo, nullptr, &m_VkSampler) != VK_SUCCESS)
			throw std::runtime_error("Failed to create texture sampler!");
	}
	
	//Trick to be able to create shared pointers while keeping Texture constructor protected
	//so textures can only be allocated by TextureFactory
	struct STexture : public Texture 
	{
		STexture(STextureDesc& desc, Buffer* pBuffer)
			: Texture(desc, pBuffer)
		{ }
	};
	std::shared_ptr<Texture> Texture::MakeShared(STextureDesc& desc, Buffer* pBuffer)
	{
		return std::make_shared<STexture>(desc,pBuffer);
	}

	void Texture::TransitionImageToLayout(VkImageLayout newLayout, VkCommandBuffer cb)
	{
		if (m_CurrentImageLayout == newLayout)
			return;

		VkCommandBuffer commandBuffer = cb==VK_NULL_HANDLE ? Context::BuildSingleTimeCommandBuffer() : cb;	

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = m_CurrentImageLayout;
		barrier.newLayout = newLayout;

		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

		barrier.image = m_VkImage;
		barrier.subresourceRange.aspectMask = m_TextureDesc._aspect;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;
		if (m_CurrentImageLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			//We can start the transition asap but we have to wait until transition is done before we transfer data
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (/*m_CurrentImageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&*/ newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			//Wait until transfer is complete before transition and then allow shader read
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if (/*m_CurrentImageLayout == VK_IMAGE_LAYOUT_UNDEFINED &&*/ newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		}
		else if (/*m_CurrentImageLayout == VK_IMAGE_LAYOUT_UNDEFINED &&*/ newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		}
		else if (newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			
			sourceStage = VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT;
		}
		else if (newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT;
		}
		else
			throw std::invalid_argument("Unsupported layout transition!");

		vkCmdPipelineBarrier( commandBuffer, sourceStage, destinationStage, 0, 0, nullptr,	0, nullptr, 1, &barrier	);

		if(cb == VK_NULL_HANDLE)
			Context::SubmitAndFreeCommandBuffer(commandBuffer);

		m_CurrentImageLayout = newLayout;
	}

	ImTextureID Texture::GetImGuiTextureID()
	{
		if(m_ImGuiDescriptorSet== VK_NULL_HANDLE)
			m_ImGuiDescriptorSet = ImGui_ImplVulkan_AddTexture(m_VkSampler, m_VkImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		return (ImTextureID)m_ImGuiDescriptorSet;
	}


	std::vector<std::string> TextureFactory::m_vPaths;
	std::string TextureFactory::m_sForcedExtension;
	std::map<std::string, std::weak_ptr<Texture>> TextureFactory::m_mTextureDictionary;
	void TextureFactory::AddTexturePath(const std::string& strPath)
	{
		m_vPaths.push_back(strPath);
	}

	void TextureFactory::SetTextureExtension(const std::string& strExt)
	{
		m_sForcedExtension = strExt;
	}

	std::shared_ptr<Texture> TextureFactory::LoadTextureFromMemory(STextureDesc& desc, Buffer* pBuffer)
	{
		std::shared_ptr<Texture> pTexture = Texture::MakeShared(desc, pBuffer);//std::make_shared<Texture>(desc, pBuffer);
		m_mTextureDictionary[desc._strName] = pTexture;
		return pTexture;
	}

	std::shared_ptr<Texture> TextureFactory::LoadTextureFromFile(const std::string& strPath)
	{
		size_t pos = strPath.find_last_of("\\");
		std::string strFilename = strPath.substr(pos + 1);
		std::string strDirPath = strPath.substr(0, pos);
		if (!m_sForcedExtension.empty())
		{
			pos = strFilename.rfind('.');
			strFilename.replace(pos + 1, strFilename.size() - pos, m_sForcedExtension.c_str());
		}

		std::string strRealPath = FindTexture(strDirPath,strFilename);
		if (strRealPath.empty())
			return nullptr;

		std::shared_ptr<Texture> pTexture = nullptr;
		if (strRealPath.find(".dds") != std::string::npos)
			pTexture = LoadTextureFromDDS(strFilename, strRealPath);
		else
			pTexture = LoadTextureFromSTBI(strFilename, strRealPath);

		if (pTexture != nullptr)
			m_mTextureDictionary[strFilename] = pTexture;

		return pTexture;
	}

	void TextureFactory::ImGuiDrawTextureDebug()
	{
		static int32_t iSelected = -1;
		std::weak_ptr<Texture> pPreview,pView;

		static ImGuiTextFilter filter;
		filter.Draw();

		if (ImGui::BeginListBox("Texture list"))
		{
			int iCount = 0;
			for (auto& texture : m_mTextureDictionary)
			{				
				std::shared_ptr<Texture> pTex = texture.second.lock();
				if (pTex->GetCurrentLayout() != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL || !filter.PassFilter(pTex->m_TextureDesc._strName.c_str()))
					continue;

				const bool bIsSelected = (iSelected == iCount);
				if (ImGui::Selectable(texture.first.c_str(), bIsSelected))
					iSelected = iCount;

				if (ImGui::IsItemHovered())
					pPreview = texture.second;

				if (bIsSelected)
				{
					ImGui::SetItemDefaultFocus();
					pView = texture.second;
				}

				iCount++;
			}
			ImGui::EndListBox();
		}

		std::shared_ptr<Texture> psPreview = pPreview.lock();
		std::shared_ptr<Texture> psView = pView.lock();
		if (psPreview !=nullptr)
		{			
			ImVec2 size(100,100); //preview
			ImGui::ImageWithBg(psPreview->GetImGuiTextureID(), size);
		}

		if (iSelected>-1 && psView != nullptr)
		{
			STextureDesc desc = psView->GetDesc();
			float ratio = static_cast<float>(desc._height) / static_cast<float>(desc._width);
			float texW = static_cast<float>(std::min(desc._width, 1024u));
			float texH = texW * ratio;
			ImVec2 size(static_cast<float>(std::min(desc._width, 1024u) + 10), static_cast<float>(std::min(desc._height, 1024u) + 10));
			ImGui::Begin("Texture Inspector");
			ImGui::SetWindowSize(ImVec2(texW+10, texH+10));
			ImGui::Image(psView->GetImGuiTextureID(), ImVec2(texW,texH));
			ImGui::End();
		}
	}

	std::string TextureFactory::FindTexture(const std::string& strDirPath, const std::string& strFilename)
	{
		std::string strPath = strDirPath + "\\" + strFilename;
		if (std::filesystem::exists(strPath))
			return strPath;
		else
		{
			for (auto& path : m_vPaths)
			{
				strPath = path + "\\" + strFilename;
				if (std::filesystem::exists(strPath))
					return strPath;
			}
		}

		return "";
	}

	std::shared_ptr<Texture> TextureFactory::LoadTextureFromDDS(const std::string& strName, const std::string& strPath)
	{
		gli::texture ddsTexture = gli::load_dds(strPath);
		ddsTexture = gli::flip(ddsTexture);

		uint32_t uiWidth = ddsTexture.extent().x;
		uint32_t uiHeight = ddsTexture.extent().y;
		uint32_t uiLevels = static_cast<uint32_t>(ddsTexture.levels());
		uint64_t uiSize = ddsTexture.size(0);
		Buffer stagingBuffer(uiSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, ddsTexture.data());		
		
		STextureDesc desc;
		desc._strName = strName;
		desc._width = uiWidth;
		desc._height = uiHeight;
		desc._levels = 0;// uiLevels; TODO support mipmaps

		switch(ddsTexture.format())
		{
		case gli::FORMAT_RGBA_DXT1_UNORM_BLOCK8:
			desc._format = VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
			break;
		case gli::FORMAT_RGBA_DXT5_UNORM_BLOCK16:
			desc._format = VK_FORMAT_BC3_UNORM_BLOCK;
			break;
		default:
			throw std::invalid_argument("Unsupported DDS texture format!");
		}
		desc._usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		desc._layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		return Texture::MakeShared(desc, &stagingBuffer);
	}

	std::shared_ptr<Texture> TextureFactory::LoadTextureFromSTBI(const std::string& strName, const std::string& strPath)
	{
		//Load file and create staging buffer
		int32_t w, h, c;
		stbi_set_flip_vertically_on_load(true);
		stbi_uc* pData = stbi_load(strPath.c_str(), &w, &h, &c, STBI_rgb_alpha);
		if (pData == nullptr)
		{
			std::cout << "Failed to load image " << strPath << std::endl;
			return nullptr;
		}

		uint64_t uiSize = w * h * 4;
		Buffer stagingBuffer(uiSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, pData);

		stbi_image_free(pData);

		STextureDesc desc;
		desc._strName = strName;
		desc._width = w;
		desc._height = h;
		desc._usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		desc._layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		return Texture::MakeShared(desc, &stagingBuffer);
	}
}