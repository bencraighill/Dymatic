#pragma once

#include "Dymatic/Asset/AssetManager.h"
#include "Dymatic/Asset/AssetSerializer.h"
#include "Dymatic/Video/Subtitle.h"

#include "Dymatic/Core/Filesystem.h"

namespace Dymatic {

	class SubtitleSerializer : public AssetSerializer
	{
	public:
		virtual void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const override {}

		virtual bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset, bool multithreaded) const override
		{
			const std::filesystem::path filepath = AssetManager::GetFileSystemPathString(metadata.Handle);
			asset = CreateRef<Subtitle>(filepath);

			return true;
		}

		virtual bool SerializeToAssetPack(const AssetMetadata& metadata, const AssetHandle handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo) const override
		{
			Buffer fileData = FileSystem::ReadBytes(AssetManager::GetFileSystemPathString(metadata));
			stream.WriteData(fileData.As<const char>(), fileData.Size);
			fileData.Release();
			return true;
		}

		virtual bool DeserializeFromAssetPack(const AssetMetadata& metadata, Ref<Asset>& asset, FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo) const override
		{
			asset = CreateRef<Subtitle>(AssetManager::GetAssetPackFilepath(), assetInfo.Offset, assetInfo.Size);
			return true;
		}
	};

}