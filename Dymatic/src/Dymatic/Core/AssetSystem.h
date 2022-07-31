#pragma once
#include "Dymatic/Core/UUID.h"
#include <yaml-cpp/yaml.h>

namespace Dymatic {

	enum class AssetType : uint16_t
	{
		None = 0,
		Scene = 1,
		MeshAsset = 2,
		Mesh = 3,
		Material = 4,
		Texture = 5
	};

	enum class AssetFlag : uint16_t
	{
		None = 0,
		Invalid = 1
	};

	struct AssetMetadata
	{
		AssetMetadata() = default;

		UUID Handle;
		AssetType Type;
		std::filesystem::path FilePath;
		bool IsDataLoaded;

		bool IsValid();
	};

	class Asset
	{
		UUID Handle;
		virtual AssetType GetAssetType() const = 0;
	};

	class AssetManager
	{
		static std::unordered_map<UUID, Ref<Asset>> s_AssetRegistry;
		static std::unordered_map<UUID, AssetMetadata> s_MetadataRegistry;

		template<typename T>
		static Ref<T> GetAsset(UUID assetID) { return s_AssetRegistry[assetID]; }

		template<typename T, typename ... Args>
		static Ref<T> CreateNewAsset(const std::string& filename, const std::string& directoryPath, Args&& ... args)
		{
			UUID()
				return CreateRef<T>(Args);
		}

		static UUID GetAssetHandleFromFilePath(const std::filesystem::path& path)
		{
			for (auto i = s_MetadataRegistry.begin(); i != s_MetadataRegistry.end(); i++)
				if (i->second.FilePath == path)
					return i->second.Handle;
			return 0;
		}

		static const AssetMetadata& GetMetadata(UUID assetID) { return s_MetadataRegistry[assetID]; }
		static const AssetMetadata& GetMetadata(const std::filesystem::path& path) { GetMetadata(GetAssetHandleFromFilePath(path)); }

		static AssetType AssetTypeFromString(const std::string& assetType)
		{
			if (assetType == "None") return AssetType::None;
			if (assetType == "Scene") return AssetType::Scene;
			if (assetType == "MeshAsset") return AssetType::MeshAsset;
			if (assetType == "Mesh") return AssetType::Mesh;
			if (assetType == "Material") return AssetType::Material;
			if (assetType == "Texture") return AssetType::Texture;

			DY_CORE_ASSERT(false, "Unknown Asset Type");
			return AssetType::None;
		}

		static const char* AssetTypeFromString(AssetType assetType)
		{
			switch (assetType)
			{
			case AssetType::None: return "None";
			case AssetType::Scene: return "Scene";
			case AssetType::MeshAsset: return "MeshAsset";
			case AssetType::Mesh: return "Mesh";
			case AssetType::Material: return "Material";
			case AssetType::Texture: return "Texture";
			}

			DY_CORE_ASSERT(false, "Unknown Asset Type");
			return "None";
		}
	};

	class AssetImporter
	{
		static std::map<AssetType, Scope<Asset>> s_Serializers;

		static void Init()
		{
		}
	};

	class AssetSerializer
	{
		virtual void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const = 0;
		virtual bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const = 0;
	};
}