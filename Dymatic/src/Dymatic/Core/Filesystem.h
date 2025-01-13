#pragma once

#include "Dymatic/Core/Buffer.h"

namespace Dymatic {

	class FileSystem
	{
	public:
		static bool CreateDirectory(const std::filesystem::path& directory);
		static bool Exists(const std::filesystem::path& filepath);
		static bool DeleteFile(const std::filesystem::path& filepath);
		static bool MoveFile(const std::filesystem::path& filepath, const std::filesystem::path& destination);
		static bool CopyFile(const std::filesystem::path& filepath, const std::filesystem::path& destination);
		static bool IsDirectory(const std::filesystem::path& directory);
		
		static size_t GetFileSize(const std::filesystem::path& filepath);
		static std::time_t GetLastWriteTime(const std::filesystem::path& filepath);

		static bool IsNewer(const std::filesystem::path& fileA, const std::filesystem::path& fileB);
		
		static bool Rename(const std::filesystem::path& oldFilepath, const std::filesystem::path& newFilepath);
		static bool RenameFilename(const std::filesystem::path& filepath, const std::string& name);
		
		static bool ShowFileInExplorer(const std::filesystem::path& path);
		static bool OpenDirectoryInExplorer(const std::filesystem::path& path);
		static bool OpenExternally(const std::filesystem::path& path);

		static bool WriteBytes(const std::filesystem::path& filepath, const Buffer& buffer);
		static void ReadBytes(const std::filesystem::path& filepath, Buffer& buffer);
		static Buffer ReadBytes(const std::filesystem::path& filepath);
		
		static std::filesystem::path GetUniqueFileName(const std::filesystem::path& filepath);
		
	public:
		struct FileDialogFilterItem
		{
			const char* Name;
			const char* Spec;
		};

		static std::filesystem::path OpenFileDialog();
		static std::filesystem::path OpenFolderDialog();
		static std::filesystem::path SaveFileDialog();

		static std::filesystem::path GetPersistentStoragePath();
	};

}