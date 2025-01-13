#pragma once
#include "Dymatic/Asset/AssetSerializer.h"
#include "Dymatic/Renderer/EnvironmentMap.h"

namespace Dymatic {

	class EnvironmentMapSerializer : public AssetSerializer
	{
	public:
		virtual void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const override {}

		virtual bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset, bool multithreaded) const override
		{
			// Create the environment map
			asset = EnvironmentMap::Create(AssetManager::GetFileSystemPathString(metadata));

			// Verify the texture was loaded successfully
			const bool result = As<EnvironmentMap>(asset)->IsLoaded();
			return result;
		}

		virtual bool SerializeToAssetPack(const AssetMetadata& metadata, const AssetHandle handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo) const override
		{
			// Locate the asset file
			const std::string& filepath = AssetManager::GetFileSystemPathString(metadata);

			// Locate the asset file and write the raw file data to the asset pack
			Buffer fileData = FileSystem::ReadBytes(AssetManager::GetFileSystemPathString(metadata));
			stream.WriteBuffer(fileData);
			fileData.Release();

			return true;
		}

		virtual bool DeserializeFromAssetPack(const AssetMetadata& metadata, Ref<Asset>& asset, FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo) const override
		{
			ScopedBuffer fileData;
			stream.ReadBuffer(fileData);

			// Note: Here we use the FILE data constructor (so stbi still does decompression at runtime directly in memory)
			asset = EnvironmentMap::Create(fileData);

			// Verify the texture was loaded successfully
			const bool result = As<EnvironmentMap>(asset)->IsLoaded();
			return result;
		}
	};

}