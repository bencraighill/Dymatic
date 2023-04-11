#pragma once

#include <stack>
#include <filesystem>

#include "Dymatic/Renderer/Texture.h"

#include "PopupsAndNotifications.h"

#include "Filesystem/FileManager.h"

namespace Dymatic {

	class ContentBrowserPanel
	{
	public:
		struct FileEntry
		{
			FileEntry(const std::filesystem::path& path, const std::filesystem::path& base, bool isDirectory = false, Ref<Texture2D> texture = nullptr);

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

		void Init();
		void SetOpenFileCallback(const std::function<void(const std::filesystem::path&)>& callback) { m_OpenFileEditorCallback = callback; }

		void OnImGuiRender(bool m_IsDragging);

		void MoveToDirectory(const std::filesystem::path& path);

		void OnExternalFileDrop(std::vector<std::string> filepaths);

		// Actions
		void OpenExternally(const std::filesystem::path& path);
		bool DuplicateFile(const std::filesystem::path& file);
		void CopyFile(const std::filesystem::path& file);
		void PasteFile();
		void RenameFile(const std::filesystem::path& path);
		
	private:
		// Updating Directory
		void AddDirectoryHistory(const std::filesystem::path& path);
		void NavForwardDirectory();
		void NavBackDirectory();
		void SetDirectoryInternal(const std::filesystem::path& path);

		// Updating File View
		void UpdateDisplayFiles();
		void InsertDirectoryEntry(const std::filesystem::directory_entry& directoryEntry, bool searching);
		void UpdateDirectorySplit();

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

		void ImportExternalFiles();

		// Internal Action
		void CreateFolder();
		bool DeleteFile(const std::filesystem::path& path, bool reload = true);
		void RenameFile(const std::filesystem::path& oldPath, const std::filesystem::path& newPath);
		bool MoveFileToDirectory(const std::filesystem::path& file, const std::filesystem::path& dir, bool reload = true);
		bool CopyFileToDirectory(const std::filesystem::path& file, const std::filesystem::path& dir, bool moving = false);
		
		uint32_t GetNumberOfFoldersInDirectory(const std::filesystem::path& directory);

		void DrawCreateMenu();
		void DrawRenameInput(const std::filesystem::path& path);

		void CheckFileAssetData(const std::filesystem::path& path);

		// File Info
		static Ref<Texture2D> GetFileIcon(FileType type);
		static std::string FileTypeToString(FileType type);

	private:
		bool m_Init = false;

		std::filesystem::path m_BaseDirectory;
		std::filesystem::path m_CurrentDirectory;

		bool m_InContentBounds = false;
		bool m_ScrollToTop = false;

		std::vector<FileEntry> m_DisplayFiles;
		std::vector<std::string> m_DirectorySplit;
		DirectoryEntry m_DisplayDirectories;

		std::vector<FileEntry> m_SelectionContext;

		bool m_StartRename = false;
		std::filesystem::path m_RenameContext;

		std::vector<std::filesystem::path> m_DirectoryHistory;
		uint32_t m_DirectoryHistoryIndex = 0;

		std::filesystem::path m_DirectoryViewDropdownPath;

		std::string m_SearchbarBuffer;

		glm::vec2 m_SelectionPos { -1.0f, -1.0f };
		
		std::function<void(const std::filesystem::path&)> m_OpenFileEditorCallback;
	};

}