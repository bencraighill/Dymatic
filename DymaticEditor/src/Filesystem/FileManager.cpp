#include "FileManager.h"

#include "Dymatic/Core/Base.h"


namespace Dymatic {

	FileType FileManager::GetFileType(const std::string& extension)
	{
		if (extension == ".dymatic")
			return FileType::FileTypeScene;
		if (extension == ".fbx" || extension == ".obj" || extension == ".gltf" || extension == ".dae" || extension == ".blend")
			return FileType::FileTypeMesh;
		if (extension == ".dymaterial")
			return FileType::FileTypeMaterial;
		if (extension == ".png" || extension == ".jpg" || extension == ".jpeg" || extension == ".tga" || extension == ".bmp")
			return FileType::FileTypeTexture;
		if (extension == ".ttf")
			return FileType::FileTypeFont;
		if (extension == ".wav" || extension == ".mp3")
			return FileType::FileTypeAudio;
		return FileType::FileTypeFile;
	}

	FileType FileManager::GetFileType(const std::filesystem::path& path)
	{
		if (std::filesystem::exists(path))
			if (std::filesystem::is_directory(path))
				return FileType::FileTypeDirectory;

		auto& extension = path.extension().string();

		return GetFileType(extension);
	}

	AssetType FileManager::GetAssetType(const FileType& type)
	{
		switch (type)
		{
		case FileType::FileTypeScene: return AssetType::Scene;
		case FileType::FileTypeMesh: return AssetType::Mesh;
		case FileType::FileTypeMaterial: return AssetType::Material;
		case FileType::FileTypeTexture: return AssetType::Texture;
		case FileType::FileTypeFont: return AssetType::Font;
		case FileType::FileTypeAudio: return AssetType::Audio;
		default: return AssetType::None;
		}
	}

	AssetType FileManager::GetAssetType(const std::string& extension)
	{
		return GetAssetType(FileManager::GetFileType(extension));
	}

	bool FileManager::Equivalent(const std::filesystem::path& a, const std::filesystem::path& b)
	{
		return _stricmp(a.generic_string().c_str(), b.generic_string().c_str()) == 0;
	}

	bool FileManager::IsFilenameValid(const std::filesystem::path& filename)
	{
		return filename.string().find_first_of(":*?\"<>|/\\") == std::string::npos;
	}

	std::filesystem::path FileManager::GetNextOfNameInDirectory(std::filesystem::path filename, const std::filesystem::path& directory, const std::filesystem::path& omitted)
	{
		filename = filename.filename();

		std::string nameString = filename.stem().string();
		std::string rawName = nameString;
		uint32_t last_index = nameString.find_last_not_of("0123456789");
		if (last_index != std::string::npos && last_index != nameString.size() - 1)
			rawName = nameString.substr(0, last_index);

		std::filesystem::path currentName = filename; // Confirm that the current path is not available first
		uint32_t index = 0;

		bool found = true;
		while (found)
		{
			found = false;
			
			for (auto& entry : std::filesystem::directory_iterator(directory)) 
			{
				std::filesystem::path entryFilename = entry.path().filename();
				if (Equivalent(currentName, entryFilename) && (omitted.empty() || (!Equivalent(entryFilename, omitted))))
				{
					found = true;
					break;
				}
			}
			
			if (found)
			{
				index++;
				currentName = fmt::format("{}_{}{}", rawName, index, filename.extension().string());
			}
		}
		return currentName;
	}

}