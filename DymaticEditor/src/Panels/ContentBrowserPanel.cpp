#include "dypch.h"
#include "ContentBrowserPanel.h"

#include "Dymatic/Project/Project.h"
#include "Dymatic/Asset/AssetManager.h"
#include "SourceControl.h"
#include "Settings/ProjectSettings.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <glm/gtc/type_ptr.hpp>

#include "Dymatic/Utils/PlatformUtils.h"
#include "Dymatic/Math/StringUtils.h"

#include "Dymatic/Core/Input.h"

#include "../TextSymbols.h"

namespace Dymatic {

	static bool s_Init = false;

	static Ref<Texture2D> s_DirectoryIcon;
	static Ref<Texture2D> s_FileIcon;

	// Specific File Types
	static Ref<Texture2D> s_SceneIcon;
	static Ref<Texture2D> s_MeshIcon;
	static Ref<Texture2D> s_MaterialIcon;
	static Ref<Texture2D> s_TextureIcon;
	static Ref<Texture2D> s_FontIcon;
	static Ref<Texture2D> s_AudioIcon;

	static Ref<Texture2D> s_NewIcon;
	static Ref<Texture2D> s_SettingsIcon;

	static Ref<Texture2D> s_RightArrowIcon;
	static Ref<Texture2D> s_RefreshIcon;
	static Ref<Texture2D> s_ForwardDirectoryIcon;
	static Ref<Texture2D> s_BackDirectoryIcon;
	static Ref<Texture2D> s_UpDirectoryIcon;
	
	static Ref<Texture2D> s_SourceControlUntrackedIcon;
	static Ref<Texture2D> s_SourceControlAddedIcon;
	static Ref<Texture2D> s_SourceControlModifiedIcon;

	ContentBrowserPanel::ContentBrowserPanel()
	{
		if (!s_Init)
		{
			s_Init = true;
		
			s_DirectoryIcon = Texture2D::Create("Resources/Icons/ContentBrowser/DirectoryIcon.png");
			s_FileIcon = Texture2D::Create("Resources/Icons/ContentBrowser/FileIcon.png");

			// Specific File Types
			s_SceneIcon = Texture2D::Create("Resources/Icons/ContentBrowser/SceneIcon.png");
			s_MeshIcon = Texture2D::Create("Resources/Icons/ContentBrowser/MeshIcon.png");
			s_MaterialIcon = Texture2D::Create("Resources/Icons/ContentBrowser/MaterialIcon.png");
			s_TextureIcon = Texture2D::Create("Resources/Icons/ContentBrowser/TextureIcon.png");
			s_FontIcon = Texture2D::Create("Resources/Icons/ContentBrowser/FontIcon.png");
			s_AudioIcon = Texture2D::Create("Resources/Icons/ContentBrowser/AudioIcon.png");

			s_NewIcon = Texture2D::Create("Resources/Icons/ContentBrowser/NewIcon.png");
			s_SettingsIcon = Texture2D::Create("Resources/Icons/ContentBrowser/SettingsIcon.png");

			s_RightArrowIcon = Texture2D::Create("Resources/Icons/ContentBrowser/RightArrowIcon.png");
			s_RefreshIcon = Texture2D::Create("Resources/Icons/ContentBrowser/RefreshIcon.png");
			s_ForwardDirectoryIcon = Texture2D::Create("Resources/Icons/ContentBrowser/ForwardDirectoryIcon.png");
			s_BackDirectoryIcon = Texture2D::Create("Resources/Icons/ContentBrowser/BackDirectoryIcon.png");
			s_UpDirectoryIcon = Texture2D::Create("Resources/Icons/ContentBrowser/UpDirectoryIcon.png");
			
			s_SourceControlUntrackedIcon = Texture2D::Create("Resources/Icons/ContentBrowser/SourceControl/SourceControlUntrackedIcon.png");
			s_SourceControlAddedIcon = Texture2D::Create("Resources/Icons/ContentBrowser/SourceControl/SourceControlAddedIcon.png");
			s_SourceControlModifiedIcon = Texture2D::Create("Resources/Icons/ContentBrowser/SourceControl/SourceControlModifiedIcon.png");
		}

		if (Project::GetActive())
			Init();
	}

	void ContentBrowserPanel::Init()
	{
		m_BaseDirectory = Project::GetAssetDirectory();

		m_DisplayDirectories.Path = m_BaseDirectory;
		MoveToDirectory(m_BaseDirectory);
		m_Init = true;

		// Check all files are registered with the asset registry
		for (auto& directoryEntry : std::filesystem::recursive_directory_iterator(Project::GetAssetDirectory()))
			if (!directoryEntry.is_directory())
				CheckFileAssetData(std::filesystem::relative(directoryEntry.path(), m_BaseDirectory));
	}

	static bool DoesRectOverlap(ImVec2 l1, ImVec2 r1, ImVec2 l2, ImVec2 r2)
	{
		if (l1.x < r2.x && r1.x > l2.x &&
			r1.y > l2.y && l1.y < r2.y)
			return true;
		return false;
	}

	static void SelectedBubble()
	{
		ImGui::GetWindowDrawList()->AddCircleFilled(ImGui::GetItemRectMin() + ImVec2(ImGui::GetItemRectSize().y, ImGui::GetItemRectSize().y) * 0.5f, ImGui::GetItemRectSize().y * 0.2f, ImGui::GetColorU32(ImGuiCol_Text));
	}

	static void SourceControlCommitUI(const std::vector<ContentBrowserPanel::FileEntry>& selectionContext)
	{
		ImGui::BeginChild("##SourceControlMessageChild", ImVec2(400.0f, 75.0f));
		ImGui::Indent();
		for (auto& file : selectionContext)
			ImGui::TextDisabled(file.Path.string().c_str());
		ImGui::Unindent();
		ImGui::EndChild();

		ImGui::Separator();

		static char s_CommitMessage[256] = { 0 };
		static char s_CommitDescription[1024] = { 0 };
		ImGui::Text("Message:");
		ImGui::InputText("##CommitMessage", s_CommitMessage, 256);
		ImGui::Text("Description:");
		ImGui::InputTextMultiline("##CommitDescription", s_CommitDescription, 1024, ImVec2(400.0f, 100.0f));
		ImGui::Dummy(ImVec2((ImGui::GetContentRegionAvailWidth() - ImGui::CalcTextSize("CommitCancel").x - ImGui::GetStyle().FramePadding.x * 4.0f) * 0.5f, 0.0f));
		ImGui::SameLine();
		if (ImGui::Button("Commit"))
		{
			std::string fileString;
			for (auto& file : selectionContext)
				fileString += "\"" + (Project::GetRelativeAssetDirectory() / file.Path).string() + "\" ";

			SourceControl::Commit(fileString, s_CommitMessage, s_CommitDescription);
			Popup::RemoveTopmostPopup();

			s_CommitMessage[0] = 0;
			s_CommitDescription[0] = 0;
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel"))
			Popup::RemoveTopmostPopup();
	}

	static const ImColor GetFileTypeColor(FileType type)
	{
		switch (type)
		{
		case FileType::FileTypeFile: return ImColor(65, 65, 65);
		case FileType::FileTypeScene: return ImColor(145, 91, 54);
		case FileType::FileTypeMesh: return ImColor(77, 137, 184);
		case FileType::FileTypeMaterial: return ImColor(56, 122, 70);
		case FileType::FileTypeTexture: return ImColor(145, 56, 54);
		case FileType::FileTypeFont: return ImColor(99, 112, 53);
		case FileType::FileTypeAudio: return ImColor(99, 80, 150);
		}
	};

	void ContentBrowserPanel::OnImGuiRender(bool isDragging)
	{
		auto& contentBrowserVisible = Preferences::GetEditorWindowVisible(Preferences::EditorWindow::ContentBrowser);
		if (!contentBrowserVisible)
			return;

		ImGui::Begin(CHARACTER_ICON_FOLDER " Content Browser", &contentBrowserVisible, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

		if (m_Init)
		{
			auto& style = ImGui::GetStyle();
			auto drawlist = ImGui::GetWindowDrawList();

			ImGui::BeginChild("##ContentBrowserDragDropArea", {}, false, ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar);
			
			ImVec2 mousePos = ImGui::GetMousePos();

			ImGuiWindow* hoveredWindow = GImGui->HoveredWindow;
			ImGuiWindow* currentWindow = ImGui::GetCurrentWindow();
			m_InContentBounds = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem | ImGuiHoveredFlags_ChildWindows);

			if (isDragging && m_InContentBounds)
			{
				ImGui::SetNextWindowPos(mousePos);
				ImGui::Begin("##ExternalDragDropTooltip", nullptr, ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDocking);
				ImGui::Text(CHARACTER_ICON_IMPORT " Import External Files...");
				ImGui::End();

				drawlist->AddRect(ImGui::GetWindowPos() - ImVec2(3.5f, 3.5f), ImGui::GetWindowPos() + ImGui::GetWindowSize() + ImVec2(3.5f, 3.5f), ImGui::GetColorU32(ImGuiCol_DragDropTarget), 0.0f, 0, 2.0f);
			}

			if (ImGui::ImageButton((ImTextureID)s_NewIcon->GetRendererID(), { 20.0f, 20.0f }, { 0, 1 }, { 1, 0 }, -1, {}, ImGui::GetStyleColorVec4(ImGuiCol_Text)))
				ImGui::OpenPopup("##ContentBrowserCreatePopup");

			ImGui::SameLine();

			if (ImGui::ImageButton((ImTextureID)s_BackDirectoryIcon->GetRendererID(), { 20.0f, 20.0f }, { 0, 1 }, { 1, 0 }, -1, {}, ImGui::GetStyleColorVec4(m_DirectoryHistoryIndex == 0 ? ImGuiCol_TextDisabled : ImGuiCol_Text)))
				NavBackDirectory();
			ImGui::SameLine();
			if (ImGui::ImageButton((ImTextureID)s_ForwardDirectoryIcon->GetRendererID(), { 20.0f, 20.0f }, { 0, 1 }, { 1, 0 }, -1, {}, ImGui::GetStyleColorVec4(m_DirectoryHistoryIndex == m_DirectoryHistory.size() - 1 ? ImGuiCol_TextDisabled : ImGuiCol_Text)))
				NavForwardDirectory();

			ImGui::SameLine();
			bool hasParent = m_CurrentDirectory != std::filesystem::path(m_BaseDirectory);
			if (ImGui::ImageButton((ImTextureID)s_UpDirectoryIcon->GetRendererID(), { 20.0f, 20.0f }, { 0, 1 }, { 1, 0 }, -1, {}, ImGui::GetStyleColorVec4(hasParent ? ImGuiCol_Text : ImGuiCol_TextDisabled)))
				if (hasParent)
					MoveToDirectory(m_CurrentDirectory.parent_path());

			ImGui::SameLine();
			if (ImGui::ImageButton((ImTextureID)s_RefreshIcon->GetRendererID(), { 20.0f, 20.0f }, { 0, 1 }, { 1, 0 }, -1, {}, ImGui::GetStyleColorVec4(ImGuiCol_Text)))
				UpdateDisplayFiles();

			bool openPreviewDirectoriesPopup = false;

			// Path View
			{
				std::filesystem::path path;
				uint32_t index = 0;
				std::filesystem::path parent = m_BaseDirectory.parent_path();
				for (auto& split : m_DirectorySplit)
				{
					path = path / split;

					ImGui::PushID(index);
					ImGui::SameLine();
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0, 0, 0 ,0 });
					if (ImGui::Button(split.c_str()))
						MoveToDirectory(parent / path);
					if (ImGui::BeginDragDropTarget())
					{
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
						{
							for (auto& item : m_SelectionContext)
								MoveFileToDirectory(m_BaseDirectory / item.Path, parent / path, false);
							UpdateDisplayFiles();
						}
						ImGui::EndDragDropTarget();
					}
					if (GetNumberOfFoldersInDirectory(parent / path) != 0)
					{
						ImGui::SameLine();
						ImGui::BeginGroup();
						ImGui::Dummy({});

						if (ImGui::ImageButton((ImTextureID)s_RightArrowIcon->GetRendererID(), ImVec2{ 10, 10 }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 }))
						{
							m_DirectoryViewDropdownPath = parent / path;
							openPreviewDirectoriesPopup = true;
						}

						ImGui::EndGroup();
					}
					ImGui::PopStyleColor();
					ImGui::PopID();
					index++;
				}
			}

			if (openPreviewDirectoriesPopup)
				ImGui::OpenPopup("##DirectoryPreviewPopup");

			ImGui::SameLine();
			ImGui::Dummy({ ImGui::GetContentRegionAvail().x - 40.0f, 0.0f });
			ImGui::SameLine();
			if (ImGui::ImageButton((ImTextureID)s_SettingsIcon->GetRendererID(), { 20.0f, 20.0f }, { 0, 1 }, { 1, 0 }, -1, {}, ImGui::GetStyleColorVec4(ImGuiCol_Text)))
				ImGui::OpenPopup("##ContentBrowserSettingsPopup");
			if (ImGui::BeginPopup("##ContentBrowserSettingsPopup"))
			{
				ImGui::TextDisabled("Content Browser Settings");
				ImGui::Separator();
				if (ImGui::BeginMenu("Sort"))
				{
					if (ImGui::MenuItem("Directories First", "", &Preferences::GetData().DirectoriesFirst))
						UpdateDisplayFiles();
					ImGui::Separator();

					auto& sort = Preferences::GetData().ContentBrowserSortType;
					auto& ascending = Preferences::GetData().Ascending;

					if (ImGui::MenuItem("     Name")) { sort = Preferences::PreferencesData::ContentBrowserSortType::Name; UpdateDisplayFiles(); }
					if (sort == Preferences::PreferencesData::ContentBrowserSortType::Name)
						SelectedBubble();

					if (ImGui::MenuItem("     Date")) { sort = Preferences::PreferencesData::ContentBrowserSortType::Date; UpdateDisplayFiles(); }
					if (sort == Preferences::PreferencesData::ContentBrowserSortType::Date)
						SelectedBubble();

					if (ImGui::MenuItem("     Type")) { sort = Preferences::PreferencesData::ContentBrowserSortType::Type; UpdateDisplayFiles(); }
					if (sort == Preferences::PreferencesData::ContentBrowserSortType::Type)
						SelectedBubble();

					if (ImGui::MenuItem("     Size")) { sort = Preferences::PreferencesData::ContentBrowserSortType::Size; UpdateDisplayFiles(); }
					if (sort == Preferences::PreferencesData::ContentBrowserSortType::Size)
						SelectedBubble();

					ImGui::Separator();

					if (ImGui::MenuItem("     Ascending")) { ascending = true; UpdateDisplayFiles(); }
					if (ascending)
						SelectedBubble();

					if (ImGui::MenuItem("     Descending")) { ascending = false; UpdateDisplayFiles(); }
					if (!ascending)
						SelectedBubble();

					ImGui::EndMenu();
				}
				if (ImGui::BeginMenu("Filter"))
				{
					auto& preferences = Preferences::GetData();

					int index = -1;

					bool allSelected = false;
					for (uint32_t i = 0; i < FileType::FILE_TYPE_SIZE; i++)
					{
						if (preferences.Filters[i] == false)
						{
							allSelected = false;
							break;
						}
					}

					if (ImGui::MenuItem("     All")) index = -2;
					if (allSelected) SelectedBubble();

					ImGui::Separator();
					if (ImGui::MenuItem("     Directory")) index = 0;
					if (preferences.Filters[0]) SelectedBubble();

					if (ImGui::MenuItem("     File")) index = 1;
					if (preferences.Filters[1]) SelectedBubble();

					if (ImGui::MenuItem("     Scene")) index = 2;
					if (preferences.Filters[2]) SelectedBubble();

					if (ImGui::MenuItem("     Mesh")) index = 3;
					if (preferences.Filters[3]) SelectedBubble();

					if (ImGui::MenuItem("     Material")) index = 4;
					if (preferences.Filters[4]) SelectedBubble();

					if (ImGui::MenuItem("     Texture")) index = 5;
					if (preferences.Filters[5]) SelectedBubble();

					if (ImGui::MenuItem("     Font")) index = 6;
					if (preferences.Filters[6]) SelectedBubble();

					if (ImGui::MenuItem("     Audio")) index = 7;
					if (preferences.Filters[7]) SelectedBubble();

					ImGui::EndMenu();

					if (index != -1)
					{
						if (index == -2)
						{
							for (size_t i = 0; i < FILE_TYPE_SIZE; i++)
								preferences.Filters[i] = true;
						}
						else
						{
							if (!ImGui::GetIO().KeyShift)
								for (size_t i = 0; i < FILE_TYPE_SIZE; i++)
									preferences.Filters[i] = false;
							preferences.Filters[index] = !preferences.Filters[index];
						}
						UpdateDisplayFiles();
					}
				}
				if (ImGui::BeginMenu("View"))
				{
					if (ImGui::MenuItem("Show Thumbnails", "", &Preferences::GetData().ShowThumbnails))
						UpdateDisplayFiles();
					ImGui::Text("Thumbnail Size");
					ImGui::SameLine();
					ImGui::DragInt("##ThumbnailSizeInput", &Preferences::GetData().ThumbnailSize, 1, 16, 256);
					ImGui::EndMenu();
				}
				if (ImGui::MenuItem("Invert Selection"))
					InvertSelection();
				ImGui::EndPopup();
			}

			if (ImGui::BeginPopup("##DirectoryPreviewPopup"))
			{
				for (auto& file : std::filesystem::directory_iterator(m_DirectoryViewDropdownPath))
					if (file.is_directory())
						if (ImGui::MenuItem(file.path().filename().string().c_str()))
							MoveToDirectory(file.path());
				ImGui::EndPopup();
			}

			if (ImGui::BeginPopup("##ContentBrowserCreatePopup"))
			{
				ImGui::TextDisabled("Create");
				ImGui::Separator();
				DrawCreateMenu();
				ImGui::EndPopup();
			}

			ImGui::Separator();

			if (ImGui::BeginTable("##OuterTable", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_NoPadInnerX))
			{
				ImGui::TableNextColumn();

				{
					ImGui::BeginChild("##DirectoryViewRegion");

					auto drawlist = ImGui::GetWindowDrawList();

					auto min = ImGui::GetWindowPos() + ImVec2(ImGui::GetColumnWidth(ImGui::GetColumnIndex()) - 20.0f, 0.0f);
					auto max = ImGui::GetWindowPos() + ImVec2(ImGui::GetColumnWidth(ImGui::GetColumnIndex()) + style.WindowPadding.x * 0.5f, ImGui::GetContentRegionAvail().y + 20.0f);

					ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
					bool open = ImGui::TreeNodeEx("##ContentTreeNode", ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_OpenOnArrow, "Content");

					// Drag Drop Target
					if (ImGui::BeginDragDropTarget())
					{
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
						{
							for (auto& item : m_SelectionContext)
								MoveFileToDirectory(m_BaseDirectory / item.Path, m_BaseDirectory, false);
							UpdateDisplayFiles();
						}
						ImGui::EndDragDropTarget();
					}

					if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
						MoveToDirectory(m_BaseDirectory);
					ImGui::PopStyleVar();
					if (open)
					{
						std::filesystem::path path;
						DrawDirectoryView(m_DisplayDirectories, path);
						if (!path.empty())
							MoveToDirectory(path);
						ImGui::TreePop();
					}

					ImVec4 color = ImGui::GetStyleColorVec4(ImGuiCol_BorderShadow);
					ImU32 color_full = ImGui::GetColorU32(color);
					ImU32 color_trans = ImGui::ColorConvertFloat4ToU32({ color.x, color.y, color.z, 0.0f });
					drawlist->AddRectFilledMultiColor(min, max, color_trans, color_full, color_full, color_trans);

					ImGui::EndChild();
				}

				ImGui::TableNextColumn();

				char buffer[256];
				memset(buffer, 0, sizeof(buffer));
				std::strncpy(buffer, m_SearchbarBuffer.c_str(), sizeof(buffer));
				ImGui::SetNextItemWidth(-1);
				if (ImGui::InputTextWithHint("##ContentBrowserSearchbar", "Search...", buffer, sizeof(buffer)))
				{
					m_SearchbarBuffer = std::string(buffer);
					UpdateDisplayFiles();
				}

				ImGui::Separator();

				const float padding = 16.0f;
				const float thumbnailSize = Preferences::GetData().ThumbnailSize;
				float cellSize = thumbnailSize + padding;

				float panelWidth = ImGui::GetContentRegionAvail().x;
				int columnCount = std::clamp((int)(panelWidth / cellSize), 1, IMGUI_TABLE_MAX_COLUMNS);
				if (columnCount < 1)
					columnCount = 1;

				ImGui::Dummy({ (ImGui::GetContentRegionAvail().x - columnCount * cellSize - padding * 1.0f) * 0.5f, 0.0f });
				ImGui::SameLine();

				ImGui::BeginChild("##ContentRegion");

				auto ContentBrowserMin = ImGui::GetCursorScreenPos();
				auto ContentBrowserSize = ImGui::GetContentRegionAvail();

				if (m_ScrollToTop)
				{
					m_ScrollToTop = false;
					ImGui::SetScrollHereY();
				}

				if (ImGui::BeginTable("##Innertable", columnCount))
				{
					ImGui::TableNextColumn();

					uint8_t contextIndex = 0;
					std::filesystem::path contextPath;

					auto drawList = ImGui::GetWindowDrawList();
					int columnIndex = 0;
					bool selecting = m_SelectionPos.x != -1.0f;
					bool stoppingSelection = false;
					ImRect selectionRect{ ImGui::GetMousePos(), { m_SelectionPos.x, m_SelectionPos.y } };

					// Manage Drag Selection
					{
						if (ImGui::IsMouseReleased(0) && selecting)
						{
							selecting = false;

							m_SelectionPos = { -1.0f, -1.0f };
							stoppingSelection = true;
						}

						// Ensure Rect min/max is correct (only calculated when drag selection stops)
						if (stoppingSelection)
						{
							if (selectionRect.Min.x > selectionRect.Max.x) std::swap(selectionRect.Min.x, selectionRect.Max.x);
							if (selectionRect.Min.y > selectionRect.Max.y) std::swap(selectionRect.Min.y, selectionRect.Max.y);
						}
					}
					
					if (Input::IsKeyPressed(Key::F2) && !m_SelectionContext.empty())
						RenameFile(m_SelectionContext.back().Path);

					// Iterate over and handle all the displayed files
					for (size_t fileIndex = 0; fileIndex < m_DisplayFiles.size(); fileIndex++)
					{
						auto& file = m_DisplayFiles[fileIndex];

						// Spacing Above
						if (columnIndex >= columnCount)
							ImGui::Dummy({ 0.0f, padding });

						ImGui::PushID(file.Path.filename().string().c_str());

						if (file.IsDirectory)
						{
							ImGui::PushStyleColor(ImGuiCol_Button, {});
							glm::vec3 color = ProjectSettings::HasFolderColor(file.Path) ? ProjectSettings::GetFolderColor(file.Path) : glm::vec3(1.0f);
							ImGui::ImageButton((ImTextureID)s_DirectoryIcon->GetRendererID(), { thumbnailSize, thumbnailSize }, { 0, 1 }, { 1, 0 }, -1, ImVec4(0, 0, 0, 0), ImVec4(color.x, color.y, color.z, 1.0f));
							ImGui::PopStyleColor();

							const ImVec2 itemMin = ImGui::GetItemRectMin();
							const ImVec2 itemMax = ImGui::GetItemRectMax();
							const ImVec2 itemSize = ImGui::GetItemRectSize();

							if (m_RenameContext == file.Path)
								DrawRenameInput(file.Path);
							else
								ImGui::GetWindowDrawList()->AddText(ImGui::GetCursorScreenPos(), ImGui::GetColorU32(ImGuiCol_Text), file.Path.filename().stem().string().c_str());

							// Render Item Shadow
							ImDrawFlags draw_flags = ImDrawFlags_RoundCornersBottom | ImDrawFlags_ShadowCutOutShapeBackground;
							ImGui::GetWindowDrawList()->AddShadowRect(itemMin, itemMax, ImGui::GetColorU32(ImGuiCol_WindowShadow), 20.0f, ImVec2(5.0f, 5.0f), draw_flags, style.FrameRounding);

							// Render Item Outline
							ImGui::GetWindowDrawList()->AddRect(itemMin, itemMax, IsFileSelected(file.Path) ? ImGui::GetColorU32(ImVec4(1.0f, 0.92f, 0.57f, 1.0f)) : ((ImGui::IsItemHovered() && !selecting) ? ImGui::GetColorU32(ImVec4(0.5f, 0.5f, 0.5f, 1.0f)) : ImGui::GetColorU32(ImGuiCol_WindowShadow)), style.FrameRounding, ImDrawFlags_None, (ImGui::IsItemHovered() && !selecting) ? 2.0f : 1.0f);
						}
						else
						{
							const float frameRoundingMultiplier = 3.0f;
							const float thumbnailAspectRatio = 1.5f;

							ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, style.FrameRounding * frameRoundingMultiplier);
							ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_Header));
							ImGui::ButtonCornersEx("##ItemButton", { thumbnailSize, thumbnailSize * thumbnailAspectRatio }, ImGuiButtonFlags_None, ImDrawFlags_RoundCornersBottom);
							if ( ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
								m_OpenFileEditorCallback(file.Path);
							ImGui::PopStyleColor();
							ImGui::PopStyleVar();

							const ImVec2 itemMin = ImGui::GetItemRectMin();
							const ImVec2 itemMax = ImGui::GetItemRectMax();
							const ImVec2 itemSize = ImGui::GetItemRectSize();

							ImGui::GetWindowDrawList()->AddRectFilled(itemMin, itemMin + ImVec2(thumbnailSize, thumbnailSize), ImGui::GetColorU32(ImGuiCol_Button));
							ImGui::GetWindowDrawList()->AddImage((ImTextureID)(file.Texture ? file.Texture : s_FileIcon)->GetRendererID(), itemMin, itemMin + ImVec2(thumbnailSize, thumbnailSize), { 0, 1 }, { 1, 0 });

							if (m_RenameContext == file.Path)
							{
								const ImVec2 cursorPos = ImGui::GetCursorScreenPos();
								ImGui::SetCursorScreenPos(itemMin + style.FramePadding + ImVec2(0.0f, thumbnailSize));
								DrawRenameInput(file.Path);
								ImGui::SetCursorScreenPos(cursorPos);
							}
							else
							{
								std::string text = file.Path.filename().stem().string();
								const char* c_text = text.c_str();
								ImGui::GetWindowDrawList()->AddText(ImGui::GetFont(), ImGui::GetFontSize(), itemMin + style.FramePadding + ImVec2(0.0f, thumbnailSize), ImGui::GetColorU32(ImGuiCol_Text), c_text, c_text + text.length(), thumbnailSize);
							}
							

							// Type Text
							ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
							std::string typeText = FileTypeToString(file.Type).c_str();
							const ImVec2 fontSize = ImGui::CalcTextSize(typeText.c_str());
							ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(itemMin.x, itemMax.y - fontSize.y), itemMax, GetFileTypeColor(file.Type), style.FrameRounding * frameRoundingMultiplier, ImDrawFlags_RoundCornersBottom);
							ImGui::GetWindowDrawList()->AddText(itemMax - fontSize - style.FramePadding, ImGui::GetColorU32(ImGuiCol_Text), typeText.c_str());
							ImGui::PopFont();

							// Render source control icons
							if (SourceControl::IsActive())
							{
								auto unstagedStatus = SourceControl::GetFileUnstagedStatus(Project::GetRelativeAssetDirectory() / file.Path);
								auto stagedStatus = SourceControl::GetFileStagedStatus(Project::GetRelativeAssetDirectory() / file.Path);
								if (unstagedStatus != SourceControl::Status::None)
								{
									ImVec2 iconSize = { 16.0f, 16.0f };
									ImVec2 iconPos = itemMin - style.FramePadding + ImVec2(thumbnailSize - iconSize.x * 2.0f, style.FramePadding.y * 2.0f);
									ImGui::GetWindowDrawList()->AddImage((ImTextureID)(unstagedStatus == SourceControl::Status::Added ? s_SourceControlUntrackedIcon : s_SourceControlModifiedIcon)->GetRendererID(), iconPos, iconPos + iconSize, { 0, 1 }, { 1, 0 }, ImGui::GetColorU32(ImVec4(0.9f, 0.1f, 0.2f, 1.0f)));
								}
								if (stagedStatus != SourceControl::Status::None)
								{
									ImVec2 iconSize = { 16.0f, 16.0f };
									ImVec2 iconPos = itemMin - style.FramePadding + ImVec2(thumbnailSize - iconSize.x * 1.0f, style.FramePadding.y * 2.0f);
									ImGui::GetWindowDrawList()->AddImage((ImTextureID)(stagedStatus == SourceControl::Status::Added ? s_SourceControlAddedIcon : s_SourceControlModifiedIcon)->GetRendererID(), iconPos, iconPos + iconSize, { 0, 1 }, { 1, 0 }, ImGui::GetColorU32(ImVec4(0.1f, 0.8f, 0.2f, 1.0f)));
								}
							}

							// Render Item Outline
							ImGui::GetWindowDrawList()->AddRect(itemMin, itemMax, IsFileSelected(file.Path) ? ImGui::GetColorU32(ImVec4(1.0f, 0.92f, 0.57f, 1.0f)) : ((ImGui::IsItemHovered() && !selecting) ? ImGui::GetColorU32(ImVec4(0.5f, 0.5f, 0.5f, 1.0f)) : ImGui::GetColorU32(ImGuiCol_WindowShadow)), style.FrameRounding * frameRoundingMultiplier, ImDrawFlags_RoundCornersBottom, (ImGui::IsItemHovered() && !selecting) ? 2.0f : 1.0f);

							// Render Item Shadow
							ImDrawFlags draw_flags = ImDrawFlags_RoundCornersBottom | ImDrawFlags_ShadowCutOutShapeBackground;
							ImGui::GetWindowDrawList()->AddShadowRect(itemMin, itemMax, ImGui::GetColorU32(ImGuiCol_WindowShadow), 20.0f, ImVec2(5.0f, 5.0f), draw_flags, style.FrameRounding * frameRoundingMultiplier);
						}

						if (stoppingSelection)
						{
							auto& min = ImGui::GetItemRectMin();
							auto& max = ImGui::GetItemRectMax();
							if (DoesRectOverlap(min, max, selectionRect.Min, selectionRect.Max))
								SetSelectionContext(file, true);
						}

						if (ImGui::IsMouseReleased(ImGuiMouseButton_Right) && ImGui::IsItemHovered())
						{
							if (!IsFileSelected(file.Path))
							{
								ClearSelectionContext();
								SetSelectionContext(file);
							}
						}

						if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) && ImGui::IsItemHovered() && !stoppingSelection)
						{
							if (IsFileSelected(file.Path) && !Input::IsKeyPressed(Key::LeftControl) && !Input::IsKeyPressed(Key::RightControl) && !Input::IsKeyPressed(Key::LeftShift) && !Input::IsKeyPressed(Key::RightShift))
								SetSelectionContext(file);
						}

						if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
						{
							if (Input::IsKeyPressed(Key::LeftControl) || Input::IsKeyPressed(Key::RightControl))
								ToggleSelectionContext(file);
							else if (Input::IsKeyPressed(Key::LeftShift) || Input::IsKeyPressed(Key::RightShift))
							{
								if (m_SelectionContext.empty())
									SetSelectionContext(file);
								else
									for (size_t i = 0; i < m_DisplayFiles.size(); i++)
										if (m_DisplayFiles[i].Path == m_SelectionContext.back().Path)
										{
											// Index of last selected item has been found

											while (true)
											{
												if (i < fileIndex) i++;
												else i--;

												SetSelectionContext(m_DisplayFiles[i], true);

												if (i == fileIndex)
												{
													break;
												}
											}
											break;
										}
							}
							else if (!IsFileSelected(file.Path))
								SetSelectionContext(file);
						}

						if (file.IsDirectory)
							if (ImGui::BeginDragDropTarget())
							{
								if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
								{
									for (auto& item : m_SelectionContext)
										MoveFileToDirectory(m_BaseDirectory / item.Path, m_BaseDirectory / file.Path, false);
									UpdateDisplayFiles();
								}
								ImGui::EndDragDropTarget();
							}

						if (ImGui::BeginDragDropSource())
						{
							float height = ImGui::GetTextLineHeight();
							ImGui::Image((ImTextureID)(m_SelectionContext[0].Texture ? m_SelectionContext[0].Texture : s_FileIcon)->GetRendererID(), ImVec2(height, height) * 2.0f, { 0, 1 }, { 1, 0 });
							ImGui::SameLine();
							ImGui::BeginGroup();

							std::string text;
							if (m_SelectionContext.size() == 1)
								text = m_SelectionContext[0].Path.filename().stem().string();
							else
								text = "'" + m_SelectionContext[0].Path.filename().stem().string() + "' and " + std::to_string(m_SelectionContext.size() - 1) + " others";

							ImGui::Dummy({ 0.0f, (height + style.FramePadding.y * 2.0f - ImGui::CalcTextSize(text.c_str()).y) * 0.5f });
							ImGui::Text(text.c_str());
							ImGui::EndGroup();

							const wchar_t* itemPath = (m_SelectionContext[0].Path).c_str();
							ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", itemPath, (wcslen(itemPath) + 1) * sizeof(wchar_t));
							ImGui::EndDragDropSource();
						}

						if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
						{
							if (file.IsDirectory)
							{
								contextIndex = 1;
								contextPath = file.Path;
							}
						}

						if (ImGui::BeginPopupContextItem())
						{
							if (ImGui::MenuItem("Open")) m_OpenFileEditorCallback(file.Path);
							if (ImGui::MenuItem("Rename")) { RenameFile(file.Path); }
							if (ImGui::MenuItem("Duplicate")) DuplicateFile(m_BaseDirectory / file.Path);
							if (ImGui::MenuItem("Copy")) CopyFile(m_BaseDirectory / file.Path);
							if (ImGui::MenuItem("Paste")) PasteFile();
							if (ImGui::MenuItem("Delete")) { contextIndex = 2; contextPath = file.Path; }

							ImGui::Separator();

							if (ImGui::MenuItem("Open Externally")) { OpenExternally(m_BaseDirectory / file.Path); }

							if (ImGui::MenuItem("Show in Folder View")) { contextIndex = 3; contextPath = (m_BaseDirectory / file.Path).parent_path(); }
							if (ImGui::MenuItem("Show in Explorer")) { OpenExternally(m_BaseDirectory / file.Path.parent_path()); }

							ImGui::Separator();

							if (ImGui::MenuItem("Copy File Path")) ImGui::SetClipboardText(std::filesystem::absolute(file.Path).string().c_str());

							if (file.IsDirectory)
							{
								if (ImGui::BeginMenu("Set Color"))
								{
									glm::vec3 color = ProjectSettings::HasFolderColor(file.Path) ? ProjectSettings::GetFolderColor(file.Path) : glm::vec3(1.0f);
									if (ImGui::ColorEdit3("##SetFolderColor", glm::value_ptr(color)))
										ProjectSettings::SetFolderColor(file.Path, color);
									ImGui::EndMenu();
								}
							}

							if (SourceControl::IsActive())
							{
								if (ImGui::BeginMenu("Source Control"))
								{
									if (ImGui::MenuItem("Diff"))
										;
									
									if (ImGui::MenuItem("Discard"))
									{
										SourceControl::DiscardFile(Project::GetRelativeAssetDirectory() / file.Path);
										contextIndex = 4;
									}
									if (SourceControl::IsUnstaged(Project::GetRelativeAssetDirectory() / file.Path))
									{
										if (ImGui::MenuItem("Stage"))
											SourceControl::StageFile(Project::GetRelativeAssetDirectory() / file.Path);
									}
									if (SourceControl::IsStaged(Project::GetRelativeAssetDirectory() / file.Path))
									{
										if (ImGui::MenuItem("Unstage"))
											SourceControl::UnstageFile(Project::GetRelativeAssetDirectory() / file.Path);
									}
									if (SourceControl::IsStaged(Project::GetRelativeAssetDirectory() / file.Path))
									{
										if (ImGui::MenuItem("Check In"))
										{
											Popup::Create("Source Control", "Check Files in to source control:", {}, nullptr, false,
												[this]() {
													SourceControlCommitUI(m_SelectionContext);
												}, glm::vec2(400.0f, 225.0f));
										}
									}
									ImGui::EndMenu();
								}
							}

							ImGui::EndPopup();
						}

						if (ImGui::IsItemHovered() && ImGui::GetCurrentContext()->HoveredIdNotActiveTimer > 0.50f)
						{
							ImGui::BeginTooltip();
							ImGui::Text(file.Path.filename().string().c_str());
							ImGui::EndTooltip();
						}

						ImGui::TableNextColumn();
						columnIndex++;

						ImGui::PopID();
					}

					if (contextIndex != 0)
					{
						switch (contextIndex)
						{
						case 1: MoveToDirectory(m_CurrentDirectory / contextPath.filename()); break;
						case 2:
							for (auto& item : m_SelectionContext)
							{
								if (item.IsDirectory)
									for (auto& file : std::filesystem::recursive_directory_iterator(m_BaseDirectory / item.Path))
										AssetManager::RemoveAsset(std::filesystem::relative(file, m_BaseDirectory));
								else
									AssetManager::RemoveAsset(item.Path);
								DeleteFile(m_BaseDirectory / item.Path, false);
							}
							UpdateDisplayFiles();
							break;
						case 3: m_SearchbarBuffer.clear(); MoveToDirectory(contextPath); break;
						case 4: UpdateDisplayFiles(); break;
						}
					}

					ImGui::EndTable();

					if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0))
					{
						ClearSelectionContext();

						auto& mouse = ImGui::GetMousePos();
						m_SelectionPos = { mouse.x, mouse.y };
					}

					// Render Selection Highlight (only if has been moved more than a pixel)
					if (selecting && (m_SelectionPos.x != ImGui::GetMousePos().x && m_SelectionPos.y != ImGui::GetMousePos().y))
					{
						auto drawlist = ImGui::GetWindowDrawList();
						drawlist->AddRectFilled(selectionRect.Min, selectionRect.Max, ImGui::GetColorU32(ImGuiCol_TextSelectedBg));
						ImVec4 outline_color = ImGui::GetStyleColorVec4(ImGuiCol_TextSelectedBg);
						outline_color.x *= 1.2f;
						outline_color.y *= 1.2f;
						outline_color.z *= 1.2f;
						outline_color.w *= 1.2f;
						drawlist->AddRect(selectionRect.Min, selectionRect.Max, ImGui::GetColorU32(outline_color), 0.0f, ImDrawFlags_None, 2.0f);
					}

				}

				if (ImGui::BeginPopupContextWindow("##ContentBrowserContext", 1, false))
				{
					if (ImGui::BeginMenu("Create"))
					{
						DrawCreateMenu();
						ImGui::EndMenu();
					}

					if (ImGui::MenuItem("Import...")) ImportExternalFiles();

					if (ImGui::MenuItem("Paste")) PasteFile();

					ImGui::Separator();

					if (ImGui::MenuItem("Show in Explorer"))
						OpenExternally(m_CurrentDirectory);

					ImGui::EndPopup();
				}

				ImGui::GetWindowDrawList()->AddText(ImGui::GetWindowPos() + ImVec2(ImGui::GetCursorPos().x, ImGui::GetWindowSize().y - ImGui::GetFontSize() - style.FramePadding.y), IM_COL32_WHITE, m_DisplayFiles.size() == 1 ? "1 item" : (std::to_string(m_DisplayFiles.size()) + " items").c_str());
				ImGui::EndChild();

				if (m_DisplayFiles.empty())
				{
					const char* text = "No files or folders found, create or import them.";
					drawlist->AddText(ContentBrowserMin + ((ContentBrowserSize - ImGui::CalcTextSize(text)) * 0.5f), ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_Text)), text);
				}

				//ImGui::SliderFloat("Thumbnail Size", &thumbnailSize, 16, 512);
				//ImGui::SliderFloat("Padding", &padding, 0, 32);

				// TODO: status bar
				ImGui::EndTable();
			}

			ImGui::EndChild();
		}
		else
		{
			// Content browser has not been initialized
			const char* text = "No project is currently opened.";
			ImGui::GetWindowDrawList()->AddText(ImGui::GetWindowPos() + ((ImGui::GetWindowSize() - ImGui::CalcTextSize(text)) * 0.5f), ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_Text)), text);
		}

		ImGui::End();
	}

	void ContentBrowserPanel::MoveToDirectory(const std::filesystem::path& path)
	{
		if (path == m_CurrentDirectory)
			return;

		AddDirectoryHistory(path);
		SetDirectoryInternal(path);
	}

	void ContentBrowserPanel::OnExternalFileDrop(std::vector<std::string> filepaths)
	{
		if (!m_InContentBounds)
			return;

		size_t fileCount = 0;
		for (auto filepath : filepaths)
		{
			if (CopyFileToDirectory(filepath, m_CurrentDirectory)) fileCount++;
			else
				Notification::Create("Import Failure", "\"" + filepath + "\" does not exist.", { { "Dismiss", []() {}} });
		}
		if (fileCount != 0)
			Notification::Create("Files Imported", (std::to_string(fileCount) + " files have been successfully imported."), { { "Dismiss", []() {} } });

	}

	void ContentBrowserPanel::AddDirectoryHistory(const std::filesystem::path& path)
	{
		if (!m_DirectoryHistory.empty() && m_DirectoryHistoryIndex < m_DirectoryHistory.size() - 1)
			m_DirectoryHistory.erase(m_DirectoryHistory.begin() + m_DirectoryHistoryIndex + 1, m_DirectoryHistory.end());

		m_DirectoryHistory.push_back(path);
		m_DirectoryHistoryIndex = m_DirectoryHistory.size() - 1;
	}

	void ContentBrowserPanel::NavForwardDirectory()
	{
		if (m_DirectoryHistory.empty() || m_DirectoryHistoryIndex == m_DirectoryHistory.size() - 1)
			return;

		m_DirectoryHistoryIndex++;
		SetDirectoryInternal(m_DirectoryHistory[m_DirectoryHistoryIndex]);
	}

	void ContentBrowserPanel::NavBackDirectory()
	{
		if (m_DirectoryHistory.empty() || m_DirectoryHistoryIndex == 0)
			return;

		m_DirectoryHistoryIndex--;
		SetDirectoryInternal(m_DirectoryHistory[m_DirectoryHistoryIndex]);
	}

	void ContentBrowserPanel::SetDirectoryInternal(const std::filesystem::path& path)
	{
		m_CurrentDirectory = path;
		m_ScrollToTop = true;
		UpdateDisplayFiles();
	}

	// Display File View
	void ContentBrowserPanel::UpdateDisplayFiles()
	{
		ClearSelectionContext();

		m_DisplayFiles.clear();
		const bool searching = !m_SearchbarBuffer.empty();

		if (searching)
			for (auto& directoryEntry : std::filesystem::recursive_directory_iterator(m_CurrentDirectory))
				InsertDirectoryEntry(directoryEntry, true);
		else
			for (auto& directoryEntry : std::filesystem::directory_iterator(m_CurrentDirectory))
				InsertDirectoryEntry(directoryEntry, false);

		UpdateDirectorySplit();
		UpdateDisplayDirectories();

		SourceControl::UpdateStatus();

		for (auto& file : m_DisplayFiles)
			if (!file.IsDirectory)
				CheckFileAssetData(file.Path);
	}

	void ContentBrowserPanel::UpdateDirectorySplit()
	{
		m_DirectorySplit.clear();
		String::SplitStringByDelimiter(std::filesystem::relative(m_CurrentDirectory, m_BaseDirectory.parent_path()).lexically_normal().string(), m_DirectorySplit, '\\');
	}

	void ContentBrowserPanel::InsertDirectoryEntry(const std::filesystem::directory_entry& directoryEntry, bool searching)
	{
		const auto& path = directoryEntry.path();
		const bool isDir = directoryEntry.is_directory();
		if ((!searching || (searching && !isDir && (path.filename().string().find(m_SearchbarBuffer) != std::string::npos))) && Preferences::GetData().Filters[FileManager::GetFileType(path)])
		{
			size_t index = 0;
			for (auto& files : m_DisplayFiles)
			{
				// First sort directories
				if (Preferences::GetData().DirectoriesFirst)
				{
					if (!files.IsDirectory && isDir)
						break;

					if (files.IsDirectory && !isDir)
					{
						index++;
						continue;
					}
				}

				auto& sort = Preferences::GetData().ContentBrowserSortType;
				const auto& ascending = Preferences::GetData().Ascending;
				if (sort == Preferences::PreferencesData::ContentBrowserSortType::Name)
				{
					auto cmp = files.Path.filename().compare(path.filename());
					if (ascending ? (cmp < 0) : (cmp > 0))
					{
						index++;
						break;
					}
				}
				else if (sort == Preferences::PreferencesData::ContentBrowserSortType::Date)
				{
					if (std::filesystem::last_write_time(m_BaseDirectory / files.Path) > std::filesystem::last_write_time(path))
					{
						index++;
						break;
					}
				}
				else if (sort == Preferences::PreferencesData::ContentBrowserSortType::Type)
				{
					if (files.Path.filename().extension().compare(path.filename().extension()) < 0)
					{
						index++;
						break;
					}
				}
				else if (sort == Preferences::PreferencesData::ContentBrowserSortType::Size)
				{
					if (std::filesystem::file_size(m_BaseDirectory / files.Path) < std::filesystem::file_size(path))
					{
						break;
					}
				}

				index++;
			}

			m_DisplayFiles.insert(m_DisplayFiles.begin() + index, { std::filesystem::relative(path, m_BaseDirectory), m_BaseDirectory, directoryEntry.is_directory() });
		}
	}

	void ContentBrowserPanel::UpdateDisplayDirectories()
	{
		m_DisplayDirectories.SubDirectories.clear();
		UpdateDisplayDirectories(m_DisplayDirectories);
	}

	void ContentBrowserPanel::UpdateDisplayDirectories(DirectoryEntry& directory)
	{
		for (auto& directoryEntry : std::filesystem::directory_iterator(directory.Path))
		{
			if (directoryEntry.is_directory())
			{
				directory.SubDirectories.push_back({ directoryEntry.path() });
				UpdateDisplayDirectories(directory.SubDirectories.back());
			}
		}
	}

	void ContentBrowserPanel::DrawDirectoryView(DirectoryEntry& directory, std::filesystem::path& path)
	{
		for (auto& dir : directory.SubDirectories)
		{
			bool open = ImGui::TreeNodeEx(dir.Path.filename().string().c_str(), ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth | (dir.SubDirectories.empty() ? ImGuiTreeNodeFlags_Leaf : 0));

			// Drag Drop Target
			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
				{
					for (auto& item : m_SelectionContext)
						MoveFileToDirectory(m_BaseDirectory / item.Path, m_BaseDirectory / dir.Path, false);
					UpdateDisplayFiles();
				}
				ImGui::EndDragDropTarget();
			}

			if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
				path = dir.Path;
			if (open)
			{
				DrawDirectoryView(dir, path);
				ImGui::TreePop();
			}
		}
	}

	void ContentBrowserPanel::SetSelectionContext(FileEntry& entry, bool additive)
	{
		if (!additive)
			m_SelectionContext.clear();
		m_SelectionContext.push_back(entry);
	}

	void ContentBrowserPanel::ToggleSelectionContext(FileEntry& entry)
	{
		bool found = false;
		for (size_t i = 0; i < m_SelectionContext.size(); i++)
		{
			auto& item = m_SelectionContext[i];
			if (item.Path == entry.Path)
			{
				m_SelectionContext.erase(m_SelectionContext.begin() + i);
				found = true;
			}
		}
		if (!found)
			m_SelectionContext.push_back(entry);
	}

	void ContentBrowserPanel::ClearSelectionContext()
	{
		m_SelectionContext.clear();
	}

	bool ContentBrowserPanel::IsFileSelected(const std::filesystem::path& path)
	{
		for (auto& item : m_SelectionContext)
			if (item.Path == path)
				return true;
		return false;
	}

	void ContentBrowserPanel::InvertSelection()
	{
		for (auto& file : m_DisplayFiles)
			ToggleSelectionContext(file);
	}

	void ContentBrowserPanel::OpenExternally(const std::filesystem::path& path)
	{
		// Check if file type is registered
		//Process::CreateApplicationProcess(, { std::filesystem::absolute(g_AssetPath / path).string() });

		// Call System Default start
		system(("\"start \"\" \"" + std::filesystem::absolute(path).string() + "\"\"").c_str());
	}

	bool ContentBrowserPanel::DeleteFile(const std::filesystem::path& path, bool reload)
	{
		if (!std::filesystem::exists(path))
			return false;
		std::filesystem::remove_all(path);
		if (reload)
			UpdateDisplayFiles();

		return true;
	}

	void ContentBrowserPanel::RenameFile(const std::filesystem::path& oldPath, const std::filesystem::path& newPath)
	{
		if (oldPath == newPath)
			return;

		if (!std::filesystem::exists(oldPath))
			return;

		if (std::filesystem::exists(newPath))
			return;
		
		if (std::filesystem::is_directory(oldPath))
		{
			for (auto& file : std::filesystem::recursive_directory_iterator(oldPath))
			{
				AssetManager::RenameAsset(std::filesystem::relative(file, m_BaseDirectory), std::filesystem::relative(newPath, m_BaseDirectory) / std::filesystem::relative(file, oldPath));
			}
		}
		else
			AssetManager::RenameAsset(std::filesystem::relative(oldPath, m_BaseDirectory), std::filesystem::relative(newPath, m_BaseDirectory));

		std::filesystem::rename(oldPath, newPath);
		UpdateDisplayFiles();
	}

	void ContentBrowserPanel::RenameFile(const std::filesystem::path& path)
	{
		m_StartRename = true;
		m_RenameContext = path;
	}

	bool ContentBrowserPanel::MoveFileToDirectory(const std::filesystem::path& path, const std::filesystem::path& dir, bool reload)
	{
		// Check if file is already in that directory
		if (path.has_parent_path())
			if (path.parent_path() == dir)
				return true;

		if (CopyFileToDirectory(path, dir, true))
		{
			if (std::filesystem::is_directory(path))
				for (auto& file : std::filesystem::recursive_directory_iterator(path))
					AssetManager::RenameAsset(std::filesystem::relative(file, m_BaseDirectory), std::filesystem::relative(dir / path.filename(), m_BaseDirectory) / std::filesystem::relative(file, path));
			else
				AssetManager::RenameAsset(std::filesystem::relative(path, m_BaseDirectory), std::filesystem::relative(dir / path.filename(), m_BaseDirectory));

			if (!DeleteFile(path, false))
				return false;
		}
		else
			return false;

		if (reload)
			UpdateDisplayFiles();
		
		return true;
	}

	bool ContentBrowserPanel::CopyFileToDirectory(const std::filesystem::path& file, const std::filesystem::path& dir, bool moving)
	{
		if (!std::filesystem::exists(file))
			return false;

		std::filesystem::path name = file.filename();

		if (std::filesystem::exists(dir / file.filename()))
			name = FileManager::GetNextOfNameInDirectory(file, dir);

		std::filesystem::copy(file, dir / name, std::filesystem::copy_options::recursive);

		if (!moving)
		{
			UpdateDisplayFiles();
			CheckFileAssetData(std::filesystem::relative(dir / name, m_BaseDirectory));
		}

		return true;
	}

	bool ContentBrowserPanel::DuplicateFile(const std::filesystem::path& file)
	{
		if (file.has_parent_path())
			return CopyFileToDirectory(file, file.parent_path());
		return false;
	}

	void ContentBrowserPanel::CopyFile(const std::filesystem::path& file)
	{
		ImGui::SetClipboardText(file.string().c_str());
	}

	void ContentBrowserPanel::PasteFile()
	{
		std::filesystem::path file{ ImGui::GetClipboardText() };
		CopyFileToDirectory(file, m_CurrentDirectory);
	}

	void ContentBrowserPanel::ImportExternalFiles()
	{
		std::vector<std::string> files = FileDialogs::OpenFileMultiple("");
		for (auto& file : files)
			CopyFileToDirectory(file, m_CurrentDirectory);
	}

	void ContentBrowserPanel::CreateFolder()
	{
		std::filesystem::path path = m_CurrentDirectory / FileManager::GetNextOfNameInDirectory("New Folder", m_CurrentDirectory);
		std::filesystem::create_directory(path);
		UpdateDisplayFiles();
		RenameFile(std::filesystem::relative(path, m_BaseDirectory));
	}

	uint32_t ContentBrowserPanel::GetNumberOfFoldersInDirectory(const std::filesystem::path& directory)
	{
		size_t files = 0;
		for (auto const& file : std::filesystem::directory_iterator(directory))
			if (file.is_directory())
				files++;
		return files;
	}

	void ContentBrowserPanel::DrawCreateMenu()
	{
		if (ImGui::MenuItem("Folder")) CreateFolder();
		ImGui::Separator();
		if (ImGui::MenuItem("Material"))
		{
			std::filesystem::path path = (std::filesystem::relative(m_CurrentDirectory, m_BaseDirectory) / FileManager::GetNextOfNameInDirectory("New Material.dymaterial", m_CurrentDirectory)).lexically_normal();
			AssetManager::CreateNewAsset<Material>(path, "New Material");
			UpdateDisplayFiles();
			RenameFile(path);
		}
	}

	void ContentBrowserPanel::DrawRenameInput(const std::filesystem::path& path)
	{
		auto& style = ImGui::GetStyle();

		char buffer[256];
		memset(buffer, 0, sizeof(buffer));
		strcpy_s(buffer, path.filename().stem().string().c_str());
		if (m_StartRename)
		{
			m_StartRename = false;
			ImGui::SetKeyboardFocusHere();
		}
		bool input = ImGui::InputTextEx("##RenameInput", nullptr, buffer, sizeof(buffer), ImVec2(ImGui::GetItemRectSize().x - style.FramePadding.x * 2.0f, ImGui::GetTextLineHeight() + style.FramePadding.y * 2.0f), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll)
			|| ImGui::IsItemDeactivated();

		std::string str = buffer;

		const char* invalidMessage = nullptr;
		if (str.empty())
			invalidMessage = "Please provide a filename for the asset.";
		else if (!FileManager::IsFilenameValid(str))
			invalidMessage = "Filename cannot contain the following characters :*?\"<>|";
		else
		{
			static std::unordered_set<std::string> reserved_names = { "CON", "PRN", "AUX", "NUL", "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9", "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9" };

			std::transform(str.begin(), str.end(), str.begin(), ::toupper);
			if (reserved_names.count(str) > 0)
				invalidMessage = "A file/folder cannot match any of the following:\nCON, PRN, AUX, NUL, COM1, COM2, COM3, COM4, COM5, COM6, COM7, COM8, COM9, LPT1, LPT2, LPT3, LPT4, LPT5, LPT6, LPT7, LPT8, LPT9";
		}

		if (input)
		{
			if (ImGui::IsItemDeactivatedAfterEdit() && invalidMessage == nullptr)
				RenameFile(m_BaseDirectory / path, m_BaseDirectory / path.parent_path() / (std::string(buffer) + path.extension().string()));
			m_RenameContext.clear();
		}

		if (invalidMessage != nullptr)
		{
			ImVec2 invalidMessagePosition = ImVec2(ImGui::GetItemRectMin().x, ImGui::GetItemRectMax().y);

			ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.7f, 0.1f, 0.2f, 1.0f));
			ImGui::OpenPopup("##RenameErrorPopup");
			ImGui::BeginPopup("##RenameErrorPopup", ImGuiWindowFlags_NoFocusOnAppearing);
			ImGui::SetWindowPos(invalidMessagePosition);
			ImGui::Text(invalidMessage);
			ImGui::EndPopup();
			ImGui::PopStyleColor();
		}
	}

	void ContentBrowserPanel::CheckFileAssetData(const std::filesystem::path& path)
	{
		if (!AssetManager::DoesAssetExist(path))
		{
			auto assetType = FileManager::GetAssetType(path.extension().string());
			if (assetType != AssetType::None)
				AssetManager::AddAssetMetadata(AssetMetadata(assetType, path));
		}
	}

	Ref<Texture2D> ContentBrowserPanel::GetFileIcon(FileType type)
	{
		switch (type)
		{
		case FileTypeFile: return s_FileIcon;
		case FileTypeDirectory: return s_DirectoryIcon;
		case FileTypeScene: return s_SceneIcon;
		case FileTypeMesh: return s_MeshIcon;
		case FileTypeMaterial: return s_MaterialIcon;
		case FileTypeTexture: return s_TextureIcon;
		case FileTypeFont: return s_FontIcon;
		case FileTypeAudio: return s_AudioIcon;
		default: return s_FileIcon;
		}
	}

	std::string ContentBrowserPanel::FileTypeToString(FileType type)
	{
		switch (type)
		{
		case FileTypeFile: return "FILE";
		case FileTypeDirectory: return "DIRECTORY";
		case FileTypeScene: return "SCENE";
		case FileTypeMesh: return "MESH";
		case FileTypeMaterial: return "MATERIAL";
		case FileTypeTexture: return "TEXTURE";
		case FileTypeFont: return "FONT";
		case FileTypeAudio: return "AUDIO";
		default: return "FILE";
		}
	}

	ContentBrowserPanel::FileEntry::FileEntry(const std::filesystem::path& path, const std::filesystem::path& base, bool isDirectory, Ref<Texture2D> texture)
		: Path(path), IsDirectory(isDirectory), Texture(texture)
	{
		if (IsDirectory)
			Type = FileTypeDirectory;
		else
			Type = FileManager::GetFileType(Path);

		if (Preferences::GetData().ShowThumbnails)
		{
			if (Type == FileType::FileTypeTexture)
			{
				try
				{
					Texture = Texture2D::Create((base / path).string());
				}
				catch (...)
				{
					Texture = GetFileIcon(Type);
				}
			}
			else
				Texture = GetFileIcon(Type);
		}
		else
			Texture = GetFileIcon(Type);
	}
}