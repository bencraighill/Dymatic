#pragma once
#include "Dymatic/Asset/AssetSerializer.h"
#include "Dymatic/Renderer/Texture.h"

namespace Dymatic {

	class TextureSerializer : public AssetSerializer
	{
	public:
		virtual void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const override
		{
			return;
		}

		virtual bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const override
		{
			asset = Texture2D::Create(AssetManager::GetFileSystemPathString(metadata));
			asset->Handle = metadata.Handle;

			bool result = As<Texture2D>(asset)->IsLoaded();

			//if (!result)
			//	asset->SetFlag(AssetFlag::Invalid, true);

			return result;
		}
	};

}