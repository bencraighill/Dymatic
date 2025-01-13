#include "FileManager.h"

#include "Dymatic/Core/Base.h"

#include "TextSymbols.h"

namespace Dymatic {

	FileType FileManager::GetFileType(const std::string& extension)
	{
		if (extension == ".dymatic")
			return FileType::FileTypeScene;
		if (extension == ".dyprefab")
			return FileType::FileTypePrefab;
		if (extension == ".cs")
			return FileType::FileTypeScript;
		if (extension == ".fbx" || extension == ".obj" || extension == ".gltf" || extension == ".dae" || extension == ".blend")
			return FileType::FileTypeMeshSource;
		if (extension == ".dymesh")
			return FileType::FileTypeMesh;
		if (extension == ".dymaterial")
			return FileType::FileTypeMaterial;
		if (extension == ".dymateriali")
			return FileType::FileTypeMaterialInstance;
		if (extension == ".png" || extension == ".jpg" || extension == ".jpeg" || extension == ".tga" || extension == ".bmp")
			return FileType::FileTypeTexture;
		if (extension == ".hdr")
			return FileType::FileTypeEnvironmentMap;
		if (extension == ".dyvirtex")
			return FileType::FileTypeVirtualTexture;
		if (extension == ".ttf" || extension == ".otf")
			return FileType::FileTypeFont;
		if (extension == ".wav" || extension == ".mp3")
			return FileType::FileTypeAudio;
		if (extension == ".dyparticles")
			return FileType::FileTypeParticleSystem;
		if (extension == ".dyskeleton")
			return FileType::FileTypeSkeleton;
		if (extension == ".dyanim")
			return FileType::FileTypeAnimation;
		if (extension == ".dyanimgraph")
			return FileType::FileTypeAnimationGraph;
		if (extension == ".dyphysmat")
			return FileType::FileTypePhysicsMaterial;
		if (extension == ".mp4" || extension == ".mkv" || extension == ".avi" || extension == ".mov" || extension == ".webm" || extension == ".wmv")
			return FileType::FileTypeVideo;
		if (extension == ".dymedplayer")
			return FileType::FileTypeVideoPlayer;
		if (extension == ".srt" || extension == ".ass")
			return FileType::FileTypeSubtitle;
		if (extension == ".zip")
			return FileType::FileTypeZipArchive;
		if (extension == ".sln")
			return FileType::FileTypeSolution;
		
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

	FileType FileManager::GetFileType(const AssetType type)
	{
		switch (type)
		{
		case AssetType::Scene: return FileType::FileTypeScene;
		case AssetType::Prefab: return FileType::FileTypePrefab;
		case AssetType::MeshSource: return FileType::FileTypeMeshSource;
		case AssetType::Mesh: return FileType::FileTypeMesh;
		case AssetType::Material: return FileType::FileTypeMaterial; // Warning: We cannot accurately distinguish between materials and material instances because of this!
		case AssetType::Texture: return FileType::FileTypeTexture;
		case AssetType::EnvironmentMap: return FileType::FileTypeEnvironmentMap;
		case AssetType::VirtualTexture: return FileType::FileTypeVirtualTexture;
		case AssetType::Font: return FileType::FileTypeFont;
		case AssetType::Audio: return FileType::FileTypeAudio;
		case AssetType::ParticleSystem: return FileType::FileTypeParticleSystem;
		case AssetType::Skeleton: return FileType::FileTypeSkeleton;
		case AssetType::Animation: return FileType::FileTypeAnimation;
		case AssetType::AnimationGraph: return FileType::FileTypeAnimationGraph;
		case AssetType::PhysicsMaterial: return FileType::FileTypePhysicsMaterial;
		case AssetType::Video: return FileType::FileTypeVideo;
		case AssetType::Subtitle: return FileType::FileTypeSubtitle;
		case AssetType::VideoPlayer: return FileType::FileTypeVideoPlayer;
		default: return FileType::FileTypeFile;
		}
	}

	AssetType FileManager::GetAssetType(const FileType type)
	{
		switch (type)
		{
		case FileType::FileTypeScene: return AssetType::Scene;
		case FileType::FileTypePrefab: return AssetType::Prefab;
		case FileType::FileTypeMeshSource: return AssetType::MeshSource;
		case FileType::FileTypeMesh: return AssetType::Mesh;
		case FileType::FileTypeMaterial: return AssetType::Material;
		case FileType::FileTypeMaterialInstance: return AssetType::Material;
		case FileType::FileTypeTexture: return AssetType::Texture;
		case FileType::FileTypeEnvironmentMap: return AssetType::EnvironmentMap;
		case FileType::FileTypeVirtualTexture: return AssetType::VirtualTexture;
		case FileType::FileTypeFont: return AssetType::Font;
		case FileType::FileTypeAudio: return AssetType::Audio;
		case FileType::FileTypeParticleSystem: return AssetType::ParticleSystem;
		case FileType::FileTypeSkeleton: return AssetType::Skeleton;
		case FileType::FileTypeAnimation: return AssetType::Animation;
		case FileType::FileTypeAnimationGraph: return AssetType::AnimationGraph;
		case FileType::FileTypePhysicsMaterial: return AssetType::PhysicsMaterial;
		case FileType::FileTypeVideo: return AssetType::Video;
		case FileType::FileTypeSubtitle: return AssetType::Subtitle;
		case FileType::FileTypeVideoPlayer: return AssetType::VideoPlayer;
		default: return AssetType::None;
		}
	}

	AssetType FileManager::GetAssetType(const std::string& extension)
	{
		return GetAssetType(FileManager::GetFileType(extension));
	}

	const char* FileManager::GetFileTypeCharacterIcon(const FileType type)
	{
		switch (type)
		{
		case FileTypeFile: return FILE_ICON_FILE;
		case FileTypeDirectory: return FILE_ICON_DIRECTORY;
		case FileTypeScene: return FILE_ICON_SCENE;
		case FileTypePrefab: return FILE_ICON_PREFAB;
		case FileTypeScript: return FILE_ICON_SCRIPT;
		case FileTypeMeshSource: return FILE_ICON_MESH_SOURCE;
		case FileTypeMesh: return FILE_ICON_MESH;
		case FileTypeMaterial: return FILE_ICON_MATERIAL;
		case FileTypeMaterialInstance: return FILE_ICON_MATERIAL_INSTANCE;
		case FileTypeTexture: return FILE_ICON_TEXTURE;
		case FileTypeEnvironmentMap: return FILE_ICON_ENVIRONMENT_MAP;
		case FileTypeVirtualTexture: return FILE_ICON_VIRTUAL_TEXTURE;
		case FileTypeFont: return FILE_ICON_FONT;
		case FileTypeAudio: return FILE_ICON_AUDIO;
		case FileTypeParticleSystem: return FILE_ICON_PARTICLE_SYSTEM;
		case FileTypeSkeleton: return FILE_ICON_SKELETON;
		case FileTypeAnimation: return FILE_ICON_ANIMATION;
		case FileTypeAnimationGraph: return FILE_ICON_ANIMATION_GRAPH;
		case FileTypePhysicsMaterial: return FILE_ICON_PHYSICS_MATERIAL;
		case FileTypeVideo: return FILE_ICON_VIDEO;
		case FileTypeVideoPlayer: return FILE_ICON_VIDEO_PLAYER;
		case FileTypeSubtitle: return FILE_ICON_SUBTITLE;
		case FileTypeZipArchive: return FILE_ICON_ZIP_ARCHIVE;
		case FileTypeSolution: return FILE_ICON_VS_SOLUTION;
		default: return FILE_ICON_FILE;
		}
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