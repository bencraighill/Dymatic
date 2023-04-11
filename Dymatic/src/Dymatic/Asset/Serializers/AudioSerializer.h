#pragma once

#include "Dymatic/Asset/AssetSerializer.h"
#include "Dymatic/Audio/Audio.h"

namespace Dymatic {

	class AudioSerializer : public AssetSerializer
	{
	public:
		virtual void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const override
		{
			return;
		}

		virtual bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const override
		{
			asset = Audio::Create(AssetManager::GetFileSystemPathString(metadata));
			asset->Handle = metadata.Handle;

			return true;
		}
	};

}