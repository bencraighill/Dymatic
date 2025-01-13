#pragma once

#include "Dymatic/Core/Log.h"
#include "Dymatic/Asset/AssetManager.h"

#include <yaml-cpp/yaml.h>

namespace Dymatic::Utils {

	static bool TryLoadYAMLFromFile(const AssetMetadata& metadata, YAML::Node& data)
	{
		const std::string filepath = AssetManager::GetFileSystemPathString(metadata);
		if (!std::filesystem::exists(filepath))
		{
			DY_CORE_ERROR("File '{}' does not exist", metadata.FilePath.string());
			return false;
		}

		try
		{
			data = YAML::LoadFile(filepath);
		}
		catch (YAML::ParserException e)
		{
			DY_CORE_ERROR("Failed to load file '{}'\n     {}", metadata.FilePath.string(), e.what());
			return false;
		}

		return true;
	}

}