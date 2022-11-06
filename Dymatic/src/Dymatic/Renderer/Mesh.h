#pragma once

#include "Dymatic/Renderer/Shader.h"
#include "Dymatic/Renderer/Texture.h"
#include "Dymatic/Renderer/UniformBuffer.h"

#include "Dymatic/Renderer/VertexArray.h"
#include "Dymatic/Renderer/Buffer.h"

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

	static Ref<UniformBuffer> s_MaterialUniformBuffer;
	class Material
	{
	public:
		static Ref<Material> Create(const std::string& name) { return CreateRef<Material>(name); }

		Material(const std::string& name)
			: m_Name(name)
		{
		}

		const std::string& GetName() { return m_Name; }

		enum AlphaBlendMode
		{
			Opaque = 0,
			Masked,
			Translucent,
			Dithered
		};

		static const char* AlphaBlendModeToString(AlphaBlendMode mode)
		{
			switch (mode)
			{
			case Opaque: return "Opaque";
			case Masked: return "Masked";
			case Translucent: return "Translucent";
			case Dithered: return "Dithered";
			}
			return "Unknown";
		}

		void SetAlbedo(glm::vec4 color) { m_MaterialData.Albedo = color; }
		void SetNormal(float value) { m_MaterialData.Normal = value; }
		void SetSpecular(glm::vec4 color) { m_MaterialData.Specular = color; }
		void SetRougness(float value) { m_MaterialData.Rougness = value; }
		void SetAlpha(float value) { m_MaterialData.Alpha = value; }

		void SetAlphaBlendMode(int value) { m_MaterialData.BlendMode = (AlphaBlendMode)value; }

		void SetAlbedoMap(Ref<Texture2D> texture) { m_AlbedoMap = texture; }
		void SetNormalMap(Ref<Texture2D> texture) { m_NormalMap = texture; }
		void SetSpecularMap(Ref<Texture2D> texture) { m_SpecularMap = texture; }
		void SetRougnessMap(Ref<Texture2D> texture) { m_RougnessMap = texture; }
		void SetAlphaMap(Ref<Texture2D> texture) { m_AlphaMap = texture; }

		void Bind(bool bind_shader = false)
		{
			if (!s_MaterialUniformBuffer)
				s_MaterialUniformBuffer = UniformBuffer::Create(sizeof(MaterialData), 3);

			if (m_MaterialData.UsingAlbedoMap = (bool)m_AlbedoMap) m_AlbedoMap->Bind(0);
			if (m_MaterialData.UsingNormalMap = (bool)m_NormalMap) m_NormalMap->Bind(1);
			if (m_MaterialData.UsingSpecularMap = (bool)m_SpecularMap) m_SpecularMap->Bind(2);
			if (m_MaterialData.UsingRougnessMap = (bool)m_RougnessMap) m_RougnessMap->Bind(4);
			if (m_MaterialData.UsingAlphaMap = (bool)m_AlphaMap) m_AlphaMap->Bind(5);

			s_MaterialUniformBuffer->SetData(&m_MaterialData, sizeof(MaterialData));
		}

		Ref<Texture2D> m_AlbedoMap;
		Ref<Texture2D> m_NormalMap;
		Ref<Texture2D> m_SpecularMap;
		Ref<Texture2D> m_MetalnessMap;
		Ref<Texture2D> m_RougnessMap;
		Ref<Texture2D> m_AlphaMap;

		struct MaterialData
		{
			glm::vec4 Albedo = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			glm::vec4 Specular = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
			float Metalness = 0.0f;
			float Shininess = 32.0f;
			float Rougness = 0.5f;
			float Alpha = 1.0f;
			float Normal = 1.0f;

			float UsingAlbedoMap = 0.0f;
			float UsingNormalMap = 0.0f;
			float UsingSpecularMap = 0.0f;
			float UsingMetalnessMap = 0.0f;
			float UsingRougnessMap = 0.0f;
			float UsingAlphaMap = 0.0f;

			int BlendMode = Opaque;
		};
		MaterialData m_MaterialData;

	private:
		std::string m_Name;
	};

	class Mesh
	{
	public:
		static Ref<Mesh> Create(const std::string& name, std::vector<MeshVertex> verticies, std::vector<uint32_t> indicies, Ref<Material> materialInstance) { return CreateRef<Mesh>(name, verticies, indicies, materialInstance); }

		Mesh(const std::string& name, std::vector<MeshVertex> verticies, std::vector<uint32_t> indicies, Ref<Material> materialInstance)
			: m_Name(name), m_Verticies(verticies), m_Indicies(indicies), m_Material(materialInstance)
		{
			SetupMesh();
		}

		Mesh(const Mesh& other)
		{
			m_Material = CreateRef<Material>(*other.m_Material);
		}

		void Draw(bool bindMaterial = true);

		inline void SetMaterial(Ref<Material> materialInstance) { m_Material = materialInstance; }
		inline Ref<Material> GetMaterial() const { return m_Material; }

		inline std::string GetName() const { return m_Name; }

	private:
		void SetupMesh();

	public:
		std::vector<MeshVertex> m_Verticies;
		std::vector<uint32_t> m_Indicies;


	private:
		Ref<VertexArray> m_MeshVertexArray;
		Ref<VertexBuffer> m_MeshVertexBuffer;
		Ref<IndexBuffer> m_MeshIndexBuffer;

		std::string m_Name;

		Ref<Material> m_Material;
	};

}