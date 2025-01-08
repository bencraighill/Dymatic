#pragma once

#include "Dymatic/Asset/Asset.h"
#include "Dymatic/Asset/EngineAsset.h"

#include <unordered_map>
#include <filesystem>

namespace Dymatic {

	struct EngineAssetMetadata
	{
		AssetType Type;
		std::filesystem::path Filepath;
	};

	static const std::unordered_map<EngineAsset, EngineAssetMetadata> s_EngineAssets =
	{
		{ EngineAsset::DefaultFont, { AssetType::Font, "Resources/Fonts/OpenSans-Regular.ttf" }},
		{ EngineAsset::DefaultMaterialMainMap, { AssetType::Texture, "Resources/Textures/DefaultMaterial/DefaultMaterialGrid.png" }},
		{ EngineAsset::DefaultMaterialNormalMap, { AssetType::Texture, "Resources/Textures/DefaultMaterial/DefaultMaterialGridNormal.png" }},
	};

}