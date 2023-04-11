#pragma once
#include "Dymatic/Core/Base.h"
#include "Dymatic/Core/UUID.h"

#include "Dymatic/Asset/Asset.h"
#include "Dymatic/Asset/AssetMetadata.h"
#include "Dymatic/Asset/AssetSerializer.h"

#include <yaml-cpp/yaml.h>

#include <string>
#include <filesystem>
#include <unordered_map>

namespace Dymatic {

	class AssetManager
	{
	public:
		static void Init();
		static void Shutdown();

		static void Serialize();
		static bool Deserialize();
		static void Clear();

		static std::string GetFileSystemPathString(const AssetMetadata& metadata);

		static const std::unordered_map<UUID, AssetMetadata>& GetMetadataRegistry() { return s_MetadataRegistry; }

		static Ref<Asset> GetAsset(UUID assetID);
		static Ref<Asset> GetAsset(const std::filesystem::path& path) { return GetAsset(GetAssetHandleFromFilePath(path)); }

		template<typename T>
		static Ref<T> GetAsset(UUID assetID) { return As<T>(GetAsset(assetID)); }
		
		template<typename T>
		static Ref<T> GetAsset(const std::filesystem::path& path) { return GetAsset<T>(GetAssetHandleFromFilePath(path)); }

		template<typename T, typename ... Args>
		static Ref<T> CreateNewAsset(const std::filesystem::path& filepath, Args&& ... args)
		{
			// New UUID
			UUID uuid;
			// Create the asset
			Ref<T> asset = T::Create(std::forward<Args>(args)...);
			asset->Handle = uuid;

			// Add the asset as a WeakRef to the registry.
			s_AssetRegistry[uuid] = asset;

			// Setup the metadata
			s_MetadataRegistry[uuid] = AssetMetadata();
			auto& metadata = s_MetadataRegistry[uuid];
			metadata.Handle = uuid;
			metadata.Type = asset->GetAssetType();
			metadata.FilePath = filepath.lexically_normal();

			// Serialize the metadata
			AssetManager::Serialize();

			// Serialize the default asset
			AssetImporter::Serialize(metadata, asset);

			// Return the Ref
			return asset;
		}

		static void RenameAsset(const std::filesystem::path& oldPath, const std::filesystem::path& newPath);
		static void RemoveAsset(UUID handle);
		static void RemoveAsset(const std::filesystem::path& path) { RemoveAsset(GetAssetHandleFromFilePath(path)); }
		static void AddAssetMetadata(const AssetMetadata& metadata);

		static bool DoesAssetExist(const std::filesystem::path& path);
		static UUID GetAssetHandleFromFilePath(const std::filesystem::path& path);

		static void SerializeAsset(UUID handle);
		static void SerializeAsset(Ref<Asset> asset);
		
		static const AssetMetadata& GetMetadata(UUID assetID);
		static const AssetMetadata& GetMetadata(const std::filesystem::path& path) { GetMetadata(GetAssetHandleFromFilePath(path)); }

		static AssetType AssetTypeFromString(const std::string& assetType);
		static const char* AssetTypeToString(AssetType assetType);

	private:
		static std::unordered_map<UUID, WeakRef<Asset>> s_AssetRegistry;
		static std::unordered_map<UUID, AssetMetadata> s_MetadataRegistry;

		friend class AssetManagerPanel;
	};

	class AssetImporter
	{
	public:
		static void Init();
		static void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset);
		static bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset);

	private:
		static std::unordered_map<AssetType, Scope<AssetSerializer>> s_Serializers;
	};
}