#pragma once

#include "Dymatic/Core/Base.h"
#include "Dymatic/Asset/AssetMetadata.h"

namespace Dymatic {

	class AssetSerializer
	{
	public:
		virtual void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const = 0;
		virtual bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const = 0;
	};

}