#pragma once
#include "Dymatic/Asset/Asset.h"
#include "Dymatic/Renderer/Texture.h"

namespace Dymatic {

	class Material : public Asset
	{
	public:
		static AssetType GetStaticType() { return AssetType::Material; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }

	public:
		struct MaterialData
		{
			glm::vec4 Albedo = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			glm::vec3 Emissive = glm::vec3(0.0f, 0.0f, 0.0f);
			float EmissiveIntensity = 0.0f;
			glm::vec4 Specular = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
			float Metalness = 0.0f;
			float Shininess = 32.0f;
			float Roughness = 0.5f;
			float Alpha = 1.0f;
			float AmbientOcclusion = 1.0f;

			float UsingAlbedoMap = 0.0f;
			float UsingNormalMap = 0.0f;
			float UsingEmissiveMap = 0.0f;
			float UsingSpecularMap = 0.0f;
			float UsingMetalnessMap = 0.0f;
			float UsingRougnessMap = 0.0f;
			float UsingAlphaMap = 0.0f;
			float UsingAmbientOcclusionMap = 0.0f;

			int BlendMode = Opaque;
		};

		enum AlphaBlendMode
		{
			Opaque = 0,
			Masked,
			Translucent,
			Dithered,

			ALPHA_BLEND_MODE_SIZE
		};

	public:
		static Ref<Material> Create(std::string name) { return CreateRef<Material>(name); }

		Material(std::string name)
			: m_Name(name) {}

		const std::string& GetName() { return m_Name; }

		static const char* AlphaBlendModeToString(AlphaBlendMode mode);

		const MaterialData& GetMaterialData() { return m_MaterialData; }

		glm::vec4& GetAlbedo() { return m_MaterialData.Albedo; }
		glm::vec3& GetEmissive() { return m_MaterialData.Emissive; }
		float& GetEmissiveIntensity() { return m_MaterialData.EmissiveIntensity; }
		glm::vec4& GetSpecular() { return m_MaterialData.Specular; }
		float& GetMetalness() { return m_MaterialData.Metalness; }
		float& GetRoughness() { return m_MaterialData.Roughness; }
		float& GetAlpha() { return m_MaterialData.Alpha; }
		float& GetAmbientOcclusion() { return m_MaterialData.AmbientOcclusion; }
		
		void SetAlbedo(glm::vec4 value) { m_MaterialData.Albedo = value; }
		void SetEmissive(glm::vec3 value) { m_MaterialData.Emissive = value; }
		void SetEmissiveIntensity(float value) { m_MaterialData.EmissiveIntensity = value; }
		void SetSpecular(glm::vec4 value) { m_MaterialData.Specular = value; }
		void SetMetalness(float value) { m_MaterialData.Metalness = value; }
		void SetRoughness(float value) { m_MaterialData.Roughness = value; }
		void SetAlpha(float value) { m_MaterialData.Alpha = value; }
		void SetAmbientOcclusion(float value) { m_MaterialData.AmbientOcclusion = value; }

		AlphaBlendMode GetAlphaBlendMode() { return (AlphaBlendMode)m_MaterialData.BlendMode; }
		void SetAlphaBlendMode(AlphaBlendMode value) { m_MaterialData.BlendMode = value; }

		Ref <Texture2D>& GetAlbedoMap() { return m_AlbedoMap; }
		Ref <Texture2D>& GetNormalMap() { return m_NormalMap; }
		Ref <Texture2D>& GetEmissiveMap() { return m_EmissiveMap; }
		Ref <Texture2D>& GetSpecularMap() { return m_SpecularMap; }
		Ref <Texture2D>& GetMetalnessMap() { return m_MetalnessMap; }
		Ref <Texture2D>& GetRoughnessMap() { return m_RougnessMap; }
		Ref <Texture2D>& GetAlphaMap() { return m_AlphaMap; }
		Ref <Texture2D>& GetAmbientOcclusionMap() { return m_AmbientOcclusionMap; }
		void SetAlbedoMap(Ref<Texture2D> texture) { m_AlbedoMap = texture; }
		void SetNormalMap(Ref<Texture2D> texture) { m_NormalMap = texture; }
		void SetEmissiveMap(Ref<Texture2D> texture) { m_EmissiveMap = texture; }
		void SetSpecularMap(Ref<Texture2D> texture) { m_SpecularMap = texture; }
		void SetMetalnessMap(Ref<Texture2D> texture) { m_MetalnessMap = texture; }
		void SetRoughnessMap(Ref<Texture2D> texture) { m_RougnessMap = texture; }
		void SetAlphaMap(Ref<Texture2D> texture) { m_AlphaMap = texture; }
		void SetAmbientOcclusionMap(Ref<Texture2D> texture) { m_AmbientOcclusionMap = texture; }

		void Bind();

	private:
		Ref<Texture2D> m_AlbedoMap = nullptr;
		Ref<Texture2D> m_NormalMap = nullptr;
		Ref<Texture2D> m_EmissiveMap = nullptr;
		Ref<Texture2D> m_SpecularMap = nullptr;
		Ref<Texture2D> m_MetalnessMap = nullptr;
		Ref<Texture2D> m_RougnessMap = nullptr;
		Ref<Texture2D> m_AlphaMap = nullptr;
		Ref<Texture2D> m_AmbientOcclusionMap = nullptr;

		MaterialData m_MaterialData;

		std::string m_Name;
	};
	
}