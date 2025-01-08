#include "dypch.h"
#include "Dymatic/Asset/AssetManager.h"

#include "Dymatic/Core/Base.h"
#include "Dymatic/Project/Project.h"

#include "Dymatic/Asset/AssetThread.h"
#include "Dymatic/Core/Filesystem.h"

#include "Dymatic/Asset/Serializers/SceneSerializer.h"
#include "Dymatic/Asset/Serializers/PrefabSerializer.h"
#include "Dymatic/Asset/Serializers/MeshSerializer.h"
#include "Dymatic/Asset/Serializers/MaterialSerializer.h"
#include "Dymatic/Asset/Serializers/TextureSerializer.h"
#include "Dymatic/Asset/Serializers/EnvironmentMapSerializer.h"
#include "Dymatic/Asset/Serializers/VirtualTextureSerializer.h"
#include "Dymatic/Asset/Serializers/FontSerializer.h"
#include "Dymatic/Asset/Serializers/AudioSerializer.h"
#include "Dymatic/Asset/Serializers/ParticleSystemSerializer.h"
#include "Dymatic/Asset/Serializers/SkeletonSerializer.h"
#include "Dymatic/Asset/Serializers/AnimationSerializer.h"
#include "Dymatic/Asset/Serializers/AnimationGraphSerializer.h"
#include "Dymatic/Asset/Serializers/VideoSerializer.h"
#include "Dymatic/Asset/Serializers/SubtitleSerializer.h"
#include "Dymatic/Asset/Serializers/VideoPlayerSerializer.h"

#include "Dymatic/Asset/EngineAssetMetadata.h"

#include "Dymatic/Math/StringUtils.h"
#include <yaml-cpp/yaml.h>
#include "Dymatic/Utils/YAMLUtils.h"

namespace Dymatic {
	
	std::unordered_map<UUID, WeakRef<Asset>> AssetManager::s_AssetRegistry;
	std::unordered_map<UUID, AssetMetadata> AssetManager::s_MetadataRegistry;

	std::unordered_map<std::filesystem::path, Dymatic::AssetHandle> AssetManager::s_AssetFilepathHandleMap;

	std::unordered_map<UUID, AssetPackFile::AssetInfo> AssetManager::s_AssetInfoRegistry; // For runtime (or asset pack usage) only!

	Ref<FileStreamReader> AssetManager::s_AssetPackReader;
	std::filesystem::path AssetManager::s_AssetPackFilepath;

	std::unordered_map<AssetType, Scope<AssetSerializer>> AssetImporter::s_Serializers;

	struct EngineMemoryAssetMetadata
	{
		Ref<Asset> Asset;
		AssetHandle Handle;
		std::string Name;
	};

	static std::vector<EngineMemoryAssetMetadata> s_EngineMemoryAssets;

	namespace Utils {
	
		static bool IsEngineAsset(const AssetHandle handle)
		{
			return s_EngineAssets.find((EngineAsset)(uint64_t)handle) != s_EngineAssets.end();
		}

	}

	void AssetManager::Init()
	{
		DY_PROFILE_FUNCTION();

		AssetImporter::Init();
		AssetThread::Init();

		// Internal engine assets should always be ready for usage!
		LoadEngineAssetMetadata();
	}

	void AssetManager::Shutdown()
	{
		DY_PROFILE_FUNCTION();

		AssetThread::Shutdown();
	}

	void AssetManager::OnUpdate()
	{
		DY_PROFILE_FUNCTION();

		AssetThread::ExecuteMainThreadQueue();
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
			// Check if asset is memory only
			if (metadata.MemoryOnly)
				continue;

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
		s_AssetPackFilepath.clear();
		s_AssetPackReader = nullptr;

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
		if (!registry)
			return false;
		
		// Iterate over all asset metadata entries in the file
		for (auto entry : registry)
		{
			UUID handle = entry["Handle"].as<UUID>();
			auto& metadata = s_MetadataRegistry[handle];
			metadata.Handle = handle;
			metadata.Type = AssetTypeFromString(entry["Type"].as<std::string>());
			metadata.FilePath = std::filesystem::path(entry["FilePath"].as<std::string>()).lexically_normal();

			// Update metadata filepath cache
			s_AssetFilepathHandleMap[metadata.FilePath] = metadata.Handle;
		}

		LoadEngineAssetMetadata();

		return true;
	}

	void AssetManager::LoadEngineAssetMetadata()
	{
		if (UsingAssetPack())
			return;

		// Add all engine resource metadata
		for (const auto& [asset, info] : s_EngineAssets)
		{
			const AssetHandle handle = (AssetHandle)asset;

			auto& metadata = s_MetadataRegistry[handle];
			metadata.Handle = handle;
			metadata.Type = info.Type;
			metadata.FilePath = info.Filepath.lexically_normal();
			metadata.MemoryOnly = true;

			// Update metadata filepath cache
			s_AssetFilepathHandleMap[metadata.FilePath] = handle;
		}

		// Add fixed engine memory only assets
		for (const auto& asset : s_EngineMemoryAssets)
		{
			asset.Asset->Handle = asset.Handle;

			auto& metadata = s_MetadataRegistry[asset.Handle];
			metadata.Handle = asset.Handle;
			metadata.Type = asset.Asset->GetAssetType();
			metadata.FilePath = asset.Name;
			metadata.MemoryOnly = true;

			s_AssetRegistry[asset.Handle] = asset.Asset;
			s_AssetFilepathHandleMap[metadata.FilePath] = asset.Handle;
		}
	}

	void AssetManager::Clear()
	{
		DY_PROFILE_FUNCTION();
		DY_CORE_INFO("Clearing asset registry...");
		
		s_AssetRegistry.clear();
		s_MetadataRegistry.clear();
		s_AssetFilepathHandleMap.clear();
	}

	std::string AssetManager::GetFileSystemPathString(const AssetMetadata& metadata)
	{
		if (metadata.MemoryOnly)
			return metadata.FilePath.string();

		return (Project::GetAssetDirectory() / metadata.FilePath).lexically_normal().string();
	}

	std::string AssetManager::GetFileSystemPathString(AssetHandle handle)
	{
		if (!DoesAssetExist(handle))
			return std::string();

		return GetFileSystemPathString(GetMetadata(handle));
	}

	Ref<Asset> AssetManager::GetAsset(const AssetHandle handle, bool multithreaded)
	{
		DY_PROFILE_FUNCTION();

		if (handle == 0)
			return nullptr;

		// Check if the asset is already loaded
		if (s_AssetRegistry.find(handle) != s_AssetRegistry.end())
		{
			// Check if the asset has expired
			if (s_AssetRegistry[handle].expired())
			{
				DY_CORE_INFO("Asset with ID {} has expired, loading from disk...", handle);

				// Load the asset from disk
				Ref<Asset> asset;
				if (LoadDataInternal(s_MetadataRegistry[handle], asset, multithreaded))
				{
					s_AssetRegistry[handle] = asset;
					return asset;
				}
			}
			else
			{
				// Return the asset from the registry
				DY_CORE_INFO("Loading asset with ID {} from registry...", handle);
				return s_AssetRegistry[handle].lock();
			}
		}
		// If the asset has not been loaded yet, check the metadata registry, and load it in
		else if (s_MetadataRegistry.find(handle) != s_MetadataRegistry.end())
		{
			DY_CORE_INFO("Loading asset with ID {} from disk...", handle);
			Ref<Asset> asset;
			if (LoadDataInternal(s_MetadataRegistry[handle], asset, multithreaded))
			{
				s_AssetRegistry[handle] = asset;
				return asset;
			}
			else
			{
				DY_CORE_INFO("TryLoadData for asset with ID {} failed", handle);
				return nullptr;
			}
		}

		// The asset does not exist, so we return nullptr
		DY_CORE_ERROR("Asset with ID {} does not exist", handle);
		return nullptr;
	}

	Ref<Asset> AssetManager::GetAsset(UUID assetID)
	{
		return GetAsset(assetID, false);
	}

	bool AssetManager::LoadDataInternal(const AssetMetadata& metadata, Ref<Asset>& asset, bool multithreaded)
	{
		bool result;

		if (s_AssetPackReader)
		{
			const size_t streamPosition = s_AssetPackReader->GetStreamPosition();
			const auto& assetInfo = s_AssetInfoRegistry.at(metadata.Handle);
			s_AssetPackReader->SetStreamPosition(assetInfo.Offset);
			result = AssetImporter::DeserializeFromAssetPack(metadata, asset, *s_AssetPackReader, assetInfo);
			s_AssetPackReader->SetStreamPosition(streamPosition);
		}
		else
		{
			result = AssetImporter::TryLoadData(metadata, asset, multithreaded);
		}

		if (result)
			asset->Handle = metadata.Handle;

		return result;
	}

	Ref<Asset> AssetManager::RequestAsset(AssetHandle handle)
	{
		return GetAsset(handle, true);
	}

	void AssetManager::SetAsset(UUID handle, Ref<Asset> asset)
	{
		if (handle == 0 || !DoesAssetExist(handle))
			return;

		asset->Handle = handle;
		s_AssetRegistry[handle] = asset;
	}

	void AssetManager::RenameAsset(const std::filesystem::path& oldPath, const std::filesystem::path& newPath)
	{
		DY_PROFILE_FUNCTION();

		std::filesystem::path oldPathNorm = oldPath.lexically_normal();
		std::filesystem::path newPathNorm = newPath.lexically_normal();

		DY_CORE_INFO("Renaming asset '{}' to '{}'", oldPathNorm.string(), newPathNorm.string());

		if (s_AssetFilepathHandleMap.find(oldPathNorm) == s_AssetFilepathHandleMap.end())
			return;

		const AssetHandle handle = s_AssetFilepathHandleMap.at(oldPathNorm);
		s_MetadataRegistry[handle].FilePath = newPathNorm;
		
		// Update the filepath-asset cache
		s_AssetFilepathHandleMap.erase(oldPathNorm);
		s_AssetFilepathHandleMap[newPathNorm] = handle;

		Serialize();
	}

	void AssetManager::RenameAsset(Ref<Asset> asset, const std::filesystem::path& path)
	{
		DY_PROFILE_FUNCTION();

		if (asset == nullptr)
			return;

		std::filesystem::path pathNorm = path.lexically_normal();
		
		// Search metadata registry for the asset and if it exists re target it
		if (s_AssetFilepathHandleMap.find(pathNorm) != s_AssetFilepathHandleMap.end())
		{
			const AssetHandle handle = s_AssetFilepathHandleMap.at(pathNorm);

			DY_CORE_INFO("Retargeting asset with ID {} to {}, at filepath '{}'", asset->Handle, handle, pathNorm.string());

			asset->Handle = handle;
			s_AssetRegistry[handle] = asset;

			AssetImporter::Serialize(s_MetadataRegistry.at(handle), asset);
			return;
		}

		// If the asset filepath is not registered insert it to the registry
		if (asset->Handle == 0)
			asset->Handle = UUID();

		DY_CORE_INFO("Adding asset with ID {} to registry at '{}'", asset->Handle, pathNorm.string());
		
		s_MetadataRegistry[asset->Handle] = AssetMetadata();
		auto& metadata = s_MetadataRegistry[asset->Handle];
		metadata.Handle = asset->Handle;
		metadata.Type = asset->GetAssetType();
		metadata.FilePath = pathNorm;

		// Update filepath-handle cache
		s_AssetFilepathHandleMap[metadata.FilePath] = metadata.Handle;

		s_AssetRegistry[asset->Handle] = asset;
		
		// Serialize the added asset
		AssetImporter::Serialize(metadata, asset);

		// Serialize the registry
		Serialize();
	}

	void AssetManager::RemoveAsset(UUID handle, const bool serialize)
	{
		DY_PROFILE_FUNCTION();

		if (handle == 0)
			return;

		DY_CORE_INFO("Removing asset with ID {}", handle);

		if (s_AssetRegistry.find(handle) != s_AssetRegistry.end())
			s_AssetRegistry.erase(handle);

		if (s_MetadataRegistry.find(handle) != s_MetadataRegistry.end())
		{
			s_AssetFilepathHandleMap.erase(s_MetadataRegistry.at(handle).FilePath);
			s_MetadataRegistry.erase(handle);
		}

		if (serialize)
			Serialize();
	}

	void AssetManager::AddAssetMetadata(const AssetMetadata& metadata)
	{
		DY_PROFILE_FUNCTION();

		if (AssetManager::DoesAssetExist(metadata.FilePath))
			return;

		s_MetadataRegistry[metadata.Handle] = metadata;

		// Update metadata filepath cache
		s_AssetFilepathHandleMap[metadata.FilePath] = metadata.Handle;

		AssetManager::Serialize();
	}

	bool AssetManager::DoesAssetExist(UUID handle)
	{
		if (handle == 0)
			return false;

		return s_MetadataRegistry.find(handle) != s_MetadataRegistry.end();
	}

	bool AssetManager::DoesAssetExist(const std::filesystem::path& path)
	{
		return GetAssetHandleFromFilePath(path) != 0;
	}

	UUID AssetManager::GetAssetHandleFromFilePath(const std::filesystem::path& path)
	{
		DY_PROFILE_FUNCTION();

		const std::filesystem::path filepath = path.lexically_normal();

		if (s_AssetFilepathHandleMap.find(filepath) == s_AssetFilepathHandleMap.end())
			return 0;

		return s_AssetFilepathHandleMap.at(filepath);
	}

	bool AssetManager::IsAssetAlive(UUID handle)
	{
		// Determines if requesting the specified asset will require a read from disk.
		// Check if the asset exists in the registry to begin with and if so verify that it has not expired.
		return s_AssetRegistry.find(handle) != s_AssetRegistry.end() && !s_AssetRegistry[handle].expired();
	}

	void AssetManager::SerializeAsset(UUID handle)
	{
		DY_PROFILE_FUNCTION();

		if (handle == 0)
			return;

		Ref<Asset> asset = GetAsset(handle);

		if (asset == nullptr)
			return;

		const AssetMetadata& metadata = s_MetadataRegistry[handle];

		if (metadata.MemoryOnly || metadata.FilePath.empty())
			return;
		
		AssetImporter::Serialize(metadata, asset);
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

	bool AssetManager::IsAssetTypeCompatible(const AssetType target, const AssetType source)
	{
		if (target == AssetType::Texture && source == AssetType::VirtualTexture)
			return true;

		return target == source;
	}

	AssetType AssetManager::AssetTypeFromString(const std::string& assetType)
	{
		if (assetType == "None") return AssetType::None;
		if (assetType == "Scene") return AssetType::Scene;
		if (assetType == "Prefab") return AssetType::Prefab;
		if (assetType == "Mesh Source") return AssetType::MeshSource;
		if (assetType == "Mesh") return AssetType::Mesh;
		if (assetType == "Material") return AssetType::Material;
		if (assetType == "Texture") return AssetType::Texture;
		if (assetType == "Environment Map") return AssetType::EnvironmentMap;
		if (assetType == "Virtual Texture") return AssetType::VirtualTexture;
		if (assetType == "Font") return AssetType::Font;
		if (assetType == "Audio") return AssetType::Audio;
		if (assetType == "Particle System") return AssetType::ParticleSystem;
		if (assetType == "Skeleton") return AssetType::Skeleton;
		if (assetType == "Animation") return AssetType::Animation;
		if (assetType == "Animation Graph") return AssetType::AnimationGraph;
		if (assetType == "Physics Material") return AssetType::PhysicsMaterial;
		if (assetType == "Video") return AssetType::Video;
		if (assetType == "Subtitle") return AssetType::Subtitle;
		if (assetType == "Video Player") return AssetType::VideoPlayer;

		DY_CORE_ASSERT(false, "Unknown Asset Type");
		return AssetType::None;
	}

	const char* AssetManager::AssetTypeToString(AssetType assetType)
	{
		switch (assetType)
		{
		case AssetType::None: return "None";
		case AssetType::Scene: return "Scene";
		case AssetType::Prefab: return "Prefab";
		case AssetType::MeshSource: return "Mesh Source";
		case AssetType::Mesh: return "Mesh";
		case AssetType::Material: return "Material";
		case AssetType::Texture: return "Texture";
		case AssetType::EnvironmentMap: return "Environment Map";
		case AssetType::VirtualTexture: return "Virtual Texture";
		case AssetType::Font: return "Font";
		case AssetType::Audio: return "Audio";
		case AssetType::ParticleSystem: return "Particle System";
		case AssetType::Skeleton: return "Skeleton";
		case AssetType::Animation: return "Animation";
		case AssetType::AnimationGraph: return "Animation Graph";
		case AssetType::PhysicsMaterial: return "Physics Material";
		case AssetType::Video: return "Video";
		case AssetType::Subtitle: return "Subtitle";
		case AssetType::VideoPlayer: return "Video Player";
		}

		DY_CORE_ASSERT(false, "Unknown Asset Type");
		return "None";
	}

	void AssetManager::RegisterEngineMemoryOnlyAsset(const Ref<Asset> asset, AssetHandle handle, const std::string& name)
	{
		asset->Handle = handle;
		s_EngineMemoryAssets.push_back({ asset, handle, name });
	}

	void AssetImporter::Init()
	{
		DY_PROFILE_FUNCTION();
		
		s_Serializers[AssetType::Scene] = CreateScope<SceneSerializer>();
		s_Serializers[AssetType::Prefab] = CreateScope<PrefabSerializer>();
		s_Serializers[AssetType::Mesh] = CreateScope<MeshSerializer>();
		s_Serializers[AssetType::Material] = CreateScope<MaterialSerializer>();
		s_Serializers[AssetType::Texture] = CreateScope<TextureSerializer>();
		s_Serializers[AssetType::EnvironmentMap] = CreateScope<EnvironmentMapSerializer>();
		s_Serializers[AssetType::VirtualTexture] = CreateScope<VirtualTextureSerializer>();
		s_Serializers[AssetType::Font] = CreateScope<FontSerializer>();
		s_Serializers[AssetType::Audio] = CreateScope<AudioSerializer>();
		s_Serializers[AssetType::ParticleSystem] = CreateScope<ParticleSystemSerializer>();
		s_Serializers[AssetType::Skeleton] = CreateScope<SkeletonSerializer>();
		s_Serializers[AssetType::Animation] = CreateScope<AnimationSerializer>();
		s_Serializers[AssetType::AnimationGraph] = CreateScope<AnimationGraphSerializer>();
		s_Serializers[AssetType::Video] = CreateScope<VideoSerializer>();
		s_Serializers[AssetType::Subtitle] = CreateScope<SubtitleSerializer>();
		s_Serializers[AssetType::VideoPlayer] = CreateScope<VideoPlayerSerializer>();
	}

	void AssetImporter::Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset)
	{
		DY_PROFILE_FUNCTION();

		s_Serializers[metadata.Type]->Serialize(metadata, asset);
	}

	bool AssetImporter::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset, bool multithreaded)
	{
		DY_PROFILE_FUNCTION();

		return s_Serializers[metadata.Type]->TryLoadData(metadata, asset, multithreaded);
	}

	bool AssetImporter::SerializeToAssetPack(const AssetMetadata& metadata, const AssetHandle handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo)
	{
		DY_PROFILE_FUNCTION();

		if (s_Serializers.find(metadata.Type) == s_Serializers.end())
		{
			DY_CORE_TRACE("Asset {} of type {} has no serializer. Skipping...", handle, AssetManager::AssetTypeToString(metadata.Type));
			return false;
		}

		return s_Serializers[metadata.Type]->SerializeToAssetPack(metadata, handle, stream, outInfo);
	}

	bool AssetImporter::DeserializeFromAssetPack(const AssetMetadata& metadata, Ref<Asset>& asset, FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo)
	{
		return s_Serializers[metadata.Type]->DeserializeFromAssetPack(metadata, asset, stream, assetInfo);
	}

	// Asset Pack Serialization
	
	void AssetManager::SerializeAssetPack(const std::filesystem::path& path)
	{
		DY_CORE_TRACE("Generating Asset Pack '{}'", path.string());

		std::filesystem::create_directories(path.parent_path());

		FileStreamWriter writer(path);
		
		// Write the file header
		const char header[] = { 'D', 'Y', 'A', 'P' };
		writer.WriteData(header, sizeof(header));

		// Write the Engine Version
		writer.WriteRaw<uint32_t>(DY_VERSION_MAJOR);
		writer.WriteRaw<uint32_t>(DY_VERSION_MINOR);
		writer.WriteRaw<uint32_t>(DY_VERSION_PATCH);
		
		// Write the Asset Pack Version (date and time of creation as an integer)
		const uint64_t packVersion = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		writer.WriteRaw<uint64_t>(packVersion);

		DY_CORE_INFO("Pack Build Infomation:");
		DY_CORE_INFO("    Engine Version: {}.{}.{}", DY_VERSION_MAJOR, DY_VERSION_MINOR, DY_VERSION_PATCH);
		DY_CORE_INFO("    Pack Version: {}", packVersion);
		DY_CORE_INFO("    Registry Size: {} assets", s_MetadataRegistry.size());

		// Write the metadata registry
		DY_CORE_TRACE("Serializing metadata registry to Asset Pack");
		writer.WriteRaw<size_t>(s_MetadataRegistry.size());
		for (auto& [handle, metadata] : s_MetadataRegistry)
		{
			DY_CORE_INFO("Serializing asset metadata for {} to Asset Pack", handle);

			writer.WriteRaw<UUID>(handle);
			writer.WriteRaw<uint16_t>((uint16_t)metadata.Type);
			writer.WriteString(metadata.FilePath.string());
		}

		std::unordered_map<UUID, AssetPackFile::AssetInfo> assetInfoRegistry;

		// Store the script assemblies

		DY_CORE_TRACE("Serializing Core Module to Asset Pack");
		Buffer coreModuleBuffer = FileSystem::ReadBytes(Project::GetCoreModulePath());
		writer.WriteBuffer(coreModuleBuffer);
		const size_t coreModuleSize = coreModuleBuffer.Size;
		coreModuleBuffer.Release();

		DY_CORE_TRACE("Serializing Script Module to Asset Pack");
		Buffer scriptModuleBuffer = FileSystem::ReadBytes(Project::GetScriptModulePath());
		writer.WriteBuffer(scriptModuleBuffer);
		const size_t scriptModuleSize = scriptModuleBuffer.Size;
		scriptModuleBuffer.Release();
		
		// Write data for all assets
		DY_CORE_TRACE("Serializing asset registry to Asset Pack");

		struct AssetSerializationDebugInfo
		{
			uint32_t TotalRegistry = 0;
			uint32_t TotalSerialized = 0;
			size_t TotalSize = 0;
		};

		std::array<AssetSerializationDebugInfo, (size_t)AssetType::ASSET_TYPE_SIZE> assetSerializationDebugInfo;
		for (auto& [handle, metadata] : s_MetadataRegistry)
		{
			if (metadata.MemoryOnly && metadata.FilePath.empty())
				continue;

			auto& debugInfo = assetSerializationDebugInfo[(size_t)metadata.Type];
			debugInfo.TotalRegistry++;

			if (!metadata.MemoryOnly && !std::filesystem::exists(GetFileSystemPathString((metadata))))
			{
				DY_CORE_WARN("Could not load asset {} at '{}' for serialization. Asset does not exist in filesystem!", handle, metadata.FilePath.string());
				DY_CORE_TRACE("Asset {} was excluded from Asset Pack", handle);
				continue;
			}

			const uint64_t startPosition = writer.GetStreamPosition();
			
			if (!AssetImporter::SerializeToAssetPack(metadata, handle, writer, AssetSerializationInfo()))
			{
				DY_CORE_WARN("Failed to serialize asset {} to Asset Pack", handle);
				continue;
			}

			const uint64_t endPosition = writer.GetStreamPosition();

			AssetPackFile::AssetInfo info;
			info.Handle = handle;
			info.Offset = startPosition;
			info.Size = endPosition - startPosition;
			assetInfoRegistry[handle] = info;

			debugInfo.TotalSerialized++;
			debugInfo.TotalSize += info.Size;

			DY_CORE_INFO("Serializing asset {} to Asset Pack with size {}", handle, String::FormatBytes(info.Size));
		}

		// Serialize the asset offset info registry
		DY_CORE_TRACE("Serializing asset offset info registry to Asset Pack");

		// Store the assetInfoRegistry offset so it can be written to the end of the file
		const uint64_t assetInfoRegistryOffset = writer.GetStreamPosition();

		// Write the asset info registry lookup table
		writer.WriteRaw<size_t>(assetInfoRegistry.size());
		for (auto& [handle, info] : assetInfoRegistry)
		{
			writer.WriteRaw<UUID>(handle);
			writer.WriteRaw<uint64_t>(info.Offset);
			writer.WriteRaw<uint64_t>(info.Size);
		}

		// Write the assetInfoRegistry offsets to the end of the file so we can access it
		writer.WriteRaw<uint64_t>(assetInfoRegistryOffset);

		uint32_t totalSerialized = 0;
		uint32_t totalRegistry = 0;
		size_t totalSize = 0;

		DY_CORE_TRACE("");
		DY_CORE_TRACE("Asset Pack Serialization Debug Info:");
		for (uint32_t type = (uint32_t)AssetType::Scene; type < (uint32_t)AssetType::ASSET_TYPE_SIZE; type++)
		{
			const auto& debugInfo = assetSerializationDebugInfo[type];
			DY_CORE_TRACE("    {: <25} {: ^15} ({})", AssetManager::AssetTypeToString((AssetType)type), fmt::format("{}/{}", debugInfo.TotalSerialized, debugInfo.TotalRegistry), String::FormatBytes(debugInfo.TotalSize));
			
			totalSerialized += debugInfo.TotalSerialized;
			totalRegistry += debugInfo.TotalRegistry;
			totalSize += debugInfo.TotalSize;
		}

		DY_CORE_TRACE("    {: <40}  ({})", "Core Module Assembly Image", String::FormatBytes(coreModuleSize));
		DY_CORE_TRACE("    {: <40}  ({})", "Script Module Assembly Image", String::FormatBytes(scriptModuleSize));

		DY_CORE_TRACE("");
		DY_CORE_TRACE("Asset Pack serialization finalized. Successfully serialized {}/{} assets with pack size {}.", totalSerialized, totalRegistry, String::FormatBytes(totalSize));
	}

	void AssetManager::DeserializeAssetPack(const std::filesystem::path& path)
	{
		DY_PROFILE_FUNCTION();

		Clear();

		DY_CORE_TRACE("Deserializing Asset Pack '{}'");

		if (!std::filesystem::exists(path))
		{
			DY_CORE_ERROR("Deserialization aborted! Asset Pack '{}' does not exist in filesystem.", path.string());
			return;
		}

		s_AssetPackReader = CreateRef<FileStreamReader>(path);
		s_AssetPackFilepath = path;

		// Validate the file header
		const char header[] = { 'D', 'Y', 'A', 'P' };
		char fileHeader[sizeof(header)];
		s_AssetPackReader->ReadData(fileHeader, sizeof(header));

		if (memcmp(fileHeader, header, sizeof(header)) != 0)
		{
			DY_CORE_ERROR("Invalid Asset Pack file header '{}'", path.string());
			return;
		}

		// Validate the Engine Version
		uint32_t engineVersionMajor, engineVersionMinor, engineVersionPatch;
		s_AssetPackReader->ReadRaw<uint32_t>(engineVersionMajor);
		s_AssetPackReader->ReadRaw<uint32_t>(engineVersionMinor);
		s_AssetPackReader->ReadRaw<uint32_t>(engineVersionPatch);

		if (engineVersionMajor != DY_VERSION_MAJOR || engineVersionMinor != DY_VERSION_MINOR || engineVersionPatch != DY_VERSION_PATCH)
		{
			DY_CORE_ERROR("Asset Pack was built with a different version of the engine. Expected {}.{}.{} but got {}.{}.{}",
			DY_VERSION_MAJOR, DY_VERSION_MINOR, DY_VERSION_PATCH, engineVersionMajor, engineVersionMinor, engineVersionPatch);
			return;
		}

		// Load the Asset Pack Version
		uint64_t packVersion;
		s_AssetPackReader->ReadRaw<uint64_t>(packVersion);
		
		DY_CORE_INFO("Pack Build Infomation:");
		DY_CORE_INFO("    Engine Version: {}.{}.{}", engineVersionMajor, engineVersionMinor, engineVersionPatch);
		DY_CORE_INFO("    Pack Version: {}", packVersion);

		// Load the metadata registry
		DY_CORE_TRACE("Deserializing metadata registry from Asset Pack");
		size_t metadataCount;
		s_AssetPackReader->ReadRaw<size_t>(metadataCount);

		for (size_t i = 0; i < metadataCount; i++)
		{
			UUID handle;
			s_AssetPackReader->ReadRaw<UUID>(handle);

			uint16_t assetType;
			s_AssetPackReader->ReadRaw<uint16_t>(assetType);

			std::string filePath;
			s_AssetPackReader->ReadString(filePath);

			DY_CORE_INFO("Deserializing asset metadata for {} from Asset Pack", handle);

			AssetMetadata metadata;
			metadata.Handle = handle;
			metadata.Type = (AssetType)assetType;
			metadata.FilePath = filePath;

			s_MetadataRegistry[handle] = metadata;

			// Update metadata filepath cache
			s_AssetFilepathHandleMap[metadata.FilePath] = metadata.Handle;
		}

		// Jump to the end of the file to read the asset info registry offset
		s_AssetPackReader->SetStreamPositionToEnd();
		s_AssetPackReader->SetStreamPosition(s_AssetPackReader->GetStreamPosition() - sizeof(uint64_t));
		
		uint64_t assetInfoRegistryOffset;
		s_AssetPackReader->ReadRaw<uint64_t>(assetInfoRegistryOffset);

		// Jump to the asset info registry offset
		s_AssetPackReader->SetStreamPosition(assetInfoRegistryOffset);
		
		// Read the asset info registry
		size_t assetInfoCount;
		s_AssetPackReader->ReadRaw<size_t>(assetInfoCount);

		DY_CORE_TRACE("Deserializing asset info registry from Asset Pack");

		for (size_t i = 0; i < assetInfoCount; i++)
		{
			UUID handle;
			s_AssetPackReader->ReadRaw<UUID>(handle);

			uint64_t offset;
			s_AssetPackReader->ReadRaw<uint64_t>(offset);

			uint64_t size;
			s_AssetPackReader->ReadRaw<uint64_t>(size);

			AssetPackFile::AssetInfo info;
			info.Handle = handle;
			info.Offset = offset;
			info.Size = size;

			s_AssetInfoRegistry[handle] = info;
		}

		DY_CORE_TRACE("Asset Pack deserialization finalized. Successfully deserialized {} assets.", assetInfoCount);
	}

	const std::filesystem::path AssetManager::GetAssetPackFilepath()
	{
		return s_AssetPackFilepath;
	}

	const bool AssetManager::UsingAssetPack()
	{
		return (bool)s_AssetPackReader;
	}

}