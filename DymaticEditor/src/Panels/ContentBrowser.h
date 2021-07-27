#pragma once

#include "Dymatic.h"
#include "Dymatic/Core/Base.h"

#include "../Preferences.h"
#include "TextEditor.h"

namespace Dymatic {

	enum FileCatagory
	{
		Folder = 0,
		File = 1
	};

	struct FileProperties
	{
		std::string filename;
		FileCatagory fileCatagory;
	};

	class ContentBrowser
	{
	public:
		ContentBrowser(Preferences* preferencesRef, TextEditorPannel* textEditorPannelRef);

		void OnImGuiRender(Timestep ts);

		void GeneratePathList(std::string path);

		bool DoesFileExist(std::string filename);

		void RenameFile(FileProperties fileProperties, std::string filename);

		void SetFiles(std::vector<FileProperties> filesToSet);
		
		void UpdateFileView();

		void SystemCommand(std::string _Command);

		void OpenFileBrowsed(FileProperties fileProperties);
		void DeleteFileBrowsed(std::string filepath, FileCatagory fileCatagory);
		void CopyFileToDirectory(std::string inputFilepath, std::string outputFilepath, FileCatagory fileCatagory);

		void OpenRenamePopup(FileProperties fileProperties);

		int GetNumberOfFiles(std::vector<FileProperties> filesIterator);
		int GetNumberOfFolders(std::vector<FileProperties> filesIterator);

		std::string SwapStringSlashesSingle(std::string swapString);
		std::string SwapStringSlashesDouble(std::string swapString);

		std::string ToLower(std::string inString);

		std::string FindNextNumericalName(std::string filename);

		std::string GetFullFileNameFromPath(std::string filepath);

		std::string GetFileName(std::string filename);
		std::string GetFileFormat(std::string filename);

		std::string GetBrowseDirectory() { return m_BrowsePath; }
		void SetBrowseDirectory(std::string path);
		std::vector<FileProperties> GetFilesAtDirectory(std::string filepath);

		std::vector<std::string> GetRecords(std::string record_dir_path, FileCatagory fileCatagory);

		std::string& GetFileToOpen() { return m_FileToOpen; }
		FileProperties GetSelectedFile() { return m_SelectedFile; }

		std::vector<FileProperties> GetFilesDisplayed() { return filesDisplayed; }

		std::string GetSearchBarBuffer() { return m_SearchBarBuffer; }
		void SetSearchBarBuffer(std::string buffer) { m_SearchBarBuffer = buffer; }

		bool& GetContentBrowserVisible() { return m_ContentBrowserVisible; }

	private:
		bool m_ContentBrowserVisible = true;

		Preferences* m_PreferencesReference;
		TextEditorPannel* m_TextEditorPannelReference;

		std::string m_RootPath = "C:/dev/ExperimentalContentFolder";
		std::string m_BrowsePath;
		std::vector<FileProperties> filesDisplayed;
		std::vector<FileProperties> filesAtDirectory;
		FileProperties m_SelectedFile;

		FileProperties m_CopyContext;
		std::string m_CopyBrowsePathContext;

		FileProperties m_RenamePopupContext;
		std::string m_RenameBuffer = "";
		bool m_OpenRenamePopup = false;

		std::string m_SearchBarBuffer = "";
		std::string m_DirectoryViewDropdownPath;

		std::string m_FileToOpen;

		float m_TimeSinceFilePressed = 0.0f;

		bool m_ScrollToTop = false;

		bool m_PathsWindowOpen = false;
		float variation1 = 0.0f;
		float variation2 = 0.0f;

		Ref<Texture2D> m_IconRightArrow = Texture2D::Create("assets/icons/ContentBrowser/RightArrowIcon.png");
		Ref<Texture2D> m_IconPathViewer = Texture2D::Create("assets/icons/ContentBrowser/ViewPathsIcon.png");
		Ref<Texture2D> m_IconRefresh = Texture2D::Create("assets/icons/ContentBrowser/RefreshIcon.png");

		//File Type Icons
		Ref<Texture2D> m_IconFileBackground = Texture2D::Create("assets/icons/ContentBrowser/FileTypes/FileBackground.png");
		Ref<Texture2D> m_IconFolderBackground = Texture2D::Create("assets/icons/ContentBrowser/FileTypes/FolderBackground.png");

		Ref<Texture2D> m_IconFileTypeC = Texture2D::Create("assets/icons/ContentBrowser/FileTypes/FileTypeC.png");
		Ref<Texture2D> m_IconFileTypeCpp = Texture2D::Create("assets/icons/ContentBrowser/FileTypes/FileTypeCpp.png");
		Ref<Texture2D> m_IconFileTypeCs = Texture2D::Create("assets/icons/ContentBrowser/FileTypes/FileTypeCs.png");
		Ref<Texture2D> m_IconFileTypeHeader = Texture2D::Create("assets/icons/ContentBrowser/FileTypes/FileTypeHeader.png");
		Ref<Texture2D> m_IconFileTypePch = Texture2D::Create("assets/icons/ContentBrowser/FileTypes/FileTypePch.png");
		Ref<Texture2D> m_IconFileTypeImage = Texture2D::Create("assets/icons/ContentBrowser/FileTypes/FileTypeImage.png");
		Ref<Texture2D> m_IconFileTypeDymatic = Texture2D::Create("assets/icons/ContentBrowser/FileTypes/FileTypeDymatic.png");
		Ref<Texture2D> m_IconFileTypeDytheme = Texture2D::Create("assets/icons/ContentBrowser/FileTypes/FileTypeDytheme.png");
		Ref<Texture2D> m_IconFileTypeKeybind = Texture2D::Create("assets/icons/ContentBrowser/FileTypes/FileTypeKeybind.png");
		Ref<Texture2D> m_IconFileTypePrefs = Texture2D::Create("assets/icons/ContentBrowser/FileTypes/FileTypePrefs.png");
	};

}