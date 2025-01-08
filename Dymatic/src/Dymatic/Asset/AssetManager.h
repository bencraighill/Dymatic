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

		static void OnUpdate();

		static void Serialize();
		static bool Deserialize();
		static void Clear();

		static std::string GetFileSystemPathString(const AssetMetadata& metadata);
		static std::string GetFileSystemPathString(AssetHandle handle);

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

			// Update metadata filepath cache
			s_AssetFilepathHandleMap[metadata.FilePath] = metadata.Handle;

			// Serialize the metadata
			AssetManager::Serialize();

			// Serialize the default asset
			AssetImporter::Serialize(metadata, asset);

			DY_CORE_INFO("Created new asset with ID {} at '{}'", metadata.Handle, metadata.FilePath.string());

			// Return the Ref
			return asset;
		}
		
		template<typename T, typename ... Args>
		static Ref<T> CreateMemoryOnlyAsset(Args&& ... args)
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
			metadata.MemoryOnly = true;

			// Return the Ref
			return asset;
		}

		static Ref<Asset> RequestAsset(AssetHandle handle);

		template<typename T>
		static Ref<T> RequestAsset(AssetHandle handle) { return As<T>(RequestAsset(handle)); }

		template<typename T>
		static Ref<T> OverrideAsset(AssetHandle handle, Ref<T> assetOverride)
		{
			if (!DoesAssetExist(handle) || !assetOverride)
				return nullptr;

			if (!IsAssetAlive(handle))
			{
				s_AssetRegistry[handle] = assetOverride;
				assetOverride->Handle = handle;

				return assetOverride;
			}

			Ref<T> asset = GetAsset<T>(handle);

			// Ensure we trigger the move constructor
			*asset = *assetOverride;
			asset->Handle = handle;

			return asset;
		}

		template <typename T>
		static void ForceReloadAsset(AssetHandle handle)
		{
			// Verify the asset is not already alive
			if (IsAssetAlive(handle))
			{
				DY_CORE_INFO("Asset requested for forced reload is not alive.");
				return;
			}

			// Try and load the asset data from disk and request an asset override if that succeeded
			Ref<Asset> asset;
			if (AssetImporter::TryLoadData(s_MetadataRegistry[handle], asset, false))
			{
				DY_CORE_INFO("Asset with ID {} was successfully force reloaded from disk, overiding all instances...", handle);
				OverrideAsset<T>(handle, As<T>(asset));
			}
		}

		static void SetAsset(UUID handle, Ref<Asset> asset);
			
		static void RenameAsset(Ref<Asset> asset, const std::filesystem::path& path);
		static void RenameAsset(const std::filesystem::path& oldPath, const std::filesystem::path& newPath);
		static void RemoveAsset(UUID handle, const bool serialize = true);
		static void RemoveAsset(const std::filesystem::path& path, const bool serialize = true) { RemoveAsset(GetAssetHandleFromFilePath(path), serialize); }
		static void AddAssetMetadata(const AssetMetadata& metadata);

		static bool DoesAssetExist(UUID handle);
		static bool DoesAssetExist(const std::filesystem::path& path);
		static UUID GetAssetHandleFromFilePath(const std::filesystem::path& path);

		static bool IsAssetAlive(UUID handle);

		static void SerializeAsset(UUID handle);
		static void SerializeAsset(Ref<Asset> asset);
		
		static const AssetMetadata& GetMetadata(UUID assetID);
		static const AssetMetadata& GetMetadata(const std::filesystem::path& path) { return GetMetadata(GetAssetHandleFromFilePath(path)); }

		static bool IsAssetTypeCompatible(const AssetType target, const AssetType source);

		static AssetType AssetTypeFromString(const std::string& assetType);
		static const char* AssetTypeToString(AssetType assetType);

		static void RegisterEngineMemoryOnlyAsset(const Ref<Asset> asset, AssetHandle handle, const std::string& name);

		static void SerializeAssetPack(const std::filesystem::path& path);
		static void DeserializeAssetPack(const std::filesystem::path& path);
		static const std::filesystem::path GetAssetPackFilepath();
		static const bool UsingAssetPack();

	private:
		static Ref<Asset> GetAsset(const AssetHandle handle, bool multithreaded);
		static bool LoadDataInternal(const AssetMetadata& metadata, Ref<Asset>& asset, bool multithreaded);

		static void LoadEngineAssetMetadata();

	private:
		static std::unordered_map<UUID, WeakRef<Asset>> s_AssetRegistry;
		static std::unordered_map<UUID, AssetMetadata> s_MetadataRegistry;

		// Hash map for quick filepath lookups
		static std::unordered_map<std::filesystem::path, AssetHandle> s_AssetFilepathHandleMap;

		// TODO: Perhaps just store this map inside the AssetPackFile class?
		// We can also store the Stream reader inside here as well so it can be accessed by the AssetImporter as needed.
		// Make this object reference counted
		static std::unordered_map<UUID, AssetPackFile::AssetInfo> s_AssetInfoRegistry; // For runtime (or asset pack usage) only!
		static Ref<FileStreamReader> s_AssetPackReader;
		static std::filesystem::path s_AssetPackFilepath;

		friend class AssetManagerPanel;
	};

	class AssetImporter
	{
	public:
		static void Init();
		
		static void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset);
		static bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset, bool multithreaded);

		static bool SerializeToAssetPack(const AssetMetadata& metadata, const AssetHandle handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo);
		static bool DeserializeFromAssetPack(const AssetMetadata& metadata, Ref<Asset>& asset, FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo);

	private:
		static std::unordered_map<AssetType, Scope<AssetSerializer>> s_Serializers;
	};
}