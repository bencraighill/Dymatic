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
			FileEntry(const std::filesystem::path& path, bool isDirectory = false, size_t size = 0, const std::string& lastWriteTime = std::string())
				: Path(path), IsDirectory(isDirectory), Size(size), LastWriteTime(lastWriteTime)
			{
				Type = isDirectory ? FileType::FileTypeDirectory : FileManager::GetFileType(path);
			}
			
			FileType Type;
			std::filesystem::path Path;
			bool IsDirectory;
			
			size_t Size;
			std::string LastWriteTime;
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

		void OnUpdate();
		void OnImGuiRender(bool m_IsDragging);

		void MoveToDirectory(const std::filesystem::path& path);

		void OnExternalFileDrop(std::vector<std::string> filepaths);

		// Actions
		void OpenExternally(const std::filesystem::path& path);
		bool DuplicateFile(const std::filesystem::path& file);
		void CopyFile(const std::filesystem::path& file);
		void PasteFile();
		void RenameFile(const std::filesystem::path& path);
		void Focus();
		
	private:
		// Drawing
		void DrawGridLayout();
		void DrawListLayout();
		void DrawLayoutItemContextMenu(const FileEntry& file);
		void DrawItemTooltipContents(const FileEntry& file, bool isDragging = false);
		void DrawCreateMenu();
		void DrawRenameInput(const std::filesystem::path& path, const bool expand = false);

		template<typename T, typename ... Args>
		void DrawCreateMenuItem(const char* label, const std::string& filename, Args&& ... args);

		template<typename T, typename ... Args>
		Ref<T> CreateNewAsset(const std::string& filename, Args&& ... args);

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
		void SetSelectionContext(const FileEntry& entry, bool additive = false);
		void ToggleSelectionContext(const FileEntry& entry);
		void ClearSelectionContext();
		bool IsFileSelected(const std::filesystem::path& path);
		void InvertSelection();
		inline const bool IsDragSelecting() const { return m_SelectionPosition.x != -1; }

		void ImportExternalFiles();

		// Internal Action
		void CreateFolder();
		void CreateEmptyFile(const std::filesystem::path& defaultFilename);
		bool DeleteFile(const std::filesystem::path& path, bool reload = true);
		void RenameFile(const std::filesystem::path& oldPath, const std::filesystem::path& newPath);
		bool MoveFileToDirectory(const std::filesystem::path& file, const std::filesystem::path& dir, bool reload = true);
		bool CopyFileToDirectory(const std::filesystem::path& file, const std::filesystem::path& dir, bool moving = false);
		
		uint32_t GetNumberOfFoldersInDirectory(const std::filesystem::path& directory);

		void CheckFileAssetData(const std::filesystem::path& path);

		// File Info
		static Ref<Texture2D> GetFileTypeIcon(FileType type);
		static const char* FileTypeToUpperString(FileType type);
		static const char* FileTypeToString(FileType type);

	private:
		bool m_Init = false;

		std::filesystem::path m_BaseDirectory;
		std::filesystem::path m_CurrentDirectory;

		bool m_InContentBounds = false;
		bool m_ScrollToTop = false;
		bool m_Focus = false;

		std::vector<FileEntry> m_DisplayFiles;
		std::vector<std::string> m_DirectorySplit;
		DirectoryEntry m_DisplayDirectories;

		// TODO: Would an unordered_set of std::filesystem::path make more sense here?
		std::vector<FileEntry> m_SelectionContext;

		bool m_StartRename = false;
		std::filesystem::path m_RenameContext;

		std::vector<std::filesystem::path> m_DirectoryHistory;
		uint32_t m_DirectoryHistoryIndex = 0;

		std::filesystem::path m_DirectoryViewDropdownPath;

		// Context Menu Codes
		uint32_t m_ContextIndex;
		std::filesystem::path m_ContextPath;

		std::string m_SearchbarBuffer;

		glm::vec2 m_SelectionPosition { -1.0f, -1.0f };
		
		std::function<void(const std::filesystem::path&)> m_OpenFileEditorCallback;
	};

}