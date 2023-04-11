#include "dypch.h"
#include "Dymatic/Asset/AssetManager.h"

#include "Dymatic/Core/Base.h"
#include "Dymatic/Project/Project.h"

#include "Dymatic/Asset/Serializers/MeshSerializer.h"
#include "Dymatic/Asset/Serializers/MaterialSerializer.h"
#include "Dymatic/Asset/Serializers/TextureSerializer.h"
#include "Dymatic/Asset/Serializers/FontSerializer.h"
#include "Dymatic/Asset/Serializers/AudioSerializer.h"

#include <yaml-cpp/yaml.h>
#include "Dymatic/Utils/YAMLUtils.h"

namespace Dymatic {
	
	std::unordered_map<UUID, WeakRef<Asset>> AssetManager::s_AssetRegistry;
	std::unordered_map<UUID, AssetMetadata> AssetManager::s_MetadataRegistry;

	std::unordered_map<AssetType, Scope<AssetSerializer>> AssetImporter::s_Serializers;

	void AssetManager::Init()
	{
		DY_PROFILE_FUNCTION();

		AssetImporter::Init();
	}

	void AssetManager::Shutdown()
	{
		DY_PROFILE_FUNCTION();
	}

	void AssetManager::Serialize()
	{
		DY_PROFILE_FUNCTION();

		// Serialize the asset metadata registry
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Registry" << YAML::Value << YAML::BeginSeq;
		for (auto& [key, metadata] : s_MetadataRegistry)
		{
			out << YAML::BeginMap;
			out << YAML::Key << "Handle" << YAML::Value << metadata.Handle;
			out << YAML::Key << "Type" << YAML::Value << AssetTypeToString(metadata.Type);
			out << YAML::Key << "FilePath" << YAML::Value << metadata.FilePath.string();
			out << YAML::EndMap;
		}
		out << YAML::EndSeq;
		out << YAML::EndMap;

		std::ofstream fout(Project::GetProjectDirectory() / "Registry.dyareg");
		fout << out.c_str();
	}

	bool AssetManager::Deserialize()
	{
		DY_PROFILE_FUNCTION();

		Clear();

		DY_CORE_INFO("Loading Dymatic Asset Registry...");

		auto registryPath = Project::GetProjectDirectory() / "Registry.dyareg";

		if (!std::filesystem::exists(registryPath))
		{
			DY_CORE_ERROR("Dymatic asset registry does not exist");
			return false;
		}

		// Try and load the asset metadata registry
		YAML::Node data;
		try
		{
			data = YAML::LoadFile(registryPath.string());
		}
		catch (YAML::ParserException e)
		{
			DY_CORE_ERROR("Failed to load Dymtaic asset registry: {0}", e.what());
			return false;
		}
		
		auto registry = data["Registry"];
		if (registry)
		{
			// Iterate over all asset metadata entries in the file
			for (auto entry : registry)
			{
				UUID handle = entry["Handle"].as<UUID>();
				auto& metadata = s_MetadataRegistry[handle];
				metadata.Handle = handle;
				metadata.Type = AssetTypeFromString(entry["Type"].as<std::string>());
				metadata.FilePath = std::filesystem::path(entry["FilePath"].as<std::string>()).lexically_normal();
			}
			
			return true;
		}
		return false;
	}

	void AssetManager::Clear()
	{
		DY_PROFILE_FUNCTION();
		DY_CORE_INFO("Clearing asset registry...");
		
		s_AssetRegistry.clear();
		s_MetadataRegistry.clear();
	}

	std::string AssetManager::GetFileSystemPathString(const AssetMetadata& metadata)
	{
		return (Project::GetAssetDirectory() / metadata.FilePath).lexically_normal().string();
	}

	Ref<Asset> AssetManager::GetAsset(UUID assetID)
	{
		DY_PROFILE_FUNCTION();

		if (assetID == 0)
			return nullptr;

		// Check if the asset is already loaded
		if (s_AssetRegistry.find(assetID) != s_AssetRegistry.end())
		{
			// Check if the asset has expired
			if (s_AssetRegistry[assetID].expired())
			{
				DY_CORE_INFO("Asset with ID {0} has expired, loading from disk...", assetID);

				// Load the asset from disk
				Ref<Asset> asset;
				if (AssetImporter::TryLoadData(s_MetadataRegistry[assetID], asset))
				{
					s_AssetRegistry[assetID] = asset;
					return asset;
				}
			}
			else
			{
				// Return the asset from the registry
				DY_CORE_INFO("Loading asset with ID {0} from registry...", assetID);
				return s_AssetRegistry[assetID].lock();
			}
		}
		// If the asset has not been loaded yet, check the metadata registry, and load it in
		else if (s_MetadataRegistry.find(assetID) != s_MetadataRegistry.end())
		{
			DY_CORE_INFO("Loading asset with ID {0} from disk...", assetID);
			Ref<Asset> asset;
			if (AssetImporter::TryLoadData(s_MetadataRegistry[assetID], asset))
			{
				s_AssetRegistry[assetID] = asset;
				return asset;
			}
			else
				DY_CORE_INFO("TryLoadData for asset with ID {0} failed");
		}

		// The asset does not exist, so we return nullptr
		DY_CORE_ERROR("Asset with ID {0} does not exist", assetID);
		return nullptr;
	}

	void AssetManager::RenameAsset(const std::filesystem::path& oldPath, const std::filesystem::path& newPath)
	{
		DY_PROFILE_FUNCTION();

		std::filesystem::path oldPathNorm = oldPath.lexically_normal();
		std::filesystem::path newPathNorm = newPath.lexically_normal();

		DY_CORE_INFO("Renaming asset '{0}' to '{1}'", oldPathNorm.string(), newPathNorm.string());

		for (auto& [key, metadata] : s_MetadataRegistry)
		{
			if (metadata.FilePath == oldPathNorm)
			{
				metadata.FilePath = newPathNorm;
				break;
			}
		}

		Serialize();
	}

	void AssetManager::RemoveAsset(UUID handle)
	{
		DY_PROFILE_FUNCTION();

		if (handle == 0)
			return;

		DY_CORE_INFO("Removing asset with ID {0}", handle);

		if (s_AssetRegistry.find(handle) != s_AssetRegistry.end())
			s_AssetRegistry.erase(handle);

		if (s_MetadataRegistry.find(handle) != s_MetadataRegistry.end())
			s_MetadataRegistry.erase(handle);

		Serialize();
	}

	void AssetManager::AddAssetMetadata(const AssetMetadata& metadata)
	{
		DY_PROFILE_FUNCTION();

		if (AssetManager::DoesAssetExist(metadata.FilePath))
			return;

		s_MetadataRegistry[metadata.Handle] = metadata;
		AssetManager::Serialize();
	}

	bool AssetManager::DoesAssetExist(const std::filesystem::path& path)
	{
		return GetAssetHandleFromFilePath(path) != 0;
	}

	UUID AssetManager::GetAssetHandleFromFilePath(const std::filesystem::path& path)
	{
		DY_PROFILE_FUNCTION();

		std::filesystem::path filepath = path.lexically_normal();
		for (auto [key, metadata] : s_MetadataRegistry)
			if (metadata.FilePath == filepath)
				return metadata.Handle;
		return 0;
	}

	void AssetManager::SerializeAsset(UUID handle)
	{
		DY_PROFILE_FUNCTION();

		if (handle == 0)
			return;

		Ref<Asset> asset = GetAsset(handle);

		if (asset == nullptr)
			return;

		AssetMetadata metadata = s_MetadataRegistry[handle];
		return AssetImporter::Serialize(metadata, asset);
	}

	void AssetManager::SerializeAsset(Ref<Asset> asset)
	{
		if (!asset)
			return;
		return SerializeAsset(asset->Handle);
	}

	const Dymatic::AssetMetadata& AssetManager::GetMetadata(UUID assetID)
	{
		if (assetID == 0)
			return AssetMetadata();
		
		return s_MetadataRegistry[assetID];
	}

	AssetType AssetManager::AssetTypeFromString(const std::string& assetType)
	{
		if (assetType == "None") return AssetType::None;
		if (assetType == "Scene") return AssetType::Scene;
		if (assetType == "Mesh") return AssetType::Mesh;
		if (assetType == "Material") return AssetType::Material;
		if (assetType == "Texture") return AssetType::Texture;
		if (assetType == "Font") return AssetType::Font;
		if (assetType == "Audio") return AssetType::Audio;

		DY_CORE_ASSERT(false, "Unknown Asset Type");
		return AssetType::None;
	}

	const char* AssetManager::AssetTypeToString(AssetType assetType)
	{
		switch (assetType)
		{
		case AssetType::None: return "None";
		case AssetType::Scene: return "Scene";
		case AssetType::Mesh: return "Mesh";
		case AssetType::Material: return "Material";
		case AssetType::Texture: return "Texture";
		case AssetType::Font: return "Font";
		case AssetType::Audio: return "Audio";
		}

		DY_CORE_ASSERT(false, "Unknown Asset Type");
		return "None";
	}

	void AssetImporter::Init()
	{
		DY_PROFILE_FUNCTION();
		
		s_Serializers[AssetType::Mesh] = CreateScope<MeshSerializer>();
		s_Serializers[AssetType::Material] = CreateScope<MaterialSerializer>();
		s_Serializers[AssetType::Texture] = CreateScope<TextureSerializer>();
		s_Serializers[AssetType::Font] = CreateScope<FontSerializer>();
		s_Serializers[AssetType::Audio] = CreateScope<AudioSerializer>();
	}

	void AssetImporter::Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset)
	{
		DY_PROFILE_FUNCTION();

		s_Serializers[metadata.Type]->Serialize(metadata, asset);
	}

	bool AssetImporter::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset)
	{
		DY_PROFILE_FUNCTION();

		return s_Serializers[metadata.Type]->TryLoadData(metadata, asset);
	}
}