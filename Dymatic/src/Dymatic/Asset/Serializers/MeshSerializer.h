#pragma once

#include "Dymatic/Asset/AssetSerializer.h"
#include "Dymatic/Renderer/Model.h"

namespace Dymatic {
	
	class MeshSerializer : public AssetSerializer
	{
	public:
		virtual void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const override
		{
			return;
		}

		virtual bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const override
		{
			asset = Model::Create(AssetManager::GetFileSystemPathString(metadata));
			asset->Handle = metadata.Handle;

			bool result = As<Model>(asset)->IsLoaded();

			//if (!result)
			//	asset->SetFlag(AssetFlag::Invalid, true);

			return result;
		}
	};

}