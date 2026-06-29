#include <Augusta/Scene.h>
#include <glm/gtc/matrix_transform.hpp>

namespace aug
{
	//*****************************NODE**********************************
	void Node::Rotate(const double& angle, const glm::dvec3& axis)
	{
		m_TransformMatrix = glm::rotate(m_TransformMatrix, glm::radians(angle), axis);
	}

	void Node::Translate(const glm::dvec3& trans)
	{
		m_TransformMatrix = glm::translate(m_TransformMatrix, trans);
	}

	void Node::Scale(const glm::dvec3& scale)
	{
		m_TransformMatrix = glm::scale(m_TransformMatrix, scale);
	}

	std::shared_ptr<Node> Node::GetChildNodeByName(const std::string& strName)
	{
		for (uint32_t i = 0; i < m_vChildren.size(); ++i)
		{
			if (m_vChildren[i]->m_strName == strName)
				return m_vChildren[i];
		}
		return nullptr;
	}	

	//*****************************SCENE**********************************

	Scene::Scene()
	{
		m_pRootNode = std::make_shared<Node>();
	}

	std::shared_ptr<Mesh> Scene::CreateMesh(SMeshDesc desc, std::shared_ptr<Node> pTarget)
	{
		std::shared_ptr<Mesh> pMesh(new Mesh(desc));

		pMesh->m_pMaterial = desc.pMaterial;

		AddExistingMesh(pMesh, pTarget);
		return pMesh;
	}

	void Scene::AddExistingMesh(std::shared_ptr<Mesh> pMesh, std::shared_ptr<Node> pTarget)
	{
		m_vMeshes.push_back(pMesh);

		if (pTarget)
			pTarget->AddMesh(pMesh);
		else
			m_pRootNode->AddMesh(pMesh);
	}

	std::shared_ptr<Material> Scene::GetMaterialByName(const char* szName, bool* pbAlreadyExists)
	{
		for (uint32_t i = 0; i < m_vMaterials.size(); ++i)
		{
			if (strcmp(m_vMaterials[i]->m_sName.c_str(), szName) == 0)
			{
				if (pbAlreadyExists)
					*pbAlreadyExists = true;
				return m_vMaterials[i];
			}
		}
		std::shared_ptr<Material> pMat(new Material());
		pMat->m_sName = szName;
		pMat->m_uiIndex = static_cast<uint32_t>(m_vMaterials.size());
		m_vMaterials.push_back(pMat);
		if (pbAlreadyExists)
			*pbAlreadyExists = false;
		return pMat;
	}

	void Scene::SetRootTransform(const glm::dmat4& mat)
	{
		m_pRootNode->SetTransform(mat);
	}

	//*****************************ISCENERENDERER**********************************
	void ISceneRenderer::RecursiveRender(std::shared_ptr<Node> pNode, glm::dmat4 trans)
	{
		glm::dmat4 localTrans = trans * pNode->GetTransform();
		RenderNode(pNode, localTrans);

		for (uint32_t i = 0; i < pNode->GetNbChildren(); ++i)
		{
			RecursiveRender(pNode->GetChild(i), localTrans);
		}
	}
}
