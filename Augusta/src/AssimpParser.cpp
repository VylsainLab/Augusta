#include <Augusta/AssimpParser.h>
#include <Augusta/Utils.h>
#include <map>
#include <iostream>

namespace aug
{
	AssimpParser::AssimpParser(uint32_t uiVertexComponentFlags, bool bLog, uint32_t uiImportFlags)
	{
		m_uiVertexComponentFlags = uiVertexComponentFlags;
		m_uiImportFlags = uiImportFlags;

		if (bLog)
		{
			aiLogStream stream = aiGetPredefinedLogStream(aiDefaultLogStream_STDOUT, NULL);
			aiAttachLogStream(&stream);
		}
	}

	AssimpParser::~AssimpParser()
	{
		aiDetachAllLogStreams();
	}

	void AssimpParser::RecursiveLoad(std::shared_ptr<Scene> pScene, std::shared_ptr<Node> pNode, aiNode* node)
	{
		pNode->m_strName = node->mName.C_Str();

		if (pNode != pScene->m_pRootNode)
		{
			aiMatrix4x4 m = node->mTransformation;
			glm::dmat4 mat(m.a1, m.a2, m.a3, m.a4,
				m.b1, m.b2, m.b3, m.b4,
				m.c1, m.c2, m.c3, m.c4,
				m.d1, m.d2, m.d3, m.d4);
			mat = glm::transpose(mat);
			pNode->SetTransform(mat);
		}

		for (uint32_t n = 0; n < node->mNumMeshes; ++n)
		{
			const aiMesh* pMesh = m_pAiScene->mMeshes[node->mMeshes[n]];

			if (pMesh)
			{
				pNode->AddMesh(pScene->m_vMeshes[node->mMeshes[n]]);
			}
			else
				std::cerr << "Mesh not found !\n";
		}

		for (uint32_t n = 0; n < node->mNumChildren; ++n)
		{
			std::shared_ptr<Node> pChild(new Node());
			pNode->AddChildNode(pChild);
			RecursiveLoad(pScene, pChild, node->mChildren[n]);
		}
	}

	bool AssimpParser::LoadSceneFromFile(std::shared_ptr<Scene> pTarget, const std::string& strModelPath, const std::string& strTexPath, const std::string& strForceExt)
	{
		TextureFactory::AddTexturePath(strTexPath);
		TextureFactory::SetTextureExtension(strForceExt);
		m_strFilePath = strModelPath;

		m_pAiScene = aiImportFile(strModelPath.c_str(), m_uiImportFlags);

		if (!m_pAiScene)
		{
			std::cerr << "Assimp failed to load " << strModelPath << "!\n";
			return false;
		}

		std::map<uint32_t, std::string> mMaterialIndirection;
		for (uint32_t i = 0; i < m_pAiScene->mNumMaterials; ++i)
		{
			aiMaterial* pAiMat = m_pAiScene->mMaterials[i];

			if (pAiMat)
			{
				aiColor4D color;
				float factor;
				aiString str;
				std::shared_ptr<Material> pMat;

				if (aiGetMaterialString(pAiMat, AI_MATKEY_NAME, &str) == AI_SUCCESS)
				{
					bool bAlreadyExists = false;
					pMat = pTarget->GetMaterialByName(str.C_Str(), &bAlreadyExists);
					if (bAlreadyExists)
					{
						char szNewName[256];
						snprintf(szNewName, sizeof(szNewName), "Material%u", i);
						pMat = pTarget->GetMaterialByName(szNewName);
						mMaterialIndirection[i] = szNewName;
					}
					else
						mMaterialIndirection[i] = str.C_Str();
				}
				else
					pMat = pTarget->GetMaterialByName("Default");

				/*if (aiGetMaterialColor(pAiMat, AI_MATKEY_COLOR_AMBIENT, &color) == AI_SUCCESS)
					pMat->m_MaterialUBO.m_AmbientColor = glm::vec3(color.r, color.g, color.b);
				if (aiGetMaterialColor(pAiMat, AI_MATKEY_COLOR_DIFFUSE, &color) == AI_SUCCESS)
				{
					pMat->m_MaterialUBO.m_DiffuseColor = glm::vec3(color.r, color.g, color.b);
					pMat->m_MaterialUBO.m_fTransparency = color.a;
				}
				if (aiGetMaterialColor(pAiMat, AI_MATKEY_COLOR_SPECULAR, &color) == AI_SUCCESS)
					pMat->m_MaterialUBO.m_SpecularColor = glm::vec3(color.r, color.g, color.b);
				if (aiGetMaterialFloat(pAiMat, AI_MATKEY_SHININESS, &factor) == AI_SUCCESS)
					pMat->m_MaterialUBO.m_fShininess = factor;
				if (aiGetMaterialFloat(pAiMat, AI_MATKEY_SHININESS_STRENGTH, &factor) == AI_SUCCESS)
					pMat->m_MaterialUBO.m_fShininessStrength = factor;
				if (aiGetMaterialFloat(pAiMat, AI_MATKEY_OPACITY, &factor) == AI_SUCCESS)
					pMat->m_MaterialUBO.m_fTransparency = factor;

				if (aiGetMaterialFloat(pAiMat, AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLIC_FACTOR, &factor) == AI_SUCCESS)
					pMat->m_MaterialUBO.m_fMetalness = factor;

				if (aiGetMaterialFloat(pAiMat, AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_ROUGHNESS_FACTOR, &factor) == AI_SUCCESS)
					pMat->m_MaterialUBO.m_fRoughness = factor;*/

				//TODO normalize naming with suffix for each type

				pMat->m_aTextures[ETextureChannel::TEXTURE_CHANNEL_ALBEDO] = LoadTexture(pAiMat, aiTextureType_DIFFUSE);
				std::string strBaseName = pMat->m_aTextures[ETextureChannel::TEXTURE_CHANNEL_ALBEDO]->GetDesc()._strName;
				
				pMat->m_aTextures[ETextureChannel::TEXTURE_CHANNEL_NORMAL] = LoadTexture(pAiMat, aiTextureType_NORMALS, strBaseName);				
				pMat->m_aTextures[ETextureChannel::TEXTURE_CHANNEL_AO] = LoadTexture(pAiMat, aiTextureType_AMBIENT_OCCLUSION, strBaseName);
				pMat->m_aTextures[ETextureChannel::TEXTURE_CHANNEL_ROUGHNESS] = LoadTexture(pAiMat, aiTextureType_SHININESS, strBaseName);
				pMat->m_aTextures[ETextureChannel::TEXTURE_CHANNEL_METALNESS] = LoadTexture(pAiMat, aiTextureType_METALNESS, strBaseName);
				pMat->m_aTextures[ETextureChannel::TEXTURE_CHANNEL_EMISSVE] = LoadTexture(pAiMat, aiTextureType_EMISSIVE, strBaseName);
			}
		}

		std::vector<VertexFormatComponents> vVertexFormatComponents({ VERTEX_FORMAT_VEC3F32 });
		if (m_uiVertexComponentFlags & VERTEX_COMPONENT_NORMAL)
			vVertexFormatComponents.push_back(VERTEX_FORMAT_VEC3F32);
		if (m_uiVertexComponentFlags & VERTEX_COMPONENT_TEXCOORD)
			vVertexFormatComponents.push_back(VERTEX_FORMAT_VEC2F32);
		VertexFormat vertexFormat(vVertexFormatComponents);

		for (uint32_t i = 0; i < m_pAiScene->mNumMeshes; ++i)
		{
			aiMesh* pMesh = m_pAiScene->mMeshes[i];

			if (pMesh)
			{
				SMeshDesc meshDesc;
				//meshDesc.m_uiIndex = i;
				meshDesc._pMaterial = pTarget->GetMaterialByName(mMaterialIndirection[pMesh->mMaterialIndex].c_str());
				meshDesc._usage = MESH_USAGE_STATIC;
				meshDesc._pFormat = &vertexFormat;

				if ((m_uiVertexComponentFlags & VERTEX_COMPONENT_NORMAL) && pMesh->mNormals == nullptr)
					std::cerr << "Mesh " << pMesh->mName.C_Str() << " is missing normals";
				if ((m_uiVertexComponentFlags & VERTEX_COMPONENT_NORMAL) && pMesh->mNormals == nullptr)
					std::cerr << "Mesh " << pMesh->mName.C_Str() << " is missing texture coordinates";

				if (pMesh->mPrimitiveTypes != aiPrimitiveType_LINE)
				{
					char* pBuf = new char[pMesh->mNumVertices * vertexFormat.GetStride()];
					memset(pBuf, 0, pMesh->mNumVertices * 8 * sizeof(float));
					char* p = pBuf;
					for (uint32_t j = 0; j < pMesh->mNumVertices; ++j)
					{
						memcpy(p, &pMesh->mVertices[j].x, sizeof(float)); p += sizeof(float);
						memcpy(p, &pMesh->mVertices[j].y, sizeof(float)); p += sizeof(float);
						memcpy(p, &pMesh->mVertices[j].z, sizeof(float)); p += sizeof(float);

						if (pMesh->mNormals)
						{
							memcpy(p, &pMesh->mNormals[j].x, sizeof(float)); p += sizeof(float);
							memcpy(p, &pMesh->mNormals[j].y, sizeof(float)); p += sizeof(float);
							memcpy(p, &pMesh->mNormals[j].z, sizeof(float)); p += sizeof(float);
						}
						else
							p += 3 * sizeof(float);

						if (pMesh->HasTextureCoords(0))
						{
							memcpy(p, &pMesh->mTextureCoords[0][j].x, sizeof(float)); p += sizeof(float);
							memcpy(p, &pMesh->mTextureCoords[0][j].y, sizeof(float)); p += sizeof(float);
						}
						else
							p += 2 * sizeof(float);
					}
					meshDesc._vertexCount = pMesh->mNumVertices;
					meshDesc._vertexData = pBuf;

					std::vector<uint32_t> vIndices;
					for (uint32_t j = 0; j < pMesh->mNumFaces; ++j)
					{
						vIndices.push_back(pMesh->mFaces[j].mIndices[0]);
						vIndices.push_back(pMesh->mFaces[j].mIndices[1]);
						vIndices.push_back(pMesh->mFaces[j].mIndices[2]);
					}
					meshDesc._indexCount = static_cast<uint32_t>(vIndices.size());
					meshDesc._indexData = &vIndices[0];
					pTarget->CreateMesh(meshDesc);

					delete[] pBuf;
				}
			}
		}

		RecursiveLoad(pTarget, pTarget->m_pRootNode, m_pAiScene->mRootNode);

		for (uint32_t i = 0; i < m_pAiScene->mNumAnimations; ++i)
		{
			aiAnimation* pAnim = m_pAiScene->mAnimations[i];
			//TODO
		}

		for (uint32_t i = 0; i < m_pAiScene->mNumAnimations; ++i)
		{
			aiAnimation* anim = m_pAiScene->mAnimations[i];

			for (uint32_t j = 0; j < anim->mNumChannels; ++j)
			{
				aiNodeAnim* pNA = anim->mChannels[j];
			}
		}

		aiReleaseImport(m_pAiScene);
		m_pAiScene = NULL;

		return true;
	}

	std::shared_ptr<Texture> AssimpParser::LoadTexture(const aiMaterial* pAiMat, const aiTextureType& type, const std::string& baseName)
	{
		std::string strLookFor;
		ETextureChannel eChannel;
		switch (type)
		{
		case aiTextureType_DIFFUSE:
			eChannel = TEXTURE_CHANNEL_ALBEDO;
			break;
		case aiTextureType_NORMALS:
			eChannel = TEXTURE_CHANNEL_NORMAL;
			strLookFor = "normal";
			break;
		case aiTextureType_AMBIENT_OCCLUSION:
			eChannel = TEXTURE_CHANNEL_AO;
			strLookFor = "ao";
			break;
		case aiTextureType_SHININESS:
			eChannel = TEXTURE_CHANNEL_ROUGHNESS;
			strLookFor = "roughness";
			break;
		case aiTextureType_METALNESS:
			eChannel = TEXTURE_CHANNEL_METALNESS;
			strLookFor = "metalness";
			break;
		case aiTextureType_EMISSIVE:
			eChannel = TEXTURE_CHANNEL_EMISSVE;
			strLookFor = "emissive";
			break;
		}
		aiString str;
		std::string strPath;
		if (pAiMat->GetTexture(type, 0, &str) == aiReturn_SUCCESS)
			strPath = str.C_Str();
		else if(!baseName.empty() && baseName.find("albedo")!=std::string::npos)
		{
			strPath = ReplaceString(baseName, "albedo", strLookFor.c_str());
		}
		
		return TextureFactory::LoadTextureFromFile(strPath);
	}
}