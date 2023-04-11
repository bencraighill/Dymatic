#pragma once
#include "Dymatic/Core/UUID.h"
#include "Dymatic/Asset/Asset.h"

namespace Dymatic {

	enum class AssetFlag : uint16_t
	{
		None = 0,
		Invalid = 1
	};

	struct AssetMetadata
	{
		AssetMetadata() = default;
		AssetMetadata(AssetType type, const std::filesystem::path& filePath)
			: Type(type), FilePath(filePath)
		{}

		UUID Handle;
		AssetType Type;
		std::filesystem::path FilePath;
	};

}