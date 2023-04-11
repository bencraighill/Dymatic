#pragma once

#include "Filesystem/FileType.h"
#include "Dymatic/Asset/Asset.h"

#include <filesystem>

namespace Dymatic {

	class FileManager
	{
	public:
		static FileType GetFileType(const std::string& extension);
		static FileType GetFileType(const std::filesystem::path& path);
		static AssetType GetAssetType(const FileType& type);
		static AssetType GetAssetType(const std::string& extension);

		static bool Equivalent(const std::filesystem::path& a, const std::filesystem::path& b);

		static bool IsFilenameValid(const std::filesystem::path& filename);
		static std::filesystem::path GetNextOfNameInDirectory(std::filesystem::path filename, const std::filesystem::path& directory, const std::filesystem::path& omitted = "");
	};
	
}