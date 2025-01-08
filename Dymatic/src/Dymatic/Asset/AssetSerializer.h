#pragma once

#include "Dymatic/Core/Base.h"
#include "Dymatic/Asset/AssetMetadata.h"
#include "Dymatic/Core/FileStream.h"

namespace Dymatic {

	struct AssetSerializationInfo
	{
		// TODO: This is a placeholder.
		uint32_t DataSize;
	};

	class AssetPackFile
	{
	public:
		struct AssetInfo
		{
			AssetHandle Handle;
			uint32_t Offset;
			uint32_t Size;
		};
	};

	class AssetSerializer
	{
	public:
		virtual void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const = 0;
		virtual bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset, bool multithreaded) const = 0;

		virtual bool SerializeToAssetPack(const AssetMetadata& metadata, const AssetHandle handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo) const = 0;
		virtual bool DeserializeFromAssetPack(const AssetMetadata& metadata, Ref<Asset>& asset, FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo) const = 0;
	};

}