#include "ContentBrowser.h"

#include "ImGuiCustom.h"

#include "dirent.h"
#include <stdlib.h>
#include <algorithm>

#include <filesystem>

#include "../TextSymbols.h"

namespace Dymatic {

	ContentBrowser::ContentBrowser(Preferences* preferencesRef, TextEditorPannel* textEditorPannelRef)
		: m_PreferencesReference(preferencesRef), m_TextEditorPannelReference(textEditorPannelRef)
	{
		SetBrowseDirectory(m_RootPath);
	}

	void ContentBrowser::OnImGuiRender(Timestep ts)
	{
		FileProperties fileToOpen = {};
		FileProperties fileToDelete = {};
		std::string newBrowseDirectory = "";
		bool openPreviewDirectoriesPopup = false;

		if (m_ContentBrowserVisible)
		{
			ImGui::Begin((std::string(CHARACTER_WINDOW_ICON_CONTENT_BROWSER) + " Content Browser").c_str(), &m_ContentBrowserVisible);

			if (ImGui::Button(CHARACTER_CONTENT_BROWSER_REFRESH, ImVec2{ 35.0f, 25.0f })) { UpdateFileView(); }

			ImGui::SameLine();

			//Filepath Navigation
			std::vector<int> slashLocations;
			//Needs to be negative one as they get incremented.
			slashLocations.push_back(-1);
			for (int i = 0; i < m_BrowsePath.length(); i++)
			{
				if (m_BrowsePath[i] == '\\')
				{
					slashLocations.push_back(i);
				}
			}
			//Not subracting one as it needs to be one greater, as the index has one subtracted during substr() slash removale phase.
			slashLocations.push_back(m_BrowsePath.length());

			ImGui::Dummy(ImVec2{ 0, 0 });
			for (int i = 1; i < slashLocations.size(); i++)
			{
				ImGui::PushID(i);
				ImGui::SameLine();
				std::string label;
				label = m_BrowsePath.substr(slashLocations[i - 1] + 1, slashLocations[i] - slashLocations[i - 1] - 1);
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0, 0, 0 ,0 });
				if (ImGui::Button(label.c_str()))
				{
					newBrowseDirectory = m_BrowsePath.substr(0, slashLocations[i] + (i == 1 ? 1 : 0));
					m_ScrollToTop = true;
				}
				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
					{
						const char* filepath = (const char*)payload->Data;
						SystemCommand("move \"" + std::string(filepath) + "\" \"" + m_BrowsePath.substr(0, slashLocations[i] + (i == 1 ? 1 : 0)) + "\"");
						UpdateFileView();
					}
					ImGui::EndDragDropTarget();
				}
				if (GetNumberOfFolders(filesAtDirectory) > 0 || i != slashLocations.size() - 1)
				{
					ImGui::SameLine();
					ImGui::BeginGroup();
					ImGui::Dummy(ImVec2{ 0, 0 });
					
					if (ImGui::ImageButton(reinterpret_cast<void*>(m_IconRightArrow->GetRendererID()), ImVec2{ 10, 10 }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 }))
					{
						m_DirectoryViewDropdownPath = m_BrowsePath.substr(0, slashLocations[i] + (i == 1 ? 1 : 0));
						openPreviewDirectoriesPopup = true;
					}
					
					ImGui::EndGroup();
				}
				ImGui::PopStyleColor();
				ImGui::PopID();
			}

			if (openPreviewDirectoriesPopup)
			{
				ImGui::OpenPopup("LowerDirectoriesAvalbile");
			}

			//Popup over files avalible dropdown
			if (ImGui::BeginPopup("LowerDirectoriesAvalbile", ImGuiWindowFlags_NoMove || ImGuiWindowFlags_NoResize || ImGuiWindowFlags_NoScrollbar || ImGuiWindowFlags_NoDocking))
			{
				std::vector<FileProperties> filesAtDir = GetFilesAtDirectory(m_DirectoryViewDropdownPath);
				for (int i = 0; i < filesAtDir.size(); i++)
				{
					if (filesAtDir[i].fileCatagory == FileCatagory::Folder)
					{
						if (ImGui::MenuItem(filesAtDir[i].filename.c_str()))
						{
							SetBrowseDirectory(m_DirectoryViewDropdownPath + '\\' + filesAtDir[i].filename);
							m_ScrollToTop = true;
						}
					}
				}
				ImGui::EndPopup();
			}

			ImVec4 borderColor = ImVec4{ 0.27f, 0.27f, 0.31f, 1.0f };
			ImVec2 pathWindowContentRegion = ImGui::GetContentRegionAvail();
			ImGui::GetWindowDrawList()->AddRectFilled(ImVec2{ ImGui::GetWindowPos().x, ImGui::GetWindowPos().y + 60 }, ImVec2{ ImGui::GetWindowPos().x + pathWindowContentRegion.x, ImGui::GetWindowPos().y + 2 + 60 }, ImGui::ColorConvertFloat4ToU32(borderColor));
			ImGui::Dummy(ImVec2{ 0, 10 });

			float h = ImGui::GetContentRegionAvail().y;
			float minSize = 20.0f;
			float sz1 = ImGui::GetContentRegionAvail().x / 5 - variation1;
			float sz2 = ImGui::GetContentRegionAvail().x / 5 * (m_PathsWindowOpen ? 4 : 5) - (m_PathsWindowOpen ? variation2 : 0);


			//Begin path viewer window
			if (m_PathsWindowOpen)
			{
				ImGui::Custom::Splitter(true, 2.0f, &sz1, &sz2, minSize, minSize, h);
				variation1 = (ImGui::GetContentRegionAvail().x / 5) - sz1;
				variation2 = (ImGui::GetContentRegionAvail().x / 5 * (m_PathsWindowOpen ? 4 : 5)) - sz2;

				ImGui::BeginChild("##PathsViewerWindow", ImVec2(sz1, h), false);

				if (ImGui::Button(CHARACTER_CONTENT_BROWSER_VIEW_PATHS, ImVec2{ 35.0f, 22.0f })) { m_PathsWindowOpen = false; }

				ImGui::SameLine();

				ImGui::Text("Paths:");

				ImGui::BeginChild("##PathsList", ImVec2{ ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y - 22 });

				GeneratePathList(m_RootPath);

				ImGui::EndChild();

				ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
				ImGui::Text(("Root Directory: " + m_RootPath).c_str());
				ImGui::PopFont();
				ImGui::Dummy(ImGui::GetContentRegionAvail());

				ImGui::GetWindowPos();
				ImGui::EndChild();

				ImGui::SameLine();
			}

			//Begin Content browser Section of window, with files and search bar, located on the right

			//ImGui::BeginChild("##ContentBrowserWholeWindow");
			ImGui::BeginChild("##ContentBrowserWholeWindow", ImVec2(sz2, h), false);

			if (!m_PathsWindowOpen)
			{
				if (ImGui::Button(CHARACTER_CONTENT_BROWSER_VIEW_PATHS, ImVec2{ 35.0f, 22.0f })) { m_PathsWindowOpen = true; }
			}

			ImGui::SameLine();

			ImGui::PushItemWidth(-35);

			char buffer[256];
			memset(buffer, 0, sizeof(buffer));
			std::strncpy(buffer, m_SearchBarBuffer.c_str(), sizeof(buffer));

			if (ImGui::InputTextWithHint("##SearchBar", "Search Content:", buffer, sizeof(buffer)))
			{
				m_SearchBarBuffer = std::string(buffer);
				UpdateFileView();
			}
			ImGui::PopItemWidth();
			ImGui::SameLine();
			if (ImGui::Button("X", ImVec2{ 22.0f, 22.0f }))
			{
				m_SearchBarBuffer = "";
				UpdateFileView();
			}
			ImGui::Dummy(ImVec2{ 0, 20 });

			float IconSize = 80.0f;
			int itemIndex = 0;
			int itemsPerLine = floor(ImGui::GetContentRegionAvailWidth() / IconSize - 1 * ((ImGui::GetContentRegionAvailWidth() / IconSize) / 8));

			if (filesDisplayed.empty())
			{
				std::string emptyMessage = "No files or folders found, create or import them.";
				ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
				ImGui::GetWindowDrawList()->AddText(ImGui::GetWindowPos() + ImGui::GetWindowSize() / 2 - ImGui::CalcTextSize(emptyMessage.c_str()) / 2, ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f)), emptyMessage.c_str());
				ImGui::PopFont();
			}

			ImGui::BeginChild("##ContentBrowserWindowChild", ImVec2{ ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y - 22 });

			if (m_ScrollToTop)
			{
				m_ScrollToTop = false;
				ImGui::SetScrollHereY();
			}

			//Hack to only detect clicking on empty space.
			if (ImGui::BeginPopupContextWindow(0, 0, false))
			{
				m_SelectedFile = {};
				ImGui::CloseCurrentPopup();
				ImGui::EndPopup();
			}

			//Old Code:
			//if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
			//	m_SelectedFile = {};

			if (!filesDisplayed.empty())
			{
				for (int i = 0; i < filesDisplayed.size(); i++)
				{
					ImGui::BeginGroup();

					std::string fileFormat = GetFileFormat(filesDisplayed[i].filename);
					Ref<Texture2D> FormatBackground = filesDisplayed[i].fileCatagory == FileCatagory::Folder ? m_IconFolderBackground : m_IconFileBackground;
					Ref<Texture2D> FormatTexture;
					if (fileFormat == ".cpp") { FormatTexture = m_IconFileTypeCpp; }
					else if (fileFormat == ".cs") { FormatTexture = m_IconFileTypeCs; }
					else if (fileFormat == ".c") { FormatTexture = m_IconFileTypeC; }
					else if (fileFormat == ".h") { FormatTexture = m_IconFileTypeHeader; }
					else if (fileFormat == ".pch") { FormatTexture = m_IconFileTypePch; }
					//Image Formats
					else if (fileFormat == ".png") { FormatTexture = m_IconFileTypeImage; }
					else if (fileFormat == ".jpg") { FormatTexture = m_IconFileTypeImage; }
					else if (fileFormat == ".jpeg") { FormatTexture = m_IconFileTypeImage; }
					else if (fileFormat == ".bmp") { FormatTexture = m_IconFileTypeImage; }
					else if (fileFormat == ".psd") { FormatTexture = m_IconFileTypeImage; }

					else if (fileFormat == ".dymatic") { FormatTexture = m_IconFileTypeDymatic; }
					else if (fileFormat == ".dytheme") { FormatTexture = m_IconFileTypeDytheme; }
					else if (fileFormat == ".prefs") { FormatTexture = m_IconFileTypePrefs; }
					else if (fileFormat == ".keybind") { FormatTexture = m_IconFileTypeKeybind; }

					ImGui::PushID(filesDisplayed[i].filename.c_str());
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::Custom::GetImGuiCustomColorValue(ImGui::Custom::ImGuiCol_FileHovered));
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::Custom::GetImGuiCustomColorValue(ImGui::Custom::ImGuiCol_FileSelected));
					ImGui::PushStyleColor(ImGuiCol_Button, m_SelectedFile.filename == filesDisplayed[i].filename ? ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive) : ImVec4{ 0.0f, 0.0f, 0.0f, 0.0f });
					//if (ImGui::ImageButton(reinterpret_cast<void*>(FormatTexture->GetRendererID()), ImVec2{ IconSize, IconSize }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 }, -1))
					ImVec2 curPos = ImGui::GetCursorScreenPos() + GImGui->Style.FramePadding;

					ImVec4 tintColor = ImGui::Custom::GetImGuiCustomColorValue(ImGui::Custom::ImGuiCol_FileBackground);

					for (FileColor fileColor : m_PreferencesReference->m_PreferenceData.fileColors)
					{
						std::string extensionCompareValue(fileColor.Extension);
						std::string originalExtenVal = fileFormat;

						//extensionCompareValue.erase(std::remove(extensionCompareValue.begin(), extensionCompareValue.end(), "."), extensionCompareValue.end());
						//originalExtenVal.erase(std::remove(originalExtenVal.begin(), originalExtenVal.end(), "."), originalExtenVal.end());

						if (extensionCompareValue == originalExtenVal) { tintColor = fileColor.Color; }
					}

					bool filePressed = ImGui::ImageButton(reinterpret_cast<void*>(FormatBackground->GetRendererID()), ImVec2{ IconSize, IconSize }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 }, -1, ImVec4{ 0.0f, 0.0f, 0.0f, 0.0f }, tintColor);
					if (FormatTexture) { ImGui::GetWindowDrawList()->AddImage(reinterpret_cast<void*>(FormatTexture->GetRendererID()), curPos, ImVec2{ curPos.x + IconSize, curPos.y + IconSize }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 }, ImGui::ColorConvertFloat4ToU32(ImGui::Custom::GetImGuiCustomColorValue(ImGui::Custom::ImGuiCol_FileIcon))); }
					ImGui::IsItemClicked();


					if (ImGui::BeginDragDropSource())
					{
						std::string path = (m_BrowsePath + '\\' + filesDisplayed[i].filename);
						ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", path.c_str(), sizeof(char) * path.length() + 5, ImGuiCond_Once);
						ImGui::Text((std::string(filesDisplayed[i].fileCatagory == Folder ? CHARACTER_WINDOW_ICON_CONTENT_BROWSER : CHARACTER_SYSTEM_ICON_NEW_FILE) + " Move " + filesDisplayed[i].filename).c_str());
						ImGui::EndDragDropSource();
					}
					if (filesDisplayed[i].fileCatagory == Folder)
					{
						if (ImGui::BeginDragDropTarget())
						{
							if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
							{
								const char* filepath = (const char*)payload->Data;
								SystemCommand("move \"" + std::string(filepath) + "\" \"" + m_BrowsePath + '\\' + filesDisplayed[i].filename + "\"");
								UpdateFileView();
							}
							ImGui::EndDragDropTarget();
						}
					}

					if (ImGui::IsItemHovered())
					{
						ImGui::BeginTooltip();
						ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
						ImGui::Text(filesDisplayed[i].filename.c_str());
						ImGui::PopFont();
						ImGui::EndTooltip();

						if (ImGui::IsMouseDoubleClicked(0))
						{
							fileToOpen = filesDisplayed[i];
						}
					}

					if (filePressed)
						m_SelectedFile = filesDisplayed[i];

					ImGui::PopStyleColor(3);
					if (ImGui::BeginPopupContextItem())
					{
						m_SelectedFile = filesDisplayed[i];

						if (ImGui::MenuItem("Open"))
						{
							fileToOpen = filesDisplayed[i];
						}

						if (ImGui::MenuItem("Rename"))
						{
							m_OpenRenamePopup;
							OpenRenamePopup(filesDisplayed[i]);
						}

						if (ImGui::MenuItem("Copy"))
						{
							m_CopyBrowsePathContext = m_BrowsePath;
							m_CopyContext = filesDisplayed[i];
						}

						if (ImGui::MenuItem("Duplicate"))
						{
							CopyFileToDirectory(m_BrowsePath + '\\' + filesDisplayed[i].filename, m_BrowsePath + '\\' + FindNextNumericalName(filesDisplayed[i].filename), filesDisplayed[i].fileCatagory);
						}

						if (ImGui::MenuItem("Delete"))
						{
							fileToDelete = filesDisplayed[i];
						}

						ImGui::Separator();

						if (filesDisplayed[i].fileCatagory == File)
						{
							if (ImGui::MenuItem("Edit"))
							{
								m_TextEditorPannelReference->OpenTextFileByFilepath(m_BrowsePath + "\\" + filesDisplayed[i].filename);
								m_TextEditorPannelReference->GetTextEditorVisible() = true;
							}
						}

						if (ImGui::MenuItem("Show in Explorer"))
						{
							SystemCommand(("start \"\" \"" + m_BrowsePath + "\""));
						}


						ImGui::EndPopup();
					}

					ImGui::PopID();

					std::string fileName = GetFileName(filesDisplayed[i].filename);

					std::string formedFileName = "";
					const char* fileNameChar = fileName.c_str();
					int lineLength = 0;
					int maxLineLength = 13;
					std::vector<int> newLineLocations;
					newLineLocations.push_back(0);
					for (int b = 0; b < strlen(fileNameChar); b++)
					{
						bool newLine = !islower(fileNameChar[b]) && b != 0;

						if (newLine)
						{
							newLineLocations.push_back(b);
						}

						//formedFileName = formedFileName + (newLine ? "\n" : "") + fileNameChar[b];
						//lineLength++;
						//if (newLine)
						//	lineLength = 0;
					}

					newLineLocations.push_back(fileName.length() - 1);

					for (int s = 1; s < newLineLocations.size() - 1; s++)
					{
						if (((newLineLocations[s + 1] - newLineLocations[s]) + (newLineLocations[s] - newLineLocations[s - 1])) < maxLineLength)
						{
							newLineLocations.erase(newLineLocations.begin() + s);
							s--;
						}
					}

					if (newLineLocations.size() < 3)
					{
						formedFileName = fileName;
					}
					else {
						for (int s = 1; s < newLineLocations.size(); s++)
						{
							if (s == newLineLocations.size() - 1)
							{
								formedFileName = formedFileName + fileName.substr(newLineLocations[s - 1], newLineLocations[s] - newLineLocations[s - 1] + 1);
							}
							else
							{
								formedFileName = formedFileName + fileName.substr(newLineLocations[s - 1], newLineLocations[s] - newLineLocations[s - 1]) + "\n";
							}
						}
					}

					ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);

					std::string ManiplateLine = formedFileName;
					int numberOfLines = 0;

					for (bool complete = false; complete == false && numberOfLines < 3;)
					{
						if (ManiplateLine.find_first_of("\n") != -1)
						{
							ManiplateLine = ManiplateLine.substr(ManiplateLine.find_first_of("\n") + 1, ManiplateLine.length());
						}
						else
						{
							complete = true;
						}
						numberOfLines++;
					}

					ImGui::Dummy(ImVec2{ 0, (ImGui::CalcTextSize("M").y * 1.5f * (3 - numberOfLines)) * 0.2f });

					ManiplateLine = formedFileName;
					numberOfLines = 0;

					for (bool complete = false; complete == false && numberOfLines < 3;)
					{
						if (ManiplateLine.find_first_of("\n") != -1)
						{

							std::string line = ManiplateLine.substr(0, ManiplateLine.find_first_of("\n"));
							std::string lineToDisplay = ((line.size() < maxLineLength ? line : line.substr(0, maxLineLength)) + (numberOfLines == 2 ? "..." : ""));
							ImGui::Dummy(ImVec2{ (IconSize / 2) - (ImGui::CalcTextSize(lineToDisplay.c_str()).x / 2), 0 });
							ImGui::SameLine();
							ImGui::Text(lineToDisplay.c_str());
							ManiplateLine = ManiplateLine.substr(ManiplateLine.find_first_of("\n") + 1, ManiplateLine.length());
						}
						else
						{
							std::string lineToDisplay = ManiplateLine.size() < maxLineLength ? ManiplateLine : ManiplateLine.substr(0, maxLineLength) + (numberOfLines == 2 ? "..." : "");
							ImGui::Dummy(ImVec2{ (IconSize / 2) - (ImGui::CalcTextSize(lineToDisplay.c_str()).x / 2), 0 });
							ImGui::SameLine();
							ImGui::Text(lineToDisplay.c_str());
							complete = true;
						}
						numberOfLines++;
					}

					ImGui::Dummy(ImVec2{ 0, (ImGui::CalcTextSize("M").y * 1.5f * (3 - numberOfLines)) * 0.8f });

					ImGui::PopFont();

					ImGui::EndGroup();
					itemIndex++;
					float percentageOfLine = ((float)itemIndex) / ((float)itemsPerLine);

					if ((((float)((int)percentageOfLine)) == percentageOfLine) || i == filesDisplayed.size() - 1)
					{

					}
					else
					{
						ImGui::SameLine();
					}
				}
			}

			if (ImGui::BeginPopupContextWindow("WindowID", 1, false))
			{
				if (ImGui::MenuItem("Paste"))
				{
					if (m_CopyContext.filename != "")
					{
						std::string newFile = m_CopyContext.filename;
						if (DoesFileExist(newFile))
						{
							newFile = FindNextNumericalName(m_CopyContext.filename);
						}

						CopyFileToDirectory(m_CopyBrowsePathContext + '\\' + m_CopyContext.filename, m_BrowsePath + '\\' + newFile, m_CopyContext.fileCatagory);
					}
				}

				ImGui::Separator();

				if (ImGui::MenuItem("Show in Explorer"))
				{
					SystemCommand(("start \"\" \"" + m_BrowsePath + "\""));
				}

				ImGui::EndPopup();
			}

			if (m_OpenRenamePopup)
			{
				ImGui::OpenPopup("RenameFilePopup");
				m_OpenRenamePopup = false;
			}
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 5.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
			if (ImGui::BeginPopup("RenameFilePopup", ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar))
			{
				ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
				ImGui::Text(("Renaming File: " + m_BrowsePath + '\\' + m_RenamePopupContext.filename).c_str());
				ImGui::PopFont();
				ImGui::Dummy(ImVec2{ 0, 10 });
				ImGui::PushItemWidth(-1);

				char buffer[256];
				memset(buffer, 0, sizeof(buffer));
				std::strncpy(buffer, m_RenameBuffer.c_str(), sizeof(buffer));
				if (ImGui::InputText(("##RenameInput" + m_RenamePopupContext.filename).c_str(), buffer, sizeof(buffer)))
				{
					m_RenameBuffer = std::string(buffer);
				}

				ImGui::PopItemWidth();
				bool canRename = true;
				std::string renameMessage = "";
				if (m_RenameBuffer == "") { renameMessage = "Filename cannot be empty!"; canRename = false; }
				else if (!(m_RenameBuffer.find("\\") == std::string::npos) || !(m_RenameBuffer.find('\\') == std::string::npos) || !(m_RenameBuffer.find(":") == std::string::npos) || !(m_RenameBuffer.find("*") == std::string::npos) || !(m_RenameBuffer.find("?") == std::string::npos) || !(m_RenameBuffer.find("<") == std::string::npos) || !(m_RenameBuffer.find(">") == std::string::npos) || !(m_RenameBuffer.find("|") == std::string::npos) || !(m_RenameBuffer.find("\"") == std::string::npos)) { renameMessage = "Invalid Filename! ( Cannot Include: \\/:*?<>|\" )"; canRename = false; }
				else if (DoesFileExist(m_RenameBuffer) && m_RenameBuffer != m_RenamePopupContext.filename) { renameMessage = "A file with this name already exists!"; canRename = false; }
				else if ((m_RenameBuffer.length() < 4) && m_RenamePopupContext.fileCatagory != FileCatagory::Folder) { renameMessage = "Filename is too short!"; canRename = false; }
				else if ((GetFileFormat(m_RenameBuffer) == "" || GetFileFormat(m_RenameBuffer) == ".") && m_RenamePopupContext.fileCatagory != FileCatagory::Folder) { renameMessage = "Filename must have an extension!"; canRename = false; }

				ImVec2 posC = ImGui::GetCursorPos();
				ImGui::Dummy(ImVec2{ 225.0f, 18.75f });

				if (!canRename)
				{
					ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetWindowPos() + ImVec2(ImGui::GetWindowSize().x / 2 - ImGui::CalcTextSize(renameMessage.c_str()).x / 2 - 2.5f, posC.y), ImGui::GetWindowPos() + ImVec2(ImGui::GetWindowSize().x / 2 + ImGui::CalcTextSize(renameMessage.c_str()).x / 2 + 2.5f, posC.y + 18), ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 0.0f, 0.0f, 1.0f)), 15.0f);
					ImGui::GetWindowDrawList()->AddText(ImGui::GetWindowPos() + ImVec2(ImGui::GetWindowSize().x / 2 - ImGui::CalcTextSize(renameMessage.c_str()).x / 2, posC.y + 9 - ImGui::CalcTextSize(renameMessage.c_str()).y / 2), ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f)), renameMessage.c_str());
				}

				ImGui::Dummy(ImVec2{ (ImGui::GetContentRegionAvail().x / 2) - (ImGui::CalcTextSize("   Cancel     Rename   ").x / 2), 0 });
				ImGui::SameLine();
				bool cancel = ImGui::Button("Cancel");
				ImGui::SameLine();
				bool rename = ImGui::Button("Rename");
				if ((Input::IsKeyPressed(Key::Enter) || rename) && canRename)
				{
					ImGui::CloseCurrentPopup();
					RenameFile(m_RenamePopupContext, m_RenameBuffer);
					m_RenamePopupContext = {};
				}
				else if (cancel)
				{
					ImGui::CloseCurrentPopup();
					m_RenamePopupContext = {};
				}
				ImGui::EndPopup();
			}
			else
			{

				m_RenamePopupContext = {};
			}
			ImGui::PopStyleVar(2);

			ImGui::EndChild();

			ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
			std::string numberOfFiles = std::to_string(GetNumberOfFiles(filesDisplayed) + GetNumberOfFolders(filesDisplayed)) + " items";
			ImGui::Text(numberOfFiles.c_str());
			ImGui::PopFont();

			//Ends the Whole right side, content browser window
			ImGui::EndChild();

			ImGui::End();
		}

		//Delete File
		if (fileToDelete.filename != "")
		{
			DeleteFileBrowsed(m_BrowsePath + '\\' + fileToDelete.filename, fileToDelete.fileCatagory);
		}

		//Open File
		if (fileToOpen.filename != "")
		{
			OpenFileBrowsed(fileToOpen);
		}

		//Set New Directory
		if (newBrowseDirectory != "")
		{
			SetBrowseDirectory(newBrowseDirectory);
		}
	}

	void ContentBrowser::GeneratePathList(std::string path)
	{
		std::vector<FileProperties> listOfFiles = GetFilesAtDirectory(path);

		if (!listOfFiles.empty())
		{
			for (int i = 0; i < listOfFiles.size(); i++)
			{
				if (listOfFiles[i].fileCatagory == FileCatagory::Folder)
				{
					ImGuiTreeNodeFlags flags = /*((m_SelectionContext == entity) ? ImGuiTreeNodeFlags_Selected : 0) | */ImGuiTreeNodeFlags_OpenOnArrow;
					flags |= ImGuiTreeNodeFlags_SpanAvailWidth;

					bool treeVisible = false;
					bool lastFolder = (GetNumberOfFolders(GetFilesAtDirectory(path + '\\' + listOfFiles[i].filename)) == 0);
					ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
					treeVisible = ImGui::TreeNodeEx(listOfFiles[i].filename.c_str(), flags | (lastFolder ? ImGuiTreeNodeFlags_Leaf : 0));
					ImGui::PopFont();
					if (ImGui::IsItemClicked() && (ImGui::GetMousePos().x - ImGui::GetItemRectMin().x) > ImGui::GetTreeNodeToLabelSpacing())
					{
						SetBrowseDirectory(path + '\\' + listOfFiles[i].filename);
						m_ScrollToTop = true;
					}
					
					if (ImGui::BeginDragDropTarget())
					{
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
						{
							const char* filepath = (const char*)payload->Data;
							SystemCommand("move \"" + std::string(filepath) + "\" \"" + path + "\\" + listOfFiles[i].filename + "\"");
							UpdateFileView();
						}
						ImGui::EndDragDropTarget();
					}

					if (treeVisible)
					{
						GeneratePathList(path + '\\' + listOfFiles[i].filename);
						ImGui::TreePop();
					}
				}
			}
		}
	}

	bool ContentBrowser::DoesFileExist(std::string filename)
	{
		bool exists = false;
		if (!filesAtDirectory.empty())
		{
			for (int i = 0; i < filesAtDirectory.size(); i++)
			{
				if (filesAtDirectory[i].filename == filename)
				{
					exists = true;
					break;
				}
			}
		}
		return exists;
	}

	void ContentBrowser::RenameFile(FileProperties fileProperties, std::string filename)
	{
		if (!DoesFileExist(filename))
		{
			std::string message = "rename \"" + m_BrowsePath + '\\' + fileProperties.filename + "\"  \"" + filename + "\"";
			message = SwapStringSlashesSingle(message);
			SystemCommand(message);
			UpdateFileView();
		}
	}

	void ContentBrowser::SetFiles(std::vector<FileProperties> filesToSet)
	{
		filesAtDirectory = filesToSet;

		if (m_SearchBarBuffer == "")
		{
			filesDisplayed = filesAtDirectory;
		}

		else
		{
			filesDisplayed.clear();
			for (int i = 0; i < filesAtDirectory.size(); i++)
			{
				if (ToLower(filesAtDirectory[i].filename).find(ToLower(m_SearchBarBuffer)) != std::string::npos) {
					filesDisplayed.push_back(filesAtDirectory[i]);
				}
			}
		}
	}

	void ContentBrowser::UpdateFileView()
	{
		SetBrowseDirectory(m_BrowsePath);
	}

	void ContentBrowser::SystemCommand(std::string _Command)
	{
		system((_Command + " > nul").c_str());
	}

	void ContentBrowser::OpenFileBrowsed(FileProperties fileProperties)
	{
		if (fileProperties.fileCatagory == FileCatagory::File)
		{
			m_FileToOpen = m_BrowsePath + '\\' +fileProperties.filename;
		}

		if (fileProperties.fileCatagory == FileCatagory::Folder)
		{
			SetBrowseDirectory(m_BrowsePath + '\\' + fileProperties.filename);
			m_SelectedFile = {};
			m_ScrollToTop = true;
		}
	}

	void ContentBrowser::DeleteFileBrowsed(std::string filepath, FileCatagory fileCatagory)
	{
		std::string message = "\"" + filepath + "\"";

		message = SwapStringSlashesSingle(message);

		if (fileCatagory == FileCatagory::File)
		{
			SystemCommand(("del /Q " + message));
		}
		else if (fileCatagory == FileCatagory::Folder)
		{
			SystemCommand(("rmdir / s / q " + message));
		}

		if (GetFullFileNameFromPath(filepath) == m_SelectedFile.filename)
			m_SelectedFile = {};
		if (GetFullFileNameFromPath(filepath) == m_CopyContext.filename)
		{
			m_CopyContext = {};
			m_CopyBrowsePathContext = {};
		}

		UpdateFileView();
	}

	void ContentBrowser::CopyFileToDirectory(std::string inputFilepath, std::string outputFilepath, FileCatagory fileCatagory)
	{
		std::string message = "";

		if (fileCatagory == FileCatagory::File)
		{
			message = ("copy \"" + inputFilepath + "\" \"" + outputFilepath + "\"");
		}
		else if (fileCatagory == FileCatagory::Folder)
		{
			message = ("Xcopy \"" + inputFilepath + "\" \"" + outputFilepath + "\"");
		}

		message = SwapStringSlashesSingle(message);

		if (fileCatagory == FileCatagory::Folder)
			message = message + " /E /H /C /I";

		SystemCommand(message);

		UpdateFileView();
	}

	void ContentBrowser::OpenRenamePopup(FileProperties fileProperties)
	{
		m_RenamePopupContext = fileProperties;
		m_OpenRenamePopup = true;
		m_RenameBuffer = fileProperties.filename;
	}

	int ContentBrowser::GetNumberOfFiles(std::vector<FileProperties> filesIterator)
	{
		int numberOfFiles = 0;
		if (!filesIterator.empty())
		{
			for (int i = 0; i < filesIterator.size(); i++)
			{
				if (filesIterator[i].fileCatagory == FileCatagory::File)
				{
					numberOfFiles++;
				}
			}
		}
		return numberOfFiles;
	}

	int ContentBrowser::GetNumberOfFolders(std::vector<FileProperties> filesIterator)
	{
		int numberOfFiles = 0;
		if (!filesIterator.empty())
		{
			for (int i = 0; i < filesIterator.size(); i++)
			{
				if (filesIterator[i].fileCatagory == FileCatagory::Folder)
				{
					numberOfFiles++;
				}
			}
		}
		return numberOfFiles;
	}

	std::string ContentBrowser::SwapStringSlashesSingle(std::string swapString)
	{
		int index = 0;
		if (swapString.find_first_of("/") != -1)
		{
			while (true) {
				index = swapString.find("/", index);
				if (index == std::string::npos) break;

				swapString.replace(index, 1, "\\");

				index += 1;
			}
		}
		return swapString;
	}

	std::string ContentBrowser::SwapStringSlashesDouble(std::string swapString)
	{
		int index = 0;
		if (swapString.find_first_of("\\") != -1)
		{
			while (true) {
				index = swapString.find("\\", index);
				if (index == std::string::npos) break;

				swapString.replace(index, 1, "/");

				index += 1;
			}
		}
		return swapString;
	}

	std::string ContentBrowser::ToLower(std::string inString)
	{
		transform(inString.begin(), inString.end(), inString.begin(), ::tolower);
		return inString;
	}

	std::string ContentBrowser::FindNextNumericalName(std::string filename)
	{
		std::string tempFileName = GetFileName(filename);
		std::string tempFileFormat = GetFileFormat(filename);
		if (tempFileName.find_last_of("_") != -1)
		{
			bool isNumber = true;
			for (int e = tempFileName.find_last_of("_") + 1; e < tempFileName.length(); e++)
			{
				if (!isdigit(tempFileName[e]))
					isNumber = false;
			}
			if (isNumber)
			{
				tempFileName = tempFileName.substr(0, filename.find_last_of("_"));
			}
		}
		std::string copyIndexDisplay;
		int copyIndex = 1;
		for (bool completed = false; completed == false;)
		{
			bool useable = true;
			if (!filesAtDirectory.empty())
			{
				for (int k = 0; k < filesAtDirectory.size(); k++)
				{
					copyIndexDisplay = (copyIndex < 10 ? "_0" : "_") + std::to_string(copyIndex);
					if (filesAtDirectory[k].filename == (tempFileName + copyIndexDisplay + tempFileFormat))
						useable = false;
				}
			}
			if (useable)
			{
				completed = true;
			}
			else
			{
				copyIndex++;
			}
		}
		return (tempFileName + copyIndexDisplay + tempFileFormat);
	}

	std::string ContentBrowser::GetFullFileNameFromPath(std::string filepath)
	{
		std::string name = "";
		if (filepath.find_first_of('\\') != -1)
		{
			name = filepath.substr(filepath.find_last_of('\\') + 1, filepath.length() - filepath.find_last_of('\\') - 1);
		}
		return name;
	}

	std::string ContentBrowser::GetFileName(std::string filename)
	{
		std::string name = "";
		{
			name = filename.substr(0, filename.find_last_of("."));
		}
		return name;
	}

	std::string ContentBrowser::GetFileFormat(std::string filename)
	{
		std::string fileFormat = "";
		if (filename.find_last_of(".") != -1 && filename.length() > 2) {
			fileFormat = filename.substr(filename.find_last_of("."), filename.length());
		}
		return fileFormat;
	}

	void ContentBrowser::SetBrowseDirectory(std::string path)
	{
		m_BrowsePath = path;
		filesAtDirectory.clear();

		//Remove double "//"
		for (int i = 0; i < m_BrowsePath.length() - 1; i++)
		{
			std::string a = std::string(1, m_BrowsePath[i]);
			std::string b = std::string(1, m_BrowsePath[i + 1]);
			if (a == "/" && b == "/")
			{
				m_BrowsePath = m_BrowsePath.erase(i, 1);
			}
		}
		//Remove items with slash last
		if (m_BrowsePath.find_last_of('\\') == m_BrowsePath.length() - 1)
		{
			m_BrowsePath = m_BrowsePath.erase(m_BrowsePath.length() - 1, 1);
		}

		SetFiles(GetFilesAtDirectory(path));
	}

	std::vector<Dymatic::FileProperties> ContentBrowser::GetFilesAtDirectory(std::string filepath)
	{
		std::vector<Dymatic::FileProperties> filesAtLocation;

		//Add slash to end of path if it doesn't have one
		if (filepath.find_first_of('\\') == -1)
		{
			filepath = filepath + '\\';
		}

		//Folders
		std::vector<std::string> filesToAdd = GetRecords(filepath, FileCatagory::Folder);
		if (filesToAdd.empty() == false)
		{
			for (int i = 0; i < filesToAdd.size(); i++)
			{
				FileProperties fileProps;
				fileProps.filename = filesToAdd[i];
				fileProps.fileCatagory = FileCatagory::Folder;
				filesAtLocation.push_back(fileProps);
			}
		}

		//Files
		std::vector<std::string> foldersToAdd = GetRecords(filepath, FileCatagory::File);
		if (foldersToAdd.empty() == false)
		{
			for (int i = 0; i < foldersToAdd.size(); i++)
			{
				FileProperties fileProps;
				fileProps.filename = foldersToAdd[i];
				fileProps.fileCatagory = FileCatagory::File;
				filesAtLocation.push_back(fileProps);
			}
		}

		//Clean Up
		if (!filesAtLocation.empty())
		{
			for (int a = 0; a < filesAtLocation.size(); a++)
			{
				if ((filesAtLocation[a].filename.find_first_of(".")) == 0 || ((filesAtLocation[a].filename.length()) < 4 && !(filesAtLocation[a].fileCatagory == FileCatagory::Folder)))
				{
					filesAtLocation.erase(filesAtLocation.begin() + a);
					a--;
				}
			}
		}

		//Return Final Vector of Files
		return filesAtLocation;

	}

	bool ContentBrowser::IsDirectory(std::string filepath)
	{
		struct stat path_stat;
		stat(filepath.c_str(), &path_stat);
		return S_ISDIR(path_stat.st_mode);
	}

	std::vector<std::string> ContentBrowser::GetRecords(std::string record_dir_path, FileCatagory fileCatagory)
	{
		std::vector<std::string> files;
		struct dirent* entry;
		DIR* dir = opendir(record_dir_path.c_str());

		if (dir == NULL)
		{
			return files;
		}
		while ((entry = readdir(dir)) != NULL)
		{
			switch (entry->d_type)
			{
			case DT_REG:
			{
				if (fileCatagory == File)
					files.push_back(entry->d_name);
				break;
			}
			case DT_DIR:
			{
				if (fileCatagory == Folder)
					files.push_back(entry->d_name);
				break;
			}
			}
		}
		closedir(dir);

		return files;
	}

	

}