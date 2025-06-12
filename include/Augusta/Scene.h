#ifndef AUG_SCENE_H
#define AUG_SCENE_H

#include <Augusta/Mesh.h>
#include <glm/glm.hpp>

namespace aug
{
	//*****************************NODE**********************************
	class Node
	{
	public:
		void SetTransform(const glm::dmat4& mat) { m_TransformMatrix = mat; }
		void Rotate(const double& degAngle, const glm::dvec3& axis);
		void Translate(const glm::dvec3& trans);
		void Scale(const glm::dvec3& scale);

		void AddMesh(std::shared_ptr<Mesh> p) { m_vMeshes.push_back(p); }
		void RemoveMesh(uint32_t index) { m_vMeshes.erase(m_vMeshes.begin() + index); }
		void AddChildNode(std::shared_ptr<Node> pChild) { m_vChildren.push_back(pChild); }	

		//getters
		uint32_t GetNbMeshes() { return static_cast<uint32_t>(m_vMeshes.size()); }
		std::shared_ptr<Mesh> GetMesh(uint32_t index) { return m_vMeshes.at(index); }
		uint32_t GetNbChildren() const { return static_cast<uint32_t>(m_vChildren.size()); }
		std::shared_ptr<Node> GetChild(uint32_t index) const { return m_vChildren.at(index); }
		std::shared_ptr<Node> GetChildNodeByName(const std::string& strName);
		glm::dmat4 GetTransform() const { return m_TransformMatrix; }

	protected:
		std::string m_strName = "";
		glm::dmat4 m_TransformMatrix = glm::dmat4(1.);
		std::vector< std::shared_ptr<Node> > m_vChildren;
		std::vector< std::shared_ptr<Mesh> > m_vMeshes;

		friend class Scene;
		friend class AssimpParser;
	};

	//*****************************SCENE**********************************
	class Scene
	{
	public:
		Scene();

		std::shared_ptr<Mesh> CreateMesh(SMeshDesc desc, std::shared_ptr<Node> pTarget = nullptr);
		void AddExistingMesh(std::shared_ptr<Mesh> pMesh, std::shared_ptr<Node> pTarget = nullptr);

		void SetRootTransform(const glm::dmat4& mat);

		std::shared_ptr<Node> GetRootNode() const { return m_pRootNode; }

	private:
		std::shared_ptr<Node> m_pRootNode = nullptr;
		std::vector< std::shared_ptr<Mesh> > m_vMeshes;

		friend class AssimpParser;
	};

	//*****************************ISCENERENDERER**********************************
	class ISceneRenderer
	{
	public:
		virtual void RenderNode(std::shared_ptr<Node> pNode, glm::dmat4 trans) {};

		void RecursiveRender(std::shared_ptr<Node> pNode, glm::dmat4 trans);
	};
}

#endif
