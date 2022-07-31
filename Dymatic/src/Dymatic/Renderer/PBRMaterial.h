#pragma once
#include "Dymatic/Renderer/Material.h"

namespace Dymatic {

	struct MaterialData
	{
		glm::vec4 AlbedoFactor = { 1.0f, 1.0f, 1.0f, 1.0f };
		glm::vec4 SpecularFactor = { 1.0f, 1.0f, 1.0f, 1.0f };
		float HeightFactor = 0.0f;
		float MetalnessFactor = 0.5f;
		float AOFactor = 0.5f;
		float UsingNormalMap = 0.0f;
	};
	static Ref<UniformBuffer> s_PBRUniformBuffer = nullptr;

	class PBRMaterial : public Material
	{
	public:
		static Ref<PBRMaterial> Create(Ref<Shader> shader) { return CreateRef<PBRMaterial>(shader); }

		MaterialData m_MaterialData;

		PBRMaterial(Ref<Shader> shader)
			: Material(shader)
		{
			if (!s_PBRUniformBuffer)
				s_PBRUniformBuffer = UniformBuffer::Create(sizeof(MaterialData), 2);

			m_UniformBufferSize = sizeof(MaterialData);
			m_UniformBuffer = s_PBRUniformBuffer;
			m_UniformData = &m_MaterialData;
		}

		// PBR Statics
		inline void SetAlbedo(glm::vec4 color) { m_MaterialData.AlbedoFactor = color; }
		inline void SetSpecular(glm::vec4 value) { m_MaterialData.AlbedoFactor = value; }
		inline void SetHeightFactor(float value) { m_MaterialData.HeightFactor = value; }
		inline void SetMetalness(float value) { m_MaterialData.MetalnessFactor = value; }
		inline void SetAOFactor(float value) { m_MaterialData.AOFactor = value; }
		inline void SetUsingNormalMap(bool value) { m_MaterialData.UsingNormalMap = value; }

		// PBR Maps
		inline void SetAlbedoMap(Ref<Texture2D> texture) {}

		//Ref<Texture2D> GetAlbedoMap();
	};

	class PBRMaterialInstance : public MaterialInstance
	{
	public:
		static Ref<PBRMaterialInstance> Create(Ref<PBRMaterial> material) { return CreateRef<PBRMaterialInstance>(material); }

		PBRMaterialInstance(Ref<PBRMaterial> material)
			: MaterialInstance(material)
		{
		}

		// PBR Statics
		void SetAlbedo(glm::vec4 color);
		void SetNormalFactor(glm::vec4 value);
		void SetSpecular(glm::vec4 color);
		void SetHeightFactor(float value);
		void SetMetallic(float value);
		void SetAOFactor(float value);

		// PBR Maps
		void SetAlbedoMap(Ref<Texture2D> texture);
		void SetNormalMap(Ref<Texture2D> texture);
		void SetSpecularMap(Ref<Texture2D> texture);
		void SetHeightMap(Ref<Texture2D> texture);
		void SetMetallicMap(Ref<Texture2D> texture);
		void SetAOMap(Ref<Texture2D> texture);

	private:
	};

}