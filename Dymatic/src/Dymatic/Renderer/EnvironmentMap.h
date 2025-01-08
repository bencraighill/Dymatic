#pragma once

#include "Dymatic/Renderer/RendererResource.h"
#include "Dymatic/Renderer/Texture.h"

namespace Dymatic {

	class EnvironmentMap : public RendererResource
	{
	public:
		static Ref<EnvironmentMap> Create(const std::filesystem::path& filepath) { return CreateRef<EnvironmentMap>(filepath); }
		static Ref<EnvironmentMap> Create(const Buffer& fileData) { return CreateRef<EnvironmentMap>(fileData); }

		static AssetType GetStaticType() { return AssetType::EnvironmentMap; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }

	public:
		EnvironmentMap(const std::filesystem::path& filepath);
		EnvironmentMap(const Buffer& fileData);

		inline Ref<Texture2D> GetMap() const { return m_Texture; }

		inline bool IsLoaded() const { return m_Texture && m_Texture->IsLoaded(); }

	private:
		Ref<Texture2D> m_Texture;
		
		// TODO: Precompute the texture cube environment map, irradiance map, prefilter map, and BRDF LUT here
	};

}