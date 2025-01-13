#pragma once

#include "Dymatic/Renderer/Shader.h"

#include "Dymatic/Renderer/VertexArray.h"
#include "Dymatic/Renderer/Buffer.h"
#include "Dymatic/Renderer/AABB.h"

#include "Dymatic/Renderer/MaterialAsset.h"

namespace Dymatic {

#define MAX_BONE_INFLUENCE 4

	struct MeshVertex
	{
		uint32_t GlobalIndex;
		glm::vec3 Position;
		glm::vec3 Normal;
		glm::vec2 TexCoords;
		glm::vec3 Tangent;
		glm::vec3 Bitangent;
		glm::vec4 Color;
		int m_BoneIDs[MAX_BONE_INFLUENCE];
		float m_Weights[MAX_BONE_INFLUENCE];

		bool operator==(const MeshVertex& other) const
		{
			return Position == other.Position && Normal == other.Normal;
		}

		MeshVertex() = default;

		MeshVertex(const uint32_t globalIndex, const glm::vec3& position, const glm::vec3& normal, const glm::vec2& texCoords, const glm::vec3& tangent, const glm::vec3& bitangent, const glm::vec4& color)
			: GlobalIndex(globalIndex), Position(position), Normal(normal), TexCoords(texCoords), Tangent(tangent), Bitangent(bitangent), Color(color) {}
	};

	class Mesh
	{
	public:
		static Ref<Mesh> Create(const std::string& name, const std::vector<MeshVertex>& verticies, const std::vector<uint32_t>& indicies, Ref<MaterialAsset> materialInstance) { return CreateRef<Mesh>(name, verticies, indicies, materialInstance); }
		static Ref<Mesh> Create(const std::string& name, const AABB& aabb, const std::vector<MeshVertex>& verticies, const std::vector<uint32_t>& indicies, Ref<MaterialAsset> materialInstance) { return CreateRef<Mesh>(name, aabb, verticies, indicies, materialInstance); }

	public:
		Mesh(const std::string& name, const std::vector<MeshVertex>& verticies, const std::vector<uint32_t>& indicies, Ref<MaterialAsset> materialInstance);
		Mesh(const std::string& name, const AABB& aabb, const std::vector<MeshVertex>& verticies, const std::vector<uint32_t>& indicies, Ref<MaterialAsset> materialInstance);
		Mesh(const Mesh& other) = default;

		void Draw(const bool patches = false);
		void UpdateVertexData();

		inline Ref<MaterialAsset> GetMaterial() const { return m_Material; }
		inline const std::string& GetName() const { return m_Name; }

		inline const std::vector<MeshVertex>& GetVerticies() const { return m_Verticies; }
		inline const std::vector<uint32_t>& GetIndicies() const { return m_Indicies; }

		inline const size_t GetVertexCount() const { return m_Verticies.size(); }
		inline const size_t GetIndexCount() const { return m_Indicies.size(); }

		inline const AABB& GetAABB() const { return m_AABB; }

		// Modification Only (for dynamic physics runtime systems)
		Ref<Mesh> Copy() const;
		inline std::vector<MeshVertex>& GetVerticesEditable() { return m_Verticies; }

		void BindVertexBuffer(const uint32_t slot);
		void BindIndexBuffer(const uint32_t slot);

	private:
		void SetupMesh();
		void CalculateBoundingBox();

	private:		
		std::vector<MeshVertex> m_Verticies;
		std::vector<uint32_t> m_Indicies;
		
		AABB m_AABB;

		Ref<VertexArray> m_MeshVertexArray;
		Ref<VertexBuffer> m_MeshVertexBuffer;
		Ref<IndexBuffer> m_MeshIndexBuffer;

		std::string m_Name;
		Ref<MaterialAsset> m_Material;

		friend class Model;
	};

}