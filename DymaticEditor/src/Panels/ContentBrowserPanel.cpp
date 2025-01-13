#include "dypch.h"
#include "ContentBrowserPanel.h"

#include "EditorResources.h"

#include "Dymatic/Project/Project.h"
#include "Dymatic/Asset/AssetManager.h"
#include "Dymatic/Core/Filesystem.h"
#include "SourceControl.h"
#include "Settings/ProjectSettings.h"

#include "Thumbnails/ThumbnailManager.h"

#include "Fonts.h"
#include "TextSymbols.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/imgui_stdlib.h>

#include "Dymatic/UI/UI.h"
#include "Panels/UI.h"

#include "Tools/PythonTools.h"

#include <glm/gtc/type_ptr.hpp>

#include "Dymatic/Utils/PlatformUtils.h"
#include "Dymatic/Utils/AssimpInfoUtils.h"
#include "Dymatic/Math/StringUtils.h"

#include "Dymatic/Core/Input.h"

#include <spdlog/fmt/chrono.h>

#define INT_DIVISION_CEILING(dividend, divisor) ((dividend + divisor - 1) / divisor);

namespace Dymatic {

	static bool s_Init = false;

	ContentBrowserPanel::ContentBrowserPanel()
	{
		if (!s_Init)
		{
			s_Init = true;
			ThumbnailManager::Init();
		}

		if (Project::GetActive())
			Init();
	}

	void ContentBrowserPanel::Init()
	{
		m_BaseDirectory = Project::GetAssetDirectory();
		ThumbnailManager::UpdateProject();

		MoveToDirectory(m_BaseDirectory);
		m_Init = true;

		// Setup UI callback for finding asset
		UI::SetShowAssetSelectionCallback([this](AssetHandle handle) 
		{
			// Find specified asset
			if (!AssetManager::DoesAssetExist(handle))
				return;

			const std::filesystem::path& filepath = AssetManager::GetMetadata(handle).FilePath;

			// Navigate to directory
			MoveToDirectory((m_BaseDirectory / filepath.parent_path()).lexically_normal());

			// Make content browser window active
			Focus();

			// Select and search for asset
		});

		// Check all files are registered with the asset registry
		for (auto& directoryEntry : std::filesystem::recursive_directory_iterator(Project::GetAssetDirectory()))
			if (!directoryEntry.is_directory())
				CheckFileAssetData(std::filesystem::relative(directoryEntry.path(), m_BaseDirectory));

		// Verify the registry is not missing any asset files
		static std::vector<AssetMetadata> s_MissingAssets;
		static std::vector<uint8_t> s_MissingAssetSelection;
		const auto& registry = AssetManager::GetMetadataRegistry();
		for (const auto& [handle, metadata] : registry)
			if ((metadata.Type == AssetType::None) || (!metadata.MemoryOnly && !metadata.FilePath.empty() && !std::filesystem::exists(AssetManager::GetFileSystemPathString(metadata))))
				s_MissingAssets.push_back(metadata);

		// Prompt user to purge any missing assets
		if (!s_MissingAssets.empty())
		{
			s_MissingAssetSelection.resize(s_MissingAssets.size());
			EditorResources::SoundCompileFailure->Play();

			Popup::Create(FA_TRIANGLE_EXCLAMATION " Missing Asset References", "The following assets were not found during content initialization.",
			{
				{ "Cancel", []()
					{
						s_MissingAssets.clear(); s_MissingAssetSelection.clear();
					}
				},
				{ "Remove Selected References", [&]()
					{
						for (size_t i = 0; i < s_MissingAssets.size(); i++)
							if (s_MissingAssetSelection[i])
								AssetManager::RemoveAsset(s_MissingAssets[i].Handle, false);

						AssetManager::Serialize();

						s_MissingAssets.clear(); s_MissingAssetSelection.clear();
					}
				}
			}, nullptr, false, []()
			{
					ImGui::Separator();

					ImGui::BeginChild("##MissingAssetReferenceScrollRegion", ImVec2(0.0f, 225.0f));

					bool selectAll = std::all_of(s_MissingAssetSelection.begin(), s_MissingAssetSelection.end(), [](uint8_t v) { return (bool)v; });
					if (ImGui::Checkbox("##SelectAllCheckbox", &selectAll))
						for (auto& item : s_MissingAssetSelection)
							item = selectAll;

					ImGui::SameLine();
					ImGui::TextDisabled("Select All");
					ImGui::Separator();

					for (size_t i = 0; i < s_MissingAssets.size(); i++)
					{
						const auto& metadata = s_MissingAssets[i];

						ImGui::PushID(i);
						bool purgeMissingAsset = s_MissingAssetSelection[i];
						if (ImGui::Checkbox("##MissingAssetCheckbox", &purgeMissingAsset))
							s_MissingAssetSelection[i] = purgeMissingAsset;

						const std::string filepath = metadata.FilePath.lexically_normal().string();

						ImGui::SameLine();
						ImGui::Text(FileManager::GetFileTypeCharacterIcon(FileManager::GetFileType(metadata.Type)));
						ImGui::SameLine();
						if (s_MissingAssetSelection[i])
							ImGui::Text(filepath.c_str());
						else
							ImGui::TextDisabled(filepath.c_str());
						ImGui::PopID();
					}

					ImGui::EndChild();
			}, glm::vec2(500.0f, 250.0f));
		}
	}

	static ImVec4 s_SelectedColor = ImVec4(1.0f, 0.92f, 0.57f, 1.0f);

	static bool DoesRectOverlap(ImVec2 l1, ImVec2 r1, ImVec2 l2, ImVec2 r2)
	{
		if (l1.x < r2.x && r1.x > l2.x &&
			r1.y > l2.y && l1.y < r2.y)
			return true;
		return false;
	}

	static ImRect GetSelectionRect(ImVec2 origin)
	{
		// Construct the rect
		ImRect selectionRect = ImRect(ImGui::GetMousePos(), origin);

		// Ensure Rect min/max is in the correct order
		if (selectionRect.Min.x > selectionRect.Max.x)
			std::swap(selectionRect.Min.x, selectionRect.Max.x);

		if (selectionRect.Min.y > selectionRect.Max.y)
			std::swap(selectionRect.Min.y, selectionRect.Max.y);

		return selectionRect;
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
		case FileType::FileTypePrefab: return ImColor(55, 110, 186);
		case FileType::FileTypeScript: return ImColor(56, 163, 82);
		case FileType::FileTypeMeshSource: return ImColor(77, 137, 184);
		case FileType::FileTypeMesh: return ImColor(77, 137, 184);
		case FileType::FileTypeMaterial: return ImColor(56, 122, 70);
		case FileType::FileTypeMaterialInstance: return ImColor(56, 122, 70);
		case FileType::FileTypeTexture: return ImColor(145, 56, 54);
		case FileType::FileTypeEnvironmentMap: return ImColor(195, 64, 132);
		case FileType::FileTypeVirtualTexture: return ImColor(128, 64, 64);
		case FileType::FileTypeFont: return ImColor(99, 112, 53);
		case FileType::FileTypeAudio: return ImColor(99, 80, 150);
		case FileType::FileTypeParticleSystem: return ImColor(120, 120, 120);
		case FileType::FileTypeSkeleton: return ImColor(77, 137, 158);
		case FileType::FileTypeAnimation: return ImColor(80, 122, 72);
		case FileType::FileTypeAnimationGraph: return ImColor(150, 89, 12);
		case FileType::FileTypePhysicsMaterial: return ImColor(79, 77, 59);
		case FileType::FileTypeVideo: return ImColor(166, 15, 12);
		case FileType::FileTypeVideoPlayer: return ImColor(166, 15, 12);
		case FileType::FileTypeSubtitle: return ImColor(166, 15, 12);
		case FileType::FileTypeSolution: return ImColor(115, 81, 172);
		}

		return ImColor(65, 65, 65);
	};

	void ContentBrowserPanel::OnUpdate()
	{
		ThumbnailManager::OnUpdate();
	}

	void ContentBrowserPanel::OnImGuiRender(bool isDragging)
	{
		auto& contentBrowserVisible = Preferences::GetEditorWindowVisible(Preferences::EditorWindow::ContentBrowser);
		if (!contentBrowserVisible)
			return;

		if (m_Focus)
		{
			ImGui::SetNextWindowFocus();
			m_Focus = false;
		}

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

			if (ImGui::ImageButton((ImTextureID)EditorResources::NewIcon->GetRendererID(), { 20.0f, 20.0f }, { 0, 1 }, { 1, 0 }, -1, {}, ImGui::GetStyleColorVec4(ImGuiCol_Text)))
				ImGui::OpenPopup("##ContentBrowserCreatePopup");

			ImGui::SameLine();

			if (ImGui::ImageButton((ImTextureID)EditorResources::BackDirectoryIcon->GetRendererID(), { 20.0f, 20.0f }, { 0, 1 }, { 1, 0 }, -1, {}, ImGui::GetStyleColorVec4(m_DirectoryHistoryIndex == 0 ? ImGuiCol_TextDisabled : ImGuiCol_Text)))
				NavBackDirectory();
			ImGui::SameLine();
			if (ImGui::ImageButton((ImTextureID)EditorResources::ForwardDirectoryIcon->GetRendererID(), { 20.0f, 20.0f }, { 0, 1 }, { 1, 0 }, -1, {}, ImGui::GetStyleColorVec4(m_DirectoryHistoryIndex == m_DirectoryHistory.size() - 1 ? ImGuiCol_TextDisabled : ImGuiCol_Text)))
				NavForwardDirectory();

			ImGui::SameLine();
			bool hasParent = m_CurrentDirectory != std::filesystem::path(m_BaseDirectory);
			if (ImGui::ImageButton((ImTextureID)EditorResources::UpDirectoryIcon->GetRendererID(), { 20.0f, 20.0f }, { 0, 1 }, { 1, 0 }, -1, {}, ImGui::GetStyleColorVec4(hasParent ? ImGuiCol_Text : ImGuiCol_TextDisabled)))
				if (hasParent)
					MoveToDirectory(m_CurrentDirectory.parent_path());

			ImGui::SameLine();
			if (ImGui::ImageButton((ImTextureID)EditorResources::RefreshIcon->GetRendererID(), { 20.0f, 20.0f }, { 0, 1 }, { 1, 0 }, -1, {}, ImGui::GetStyleColorVec4(ImGuiCol_Text)))
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
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
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

						if (ImGui::ImageButton((ImTextureID)EditorResources::RightArrowIcon->GetRendererID(), ImVec2(10.0f, 10.0f), {0, 1}, {1, 0}))
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
			if (ImGui::ImageButton((ImTextureID)EditorResources::BrowserSettingsIcon->GetRendererID(), ImVec2(20.0f, 20.0f), { 0, 1 }, { 1, 0 }, -1, {}, ImGui::GetStyleColorVec4(ImGuiCol_Text)))
				ImGui::OpenPopup("##ContentBrowserSettingsPopup");
			if (ImGui::BeginPopup("##ContentBrowserSettingsPopup"))
			{
				ImGui::TextDisabled("Content Browser Settings");
				ImGui::Separator();
				if (ImGui::BeginMenu(FA_SORT " Sort"))
				{
					if (ImGui::MenuItem(FA_FOLDER " Directories First", "", &Preferences::GetData().DirectoriesFirst))
						UpdateDisplayFiles();
					ImGui::Separator();

					auto& sort = Preferences::GetData().ContentBrowserSortType;
					auto& ascending = Preferences::GetData().Ascending;

					if (ImGui::MenuItem(ascending ? "     " FA_ARROW_UP_Z_A " Name" : "     " FA_ARROW_DOWN_Z_A " Name")) { sort = Preferences::PreferencesData::ContentBrowserSortType::Name; UpdateDisplayFiles(); }
					if (sort == Preferences::PreferencesData::ContentBrowserSortType::Name)
						SelectedBubble();

					if (ImGui::MenuItem(ascending ? "     " FA_ARROW_UP_1_9 " Date" : "     " FA_ARROW_DOWN_1_9 " Date")) { sort = Preferences::PreferencesData::ContentBrowserSortType::Date; UpdateDisplayFiles(); }
					if (sort == Preferences::PreferencesData::ContentBrowserSortType::Date)
						SelectedBubble();

					if (ImGui::MenuItem(ascending ? "     " FA_ARROW_UP_SQUARE_TRIANGLE " Type" : "     " FA_ARROW_DOWN_SQUARE_TRIANGLE " Type")) { sort = Preferences::PreferencesData::ContentBrowserSortType::Type; UpdateDisplayFiles(); }
					if (sort == Preferences::PreferencesData::ContentBrowserSortType::Type)
						SelectedBubble();

					if (ImGui::MenuItem(ascending ? "     " FA_ARROW_UP_SMALL_BIG " Size" : "     " FA_ARROW_DOWN_SMALL_BIG " Size")) { sort = Preferences::PreferencesData::ContentBrowserSortType::Size; UpdateDisplayFiles(); }
					if (sort == Preferences::PreferencesData::ContentBrowserSortType::Size)
						SelectedBubble();

					ImGui::Separator();

					if (ImGui::MenuItem("     " FA_ARROW_UP_SHORT_WIDE " Ascending")) { ascending = true; UpdateDisplayFiles(); }
					if (ascending)
						SelectedBubble();

					if (ImGui::MenuItem("     " FA_ARROW_DOWN_SHORT_WIDE " Descending")) { ascending = false; UpdateDisplayFiles(); }
					if (!ascending)
						SelectedBubble();

					ImGui::EndMenu();
				}
				if (ImGui::BeginMenu(FA_FILTER " Filter"))
				{
					auto& preferences = Preferences::GetData();

					// Determine if all the filters are selected
					bool allSelected = true;
					for (uint32_t i = 0; i < FileType::FILE_TYPE_SIZE; i++)
					{
						if (!preferences.Filters[i])
						{
							allSelected = false;
							break;
						}
					}

					// Draw the 'All' filters option
					if (ImGui::MenuItem("     " FA_CHECK_DOUBLE " All"))
					{
						// Ensure all filters are selected
						for (uint32_t i = 0; i < FILE_TYPE_SIZE; i++)
							preferences.Filters[i] = true;

						// Update the display
						UpdateDisplayFiles();
					}

					if (allSelected)
						SelectedBubble();

					ImGui::Separator();

					// Draw all the other file types
					for (uint32_t index = 0; index < FILE_TYPE_SIZE; index++)
					{
						const FileType type = (FileType)index;
						if (ImGui::MenuItem(fmt::format("     {} {}", FileManager::GetFileTypeCharacterIcon(type), FileTypeToString(type)).c_str()))
						{
							// If we are not holding the 'shift' key, deselect all other filters.
							if (!ImGui::GetIO().KeyShift)
								for (uint32_t i = 0; i < FILE_TYPE_SIZE; i++)
									preferences.Filters[i] = false;

							// Select the specified filter regardless.
							preferences.Filters[index] = !preferences.Filters[index];

							// Update the display
							UpdateDisplayFiles();
						}
						
						// Draw a selection bubble if the option is selected
						if (preferences.Filters[FileType::FileTypeDirectory])
							SelectedBubble();
					}

					ImGui::EndMenu();
				}
				if (ImGui::BeginMenu(FA_LIST " View"))
				{
					if (ImGui::BeginMenu(FA_TABLE_COLUMNS " Layout"))
					{
						if (ImGui::MenuItem(FA_GRID " Grid", nullptr, Preferences::GetData().LayoutType == Preferences::PreferencesData::ContentBrowserLayoutType::Grid))
							Preferences::GetData().LayoutType = Preferences::PreferencesData::ContentBrowserLayoutType::Grid;
						if (ImGui::MenuItem(FA_LIST " List", nullptr, Preferences::GetData().LayoutType == Preferences::PreferencesData::ContentBrowserLayoutType::List))
							Preferences::GetData().LayoutType = Preferences::PreferencesData::ContentBrowserLayoutType::List;

						ImGui::EndMenu();
					}
					
					if (Preferences::GetData().LayoutType == Preferences::PreferencesData::ContentBrowserLayoutType::Grid)
					{
						if (ImGui::MenuItem(FA_IMAGE_POLAROID " Show Thumbnails", "", &Preferences::GetData().ShowThumbnails))
							UpdateDisplayFiles();
						ImGui::Text(FA_EXPAND " Thumbnail Size");
						ImGui::SameLine();
						ImGui::DragInt("##ThumbnailSizeInput", &Preferences::GetData().ThumbnailSize, 1, 16, 256);
					}
					else
					{
						ImGui::Text(FA_LINE_HEIGHT " List Spacing");
						ImGui::SameLine();
						ImGui::DragInt("##ListLayoutItemSpacingInput", &Preferences::GetData().ListItemSpacing, 1, 0, 100);
					}

					ImGui::EndMenu();
				}

				if (ImGui::MenuItem(FA_RIGHT_LEFT " Invert Selection"))
					InvertSelection();

				ImGui::EndPopup();
			}

			if (ImGui::BeginPopup("##DirectoryPreviewPopup"))
			{
				ImDrawList* drawList = ImGui::GetWindowDrawList();

				for (auto& file : std::filesystem::directory_iterator(m_DirectoryViewDropdownPath))
				{
					if (file.is_directory())
					{
						const ImVec2 folderDrawPosition = ImGui::GetCursorScreenPos() + ImVec2(0.0f, 0.0f);

						if (ImGui::MenuItem(fmt::format("         {}", file.path().filename().string()).c_str()))
							MoveToDirectory(file.path());

						const ImVec4 folderColor = glm::vec4(ProjectSettings::GetFolderColor(std::filesystem::relative(file.path(), m_BaseDirectory)), 1.0f);
						drawList->AddText(folderDrawPosition, ImGui::ColorConvertFloat4ToU32(folderColor), FA_FOLDER);
					}
				}
				ImGui::EndPopup();
			}

			if (ImGui::BeginPopup("##ContentBrowserCreatePopup"))
			{
				ImGui::TextDisabled(CHARACTER_ICON_ADD " Create");
				ImGui::Separator();
				DrawCreateMenu();
				ImGui::EndPopup();
			}

			ImGui::Separator();

			if (ImGui::BeginTable("##OuterTable", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_NoPadInnerX))
			{
				ImGui::TableNextColumn();

				// Draw the directory view side panel
				{
					ImGui::BeginChild("##DirectoryViewRegion");

					auto drawlist = ImGui::GetWindowDrawList();

					auto min = ImGui::GetWindowPos() + ImVec2(ImGui::GetColumnWidth(ImGui::GetColumnIndex()) - 20.0f, 0.0f);
					auto max = ImGui::GetWindowPos() + ImVec2(ImGui::GetColumnWidth(ImGui::GetColumnIndex()) + style.WindowPadding.x * 0.5f, ImGui::GetContentRegionAvail().y + 20.0f);

					ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));
					bool open = ImGui::TreeNodeEx("##ContentTreeNode", ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_OpenOnArrow, FA_FOLDER " Assets");

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
							MoveToDirectory(m_BaseDirectory / path);
						ImGui::TreePop();
					}

					ImVec4 color = ImGui::GetStyleColorVec4(ImGuiCol_BorderShadow);
					ImU32 color_full = ImGui::GetColorU32(color);
					ImU32 color_trans = ImGui::ColorConvertFloat4ToU32({ color.x, color.y, color.z, 0.0f });
					drawlist->AddRectFilledMultiColor(min, max, color_trans, color_full, color_full, color_trans);

					ImGui::EndChild();
				}

				ImGui::TableNextColumn();

				ImVec2 contentBrowserMin = ImGui::GetCursorScreenPos();
				ImVec2 contentBrowserSize = ImGui::GetContentRegionAvail();

				// Draw the search buffer
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

				ImGui::BeginChild("##ContentRegion");

				// Scroll back to the top when reloading states.
				if (m_ScrollToTop)
				{
					m_ScrollToTop = false;
					ImGui::SetScrollHereY();
				}

				// Handle renaming events
				if (Input::IsKeyPressed(Key::F2) && !m_SelectionContext.empty())
					RenameFile(m_SelectionContext.back().Path);

				// Reset the context event variables prior to rendering.
				m_ContextIndex = 0;
				m_ContextPath.clear();

				// Draw the displayed files in the specified layout
				switch (Preferences::GetData().LayoutType)
				{
				case Preferences::PreferencesData::ContentBrowserLayoutType::Grid: DrawGridLayout(); break;
				case Preferences::PreferencesData::ContentBrowserLayoutType::List: DrawListLayout(); break;
				}

				// Handle the selection context
				if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
				{
					ClearSelectionContext();

					m_SelectionPosition = ImGui::GetMousePos();
				}

				// Render Selection Highlight (only if has been moved more than a pixel)
				if (IsDragSelecting() && (m_SelectionPosition.x != ImGui::GetMousePos().x && m_SelectionPosition.y != ImGui::GetMousePos().y))
				{
					const ImRect selectionRect = GetSelectionRect(m_SelectionPosition);

					// Draw the transparent selection
					ImDrawList* drawlist = ImGui::GetWindowDrawList();
					drawlist->AddRectFilled(selectionRect.Min, selectionRect.Max, ImGui::GetColorU32(ImGuiCol_TextSelectedBg));

					// Draw the solid outline
					ImVec4 outline_color = ImGui::GetStyleColorVec4(ImGuiCol_TextSelectedBg);
					outline_color.x *= 1.2f;
					outline_color.y *= 1.2f;
					outline_color.z *= 1.2f;
					outline_color.w *= 1.2f;
					drawlist->AddRect(selectionRect.Min, selectionRect.Max, ImGui::GetColorU32(outline_color), 0.0f, ImDrawFlags_None, 2.0f);
				}
				
				// If we've just stopped dragging then clear the selection position
				if (!ImGui::IsMouseDown(ImGuiMouseButton_Left) && IsDragSelecting())
					m_SelectionPosition = { -1.0f, -1.0f };

				// Handle context events after rendering of the interface has finished:
				if (m_ContextIndex)
				{
					switch (m_ContextIndex)
					{
					case 1: MoveToDirectory(m_CurrentDirectory / m_ContextPath.filename()); break;
					case 2:
						for (auto& item : m_SelectionContext)
						{
							if (item.IsDirectory)
							{
								for (auto& file : std::filesystem::recursive_directory_iterator(m_BaseDirectory / item.Path))
									AssetManager::RemoveAsset(std::filesystem::relative(file, m_BaseDirectory), false);

								AssetManager::Serialize();
							}
							else
							{
								AssetManager::RemoveAsset(item.Path);
							}
							DeleteFile(m_BaseDirectory / item.Path, false);
						}

						UpdateDisplayFiles();
						break;
					case 3: m_SearchbarBuffer.clear(); MoveToDirectory(m_ContextPath); break;
					case 4: UpdateDisplayFiles(); break;
					}
				}

				// Window Context menu for item creation etc.
				if (ImGui::BeginPopupContextWindow("##ContentBrowserContext", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
				{
					if (ImGui::BeginMenu(CHARACTER_ICON_ADD " Create"))
					{
						DrawCreateMenu();
						ImGui::EndMenu();
					}

					if (ImGui::MenuItem(CHARACTER_ICON_IMPORT " Import...")) ImportExternalFiles();

					if (ImGui::MenuItem(CHARACTER_ICON_PASTE " Paste")) PasteFile();

					ImGui::Separator();

					if (ImGui::MenuItem(FA_FOLDER_TREE " Show in Explorer"))
						OpenExternally(m_CurrentDirectory);

					ImGui::EndPopup();
				}

				ImGui::GetWindowDrawList()->AddText(ImGui::GetWindowPos() + ImVec2(ImGui::GetCursorPos().x, ImGui::GetWindowSize().y - ImGui::GetFontSize() - style.FramePadding.y), IM_COL32_WHITE, m_DisplayFiles.size() == 1 ? "1 item" : (std::to_string(m_DisplayFiles.size()) + " items").c_str());
				ImGui::EndChild();

				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCENE_HIERARCHY_ENTITY"))
					{
						Entity entity = *((Entity*)payload->Data);
						const auto& tag = entity.GetComponent<TagComponent>().Tag;
						Ref<Prefab> prefab = CreateNewAsset<Prefab>(fmt::format("{}.dyprefab", tag), entity);

						if (!entity.HasComponent<PrefabComponent>())
							entity.AddComponent<PrefabComponent>();

						entity.GetComponent<PrefabComponent>().PrefabID = prefab->Handle;
					}

					ImGui::EndDragDropTarget();
				}

				if (m_DisplayFiles.empty())
				{
					const char* text = "No files or folders found, create or import them.";
					drawlist->AddText(contentBrowserMin + ((contentBrowserSize - ImGui::CalcTextSize(text)) * 0.5f), ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_Text)), text);
				}

				// TODO: status bar
				ImGui::EndTable();
			}

			ImGui::EndChild();
		}
		else
		{
			// Content browser has not been initialized
			const char* text = CHARACTER_ICON_OPEN_PROJECT " No project is currently opened.";
			ImGui::GetWindowDrawList()->AddText(ImGui::GetWindowPos() + ((ImGui::GetWindowSize() - ImGui::CalcTextSize(text)) * 0.5f), ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_Text)), text);
		}

		ImGui::End();
	}

	void ContentBrowserPanel::DrawGridLayout()
	{
		ImGuiStyle& style = ImGui::GetStyle();

		const float padding = 16.0f;
		const float thumbnailSize = Preferences::GetData().ThumbnailSize;
		const float cellSize = thumbnailSize + padding;

		float panelWidth = ImGui::GetContentRegionAvail().x;
		const uint32_t columnCount = std::clamp((int)(panelWidth / cellSize), 1, IMGUI_TABLE_MAX_COLUMNS);

		ImGui::Dummy({ (ImGui::GetContentRegionAvail().x - columnCount * cellSize - padding * 1.0f) * 0.5f, 0.0f });
		ImGui::SameLine();

		if (ImGui::BeginTable("##Innertable", columnCount))
		{
			ImDrawList* drawList = ImGui::GetWindowDrawList();

			const bool isSelecting = IsDragSelecting();

			// Calculate thumbnail constants
			const uint32_t itemCount = m_DisplayFiles.size();
			const float frameRoundingMultiplier = 3.0f;
			const float thumbnailAspectRatio = 1.5f;
			const float thumbnailWidth = thumbnailSize;
			const float thumbnailHeight = thumbnailSize * thumbnailAspectRatio;
			const float cellHeight = thumbnailHeight + style.FramePadding.y * 2.0f;
			const float rowHeight = cellHeight + padding;
			const uint32_t rowCount = INT_DIVISION_CEILING(itemCount, columnCount);

			ImGuiTable* table = ImGui::GetCurrentTable();

			// Iterate over and handle all the displayed files.
			// Note: We manually specify the clipper row height to avoid any issues with folders
			ImGuiListClipper clipper;
			clipper.Begin(rowCount, rowHeight);
			while (clipper.Step())
			{
				ImGui::TableNextColumn();
				for (size_t fileIndex = clipper.DisplayStart * columnCount; (fileIndex < clipper.DisplayEnd * columnCount) && (fileIndex < itemCount); fileIndex++)
				{
					table->RowMinHeight = 100.0f;

					// Retrieve the file entry at the current index
					auto& file = m_DisplayFiles[fileIndex];

					ImGui::PushID(file.Path.filename().string().c_str());

					const float originalCellY = ImGui::GetCursorPosY();

					if (file.IsDirectory)
					{
						ImGui::PushStyleColor(ImGuiCol_Button, {});
						glm::vec3 color = ProjectSettings::GetFolderColor(file.Path);
						ImGui::ImageButton((ImTextureID)EditorResources::DirectoryIcon->GetRendererID(), { thumbnailSize, thumbnailSize }, { 0, 1 }, { 1, 0 }, -1, ImVec4(0, 0, 0, 0), ImVec4(color.x, color.y, color.z, 1.0f));
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
						ImGui::GetWindowDrawList()->AddRect(itemMin, itemMax, IsFileSelected(file.Path) ? ImGui::GetColorU32(s_SelectedColor) : ((ImGui::IsItemHovered() && !isSelecting) ? ImGui::GetColorU32(ImVec4(0.5f, 0.5f, 0.5f, 1.0f)) : ImGui::GetColorU32(ImGuiCol_WindowShadow)), style.FrameRounding, ImDrawFlags_None, (ImGui::IsItemHovered() && !isSelecting) ? 2.0f : 1.0f);
					}
					else
					{
						ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, style.FrameRounding * frameRoundingMultiplier);
						ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_Header));
						ImGui::ButtonCornersEx("##ItemButton", ImVec2(thumbnailWidth, thumbnailHeight), ImGuiButtonFlags_None, ImDrawFlags_RoundCornersBottom);
						ImGui::PopStyleColor();
						ImGui::PopStyleVar();

						const ImVec2 itemMin = ImGui::GetItemRectMin();
						const ImVec2 itemMax = ImGui::GetItemRectMax();
						const ImVec2 itemSize = ImGui::GetItemRectSize();

						ImGui::GetWindowDrawList()->AddRectFilled(itemMin, itemMin + ImVec2(thumbnailSize, thumbnailSize), ImGui::GetColorU32(ImGuiCol_Button));

						Ref<Texture2D> icon;

						// Find the appropriate icon (if we are displaying thumbnails)
						if (!Preferences::GetData().ShowThumbnails || file.Type == FileType::FileTypeFile || file.Type == FileType::FileTypeDirectory)
							icon = GetFileTypeIcon(file.Type);
						else if (auto thumbnail = ThumbnailManager::GetOrCreateThumbnail(AssetManager::GetMetadata(file.Path).Handle))
							icon = thumbnail;
						else
							icon = GetFileTypeIcon(file.Type);

						// Render the icon
						ImGui::GetWindowDrawList()->AddImage((ImTextureID)icon->GetRendererID(), itemMin, itemMin + ImVec2(thumbnailSize, thumbnailSize), { 0, 1 }, { 1, 0 });

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
						UI::PushFont(FontType::Bold);
						ImGui::SetWindowFontScale(0.85f);
						const char* typeText = FileTypeToUpperString(file.Type);
						const ImVec2 fontSize = ImGui::CalcTextSize(typeText);
						ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(itemMin.x, itemMax.y - fontSize.y - style.FramePadding.y * 2.0f), itemMax, GetFileTypeColor(file.Type), style.FrameRounding * frameRoundingMultiplier, ImDrawFlags_RoundCornersBottom);
						ImGui::GetWindowDrawList()->AddText(itemMax - fontSize - style.FramePadding * 1.5f, ImGui::GetColorU32(ImGuiCol_Text), typeText);
						ImGui::SetWindowFontScale(1.0f);
						UI::PopFont();

						// Render source control icons
						if (SourceControl::IsActive())
						{
							auto unstagedStatus = SourceControl::GetFileUnstagedStatus(Project::GetRelativeAssetDirectory() / file.Path);
							auto stagedStatus = SourceControl::GetFileStagedStatus(Project::GetRelativeAssetDirectory() / file.Path);
							if (unstagedStatus != SourceControl::Status::None)
							{
								ImVec2 iconSize = { 16.0f, 16.0f };
								ImVec2 iconPos = itemMin - style.FramePadding + ImVec2(thumbnailSize - iconSize.x * 2.0f, style.FramePadding.y * 2.0f);
								ImGui::GetWindowDrawList()->AddImage((ImTextureID)(unstagedStatus == SourceControl::Status::Added ? EditorResources::SourceControlUntrackedIcon : EditorResources::SourceControlModifiedIcon)->GetRendererID(), iconPos, iconPos + iconSize, { 0, 1 }, { 1, 0 }, ImGui::GetColorU32(ImVec4(0.9f, 0.1f, 0.2f, 1.0f)));
							}
							if (stagedStatus != SourceControl::Status::None)
							{
								ImVec2 iconSize = { 16.0f, 16.0f };
								ImVec2 iconPos = itemMin - style.FramePadding + ImVec2(thumbnailSize - iconSize.x * 1.0f, style.FramePadding.y * 2.0f);
								ImGui::GetWindowDrawList()->AddImage((ImTextureID)(stagedStatus == SourceControl::Status::Added ? EditorResources::SourceControlAddedIcon : EditorResources::SourceControlModifiedIcon)->GetRendererID(), iconPos, iconPos + iconSize, { 0, 1 }, { 1, 0 }, ImGui::GetColorU32(ImVec4(0.1f, 0.8f, 0.2f, 1.0f)));
							}
						}

						// Render Item Outline
						ImGui::GetWindowDrawList()->AddRect(itemMin, itemMax, IsFileSelected(file.Path) ? ImGui::GetColorU32(s_SelectedColor) : ((ImGui::IsItemHovered() && !isSelecting) ? ImGui::GetColorU32(ImVec4(0.5f, 0.5f, 0.5f, 1.0f)) : ImGui::GetColorU32(ImGuiCol_WindowShadow)), style.FrameRounding * frameRoundingMultiplier, ImDrawFlags_RoundCornersBottom, (ImGui::IsItemHovered() && !isSelecting) ? 2.0f : 1.0f);

						// Render Item Shadow
						ImDrawFlags draw_flags = ImDrawFlags_RoundCornersBottom | ImDrawFlags_ShadowCutOutShapeBackground;
						ImGui::GetWindowDrawList()->AddShadowRect(itemMin, itemMax, ImGui::GetColorU32(ImGuiCol_WindowShadow), 20.0f, ImVec2(5.0f, 5.0f), draw_flags, style.FrameRounding * frameRoundingMultiplier);
					}

					DrawLayoutItemContextMenu(file);

					// Add padding below to ensure that the overall height is consistent (e.g. in case we have a full row of folders)
					// Note: There is a small error in this padding calculation which results in a small jitter when above items go off screen (can diagnose by logging below value).
					// The value of 2.0f is there to compensate for this at the default scaling. This is negligible but an error regardless.
					const float cellDeltaY = ImGui::GetCursorPosY() - originalCellY;
					ImGui::Dummy(ImVec2(0.0f, cellHeight - cellDeltaY - 2.0f));

					// Padding is placed for spacing above
					ImGui::Dummy(ImVec2(0.0f, padding));

					ImGui::TableNextColumn();

					ImGui::PopID();
				}
			}

			ImGui::EndTable();
		}
	}

	void ContentBrowserPanel::DrawListLayout()
	{
		ImGuiStyle& style = ImGui::GetStyle();
		const float lineHeight = ImGui::GetTextLineHeight();
		const float padding = 5.0f;
		const float sidePadding = 20.0f;
		const int listItemSpacing = Preferences::GetData().ListItemSpacing;

		ImDrawList* drawList = ImGui::GetWindowDrawList();

		const float width = ImGui::GetContentRegionAvail().x;
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + sidePadding);

		const ImGuiTableFlags flags = ImGuiTableFlags_PadOuterX | ImGuiTableFlags_NoBordersInBody;
		if (ImGui::BeginTable("##ContentBrowserListLayoutTable", 4, flags, ImVec2(ImGui::GetContentRegionAvailWidth() - width, 0.0f)))
		{
			ImGui::TableSetupColumn(FA_TAG " Name", ImGuiTableColumnFlags_WidthStretch, 400.0f);
			ImGui::TableSetupColumn(FA_FILE " Type", ImGuiTableColumnFlags_WidthFixed, 200.0f);
			ImGui::TableSetupColumn(FA_CLOCK " Date Modified", ImGuiTableColumnFlags_WidthFixed, 200.0f);
			ImGui::TableSetupColumn(FA_BOX " Size", ImGuiTableColumnFlags_WidthFixed, 100.0f);
			ImGui::TableSetupScrollFreeze(4, 1);
			ImGui::TableHeadersRow();

			ImDrawList* drawList = ImGui::GetWindowDrawList();

			ImGuiListClipper clipper;
			// We add an extra tab at the bottom
			clipper.Begin(m_DisplayFiles.size());
			while (clipper.Step())
			{
				for (uint32_t i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
				{
					auto& file = m_DisplayFiles[i];

					ImGui::PushID(i);

					ImGui::TableNextRow();
					ImGui::TableNextColumn();

					// Draw selectable
					bool even = i % 2 == 0;
					UI::ScopedStyleColor headerStyleColor(ImGuiCol_HeaderActive, ImGui::GetStyleColorVec4(ImGuiCol_HeaderActive) * ImVec4(0.95f, 0.95f, 0.95f, 1.0f), even);
					ImGui::Selectable("##FileSelectable", even, ImGuiSelectableFlags_SpanAllColumns, ImVec2(0.0f, lineHeight + padding));				

					// Draw color line along the bottom corresponding to type
					if (!file.IsDirectory)
					{
						const ImVec2 itemMin = ImGui::GetItemRectMin();
						const ImVec2 itemMax = ImGui::GetItemRectMax();

						drawList->AddRectFilled(ImVec2(itemMin.x, itemMax.y - 2.0f), itemMax, ImGui::GetColorU32(GetFileTypeColor(file.Type).Value));
					}

					// Draw outline if folder is selected
					if (IsFileSelected(file.Path))
					{
						const ImVec2 itemMin = ImGui::GetItemRectMin();
						const ImVec2 itemMax = ImGui::GetItemRectMax();
						drawList->AddRect(itemMin, itemMax + ImVec2(0.0f, 1.0f), ImGui::GetColorU32(s_SelectedColor), 1.0f, 0, 1.0f);
					}

					// Get the folder color
					const ImVec4 iconColor = file.IsDirectory ? glm::vec4(ProjectSettings::GetFolderColor(file.Path), 1.0f) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

					// Context Menus and Tooltips
					DrawLayoutItemContextMenu(file);

					// Draw the type icon and the filename
					ImGui::SameLine();
					ImGui::TextColored(iconColor, FileManager::GetFileTypeCharacterIcon(file.Type));
					ImGui::SameLine();

					if (m_RenameContext == file.Path)
						DrawRenameInput(file.Path, true);
					else
						ImGui::Text(file.Path.stem().string().c_str());

					// Add line spacing in the first column
					if (listItemSpacing != 0)
						ImGui::ItemSize(ImVec2(0.0f, listItemSpacing));

					ImGui::TableNextColumn();

					ImGui::Text(FileTypeToString(file.Type));

					ImGui::TableNextColumn();

					ImGui::Text(file.LastWriteTime.c_str());

					ImGui::TableNextColumn();
					
					if (!file.IsDirectory)
						ImGui::Text(String::FormatBytes(file.Size).c_str());

					ImGui::PopID();
				}
			}

			ImGui::EndTable();
		}
	}

	void ContentBrowserPanel::DrawLayoutItemContextMenu(const FileEntry& file)
	{
		const ImGuiStyle& style = ImGui::GetStyle();

		// Check if the file entry has been double clicked
		if (!file.IsDirectory && ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
			m_OpenFileEditorCallback(file.Path);

		// Handle Selection when we stop
		const bool stoppingSelection = ImGui::IsMouseReleased(0) && IsDragSelecting();
		if (stoppingSelection)
		{
			const ImVec2& min = ImGui::GetItemRectMin();
			const ImVec2& max = ImGui::GetItemRectMax();
			const ImRect& selectionRect = GetSelectionRect(m_SelectionPosition);
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
				{
					// Find the index of the selected item
					uint32_t selectionIndex = 0;
					while (selectionIndex < m_DisplayFiles.size() && m_DisplayFiles[selectionIndex].Path != file.Path)
						selectionIndex++;
					
					// Iterate over all files until we find the index for the back of the selection context of the selection context
					uint32_t backIndex = 0;
					while (backIndex < m_DisplayFiles.size() && m_DisplayFiles[backIndex].Path != m_SelectionContext.back().Path)
						backIndex++;

					// Determine the direction to move during selection
					const int direction = (backIndex < selectionIndex ? 1 : -1);

					// Move in the specified direction until reaching the target (inclusive)
					uint32_t index = backIndex;
					while (index != selectionIndex + direction)
					{
						SetSelectionContext(m_DisplayFiles[index], true);
						index += direction;
					}
				}
			}
			else if (!IsFileSelected(file.Path))
				SetSelectionContext(file);
		}

		if (file.IsDirectory)
		{
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
		}

		if (ImGui::BeginDragDropSource())
		{
			// Draw the dragging tooltip.
			DrawItemTooltipContents(m_SelectionContext[0], true);

			// Set the drag drop payload.
			const wchar_t* itemPath = (m_SelectionContext[0].Path).c_str();
			ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", itemPath, (wcslen(itemPath) + 1) * sizeof(wchar_t));
			ImGui::EndDragDropSource();
		}

		if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
		{
			if (file.IsDirectory)
			{
				m_ContextIndex = 1;
				m_ContextPath = file.Path;
			}
		}

		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem(FA_FILE " Open"))
			{
				if (file.IsDirectory)
					MoveToDirectory(m_BaseDirectory / file.Path);
				else
					m_OpenFileEditorCallback(file.Path);
			}
			if (ImGui::MenuItem(FA_PEN " Rename")) { RenameFile(file.Path); }
			if (ImGui::MenuItem(FA_COPY " Duplicate")) DuplicateFile(m_BaseDirectory / file.Path);
			if (ImGui::MenuItem(CHARACTER_ICON_COPY " Copy")) CopyFile(m_BaseDirectory / file.Path);
			if (ImGui::MenuItem(CHARACTER_ICON_PASTE " Paste")) PasteFile();
			if (ImGui::MenuItem(CHARACTER_ICON_DELETE " Delete")) { m_ContextIndex = 2; m_ContextPath = file.Path; }

			ImGui::Separator();

			// Asset specific actions
			{
				bool hasAssetActions = true;
				if (file.Type == FileType::FileTypeMaterial || file.Type == FileType::FileTypeMaterialInstance)
				{
					if (ImGui::MenuItem(FILE_ICON_MATERIAL_INSTANCE " Create Material Instance"))
					{
						std::filesystem::path directory = m_BaseDirectory / file.Path.parent_path();
						std::filesystem::path path = (std::filesystem::relative(directory, m_BaseDirectory) / FileManager::GetNextOfNameInDirectory(file.Path.stem().concat(" Instance.dymateriali"), directory)).lexically_normal();
						Ref<MaterialAsset> materialParent = AssetManager::GetAsset<MaterialAsset>(file.Path.lexically_normal());
						AssetManager::CreateNewAsset<MaterialInstance>(path, materialParent);
						UpdateDisplayFiles();
						RenameFile(path);
					}
				}
				else if (file.Type == FileType::FileTypeMeshSource)
				{
					if (ImGui::MenuItem(FA_FILE_PLUS " Create from Source..."))
					{
						static bool s_ImportMesh;
						static int s_MeshType;
						static AssetHandle s_SkeletonHandle;
						static std::vector<AssimpInfo::AnimationInfo> s_AnimationInfo;
						static std::vector<uint8_t> s_AnimationImport;

						s_ImportMesh = true;
						s_MeshType = 0;
						s_SkeletonHandle = 0;
						s_AnimationInfo = AssimpInfo::GetAnimationInfo(m_BaseDirectory / file.Path);
						s_AnimationImport = std::vector<uint8_t>(s_AnimationInfo.size(), 1);

						Popup::Create("Create from Mesh Source", std::string(),
							{
								ButtonData("Cancel"),
								ButtonData("Create", [&]()
								{
									AssetHandle sourceHandle = AssetManager::GetAssetHandleFromFilePath(file.Path);

									// TODO: Add constructors to meshes, animations and skeletons such that we don't actually load the data as this is unnesssiary

									// Create or get skeleton
									Ref<Skeleton> skeleton = nullptr;
									if (!s_ImportMesh || s_MeshType == 1)
									{
										if (s_SkeletonHandle == 0)
											skeleton = AssetManager::CreateNewAsset<Skeleton>(file.Path.parent_path() / (file.Path.stem().string() + ".dyskeleton"), sourceHandle);
										else
											skeleton = AssetManager::GetAsset<Skeleton>(s_SkeletonHandle);
									}

									// Create mesh
									if (s_ImportMesh)
										AssetManager::CreateNewAsset<Model>(file.Path.parent_path() / (file.Path.stem().string() + ".dymesh"), sourceHandle, skeleton);

									// Create animations
									for (uint32_t i = 0; i < s_AnimationInfo.size(); i++)
									{
										const auto& animation = s_AnimationInfo[i];

										if (s_AnimationImport[i])
										{
											AssetManager::CreateNewAsset<Animation>(fmt::format("{}_{}.dyanim", (file.Path.parent_path() / file.Path.stem()).string(), i), sourceHandle, i, skeleton);
										}
									}

									UpdateDisplayFiles();
								})
							},
							nullptr, false,
							[&]()
							{
								// TODO: Add options for normal and other settings etc

								ImGui::Text("Import Mesh");
								ImGui::SameLine();
								ImGui::Checkbox("##ImportMeshCheckbox", &s_ImportMesh);

								if (s_ImportMesh)
								{
									const char* meshTypeNames[] = { "Static Mesh", "Skeletal Mesh"};
									ImGui::Text("Mesh Type");
									if (ImGui::BeginCombo("##MeshTypeDropdown", meshTypeNames[s_MeshType]))
									{
										if (ImGui::Selectable(meshTypeNames[0], s_MeshType == 0))
											s_MeshType = 0;
										if (ImGui::Selectable(meshTypeNames[1], s_MeshType == 1))
											s_MeshType = 1;
										
										ImGui::EndCombo();
									}
								}

								if (s_MeshType == 1 || !s_ImportMesh)
								{
									ImGui::Separator();

									ImGui::Text("Skeleton");
									if (!s_SkeletonHandle)
										ImGui::TextDisabled("No skeleton has been specified, a new skeleton will be created for this mesh.");
									UI::DrawAssetSelectionDropdown(AssetType::Skeleton, s_SkeletonHandle, [](AssetHandle handle)
									{
										s_SkeletonHandle = handle;
									});

									ImGui::Separator();

									ImGui::Text("Animations");

									for (uint32_t i = 0; i < s_AnimationInfo.size(); i++)
									{
										ImGui::PushID(i);
										const auto& animation = s_AnimationInfo[i];
										
										ImGui::Checkbox("##ImportAnimationCheckbox", (bool*)(&s_AnimationImport[i]));
										ImGui::SameLine();
										ImGui::Text(animation.Name.c_str());
										ImGui::SameLine();
										std::string buffer;
										ImGui::InputText("##ImportAnimationPathName", &buffer);
										ImGui::PopID();
									}
									
								}

								ImGui::Dummy(ImVec2(0.0f, ImGui::GetContentRegionAvail().y - 40.0f));

							},
							glm::vec2(450.0f, 550.0f)
						);
					}
				}
				else if (file.Type == FileType::FileTypeAudio)
				{
					if (ImGui::MenuItem(FA_VOLUME " Play Audio"))
					{
						if (Ref<Audio> audio = AssetManager::GetAsset<Audio>(file.Path.lexically_normal()))
							audio->Play();
					}
				}
				else if (file.Type == FileType::FileTypeZipArchive)
				{
					ImGui::MenuItem(FA_FILE_ZIPPER " Extract");
				}
				else
					hasAssetActions = false;

				if (hasAssetActions)
					ImGui::Separator();
			}

			if (ImGui::MenuItem(FA_SQUARE_UP_RIGHT " Open Externally")) { OpenExternally(m_BaseDirectory / file.Path); }

			if (ImGui::MenuItem(FA_FOLDER " Show in Folder View")) { m_ContextIndex = 3; m_ContextPath = (m_BaseDirectory / file.Path).parent_path(); }
			if (ImGui::MenuItem(FA_FOLDER_TREE " Show in Explorer")) { OpenExternally(m_BaseDirectory / file.Path.parent_path()); }

			ImGui::Separator();

			if (ImGui::MenuItem(FA_FOLDER_PLUS " Copy File Path"))
				ImGui::SetClipboardText(std::filesystem::absolute(file.Path).string().c_str());

			const AssetHandle handle = AssetManager::GetAssetHandleFromFilePath(file.Path);
			if (handle)
			{
				if (ImGui::MenuItem(FA_KEY " Copy Asset Handle"))
					ImGui::SetClipboardText(fmt::format("{}", handle).c_str());

				if (ImGui::MenuItem(FA_ROTATE_RIGHT " Force Reload Asset"))
				{
					// Trigger a forced reload in the asset system
					switch (file.Type)
					{
					//case FileTypeScene: AssetManager::ForceReloadAsset<Scene>(handle); break;
					//case FileTypePrefab: AssetManager::ForceReloadAsset<Prefab>(handle); break;
					case FileTypeMesh: AssetManager::ForceReloadAsset<Model>(handle); break;
					case FileTypeMaterial: AssetManager::ForceReloadAsset<MaterialSource>(handle); break;
					case FileTypeMaterialInstance: AssetManager::ForceReloadAsset<MaterialInstance>(handle); break;
					case FileTypeTexture: AssetManager::ForceReloadAsset<Texture2D>(handle); break;
					case FileTypeEnvironmentMap: AssetManager::ForceReloadAsset<EnvironmentMap>(handle); break;
					case FileTypeFont: AssetManager::ForceReloadAsset<Font>(handle); break;
					case FileTypeAudio: AssetManager::ForceReloadAsset<Audio>(handle); break;
					case FileTypeSkeleton: AssetManager::ForceReloadAsset<Skeleton>(handle); break;
					case FileTypeAnimation: AssetManager::ForceReloadAsset<Animation>(handle); break;
					case FileTypeAnimationGraph: AssetManager::ForceReloadAsset<AnimationGraph>(handle); break;
					}

					ThumbnailManager::InvalidateThumbnail(handle);
				}
			}

			if (file.IsDirectory)
			{
				if (ImGui::BeginMenu(FA_PALETTE " Set Color"))
				{
					glm::vec3 color = ProjectSettings::HasFolderColor(file.Path) ? ProjectSettings::GetFolderColor(file.Path) : glm::vec3(1.0f);
					if (ImGui::ColorEdit3("##SetFolderColor", glm::value_ptr(color)))
						ProjectSettings::SetFolderColor(file.Path, color);
					ImGui::EndMenu();
				}
			}

			PythonTools::OnImGuiRenderContentBrowser(file.Path.extension().string());

			if (SourceControl::IsActive())
			{
				if (ImGui::BeginMenu(FA_CODE_BRANCH " Source Control"))
				{
					if (ImGui::MenuItem(FA_CODE_COMMIT "Diff"))
						;

					if (ImGui::MenuItem(FA_TRASH " Discard"))
					{
						SourceControl::DiscardFile(Project::GetRelativeAssetDirectory() / file.Path);
						m_ContextIndex = 4;
					}
					if (SourceControl::IsUnstaged(Project::GetRelativeAssetDirectory() / file.Path))
					{
						if (ImGui::MenuItem(FA_FILE_CHECK " Stage"))
							SourceControl::StageFile(Project::GetRelativeAssetDirectory() / file.Path);
					}
					if (SourceControl::IsStaged(Project::GetRelativeAssetDirectory() / file.Path))
					{
						if (ImGui::MenuItem(FA_FILE_XMARK " Unstage"))
							SourceControl::UnstageFile(Project::GetRelativeAssetDirectory() / file.Path);
					}
					if (SourceControl::IsStaged(Project::GetRelativeAssetDirectory() / file.Path))
					{
						if (ImGui::MenuItem(FA_CODE_MERGE " Check In"))
						{
							Popup::Create("Source Control", "Check Files in to source control:", {}, nullptr, false, [this]()
							{
								SourceControlCommitUI(m_SelectionContext);
							}, glm::vec2(400.0f, 225.0f));
						}
					}
					ImGui::EndMenu();
				}
			}

			ImGui::EndPopup();
		}

		if (UI::IsItemHoveredTooltip())
		{
			ImGui::BeginTooltip();

			DrawItemTooltipContents(file);

			ImGui::EndTooltip();
		}
	}

	void ContentBrowserPanel::DrawItemTooltipContents(const FileEntry& file, bool isDragging)
	{
		const ImGuiStyle& style = ImGui::GetStyle();
		const float height = ImGui::GetTextLineHeight();

		// Only display a thumbnail is one has/can be generated for the asset
		if (Preferences::GetData().ShowThumbnails && AssetManager::DoesAssetExist(file.Path))
		{
			if (Ref<Texture2D> thumbnail = ThumbnailManager::GetOrCreateThumbnail(AssetManager::GetMetadata(file.Path).Handle))
			{
				ImGui::Image((ImTextureID)thumbnail->GetRendererID(), ImVec2(height, height) * 2.0f, { 0, 1 }, { 1, 0 });
				ImGui::SameLine();
			}
		}

		ImGui::BeginGroup();

		ImGui::Dummy(ImVec2(0.0f, style.FramePadding.y * 2.0f));
		ImGui::TextColored(file.IsDirectory ? glm::vec4(ProjectSettings::GetFolderColor(file.Path), 1.0f) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f), FileManager::GetFileTypeCharacterIcon(file.Type));
		ImGui::SameLine();

		std::string filenameString = file.Path.stem().string();

		const uint32_t additionalItems = m_SelectionContext.size() - 1;
		const bool displayAdditional = isDragging && additionalItems;
		ImGui::Text((displayAdditional ? fmt::format("'{}' and {} {}", filenameString, additionalItems, additionalItems == 1 ? "other" : "others") : filenameString).c_str());

		ImGui::Dummy(ImVec2(0.0f, style.FramePadding.y * 2.0f));
		ImGui::EndGroup();

		// Show properties if the alt key is held
		if (!isDragging && !file.IsDirectory && (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl)))
		{
			ImGui::Separator();
			ImGui::TextDisabled(FA_SLIDERS " Properties");

			ImGui::TextDisabled(FA_FILE_LINES " Path: ");
			ImGui::SameLine();
			ImGui::Text(file.Path.string().c_str());

			ImGui::TextDisabled(FA_FILE_LINES " Type: ");
			ImGui::SameLine();
			ImGui::Text(FileTypeToString(file.Type));

			ImGui::TextDisabled(FA_FILE_SIGNATURE " Extension: ");
			ImGui::SameLine();
			ImGui::Text(file.Path.extension().string().c_str());

			ImGui::TextDisabled(FA_BOX " Size: ");
			ImGui::SameLine();
			ImGui::Text(String::FormatBytes(file.Size).c_str());

			ImGui::TextDisabled(FA_CLOCK " Last Write Time: ");
			ImGui::SameLine();
			ImGui::Text(file.LastWriteTime.c_str());

			const uint64_t handle = (uint64_t)AssetManager::GetAssetHandleFromFilePath(file.Path);
			const bool exists = handle != 0;

			ImGui::TextDisabled(FA_RECTANGLE_LIST " Registry: ");
			ImGui::SameLine();
			ImGui::Text(exists ? (AssetManager::IsAssetAlive(handle) ? "Live" : "Unloaded") : "Unregistered");

			if (exists)
			{
				ImGui::TextDisabled(FA_KEY " Asset Handle: ");
				ImGui::SameLine();
				ImGui::Text("%llu", handle);
			}

			ImGui::Dummy(ImVec2(0.0f, style.FramePadding.y * 2.0f));
		}

		// Draw a color bar corresponding to the asset type at the bottom of the tooltip window.
		if (!file.IsDirectory)
		{
			const ImVec2 min = ImGui::GetWindowPos();
			const ImVec2 max = min + ImGui::GetWindowSize();
			ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(min.x - 25.0f, max.y - style.FramePadding.y * 2.0f), ImVec2(max.x + 25.0f, max.y), ImGui::GetColorU32(GetFileTypeColor(file.Type).Value));
		}
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
			for (auto& directoryEntry : std::filesystem::recursive_directory_iterator(m_BaseDirectory / m_CurrentDirectory))
				InsertDirectoryEntry(directoryEntry, true);
		else
			for (auto& directoryEntry : std::filesystem::directory_iterator(m_BaseDirectory / m_CurrentDirectory))
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
		const bool isDirectory = directoryEntry.is_directory();
		if ((!searching || (searching && !isDirectory && (path.filename().string().find(m_SearchbarBuffer) != std::string::npos))) && Preferences::GetData().Filters[FileManager::GetFileType(path)])
		{
			size_t index = 0;
			for (auto& files : m_DisplayFiles)
			{
				// First sort directories
				if (Preferences::GetData().DirectoriesFirst)
				{
					if (!files.IsDirectory && isDirectory)
						break;

					if (files.IsDirectory && !isDirectory)
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

			// Insert the new file entry at the corresponding position.
			m_DisplayFiles.insert(m_DisplayFiles.begin() + index,
				FileEntry(
					std::filesystem::relative(path, m_BaseDirectory),
					isDirectory,
					FileSystem::GetFileSize(path),
					isDirectory ? std::string() : fmt::format("{:%m/%d/%Y %H:%M}", fmt::localtime(FileSystem::GetLastWriteTime(path)))
				)
			);
		}
	}

	void ContentBrowserPanel::UpdateDisplayDirectories()
	{
		m_DisplayDirectories.SubDirectories.clear();
		UpdateDisplayDirectories(m_DisplayDirectories);
	}

	void ContentBrowserPanel::UpdateDisplayDirectories(DirectoryEntry& directory)
	{
		for (auto& directoryEntry : std::filesystem::directory_iterator(m_BaseDirectory / directory.Path))
		{
			if (directoryEntry.is_directory())
			{
				directory.SubDirectories.push_back({ std::filesystem::relative(directoryEntry.path(), m_BaseDirectory) });
				UpdateDisplayDirectories(directory.SubDirectories.back());
			}
		}
	}

	void ContentBrowserPanel::DrawDirectoryView(DirectoryEntry& directory, std::filesystem::path& path)
	{
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		ImGuiStyle& style = ImGui::GetStyle();

		for (auto& dir : directory.SubDirectories)
		{
			const ImVec2 folderDrawPosition = ImGui::GetCursorScreenPos() + ImVec2(style.FramePadding.x * 4.0f, 0.0f);
			bool open = ImGui::TreeNodeEx(fmt::format("      {}", dir.Path.filename().string()).c_str(), ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth | (dir.SubDirectories.empty() ? ImGuiTreeNodeFlags_Leaf : 0));
			
			glm::vec3 folderColor = ProjectSettings::GetFolderColor(dir.Path);
			drawList->AddText(folderDrawPosition, ImGui::ColorConvertFloat4ToU32(ImVec4(folderColor.x, folderColor.y, folderColor.z, 1.0f)), FA_FOLDER);

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

	void ContentBrowserPanel::SetSelectionContext(const FileEntry& entry, bool additive)
	{
		// If additive, check we don't have the entry already
		if (additive)
		{
			if (IsFileSelected(entry.Path))
				return;
		}
		else
		{
			// Otherwise, if not additive, clear the current context
			m_SelectionContext.clear();
		}

		// Push the object to the selection context
		m_SelectionContext.push_back(entry);
	}

	void ContentBrowserPanel::ToggleSelectionContext(const FileEntry& entry)
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
			// Ensure that all assets that exist in subdirectories get updated
			for (auto& file : std::filesystem::recursive_directory_iterator(oldPath))
			{
				AssetManager::RenameAsset(std::filesystem::relative(file, m_BaseDirectory), std::filesystem::relative(newPath, m_BaseDirectory) / std::filesystem::relative(file, oldPath));
			}

			// Ensure that if the directory folder had a color associated with it, that gets updated too
			const std::filesystem::path oldRelativePath = std::filesystem::relative(oldPath, m_BaseDirectory);
			if (ProjectSettings::HasFolderColor(oldRelativePath))
			{
				const glm::vec3 folderColor = ProjectSettings::GetFolderColor(oldRelativePath);
				ProjectSettings::ClearFolderColor(oldRelativePath);
				ProjectSettings::SetFolderColor(std::filesystem::relative(newPath, m_BaseDirectory), folderColor);
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

	void ContentBrowserPanel::Focus()
	{
		m_Focus = true;
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
		const std::filesystem::path path = m_CurrentDirectory / FileManager::GetNextOfNameInDirectory("New Folder", m_CurrentDirectory);
		std::filesystem::create_directory(path);
		UpdateDisplayFiles();
		RenameFile(std::filesystem::relative(path, m_BaseDirectory));
	}

	void ContentBrowserPanel::CreateEmptyFile(const std::filesystem::path& defaultFilename)
	{
		const std::filesystem::path path = m_CurrentDirectory / FileManager::GetNextOfNameInDirectory(defaultFilename, m_CurrentDirectory);

		std::ofstream ofs(path);
		ofs.close();

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
		if (ImGui::MenuItem(FA_FOLDER " Folder")) CreateFolder();

		ImGui::Separator();

		DrawCreateMenuItem<Scene>(FILE_ICON_SCENE " Scene", "New Scene.dymatic");

		if (ImGui::BeginMenu(FILE_ICON_MATERIAL " Materials"))
		{
			DrawCreateMenuItem<MaterialSource>(FILE_ICON_MATERIAL " Material", "New Material.dymaterial");
			DrawCreateMenuItem<MaterialInstance>(FILE_ICON_MATERIAL_INSTANCE " Material Instance", "New Material Instance.dymateriali", nullptr);

			ImGui::EndMenu();
		}

		DrawCreateMenuItem<ParticleSystem>(FILE_ICON_PARTICLE_SYSTEM " Particle System", "New Particle System.dyparticles");
		DrawCreateMenuItem<AnimationGraph>(FILE_ICON_ANIMATION_GRAPH " Animation Graph", "New Animation Graph.dyanimgraph", nullptr, nullptr);

		if (ImGui::BeginMenu(FA_RADIO " Media"))
		{
			if (ImGui::MenuItem(FILE_ICON_VIRTUAL_TEXTURE " Virtual Texture"))
			{
				TextureSpecification specification;
				specification.Format = TextureFormat::RGB8;
				specification.Width = 512;
				specification.Height = 512;

				CreateNewAsset<Texture2D>("New Virtual Texture.dyvirtex", specification);
			}

			DrawCreateMenuItem<VideoReader>(FILE_ICON_VIDEO_PLAYER " Video Player", "New Video Player.dymedplayer");

			ImGui::EndMenu();
		}

		if (ImGui::MenuItem(FA_FILE_LINES " Text"))
			CreateEmptyFile("New Text Document.txt");
	}

	void ContentBrowserPanel::DrawRenameInput(const std::filesystem::path& path, const bool expand)
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

		const ImVec2 size = ImVec2(expand ? ImGui::GetContentRegionAvailWidth() : ImGui::GetItemRectSize().x - style.FramePadding.x * 2.0f, ImGui::GetTextLineHeight() + style.FramePadding.y * 2.0f);

		bool input = ImGui::InputTextEx("##RenameInput", nullptr, buffer, sizeof(buffer), size, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll)
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

	template<typename T, typename ... Args>
	void ContentBrowserPanel::DrawCreateMenuItem(const char* label, const std::string& filename, Args&& ... args)
	{
		if (ImGui::MenuItem(label))
			CreateNewAsset<T>(filename, std::forward<Args>(args)...);
	}

	template<typename T, typename ... Args>
	Ref<T> ContentBrowserPanel::CreateNewAsset(const std::string& filename, Args&& ... args)
	{
		const std::filesystem::path path = (std::filesystem::relative(m_CurrentDirectory, m_BaseDirectory) / FileManager::GetNextOfNameInDirectory(filename, m_CurrentDirectory)).lexically_normal();
		Ref<T> asset = AssetManager::CreateNewAsset<T>(path, std::forward<Args>(args)...);
		UpdateDisplayFiles();
		RenameFile(path);

		return asset;
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

	Ref<Texture2D> ContentBrowserPanel::GetFileTypeIcon(FileType type)
	{
		switch (type)
		{
		case FileTypeFile: return EditorResources::FileIcon;
		case FileTypeDirectory: return EditorResources::DirectoryIcon;
		case FileTypeScene: return EditorResources::SceneIcon;
		case FileTypePrefab: return EditorResources::PrefabIcon;
		case FileTypeScript: return EditorResources::ScriptIcon;
		case FileTypeMeshSource: return EditorResources::MeshIcon;
		case FileTypeMesh: return EditorResources::MeshIcon;
		case FileTypeMaterial: return EditorResources::MaterialIcon;
		case FileTypeMaterialInstance: return EditorResources::MaterialIcon;
		case FileTypeTexture: return EditorResources::TextureIcon;
		case FileTypeEnvironmentMap: return EditorResources::EnvironmentMapIcon;
		case FileTypeVirtualTexture: return EditorResources::VirtualTextureIcon;
		case FileTypeFont: return EditorResources::FontIcon;
		case FileTypeAudio: return EditorResources::AudioIcon;
		case FileTypeParticleSystem: return EditorResources::ParticleSystemIcon;
		case FileTypeSkeleton: return EditorResources::SkeletonIcon;
		case FileTypeAnimation: return EditorResources::AnimationIcon;
		case FileTypeAnimationGraph: return EditorResources::AnimationGraphIcon;
		case FileTypePhysicsMaterial: return EditorResources::MaterialIcon;
		case FileTypeVideo: return EditorResources::VideoIcon;
		case FileTypeVideoPlayer: return EditorResources::VideoPlayerIcon;
		case FileTypeSubtitle: return EditorResources::SubtitleIcon;
		case FileTypeZipArchive: return EditorResources::ZipArchiveIcon;
		case FileTypeSolution : return EditorResources::SolutionIcon;
		default: return EditorResources::FileIcon;
		}
	}

	const char* ContentBrowserPanel::FileTypeToUpperString(FileType type)
	{
		switch (type)
		{
		case FileTypeFile: return "FILE";
		case FileTypeDirectory: return "DIRECTORY";
		case FileTypeScene: return "SCENE";
		case FileTypePrefab: return "PREFAB";
		case FileTypeScript: return "SCRIPT";
		case FileTypeMeshSource: return "MESH SOURCE";
		case FileTypeMesh: return "MESH";
		case FileTypeMaterial: return "MATERIAL";
		case FileTypeMaterialInstance: return "MATERIAL INSTANCE";
		case FileTypeTexture: return "TEXTURE";
		case FileTypeVirtualTexture: return "VIRTUAL TEXTURE";
		case FileTypeEnvironmentMap: return "ENVIRONMENT MAP";
		case FileTypeFont: return "FONT";
		case FileTypeAudio: return "AUDIO";
		case FileTypeParticleSystem: return "PARTICLE SYSTEM";
		case FileTypeSkeleton: return "SKELETON";
		case FileTypeAnimation: return "ANIMATION";
		case FileTypeAnimationGraph: return "ANIMATION GRAPH";
		case FileTypePhysicsMaterial: return "PHYSICS MATERIAL";
		case FileTypeVideo: return "VIDEO";
		case FileTypeVideoPlayer: return "VIDEO PLAYER";
		case FileTypeSubtitle: return "SUBTITLE";
		case FileTypeZipArchive: return "ZIP ARCHIVE";
		case FileTypeSolution: return "SOLUTION";
		default: return "FILE";
		}
	}

	const char* ContentBrowserPanel::FileTypeToString(FileType type)
	{
		switch (type)
		{
		case FileTypeFile: return "File";
		case FileTypeDirectory: return "Directory";
		case FileTypeScene: return "Scene";
		case FileTypePrefab: return "Prefab";
		case FileTypeScript: return "Script";
		case FileTypeMeshSource: return "Mesh Source";
		case FileTypeMesh: return "Mesh";
		case FileTypeMaterial: return "Material";
		case FileTypeMaterialInstance: return "Material Instance";
		case FileTypeTexture: return "Texture";
		case FileTypeEnvironmentMap: return "Environment Map";
		case FileTypeVirtualTexture: return "Virtual Texture";
		case FileTypeFont: return "Font";
		case FileTypeAudio: return "Audio";
		case FileTypeParticleSystem: return "Particle System";
		case FileTypeSkeleton: return "Skeleton";
		case FileTypeAnimation: return "Animation";
		case FileTypeAnimationGraph: return "Animation Graph";
		case FileTypePhysicsMaterial: return "Physics Material";
		case FileTypeVideo: return "Video";
		case FileTypeVideoPlayer: return "Video Player";
		case FileTypeSubtitle: return "Subtitle";
		case FileTypeZipArchive: return "Zip Archive";
		case FileTypeSolution: return "Solution";
		default: return "File";
		}
	}

}