#pragma once

#include "Dymatic/Asset/AssetHandle.h"

namespace Dymatic {

	enum class EngineAsset : AssetHandle
	{
		// Note: Handle 0 is for null resources in the manager
		None = 0,

		DefaultFont,

		DefaultMaterial,
		DefaultMaterialMainMap,
		DefaultMaterialNormalMap,
	};

}