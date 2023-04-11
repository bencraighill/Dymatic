#pragma once

#include "Dymatic/Renderer/Shader.h"

#include "Dymatic/Renderer/VertexArray.h"
#include "Dymatic/Renderer/Buffer.h"

#include "Dymatic/Renderer/Material.h"

namespace Dymatic {

#define MAX_BONE_INFLUENCE 4

	struct MeshVertex
	{
		glm::vec3 Position;
		glm::vec3 Normal;
		glm::vec2 TexCoords;
		glm::vec3 Tangent;
		glm::vec3 Bitangent;
		int m_BoneIDs[MAX_BONE_INFLUENCE];
		float m_Weights[MAX_BONE_INFLUENCE];
	};

	class Mesh
	{
	public:
		static Ref<Mesh> Create(const std::string& name, const std::vector<MeshVertex>& verticies, const std::vector<uint32_t>& indicies, const std::unordered_map<std::string, std::vector<glm::vec3>>& blendShapes, Ref<Material> materialInstance) { return CreateRef<Mesh>(name, verticies, indicies, blendShapes, materialInstance); }

		Mesh(const std::string& name, const std::vector<MeshVertex>& verticies, const std::vector<uint32_t>& indicies, const std::unordered_map<std::string, std::vector<glm::vec3>>& blendShapes, Ref<Material> materialInstance)
			: m_Name(name), m_Verticies(verticies), m_Indicies(indicies), m_BlendShapes(blendShapes), m_Material(materialInstance)
		{
			SetupMesh();
			UpdateVertexData();
		}

		Mesh(const Mesh& other)
		{
			m_Material = CreateRef<Material>(*other.m_Material);
		}

		void Draw();
		void UpdateVertexData();

		inline Ref<Material> GetMaterial() const { return m_Material; }
		inline const std::string& GetName() const { return m_Name; }

		inline const std::vector<MeshVertex>& GetVerticies() const { return m_Verticies; }
		inline const std::vector<uint32_t>& GetIndicies() const { return m_Indicies; }
		inline const std::unordered_map<std::string, std::vector<glm::vec3>>& GetBlendShapes() const { return m_BlendShapes; }

	private:
		void SetupMesh();

	private:
		std::vector<MeshVertex> m_Verticies;
		std::vector<uint32_t> m_Indicies;

		std::unordered_map<std::string, std::vector<glm::vec3>> m_BlendShapes;

		Ref<VertexArray> m_MeshVertexArray;
		Ref<VertexBuffer> m_MeshVertexBuffer;
		Ref<IndexBuffer> m_MeshIndexBuffer;

		std::string m_Name;
		Ref<Material> m_Material;

		friend class Model;
	};

}