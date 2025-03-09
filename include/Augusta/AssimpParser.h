#ifndef AUG_ASSIMPPARSER_H
#define AUG_ASSIMPPARSER_H

#include <Augusta/Scene.h>
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace aug
{
	//Position is a mandatory component :)
	enum VertexComponentFlags
	{
		VERTEX_COMPONENT_NORMAL = 0x0001,
		VERTEX_COMPONENT_TEXCOORD = 0x0002
	};

	class AssimpParser
	{
	public:
		AssimpParser(uint32_t uiVertexComponentFlags, bool bLog = true, uint32_t uiImportFlags = aiProcess_Triangulate);
		virtual ~AssimpParser();

		void RecursiveLoad(std::shared_ptr<Scene> pScene, std::shared_ptr<Node> pNode, aiNode* node);
		bool LoadSceneFromFile(std::shared_ptr<Scene> pTarget, const std::string& strModelPath, const std::string& strTexPath = "", const std::string& strForceExt = "");
		//virtual void WriteScene(AAScene* pScene);

	protected:
		//void LoadAssimpTexture(aiMaterial* paiMat, std::shared_ptr<meMaterial> pMat, aiTextureType type, std::string optionalPath = "");

		const aiScene* m_pAiScene = NULL;
		std::string m_strFilePath;
		std::string m_strTexExtension;
		std::vector<std::string> m_vTexPaths;
		uint32_t m_uiVertexComponentFlags = 0;
		uint32_t m_uiImportFlags = 0;
	};
}

#endif