#include "dypch.h"
#include "Dymatic/Renderer/Mesh.h"
#include "Dymatic/Renderer/RenderCommand.h"

#include <glad/glad.h>

namespace Dymatic {

	Mesh::Mesh(const std::string& name, const std::vector<MeshVertex>& verticies, const std::vector<uint32_t>& indicies, Ref<MaterialAsset> materialInstance)
		: m_Name(name), m_Verticies(verticies), m_Indicies(indicies), m_Material(materialInstance)
	{
		CalculateBoundingBox();

		SetupMesh();
		UpdateVertexData();
	}

	Mesh::Mesh(const std::string& name, const AABB& aabb, const std::vector<MeshVertex>& verticies, const std::vector<uint32_t>& indicies, Ref<MaterialAsset> materialInstance)
		: m_Name(name), m_AABB(aabb), m_Verticies(verticies), m_Indicies(indicies), m_Material(materialInstance)
	{
		SetupMesh();
		UpdateVertexData();
	}

	void Mesh::Draw(const bool patches)
	{
		if (patches)
			RenderCommand::DrawIndexedPatches(m_MeshVertexArray, m_MeshIndexBuffer->GetCount());
		else
			RenderCommand::DrawIndexed(m_MeshVertexArray, m_MeshIndexBuffer->GetCount());
	}

	void Mesh::UpdateVertexData()
	{
		m_MeshVertexBuffer->SetData(m_Verticies.data(), m_Verticies.size() * sizeof(MeshVertex));
	}

	Ref<Mesh> Mesh::Copy() const
	{
		return Mesh::Create(m_Name, m_AABB, m_Verticies, m_Indicies, m_Material);
	}

	void Mesh::BindVertexBuffer(const uint32_t slot)
	{
		m_MeshVertexBuffer->BindAsBuffer(slot);
	}

	void Mesh::BindIndexBuffer(const uint32_t slot)
	{
		m_MeshIndexBuffer->BindAsBuffer(slot);
	}

	void Mesh::SetupMesh()
	{
		m_MeshVertexArray = VertexArray::Create();

		m_MeshVertexBuffer = VertexBuffer::Create(m_Verticies.size() * sizeof(MeshVertex));
		m_MeshVertexBuffer->SetLayout({
			{ ShaderDataType::UInt, "a_GlobalIndex" },
			{ ShaderDataType::Float3, "a_Position" },
			{ ShaderDataType::Float3, "a_Normal" },
			{ ShaderDataType::Float2, "a_TexCoord" },
			{ ShaderDataType::Float3, "a_Tangent" },
			{ ShaderDataType::Float3, "a_Bitangent" },
			{ ShaderDataType::Float4, "a_Color" },
			{ ShaderDataType::Int4, "a_BoneIDs" },
			{ ShaderDataType::Float4, "a_Weights" }
		});
		m_MeshVertexArray->AddVertexBuffer(m_MeshVertexBuffer);

		m_MeshIndexBuffer = IndexBuffer::Create(m_Indicies.data(), m_Indicies.size());
		m_MeshVertexArray->SetIndexBuffer(m_MeshIndexBuffer);
	}

	void Mesh::CalculateBoundingBox()
	{
		m_AABB = AABB();

		for (const auto& vertex : m_Verticies)
			m_AABB.Extend(vertex.Position);
	}

}