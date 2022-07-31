#pragma once

#include <stack>
#include <filesystem>

#include "Dymatic/Renderer/Texture.h"

#include "PopupsAndNotifications.h"

namespace Dymatic {

	class ContentBrowserPanel
	{
	public:
		enum FileType
		{
			Directory = 0,
			File,
			SceneAsset,
			MeshAsset,
			TextureAsset,
			FontAsset
		};

		struct FileEntry
		{
			FileEntry(const std::filesystem::path& path, bool isDirectory = false, Ref<Texture2D> texture = nullptr);

			Ref<Texture2D> Texture = nullptr;
			FileType Type;

			std::filesystem::path Path;
			bool IsDirectory;
		};

		struct DirectoryEntry
		{
			std::filesystem::path Path;
			std::vector<DirectoryEntry> SubDirectories;
		};

	public:

		ContentBrowserPanel();

		bool& GetContentBrowserVisible() { return m_ContentBrowserVisible; }

		void OnImGuiRender(bool m_IsDragging);

		void MoveToDirectory(const std::filesystem::path& path);

		void OnExternalFileDrop(std::vector<std::string> filepaths);

		static FileType GetFileType(const std::filesystem::path& path);
	private:
		// Updating Directory
		void AddDirectoryHistory(const std::filesystem::path& path);
		void NavForwardDirectory();
		void NavBackDirectory();
		void SetDirectoryInternal(const std::filesystem::path& path);

		// Updating File View
		void UpdateDisplayFiles();
		void InsertDirectoryEntry(const std::filesystem::directory_entry& directoryEntry, bool searching);

		// Display Directory View
		void UpdateDisplayDirectories();
		void UpdateDisplayDirectories(DirectoryEntry& directory);
		void DrawDirectoryView(DirectoryEntry& directory, std::filesystem::path& path);

		// Selection
		void SetSelectionContext(FileEntry& entry, bool additive = false);
		void ToggleSelectionContext(FileEntry& entry);
		void ClearSelectionContext();
		bool IsFileSelected(const std::filesystem::path& path);
		void InvertSelection();

		// Actions
		void OpenExternally(const std::filesystem::path& path);
		bool DuplicateFile(const std::filesystem::path& file);
		void CopyFile(const std::filesystem::path& file);
		void PasteFile();

		void ImportExternalFiles();

		// Internal Action
		void CreateFolder();
		bool DeleteFile(const std::filesystem::path& path, bool reload = true);
		bool MoveFileToDirectory(const std::filesystem::path& file, const std::filesystem::path& dir, bool reload = true);
		bool CopyFileToDirectory(const std::filesystem::path& file, const std::filesystem::path& dir, bool reload = true);

		// Calculations
		size_t GetNumberOfFoldersInDirectory(const std::filesystem::path& dir);
		std::filesystem::path GetNextOfNameInDirectory(std::filesystem::path name, const std::filesystem::path& dir);

		// File Info
		static Ref<Texture2D> GetFileIcon(FileType type);
		static std::string FileTypeToString(FileType type);
	private:
		bool m_ContentBrowserVisible = true;

		std::filesystem::path m_CurrentDirectory;

		bool m_InContentBounds = false;
		bool m_ScrollToTop = false;

		std::vector<FileEntry> m_DisplayFiles;
		DirectoryEntry m_DisplayDirectories;

		std::vector<FileEntry> m_SelectionContext;

		std::vector<std::filesystem::path> m_DirectoryHistory;
		uint32_t m_DirectoryHistoryIndex = 0;

		std::filesystem::path m_DirectoryViewDropdownPath;

		std::string m_SearchbarBuffer;

		glm::vec2 m_SelectionPos { -1.0f, -1.0f };
	};

}