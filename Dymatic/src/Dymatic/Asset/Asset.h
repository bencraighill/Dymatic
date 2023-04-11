#pragma once
#include "Dymatic/Core/UUID.h"

namespace Dymatic {

	enum class AssetType : uint16_t
	{
		None = 0,
		Scene = 1,
		Mesh = 2,
		Material = 3,
		Texture = 4,
		Font = 5,
		Audio = 6
	};

	class Asset
	{
	public:
		UUID Handle;
		virtual AssetType GetAssetType() const = 0;
	};
	
}