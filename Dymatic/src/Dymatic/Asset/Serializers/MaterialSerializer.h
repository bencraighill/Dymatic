#pragma once

#include "Dymatic/Asset/AssetSerializer.h"
#include "Dymatic/Renderer/Material.h"

#include <yaml-cpp/yaml.h>
#include "Dymatic/Utils/YAMLUtils.h"

#include "Dymatic/Project/Project.h"

#include <fstream>

namespace Dymatic {

	class MaterialSerializer : public AssetSerializer
	{
	public:
		virtual void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const override
		{
			const Ref<Material> material = As<Material>(asset);
			
			YAML::Emitter out;
			out << YAML::BeginMap << YAML::Key << "Material" << YAML::Value << YAML::BeginMap;

			out << YAML::Key << "Albedo" << YAML::Value << material->GetAlbedo();
			out << YAML::Key << "Emissive" << YAML::Value << material->GetEmissive();
			out << YAML::Key << "EmissiveIntensity" << YAML::Value << material->GetEmissiveIntensity();
			out << YAML::Key << "Specular" << YAML::Value << material->GetSpecular();
			out << YAML::Key << "Metalness" << YAML::Value << material->GetMetalness();
			out << YAML::Key << "Roughness" << YAML::Value << material->GetRoughness();
			out << YAML::Key << "Alpha" << YAML::Value << material->GetAlpha();
			out << YAML::Key << "Ambient Occlusion" << YAML::Value << material->GetAmbientOcclusion();

			auto& albedoMap = material->GetAlbedoMap();
			out << YAML::Key << "AlbedoMap" << YAML::Value << (albedoMap ? albedoMap->Handle : 0);
			auto& normalMap = material->GetNormalMap();
			out << YAML::Key << "NormalMap" << YAML::Value << (normalMap ? normalMap->Handle : 0);
			auto& emissiveMap = material->GetEmissiveMap();
			out << YAML::Key << "EmissiveMap" << YAML::Value << (emissiveMap ? emissiveMap->Handle : 0);
			auto& specularMap = material->GetSpecularMap();
			out << YAML::Key << "SpecularMap" << YAML::Value << (specularMap ? specularMap->Handle : 0);
			auto& metalnessMap = material->GetMetalnessMap();
			out << YAML::Key << "MetalnessMap" << YAML::Value << (metalnessMap ? metalnessMap->Handle : 0);
			auto& roughnessMap = material->GetRoughnessMap();
			out << YAML::Key << "RoughnessMap" << YAML::Value << (roughnessMap ? roughnessMap->Handle : 0);
			auto& alphaMap = material->GetAlphaMap();
			out << YAML::Key << "AlphaMap" << YAML::Value << (alphaMap ? alphaMap->Handle : 0);
			auto& ambientOcclusionMap = material->GetAmbientOcclusionMap();
			out << YAML::Key << "AmbientOcclusionMap" << YAML::Value << (ambientOcclusionMap ? ambientOcclusionMap->Handle : 0);
			
			out << YAML::Key << "AlphaBlendMode" << YAML::Value << (int)material->GetAlphaBlendMode();
			
			out << YAML::EndMap; // Material
			out << YAML::EndMap;

			std::ofstream fout(Project::GetAssetFileSystemPath(metadata.FilePath));
			fout << out.c_str();

			return;
		}

		virtual bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const override
		{
			Ref<Material> material = Material::Create(metadata.FilePath.filename().stem().string());

			YAML::Node data;
			try
			{
				data = YAML::LoadFile(AssetManager::GetFileSystemPathString(metadata));
			}
			catch (YAML::ParserException e)
			{
				DY_CORE_ERROR("Failed to load material file '{0}'\n     {1}", metadata.FilePath.string(), e.what());
				return false;
			}

			bool result = false;
			if (auto materialNode = data["Material"])
			{
				if (auto albedoNode = materialNode["Albedo"])
					material->SetAlbedo(albedoNode.as<glm::vec4>());
				if (auto emissiveNode = materialNode["Emissive"])
					material->SetEmissive(emissiveNode.as<glm::vec3>());
				if (auto emissiveIntensityNode = materialNode["EmissiveIntensity"])
					material->SetEmissiveIntensity(emissiveIntensityNode.as<float>());
				if (auto specularNode = materialNode["Specular"])
					material->SetSpecular(specularNode.as<glm::vec4>());
				if (auto metalnessNode = materialNode["Metalness"])
					material->SetMetalness(metalnessNode.as<float>());
				if (auto roughnessNode = materialNode["Roughness"])
					material->SetRoughness(roughnessNode.as<float>());
				if (auto alphaNode = materialNode["Alpha"])
					material->SetAlpha(alphaNode.as<float>());
				if (auto ambientOcclusionNode = materialNode["Ambient Occlusion"])
					material->SetAmbientOcclusion(ambientOcclusionNode.as<float>());
				
				if (auto albedoMapNode = materialNode["AlbedoMap"])
					material->SetAlbedoMap(AssetManager::GetAsset<Texture2D>(albedoMapNode.as<UUID>()));
				if (auto normalMapNode = materialNode["NormalMap"])
					material->SetNormalMap(AssetManager::GetAsset<Texture2D>(normalMapNode.as<UUID>()));
				if (auto emissiveMapNode = materialNode["EmissiveMap"])
					material->SetEmissiveMap(AssetManager::GetAsset<Texture2D>(emissiveMapNode.as<UUID>()));
				if (auto specularMapNode = materialNode["SpecularMap"])
					material->SetSpecularMap(AssetManager::GetAsset<Texture2D>(specularMapNode.as<UUID>()));
				if (auto metalnessMapNode = materialNode["MetalnessMap"])
					material->SetMetalnessMap(AssetManager::GetAsset<Texture2D>(metalnessMapNode.as<UUID>()));
				if (auto roughnessMapNode = materialNode["RoughnessMap"])
					material->SetRoughnessMap(AssetManager::GetAsset<Texture2D>(roughnessMapNode.as<UUID>()));
				if (auto alphaMapNode = materialNode["AlphaMap"])
					material->SetAlphaMap(AssetManager::GetAsset<Texture2D>(alphaMapNode.as<UUID>()));
				if (auto ambientOcclusionMapNode = materialNode["AmbientOcclusionMap"])
					material->SetAmbientOcclusionMap(AssetManager::GetAsset<Texture2D>(ambientOcclusionMapNode.as<UUID>()));
				
				if (auto alphaBlendModeNode = materialNode["AlphaBlendMode"])
					material->SetAlphaBlendMode((Material::AlphaBlendMode)alphaBlendModeNode.as<int>());
				
				result = true;
			}

			asset = material;
			asset->Handle = metadata.Handle;

			//if (!result)
			//	asset->SetFlag(AssetFlag::Invalid, true);

			return result;
		}
	};

}