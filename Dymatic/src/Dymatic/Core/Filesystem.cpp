#include "dypch.h"
#include "FileSystem.h"

#include "Dymatic/Core/FileStream.h"

namespace Dymatic {

	bool FileSystem::CreateDirectory(const std::filesystem::path& directory)
	{
		if (!std::filesystem::exists(directory))
			std::filesystem::create_directories(directory);

		return true;
	}

	bool FileSystem::Exists(const std::filesystem::path& filepath)
	{
		return std::filesystem::exists(filepath);
	}

	bool FileSystem::DeleteFile(const std::filesystem::path& filepath)
	{
		return std::filesystem::remove(filepath);
	}

	bool FileSystem::MoveFile(const std::filesystem::path& filepath, const std::filesystem::path& destination)
	{
		return false;
	}

	bool FileSystem::CopyFile(const std::filesystem::path& filepath, const std::filesystem::path& destination)
	{
		return false;
	}

	bool FileSystem::IsDirectory(const std::filesystem::path& directory)
	{
		return false;
	}

	size_t FileSystem::GetFileSize(const std::filesystem::path& filepath)
	{
		return std::filesystem::file_size(filepath);
	}

	template <typename TP>
	static std::time_t to_time_t(TP tp)
	{
		using namespace std::chrono;
		auto sctp = time_point_cast<system_clock::duration>(tp - TP::clock::now()
			+ system_clock::now());
		return system_clock::to_time_t(sctp);
	}

	std::time_t FileSystem::GetLastWriteTime(const std::filesystem::path& filepath)
	{
		return to_time_t(std::filesystem::last_write_time(filepath));
	}

	bool FileSystem::IsNewer(const std::filesystem::path& fileA, const std::filesystem::path& fileB)
	{
		return false;
	}

	bool FileSystem::Rename(const std::filesystem::path& oldFilepath, const std::filesystem::path& newFilepath)
	{
		return false;
	}

	bool FileSystem::RenameFilename(const std::filesystem::path& filepath, const std::string& name)
	{
		return false;
	}

	bool FileSystem::ShowFileInExplorer(const std::filesystem::path& path)
	{
		return false;
	}

	bool FileSystem::OpenDirectoryInExplorer(const std::filesystem::path& path)
	{
		return false;
	}

	bool FileSystem::OpenExternally(const std::filesystem::path& path)
	{
		return false;
	}

	bool FileSystem::WriteBytes(const std::filesystem::path& filepath, const Buffer& buffer)
	{
		FileStreamWriter stream(filepath);
		stream.WriteData(buffer.As<const char>(), buffer.Size);

		return true;
	}

	void FileSystem::ReadBytes(const std::filesystem::path& filepath, Buffer& buffer)
	{
		std::ifstream stream(filepath, std::ios::binary | std::ios::ate);

		if (!stream)
		{
			// Failed to open the file
			return;
		}


		std::streampos end = stream.tellg();
		stream.seekg(0, std::ios::beg);
		uint64_t size = end - stream.tellg();

		if (size == 0)
		{
			// File is empty
			return;
		}

		buffer.Allocate(size);
		stream.read(buffer.As<char>(), size);
		stream.close();
	}

	Buffer FileSystem::ReadBytes(const std::filesystem::path& filepath)
	{
		Buffer buffer;
		ReadBytes(filepath, buffer);
		return buffer;
	}

	std::filesystem::path FileSystem::GetUniqueFileName(const std::filesystem::path& filepath)
	{
		return "";
	}

	std::filesystem::path FileSystem::OpenFileDialog()
	{
		return false;
	}

	std::filesystem::path FileSystem::OpenFolderDialog()
	{
		return "";
	}

	std::filesystem::path FileSystem::SaveFileDialog()
	{
		return "";
	}

	std::filesystem::path FileSystem::GetPersistentStoragePath()
	{
		return "";
	}

}