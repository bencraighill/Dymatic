#include "dypch.h"
#include "Dymatic/Renderer/Mesh.h"
#include "Dymatic/Renderer/RenderCommand.h"

#include <glad/glad.h>

namespace Dymatic {

	void Mesh::Draw(bool bindMaterial)
	{
		m_MeshVertexBuffer->SetData(m_Verticies.data(), m_Verticies.size() * sizeof(MeshVertex));

		if (bindMaterial)
			m_Material->Bind();

		if (m_Material->m_MaterialData.BlendMode == Material::Masked)
		{
			glDisable(GL_CULL_FACE);
			RenderCommand::DrawIndexed(m_MeshVertexArray, m_MeshIndexBuffer->GetCount());
			glEnable(GL_CULL_FACE);
		}
		else
			RenderCommand::DrawIndexed(m_MeshVertexArray, m_MeshIndexBuffer->GetCount());
	}

	void Mesh::SetupMesh()
	{
		m_MeshVertexArray = VertexArray::Create();

		m_MeshVertexBuffer = VertexBuffer::Create(m_Verticies.size() * sizeof(MeshVertex));
		m_MeshVertexBuffer->SetLayout({
			{ ShaderDataType::Float3, "a_Position" },
			{ ShaderDataType::Float3, "a_Normal" },
			{ ShaderDataType::Float2, "a_TexCoord" },
			{ ShaderDataType::Float3, "a_Tangent" },
			{ ShaderDataType::Float3, "a_Bitangent" },
			{ ShaderDataType::Int4, "a_BoneIDs" },
			{ ShaderDataType::Float4, "a_Weights" }
			});
		m_MeshVertexArray->AddVertexBuffer(m_MeshVertexBuffer);

		m_MeshIndexBuffer = IndexBuffer::Create(m_Indicies.data(), m_Indicies.size());
		m_MeshVertexArray->SetIndexBuffer(m_MeshIndexBuffer);
	}

}