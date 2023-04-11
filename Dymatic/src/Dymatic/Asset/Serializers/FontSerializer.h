#pragma once

#include "Dymatic/Asset/AssetSerializer.h"
#include "Dymatic/Renderer/Font.h"

namespace Dymatic {

	class FontSerializer : public AssetSerializer
	{
	public:
		virtual void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const override
		{
			return;
		}

		virtual bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const override
		{
			asset = Font::Create(AssetManager::GetFileSystemPathString(metadata));
			asset->Handle = metadata.Handle;

			return true;
		}
	};

}