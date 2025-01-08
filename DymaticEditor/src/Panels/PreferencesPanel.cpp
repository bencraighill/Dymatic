#include "PreferencesPanel.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/imgui_stdlib.h>

#include <glm/gtc/type_ptr.hpp>

#include "Settings/Preferences.h"
#include "Tools/PythonTools.h"
#include "PopupsAndNotifications.h"

#include "Fonts.h"
#include "TextSymbols.h"

#include "Dymatic/Math/Math.h"
#include "Dymatic/Utils/PlatformUtils.h"
#include "Dymatic/Math/StringUtils.h"
#include "Dymatic/UI/UI.h"


namespace Dymatic {

	static std::vector<System::ApplicationInstallDetails> s_ApplicationInstallDetails;

	PreferencesPanel::PreferencesPanel()
	{
		LoadAvailablePresets();
		RefreshPlugins();

		s_ApplicationInstallDetails = System::GetInstalledApplications();
	}

	static void EditThemeColor(ImGuiCol color, const char* tooltip = nullptr)
	{
		ImGui::PushID(color);

		ImGui::ColorEdit4("##ColorEdit", (float*)&ImGui::GetStyle().Colors[color]);
		ImGui::SameLine();
		ImGui::Text(Preferences::GetThemeColorName(color).c_str());

		if (tooltip != nullptr)
		{
			if (ImGui::IsItemHovered())
			{
				ImGui::BeginTooltip();
				UI::PushFont(FontType::Small);
				ImGui::Text(tooltip);
				UI::PopFont();
				ImGui::EndTooltip();
			}
		}
		ImGui::PopID();
	}

	void PreferencesPanel::OnImGuiRender()
	{
		auto& style = ImGui::GetStyle();
		auto& io = ImGui::GetIO();

		static bool previousPreferencesShowWindow = m_PreferencesPanelVisible;

		if ((previousPreferencesShowWindow && !m_PreferencesPanelVisible) && Preferences::GetData().AutosavePreferences)
		{
			Preferences::SavePreferences();
		}

		previousPreferencesShowWindow = m_PreferencesPanelVisible;

		if (m_PreferencesPanelVisible)
		{
			ImGui::Begin(CHARACTER_ICON_PREFERENCES " Preferences", &m_PreferencesPanelVisible, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

			if (ImGui::BeginTable("##PreferencesSplitterTable", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchSame))
			{
				ImGui::TableNextColumn();

				const char* categoriesA[4] = { FA_WINDOW_FLIP " Interface", FA_PALETTE " Themes", CHARACTER_ICON_VIEWPORT " Viewport", CHARACTER_ICON_TRANSFORM " Editing" };
				const char* categoriesB[3] = { FA_GAMEPAD_MODERN " Input", FA_MAP " Navigation", FA_KEYBOARD " Keymap" };
				const char* categoriesC[1] = { FA_PLUG " Plugins" };
				const char* categoriesD[3] = { FA_DESKTOP " System", FA_FLOPPY_DISK " Save & Load", FA_LINK " File Paths" };
				int currentValueA = (int)(m_CurrentCategory)-0;
				int currentValueB = (int)(m_CurrentCategory)-4;
				int currentValueC = (int)(m_CurrentCategory)-7;
				int currentValueD = (int)(m_CurrentCategory)-8;
				if (ImGui::ButtonStackEx("##PreferencesCategoryStackA", categoriesA, 4, &currentValueA, ImVec2(ImGui::GetContentRegionAvail().x - 5.0f, 120.0f), 5.0f))
				{
					m_CurrentCategory = (PreferencesCategory)(currentValueA + 0);
				}
				ImGui::Dummy(ImVec2(0.0f, 2.5f));
				if (ImGui::ButtonStackEx("##PreferencesCategoryStackB", categoriesB, 3, &currentValueB, ImVec2(ImGui::GetContentRegionAvail().x - 5.0f, 90.0f), 5.0f))
				{
					m_CurrentCategory = (PreferencesCategory)(currentValueB + 4);
				}
				ImGui::Dummy(ImVec2(0.0f, 2.5f));
				if (ImGui::ButtonStackEx("##PreferencesCategoryStackC", categoriesC, 1, &currentValueC, ImVec2(ImGui::GetContentRegionAvail().x - 5.0f, 30.0f), 5.0f))
				{
					m_CurrentCategory = (PreferencesCategory)(currentValueC + 7);
				}
				ImGui::Dummy(ImVec2(0.0f, 2.5f));
				if (ImGui::ButtonStackEx("##PreferencesCategoryStackD", categoriesD, 3, &currentValueD, ImVec2(ImGui::GetContentRegionAvail().x - 5.0f, 90.0f), 5.0f))
				{
					m_CurrentCategory = (PreferencesCategory)(currentValueD + 8);
				}

				ImGui::Dummy(ImVec2{ 0, ImGui::GetContentRegionAvail().y - 25 });
				if (ImGui::Button(CHARACTER_ICON_HAMBURGER))
					ImGui::OpenPopup("##PreferencesOptionsPopup");

				if (ImGui::BeginPopup("##PreferencesOptionsPopup", ImGuiWindowFlags_NoMove))
				{
					if (ImGui::MenuItem(FA_INDUSTRY " Load Factory Defaults")) { Preferences::LoadPreferences("saved/presets/DefaultPreferences.prefs"); }
					if (ImGui::MenuItem(CHARACTER_ICON_RESTART " Restore Saved Preferences ")) { Preferences::LoadPreferences(); }
					if (ImGui::MenuItem(CHARACTER_ICON_IMPORT " Import Preferences ")) { ImportPreferences(); }
					if (ImGui::MenuItem(CHARACTER_ICON_IMPORT " Export Preferences ")) { ExportPreferences(); }
					if (ImGui::MenuItem(CHARACTER_ICON_SAVE " Save Preferences ")) { Preferences::SavePreferences(); }
					ImGui::Checkbox("##PreferencesAutosaveCheckbox", &Preferences::GetData().AutosavePreferences);
					ImGui::SameLine();
					ImGui::Text(FA_CLOCK " Auto-Save Preferences");
					ImGui::EndPopup();
				}

				ImGui::TableNextColumn();

				ImGui::BeginChild("##PreferencesPannelContent", {}, false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
				const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
				if (m_CurrentCategory == Interface)
				{
					if (ImGui::TreeNodeEx(FA_DESKTOP " Display", treeNodeFlags | ImGuiTreeNodeFlags_DefaultOpen))
					{
						ImGui::Checkbox("Show Splash", &Preferences::GetData().ShowSplashStartup);
						ImGui::TreePop();
					}
				}
				else if (m_CurrentCategory == Themes)
				{
					float widthAvalOver = ImGui::GetContentRegionAvail().x - 40;

					static int selectedIndex = 0;

					ImGui::SetNextItemWidth(widthAvalOver / 7 * 2.5f);
					if (ImGui::BeginCombo("##ThemePresetSelectCombo", m_ThemePresets.empty() ? "No Available Preset" : m_ThemePresets[selectedIndex].Name.c_str()))
					{
						if (selectedIndex > m_ThemePresets.size() - 1)
						{
							selectedIndex = 0;
						}

						for (uint32_t i = 0; i < m_ThemePresets.size(); i++)
						{
							if (ImGui::Selectable(m_ThemePresets[i].Name.c_str()))
							{
								selectedIndex = i;
								Preferences::LoadTheme(m_ThemePresets[i].Path);
							}
						}

						ImGui::EndCombo();
					}

					ImGui::SameLine();
					if (ImGui::Button(CHARACTER_ICON_IMPORT " Import", ImVec2{ widthAvalOver / 7, 23 })) { ImportTheme(); }
					ImGui::SameLine();
					if (ImGui::Button(CHARACTER_ICON_EXPORT " Export", ImVec2{ widthAvalOver / 7, 23 })) { ExportTheme(); }
					ImGui::SameLine();
					if (ImGui::Button(CHARACTER_ICON_REFRESH " Restore Theme", ImVec2{ widthAvalOver / 7 * 1.5f, 23 })) { Preferences::LoadTheme("saved/SavedTheme.dytheme"); }
					ImGui::SameLine();
					if (ImGui::Button(CHARACTER_ICON_SAVE " Save Theme", ImVec2{ widthAvalOver / 7, 23 })) { Preferences::SaveTheme("saved/SavedTheme.dytheme"); }

					ImGui::BeginChild("##ThemePrefsChild");

					if (ImGui::TreeNodeEx("Text", treeNodeFlags))
					{
						EditThemeColor(ImGuiCol_Text, "Plain text used on all buttons, popups and windows throughout Dymatic.");
						EditThemeColor(ImGuiCol_TextDisabled, "Text that is unavalible to the user or cannot be interacted with. (Used for shortcut hints)");
						EditThemeColor(ImGuiCol_TextSelectedBg, "Highlight color behind text when selected with cursor.");
						ImGui::TreePop();
					}

					if (ImGui::TreeNodeEx("Windows", treeNodeFlags))
					{
						EditThemeColor(ImGuiCol_WindowBg, "Default background color for all windows.");
						EditThemeColor(ImGuiCol_MenuBarBg, "Main menu bar, located at the top of the main window.");
						EditThemeColor(ImGuiCol_MenuBarGrip);
						EditThemeColor(ImGuiCol_MenuBarGripBorder);
						EditThemeColor(ImGuiCol_MainWindowBorderEdit);
						EditThemeColor(ImGuiCol_MainWindowBorderPlay);
						EditThemeColor(ImGuiCol_MainWindowBorderSimulate);
						EditThemeColor(ImGuiCol_WindowShadow);
						ImGui::TreePop();
					}

					if (ImGui::TreeNodeEx("Tabs", treeNodeFlags))
					{
						EditThemeColor(ImGuiCol_Header, "Header colors are used for CollapsingHeader, TreeNode, selectable and Menu Items");
						EditThemeColor(ImGuiCol_HeaderHovered);
						EditThemeColor(ImGuiCol_HeaderActive);
						ImGui::Separator();
						EditThemeColor(ImGuiCol_Tab);
						EditThemeColor(ImGuiCol_TabHovered);
						EditThemeColor(ImGuiCol_TabActive);
						EditThemeColor(ImGuiCol_TabUnfocused);
						EditThemeColor(ImGuiCol_TabUnfocusedActive);
						ImGui::Separator();
						EditThemeColor(ImGuiCol_TitleBg);
						EditThemeColor(ImGuiCol_TitleBgActive);
						EditThemeColor(ImGuiCol_TitleBgCollapsed);
						ImGui::TreePop();
					}

					if (ImGui::TreeNodeEx("Buttons", treeNodeFlags))
					{
						EditThemeColor(ImGuiCol_Button);
						EditThemeColor(ImGuiCol_ButtonHovered);
						EditThemeColor(ImGuiCol_ButtonActive);
						EditThemeColor(ImGuiCol_ButtonToggled);
						EditThemeColor(ImGuiCol_ButtonToggledHovered);
						ImGui::TreePop();
					}

					if (ImGui::TreeNodeEx("Popups & Modals", treeNodeFlags))
					{
						EditThemeColor(ImGuiCol_PopupBg, "Background of popups, menus, tooltips windows.");
						EditThemeColor(ImGuiCol_ModalWindowDimBg);
						ImGui::TreePop();
					}
					if (ImGui::TreeNodeEx("Borders", treeNodeFlags))
					{
						EditThemeColor(ImGuiCol_Border);
						EditThemeColor(ImGuiCol_BorderShadow);
						ImGui::TreePop();
					}

					if (ImGui::TreeNodeEx("Frames", treeNodeFlags))
					{
						EditThemeColor(ImGuiCol_FrameBg, "Background of radio button, plot, slider and text input.");
						EditThemeColor(ImGuiCol_FrameBgHovered);
						EditThemeColor(ImGuiCol_FrameBgActive);
						ImGui::TreePop();
					}

					if (ImGui::TreeNodeEx("Scrollbars", treeNodeFlags))
					{
						EditThemeColor(ImGuiCol_ScrollbarBg);
						EditThemeColor(ImGuiCol_ScrollbarGrab);
						EditThemeColor(ImGuiCol_ScrollbarGrabHovered);
						EditThemeColor(ImGuiCol_ScrollbarGrabActive);
						EditThemeColor(ImGuiCol_ScrollbarDots);
						ImGui::TreePop();
					}

					if (ImGui::TreeNodeEx("Progress Bars", treeNodeFlags))
					{
						EditThemeColor(ImGuiCol_ProgressBarBg);
						EditThemeColor(ImGuiCol_ProgressBarBorder);
						EditThemeColor(ImGuiCol_ProgressBarFill);
						ImGui::TreePop();
					}

					if (ImGui::TreeNodeEx("Slider", treeNodeFlags))
					{
						EditThemeColor(ImGuiCol_SliderGrab);
						EditThemeColor(ImGuiCol_SliderGrabActive);
						ImGui::TreePop();
					}

					if (ImGui::TreeNodeEx("Separator", treeNodeFlags))
					{
						EditThemeColor(ImGuiCol_Separator);
						EditThemeColor(ImGuiCol_SeparatorHovered);
						EditThemeColor(ImGuiCol_SeparatorActive);
						ImGui::TreePop();
					}

					if (ImGui::TreeNodeEx("Resize", treeNodeFlags))
					{
						EditThemeColor(ImGuiCol_ResizeGrip);
						EditThemeColor(ImGuiCol_ResizeGripHovered);
						EditThemeColor(ImGuiCol_ResizeGripActive);
						ImGui::TreePop();
					}

					if (ImGui::TreeNodeEx("Text Editor", treeNodeFlags))
					{
						EditThemeColor(ImGuiCol_TextEditorDefault);
						EditThemeColor(ImGuiCol_TextEditorKeyword);
						EditThemeColor(ImGuiCol_TextEditorSpecialKeyword);
						EditThemeColor(ImGuiCol_TextEditorNumber);
						EditThemeColor(ImGuiCol_TextEditorString);
						EditThemeColor(ImGuiCol_TextEditorCharLiteral);
						EditThemeColor(ImGuiCol_TextEditorPunctuation);
						EditThemeColor(ImGuiCol_TextEditorPreprocessor);
						EditThemeColor(ImGuiCol_TextEditorIdentifier);
						EditThemeColor(ImGuiCol_TextEditorComment);
						EditThemeColor(ImGuiCol_TextEditorMultiLineComment);
						EditThemeColor(ImGuiCol_TextEditorLineNumber);
						EditThemeColor(ImGuiCol_TextEditorCurrentLineFill);
						EditThemeColor(ImGuiCol_TextEditorCurrentLineFillInactive);
						EditThemeColor(ImGuiCol_TextEditorCurrentLineEdge);
						ImGui::TreePop();
					}

					if (ImGui::TreeNodeEx("Log", treeNodeFlags))
					{
						EditThemeColor(ImGuiCol_LogTrace);
						EditThemeColor(ImGuiCol_LogInfo);
						EditThemeColor(ImGuiCol_LogWarn);
						EditThemeColor(ImGuiCol_LogError);
						EditThemeColor(ImGuiCol_LogCritical);
						ImGui::TreePop();
					}

					if (ImGui::TreeNodeEx("Checkboxes", treeNodeFlags))
					{
						EditThemeColor(ImGuiCol_CheckMark);
						EditThemeColor(ImGuiCol_Checkbox);
						EditThemeColor(ImGuiCol_CheckboxHovered);
						EditThemeColor(ImGuiCol_CheckboxActive);
						EditThemeColor(ImGuiCol_CheckboxTicked);
						EditThemeColor(ImGuiCol_CheckboxHoveredTicked);
						ImGui::TreePop();
					}

					if (ImGui::TreeNodeEx("Docking", treeNodeFlags))
					{
						EditThemeColor(ImGuiCol_DockingPreview, "Preview overlay color when about to docking something.");
						EditThemeColor(ImGuiCol_DockingEmptyBg, "Background color for empty node.");
						ImGui::TreePop();
					}

					if (ImGui::TreeNodeEx("Plots", treeNodeFlags))
					{
						EditThemeColor(ImGuiCol_PlotLines);
						EditThemeColor(ImGuiCol_PlotLinesHovered);
						EditThemeColor(ImGuiCol_PlotHistogram);
						EditThemeColor(ImGuiCol_PlotHistogramHovered);
						ImGui::TreePop();
					}

					if (ImGui::TreeNodeEx("Tables", treeNodeFlags))
					{
						EditThemeColor(ImGuiCol_TableHeaderBg);
						EditThemeColor(ImGuiCol_TableBorderStrong);
						EditThemeColor(ImGuiCol_TableBorderLight);
						EditThemeColor(ImGuiCol_TableRowBg);
						EditThemeColor(ImGuiCol_TableRowBgAlt);
						ImGui::TreePop();
					}

					if (ImGui::TreeNodeEx("Drag & Drop", treeNodeFlags))
					{
						EditThemeColor(ImGuiCol_DragDropTarget);
						ImGui::TreePop();
					}

					if (ImGui::TreeNodeEx("Nav", treeNodeFlags))
					{
						EditThemeColor(ImGuiCol_NavHighlight, "Current highlighted item with keyboard.");
						EditThemeColor(ImGuiCol_NavWindowingHighlight, "Highlight window when using CTRL + TAB");
						EditThemeColor(ImGuiCol_NavWindowingDimBg, "Darken/colorize entire screen behind the window list, when active");
						ImGui::TreePop();
					}

					ImGui::EndChild();
				}
				else if (m_CurrentCategory == Viewport)
				{
					ImGui::Text(FA_LOCK " Lock Viewport Mouse");
					ImGui::SameLine();
					ImGui::Checkbox("##LockViewportMouseCheckbox", &Preferences::GetData().LockViewportMouse);
				}
				else if (m_CurrentCategory == Editing)
				{
					ImGui::SetNextItemWidth(-1);
					ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
					bool open = ImGui::TreeNodeEx(FA_PENCIL " Edit Modes", treeNodeFlags | ImGuiTreeNodeFlags_DefaultOpen);
					ImGui::PopStyleVar();
					if (open)
					{
						auto& advancedEditMode = Preferences::GetData().AdvancedEditMode;

						const char* editModeMessage = "Warning! Advanced Edit Mode has been enabled.\nThis allows for direct overriding of the asset registry as well as all entity and prefab UUIDs.\nProceed with caution!";

						ImGui::Text(FA_SCREWDRIVER_WRENCH " Advanced Edit Mode");
						ImGui::SameLine();
						if (ImGui::Checkbox("##AdvancedEditModeCheckbox", &advancedEditMode))
						{
							if (advancedEditMode)
								Popup::Create(FA_TRIANGLE_EXCLAMATION " Advanced Edit Mode Enabled!", editModeMessage, { ButtonData("Ok") });
						}

						if (advancedEditMode)
						{
							ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetColorU32(ImGuiCol_TextDisabled));
							ImGui::PushTextWrapPos(ImGui::GetWindowContentRegionMax().y);
							ImGui::Text(FA_TRIANGLE_EXCLAMATION);
							ImGui::SameLine();
							ImGui::TextWrapped(editModeMessage);
							ImGui::PopTextWrapPos();
							ImGui::PopStyleColor();
						}

						const char* editModeOptions[] = { CHARACTER_ICON_SCENE_HIERARCHY " Hierarchy", FA_LIST_DROPDOWN " Component" };
						ImGui::TextUnformatted(FA_BONE " Bone Attachment Edit Mode");
						ImGui::SameLine();
						ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());
						ImGui::Combo("##BoneAttachmentDropdown", (int*)&Preferences::GetData().BoneAttachmentEditMode, editModeOptions, IM_ARRAYSIZE(editModeOptions));

						ImGui::TreePop();
					}
				}
				else if (m_CurrentCategory == Plugins)
				{
					// DLL based plugins
					ImGui::SetNextItemWidth(-1);
					ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
					bool open = ImGui::TreeNodeEx(FA_GEARS " DLL Plugins", treeNodeFlags | ImGuiTreeNodeFlags_DefaultOpen);
					ImGui::PopStyleVar();
					if (open)
					{
						ImGui::Text("Load Plugins");
						ImGui::SameLine();
						if (ImGui::Button("Refresh"))
							RefreshPlugins();
						for (auto& plugin : m_PluginInfo)
						{
							const float width = ImGui::GetContentRegionAvailWidth();

							ImGui::PushID(plugin.path.c_str());
							ImGui::SetNextItemWidth(-1);
							ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
							bool open = ImGui::TreeNodeEx(("  " + plugin.name).c_str(), treeNodeFlags);
							ImGui::PopStyleVar();
							if (ImGui::IsItemHovered() && GImGui->HoveredIdTimer > 0.5f)
								ImGui::SetTooltip(plugin.path.string().c_str());
							ImGui::SameLine(width - 25.0f);
							if (ImGui::Checkbox("##PluginCheckbox", &plugin.enabled))
								WritePluginManifest();
							if (open)
							{
								ImGui::Text("Path: ");
								ImGui::SameLine();
								ImGui::TextDisabled(plugin.path.string().c_str());

								if (!plugin.version.empty())
								{
									ImGui::Text("Version: ");
									ImGui::SameLine();
									ImGui::TextDisabled(plugin.version.c_str());
								}

								{
									ImGui::Text("Developer: ");
									ImGui::SameLine();
									if (plugin.developer.empty())
									{
										ImGui::TextDisabled("Unknown");
										ImGui::SameLine();
										ImGui::PushStyleColor(ImGuiCol_Button, {});
										ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {});
										ImGui::PushStyleColor(ImGuiCol_ButtonActive, {});
										ImGui::Button("(WARNING)", ImVec2(0.0f, ImGui::GetTextLineHeight()));
										ImGui::PopStyleColor(3);
										if (ImGui::IsItemHovered())
											ImGui::SetTooltip("WARNING: This plugin comes from an unknown developer. Please ensure it has a reputable source.");
									}
									else
										ImGui::TextDisabled(plugin.developer.c_str());
								}

								if (!plugin.description.empty())
								{
									ImGui::Text("Description: ");
									ImGui::SameLine();
									ImGui::PushTextWrapPos(ImGui::GetWindowContentRegionMax().x - ImGui::GetStyle().FramePadding.x * 2.0f);
									ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
									ImGui::TextWrapped(plugin.description.c_str());
									ImGui::PopStyleColor();
									ImGui::PopTextWrapPos();
								}

								if (!plugin.params.empty())
								{
									if (ImGui::TreeNodeEx("Parameters", ImGuiTreeNodeFlags_None))
									{
										for (auto& param : plugin.params)
										{
											bool default = true;

											ImGui::Text(param.name.c_str());
											ImGui::SameLine();
											switch (param.type)
											{
											case PluginInfo::PluginParam::Bool:
												ImGui::Checkbox("##Bool", &param.data.Bool);
												if (ImGui::IsItemDeactivatedAfterEdit()) WritePluginManifest();
												default = param.data.Bool == param.defaultValue.Bool;
												break;
											case PluginInfo::PluginParam::Int:
												ImGui::InputInt("##Int", &param.data.Int, 0);
												if (ImGui::IsItemDeactivatedAfterEdit()) WritePluginManifest();
												default = param.data.Int == param.defaultValue.Int;
												break;
											case PluginInfo::PluginParam::Float:
												ImGui::InputFloat("##Float", &param.data.Float);
												if (ImGui::IsItemDeactivatedAfterEdit()) WritePluginManifest();
												default = param.data.Float == param.defaultValue.Float;
												break;
											case PluginInfo::PluginParam::String:
												char buffer[256];
												memset(buffer, 0, sizeof(buffer));
												std::strncpy(buffer, param.data.String.c_str(), sizeof(buffer));
												ImGui::InputText("##String", buffer, sizeof(buffer));
												if (ImGui::IsItemDeactivatedAfterEdit())
												{
													std::string string = buffer;
													if (string.find(",") == std::string::npos)
													{
														param.data.String = string;
														WritePluginManifest();
													}
												}
												default = param.data.String == param.defaultValue.String;
												break;
											}

											if (!default)
											{
												ImGui::GetWindowDrawList()->AddCircleFilled(ImVec2(ImGui::GetItemRectMax().x + 10.0f, (ImGui::GetItemRectMin().y + ImGui::GetItemRectMax().y) * 0.5f), 4.0f, ImGui::GetColorU32(ImGuiCol_Text));
												if (ImGui::BeginPopupContextItem())
												{
													if (ImGui::MenuItem("Reset to Default"))
													{
														param.data = param.defaultValue;
														WritePluginManifest();
													}
													ImGui::EndPopup();
												}
											}
										}
										ImGui::TreePop();
									}
								}

								ImGui::TreePop();
							}
							ImGui::PopID();
						}
						ImGui::TreePop();
					}
					
					// Python script plugins
					{
						ImGui::SetNextItemWidth(-1);
						ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
						bool open = ImGui::TreeNodeEx(CHARACTER_ICON_PYTHON " Python Script Plugins", treeNodeFlags | ImGuiTreeNodeFlags_DefaultOpen);
						ImGui::PopStyleVar();
						if (open)
						{
							const float width = ImGui::GetContentRegionAvailWidth();

							if (ImGui::Button("Browse", ImVec2(-1, 30.0f)))
							{
								std::string path = FileDialogs::OpenFile("Python Script (*.py)\0*.py\0");
								if (!path.empty())
								{
									std::filesystem::path filepath = path;
									filepath.make_preferred();
									Preferences::GetData().PythonPlugins.push_back(Preferences::PythonPluginInformation(filepath));
									PythonTools::LoadPlugin(filepath);
								}
							}

							ImGui::Dummy(ImVec2(0.0f, 5.0f));

							auto& plugins = Preferences::GetData().PythonPlugins;
							for (uint32_t index = 0; index < plugins.size(); index++)
							{
								auto& plugin = plugins[index];

								ImGui::PushID(index);

								ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
								std::string name = plugin.Metadata.Name.empty() ? plugin.PluginPath.filename().string() : plugin.Metadata.Name;
								bool open = ImGui::TreeNodeEx(("        " + name).c_str(), treeNodeFlags);
								ImGui::PopStyleVar();

								ImGui::BeginDisabled(!plugin.Enabled);

								if (plugin.Metadata.Icon)
								{
									const ImVec2 min = ImGui::GetItemRectMin() + style.FramePadding + ImVec2(20.0f, 0.0f);
									const float size = ImGui::GetItemRectSize().y - style.FramePadding.y * 2.0f;
									const ImVec2 max = min + ImVec2(size, size);
									ImGui::GetWindowDrawList()->AddImage((ImTextureID)plugin.Metadata.Icon->GetRendererID(), min, max, { 0, 1 }, { 1, 0 });
								}

								ImGui::SameLine(width - 90.0f - ImGui::GetStyle().FramePadding.x * 8.0f);

								if (ImGui::Button(CHARACTER_ICON_RESTART, ImVec2(30.0f, ImGui::GetTextLineHeight() + ImGui::GetStyle().FramePadding.y * 2.0f)))
								{
									PythonTools::ReloadPlugin(plugin.PluginPath);
								}

								ImGui::EndDisabled();

								ImGui::SameLine();

								if (ImGui::Button(plugin.Enabled ? FA_LINK_SLASH : FA_LINK, ImVec2(30.0f, ImGui::GetTextLineHeight() + ImGui::GetStyle().FramePadding.y * 2.0f)))
								{
									plugin.Enabled = !plugin.Enabled;

									if (plugin.Enabled)
										PythonTools::LoadPlugin(plugin.PluginPath);
									else
										PythonTools::UnloadPlugin(plugin.PluginPath);
								}

								ImGui::SameLine();
								
								if (ImGui::Button(CHARACTER_ICON_CROSS, ImVec2(30.0f, ImGui::GetTextLineHeight() + ImGui::GetStyle().FramePadding.y * 2.0f)))
								{
									if (plugin.Enabled)
										PythonTools::UnloadPlugin(plugin.PluginPath);

									plugins.erase(plugins.begin() + index);
									index--;
								}

								if (open)
								{
									// Plugin Preferences
									if (plugin.Enabled)
									{
										if (ImGui::TreeNodeEx("Preferences", treeNodeFlags))
										{
											PythonTools::OnImGuiRender(plugin.PluginPath, PythonUIRenderStage::PluginPreferences);
											ImGui::TreePop();
										}
									}

									// Details
									ImGui::SetNextItemWidth(width * 0.35f);
									ImGui::InputText("##PythonScriptName", &plugin.PluginPath.filename().string(), ImGuiInputTextFlags_ReadOnly);

									ImGui::SameLine();

									ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());
									ImGui::InputText("##PythonScriptPath", &plugin.PluginPath.parent_path().string(), ImGuiInputTextFlags_ReadOnly);

									if (!plugin.Metadata.Name.empty())
									{
										ImGui::Text("Name:");
										ImGui::SameLine();
										ImGui::TextDisabled(plugin.Metadata.Name.c_str());
									}

									if (!plugin.Metadata.Version.empty())
									{
										ImGui::Text("Version:");
										ImGui::SameLine();
										ImGui::TextDisabled(plugin.Metadata.Version.c_str());
									}

									if (!plugin.Metadata.Description.empty())
									{
										ImGui::Text("Description:");
										ImGui::SameLine();
										ImGui::TextDisabled(plugin.Metadata.Description.c_str());
									}

									ImGui::Separator();

									if (!plugin.Metadata.Author.empty())
									{
										ImGui::Text("Author:");
										ImGui::SameLine();
										ImGui::TextDisabled(plugin.Metadata.Author.c_str());
									}

									if (!plugin.Metadata.CompanyName.empty())
									{
										ImGui::Text("Company Name:");
										ImGui::SameLine();
										ImGui::TextDisabled(plugin.Metadata.CompanyName.c_str());
									}

									if (!plugin.Metadata.EngineVersionRequirements.empty())
									{
										ImGui::Text("Engine Version Requirements:");
										ImGui::SameLine();
										ImGui::TextDisabled(plugin.Metadata.EngineVersionRequirements.c_str());
									}

									if (!plugin.Metadata.Dependencies.empty())
									{
										ImGui::Text("Dependencies:");
										ImGui::SameLine();
										ImGui::TextDisabled(plugin.Metadata.Dependencies.c_str());
									}

									if (!plugin.Metadata.BuildDate.empty())
									{
										ImGui::Text("Build Date:");
										ImGui::SameLine();
										ImGui::TextDisabled(plugin.Metadata.BuildDate.c_str());
									}

									ImGui::Separator();

									if (!plugin.Metadata.LegalCopyright.empty())
									{
										ImGui::Text("Legal Copyright:");
										ImGui::SameLine();
										ImGui::TextDisabled(plugin.Metadata.LegalCopyright.c_str());
									}

									if (!plugin.Metadata.LegalTrademarks.empty())
									{
										ImGui::Text("Legal Trademarks:");
										ImGui::SameLine();
										ImGui::TextDisabled(plugin.Metadata.LegalTrademarks.c_str());
									}

									if (!plugin.Metadata.License.empty())
									{
										ImGui::Text("License:");
										ImGui::InputTextMultiline("##PythonScriptLicense", &plugin.Metadata.License, ImVec2(ImGui::GetContentRegionAvailWidth(), 200.0f), ImGuiInputTextFlags_ReadOnly);
									}

									ImGui::TreePop();
								}
								
								ImGui::PopID();
							}

							ImGui::TreePop();
						}
					}
				}
				else if (m_CurrentCategory == Input)
				{
					ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2());
					ImGui::BeginChild("##InputPreferencesWindow");
					ImGui::PopStyleVar();

					ImDrawList* drawList = ImGui::GetWindowDrawList();

					// Refresh device data if appearing
					if (ImGui::IsWindowAppearing())
					{
						m_MonitorInfo = Monitor::GetMonitorInfo();
						m_NetworkInfo = Network::GetNetworkInfo();
					}

					if (ImGui::TreeNodeEx(FA_KEYBOARD " Keyboard", treeNodeFlags | ImGuiTreeNodeFlags_DefaultOpen))
					{
						ImGui::TextDisabled("Information");
						ImGui::Text(Input::GetKeyboardName().c_str());

						ImGui::TextDisabled("Settings");
						ImGui::Text("Emulate Numpad");
						ImGui::SameLine();
						ImGui::Checkbox("##KeyboardEmulateNumpadCheckbox", &Preferences::GetData().EmulateNumpad);

						ImGui::TreePop();
					}
					
					if (ImGui::TreeNodeEx(FA_COMPUTER_MOUSE " Mouse", treeNodeFlags | ImGuiTreeNodeFlags_DefaultOpen))
					{
						ImGui::TextDisabled("Information");
						ImGui::Text(Input::GetMouseName().c_str());

						ImGui::TextDisabled("Settings");
						ImGui::Text("Double Click Speed");
						ImGui::SameLine();
						if (ImGui::SliderInt("##MouseDoubleClickSpeedSlider", &Preferences::GetData().DoubleClickSpeed, 1, 1000))
							ImGui::GetIO().MouseDoubleClickTime = Preferences::GetData().DoubleClickSpeed / 1000.0f;

						ImGui::Text("Tooltip Hover Delay");
						ImGui::SameLine();
						ImGui::SliderInt("##TooltipHoverDelaySlider", &Preferences::GetData().TooltipHoverDelay, 0, 2000);

						ImGui::TreePop();
					}

					const char* networkIcon;
					if (m_NetworkInfo.Strength > 75)
						networkIcon = FA_WIFI;
					else if (m_NetworkInfo.Strength > 35)
						networkIcon = FA_WIFI_FAIR;
					else if (m_NetworkInfo.Strength > 0)
						networkIcon = FA_WIFI_WEAK;
					else
						networkIcon = FA_WIFI_SLASH;

					if (ImGui::TreeNodeEx(fmt::format("{} Network", networkIcon).c_str(), treeNodeFlags | ImGuiTreeNodeFlags_DefaultOpen))
					{
						ImGui::Text("Name: %s", m_NetworkInfo.Name.c_str());
						ImGui::Text("Signal Strength: %d", m_NetworkInfo.Strength);

						ImGui::TreePop();
					}
					
					if (ImGui::TreeNodeEx(FA_GAMEPAD_MODERN " Controllers", treeNodeFlags | ImGuiTreeNodeFlags_DefaultOpen))
					{
						for (uint32_t i = 0; i < Input::GetGamepadCount(); i++)
						{
							const char* powerIcon;
							const Input::PowerLevel powerLevel = Input::GetGamepadPowerLevel(i);

							switch (powerLevel)
							{
							case Input::PowerLevel::Unknown: powerIcon = FA_BATTERY_SLASH; break;
							case Input::PowerLevel::Empty: powerIcon = FA_BATTERY_EMPTY; break;
							case Input::PowerLevel::Low: powerIcon = FA_BATTERY_QUARTER; break;
							case Input::PowerLevel::Medium: powerIcon = FA_BATTERY_HALF; break;
							case Input::PowerLevel::Full: powerIcon = FA_BATTERY_FULL; break;
							case Input::PowerLevel::Wired: powerIcon = FA_BATTERY_BOLT; break;
							case Input::PowerLevel::Max: powerIcon = FA_BATTERY_FULL; break;
							}

							ImGui::Selectable(fmt::format("{} {}", powerLevel == Input::PowerLevel::Wired ? FA_USB : FA_BLUETOOTH_B, Input::GetGamepadName(i)).c_str(), ImGuiSelectableFlags_SpanAvailWidth);
							drawList->AddText(ImGui::GetItemRectMax() - ImGui::CalcTextSize(powerIcon) - style.FramePadding, ImGui::GetColorU32(ImGuiCol_Text), powerIcon);
						}

						ImGui::TreePop();
					}
					
					if (ImGui::TreeNodeEx(FA_DESKTOP " Displays", treeNodeFlags | ImGuiTreeNodeFlags_DefaultOpen))
					{
						const ImGuiTableFlags flags = ImGuiTableFlags_PadOuterX | ImGuiTableFlags_NoBordersInBody | ImGuiTableFlags_NoPadInnerX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_ScrollX;
						if (ImGui::BeginTable("##PreferencesDisplayTable", 4, flags, ImGui::GetContentRegionAvail()))
						{
							const float width = ImGui::GetContentRegionAvailWidth() / 4.0f;
							ImGui::TableSetupColumn(FA_DESKTOP " Name", ImGuiTableColumnFlags_WidthFixed, width);
							ImGui::TableSetupColumn(FA_CLOCK " Display Frequency", ImGuiTableColumnFlags_WidthFixed, width);
							ImGui::TableSetupColumn(FA_UP_DOWN_LEFT_RIGHT " Orientation", ImGuiTableColumnFlags_WidthStretch, width);
							ImGui::TableSetupColumn(FA_EXPAND " Size", ImGuiTableColumnFlags_WidthStretch, width);
							ImGui::TableSetupScrollFreeze(4, 1);
							ImGui::TableHeadersRow();

							ImDrawList* drawList = ImGui::GetWindowDrawList();

							ImGuiListClipper clipper;
							clipper.Begin(m_MonitorInfo.size());
							while (clipper.Step())
							{
								for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
								{
									auto& monitorInfo = m_MonitorInfo[i];

									ImGui::PushID(i);

									ImGui::TableNextRow();
									ImGui::TableNextColumn();

									// Draw selectable
									bool even = i % 2 == 0;
									UI::ScopedStyleColor headerStyleColor(ImGuiCol_HeaderActive, ImGui::GetStyleColorVec4(ImGuiCol_HeaderActive) * ImVec4(0.95f, 0.95f, 0.95f, 1.0f), even);
									ImGui::Selectable(monitorInfo.Name.c_str(), even, ImGuiSelectableFlags_SpanAllColumns);

									ImGui::TableNextColumn();
									ImGui::Text("%d Hz", monitorInfo.DisplayFrequency);
									ImGui::TableNextColumn();
									ImGui::Text(monitorInfo.GetOrientationString());
									ImGui::TableNextColumn();
									ImGui::Text("%d, %d", monitorInfo.Size.x, monitorInfo.Size.y);

									ImGui::PopID();
								}
							}

							ImGui::EndTable();
						}

						ImGui::TreePop();
					}

					ImGui::EndChild();
				}
				else if (m_CurrentCategory == Navigation)
				{

				}
				else if (m_CurrentCategory == Keymap)
				{
					float widthAvalOver = ImGui::GetContentRegionAvail().x - 40;

					auto& keymap = Preferences::Keymap::GetKeymap();

					static int selectedIndex = 0;

					ImGui::SetNextItemWidth(widthAvalOver / 7 * 2.5f);
					if (ImGui::BeginCombo("##KeymapPresetSelectCombo", m_KeymapPresets.empty() ? "No Available Preset" : m_KeymapPresets[selectedIndex].Name.c_str()))
					{
						if (selectedIndex > m_KeymapPresets.size() - 1)
						{
							selectedIndex = 0;
						}

						for (uint32_t i = 0; i < m_KeymapPresets.size(); i++)
						{
							if (ImGui::Selectable(m_KeymapPresets[i].Name.c_str()))
							{
								selectedIndex = i;
								Preferences::LoadKeymap(m_KeymapPresets[i].Path);
							}
						}

						ImGui::EndCombo();
					}

					ImGui::SameLine();

					if (ImGui::Button(CHARACTER_ICON_IMPORT " Import", ImVec2{ widthAvalOver / 7, 23 })) { ImportKeymap(); }
					ImGui::SameLine();
					if (ImGui::Button(CHARACTER_ICON_EXPORT " Export", ImVec2{ widthAvalOver / 7, 23 })) { ExportKeymap(); }
					ImGui::SameLine();
					if (ImGui::Button(CHARACTER_ICON_RESTART " Restore Binds", ImVec2{ widthAvalOver / 7 * 1.5f, 23 })) { Preferences::LoadKeymap("saved/SavedKeymap.keymap"); }
					ImGui::SameLine();
					if (ImGui::Button(CHARACTER_ICON_SAVE "Save Binds", ImVec2{ widthAvalOver / 7, 23 })) { Preferences::SaveKeymap("saved/SavedKeymap.keymap"); }

					static int currentKeyBindSearchItem = 0;
					const char* SearchTypes[2] = { "Name", "Key Binding" };
					if (ImGui::SwitchButtonEx("KeyBindsSearchBarButtonSwitch", SearchTypes, 2, &currentKeyBindSearchItem, ImVec2{ 200, 23 }))
					{
						m_SearchByNameKey = !(bool)(currentKeyBindSearchItem);
					}

					ImGui::SameLine();

					static char KeybindsSearchBuffer[200] = {};

					ImGui::PushID("KeyBindsSearchBar");
					ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - 35);
					if (ImGui::InputTextWithHint("##SearchBarIDs", "Search:", KeybindsSearchBuffer, sizeof(KeybindsSearchBuffer)))
					{
						m_KeyBindSearchBar = KeybindsSearchBuffer;
					}
					ImGui::PopItemWidth();
					ImGui::SameLine();
					if (ImGui::Button("X", ImVec2{ 23, 23 }))
					{
						memset(KeybindsSearchBuffer, 0, sizeof(KeybindsSearchBuffer));
						m_KeyBindSearchBar = "";
					}
					ImGui::PopID();

					ImGui::BeginChild("##KeymapList");

					ImGui::Dummy(ImVec2{ 0, 10 });
					auto h = ImGui::GetStyleColorVec4(ImGuiCol_Header);
					ImGui::PushStyleColor(ImGuiCol_Header, ImVec4{ h.x + 0.1f, h.y + 0.1f, h.z + 0.1f, h.w + 0.1f });
					if (ImGui::TreeNodeEx("Preferences", treeNodeFlags | ImGuiTreeNodeFlags_DefaultOpen))
					{
						static const char* b[2] = { "Mouse Left", "Mouse Right" };
						int selectedValue = keymap[Preferences::Keymap::SelectObjectBind].MouseCode == Mouse::ButtonRight ? 1 : 0;
						if (ImGui::SwitchButtonEx("##MouseSwitch", b, 2, &selectedValue, ImVec2{ ImGui::GetContentRegionAvail().x, 23 }))
						{
							keymap[Preferences::Keymap::SelectObjectBind].MouseCode = Mouse::ButtonLeft; 
							keymap[Preferences::Keymap::SelectObjectBind].BindCategory = Preferences::Keymap::BindCategory::MouseButton;
						}
						ImGui::TreePop();
					}
					ImGui::PopStyleColor();

					ImGui::Dummy(ImVec2{ 0, 30 });

					{
						bool open = true;
						static bool vis = true;
						if (m_KeyBindSearchBar == "")
						{
							open = ImGui::TreeNodeEx("File", treeNodeFlags); vis = false;
						}
						else if (vis) ImGui::Text("File");
						if (open)
						{
							bool visible = false;
							if (KeyBindInputButton(Preferences::Keymap::NewSceneBind)) visible = true;
							if (KeyBindInputButton(Preferences::Keymap::OpenSceneBind)) visible = true;
							if (KeyBindInputButton(Preferences::Keymap::SaveSceneBind)) visible = true;
							if (KeyBindInputButton(Preferences::Keymap::SaveSceneAsBind)) visible = true;
							if (KeyBindInputButton(Preferences::Keymap::QuitBind)) visible = true;
							if (KeyBindInputButton(Preferences::Keymap::RenameBind)) visible = true;
							vis = visible;
							if (m_KeyBindSearchBar == "") ImGui::TreePop();
						}
					}

					{
						bool open = true;
						static bool vis = true;
						if (m_KeyBindSearchBar == "")
						{
							open = ImGui::TreeNodeEx("Edit", treeNodeFlags); vis = false;
						}
						else if (vis) ImGui::Text("Edit");
						if (open)
						{
							bool visible = false;
							if (KeyBindInputButton(Preferences::Keymap::UndoBind)) visible = true;
							if (KeyBindInputButton(Preferences::Keymap::RedoBind)) visible = true;
							vis = visible;
							if (m_KeyBindSearchBar == "") ImGui::TreePop();
						}
					}

					{
						bool open = true;
						static bool vis = true;
						if (m_KeyBindSearchBar == "")
						{
							open = ImGui::TreeNodeEx("Viewport", treeNodeFlags); vis = false;
						}
						else if (vis) ImGui::Text("Viewport");
						if (open)
						{
							bool visible = false;
							if (KeyBindInputButton(Preferences::Keymap::SelectObjectBind)) visible = true;
							if (KeyBindInputButton(Preferences::Keymap::SceneStartBind)) visible = true;
							if (KeyBindInputButton(Preferences::Keymap::SceneSimulateBind)) visible = true;
							if (KeyBindInputButton(Preferences::Keymap::SceneStopBind)) visible = true;
							if (KeyBindInputButton(Preferences::Keymap::FocusBind)) visible = true;
							if (KeyBindInputButton(Preferences::Keymap::ReloadAssembly)) visible = true;
							if (KeyBindInputButton(Preferences::Keymap::GizmoNoneBind)) visible = true;
							if (KeyBindInputButton(Preferences::Keymap::GizmoTranslateBind)) visible = true;
							if (KeyBindInputButton(Preferences::Keymap::GizmoRotateBind)) visible = true;
							if (KeyBindInputButton(Preferences::Keymap::GizmoScaleBind)) visible = true;
							if (KeyBindInputButton(Preferences::Keymap::CreateBind)) visible = true;
							if (KeyBindInputButton(Preferences::Keymap::DuplicateBind)) visible = true;
							if (KeyBindInputButton(Preferences::Keymap::DeleteBind)) visible = true;
							if (KeyBindInputButton(Preferences::Keymap::VisualizationRenderedBind)) visible = true;
							if (KeyBindInputButton(Preferences::Keymap::VisualizationWireframeBind)) visible = true;
							if (KeyBindInputButton(Preferences::Keymap::VisualizationLightingOnlyBind)) visible = true;
							if (KeyBindInputButton(Preferences::Keymap::VisualizationAlbedoBind)) visible = true;
							if (KeyBindInputButton(Preferences::Keymap::VisualizationNormalBind)) visible = true;
							if (KeyBindInputButton(Preferences::Keymap::VisualizationEntityIDBind)) visible = true;
							if (KeyBindInputButton(Preferences::Keymap::ToggleVisualizationBind)) visible = true;
							if (KeyBindInputButton(Preferences::Keymap::ViewFrontBind)) visible = true;
							if (KeyBindInputButton(Preferences::Keymap::ViewSideBind)) visible = true;
							if (KeyBindInputButton(Preferences::Keymap::ViewTopBind)) visible = true;
							if (KeyBindInputButton(Preferences::Keymap::ViewFlipBind)) visible = true;
							if (KeyBindInputButton(Preferences::Keymap::ViewProjectionBind)) visible = true;
							if (KeyBindInputButton(Preferences::Keymap::OpenCommandLineBind)) visible = true;
							vis = visible;
							if (m_KeyBindSearchBar == "") ImGui::TreePop();
						}
					}

					{
						bool open = true;
						static bool vis = true;
						if (m_KeyBindSearchBar == "")
						{
							open = ImGui::TreeNodeEx("Text Editor", treeNodeFlags); vis = false;
						}
						else if (vis) ImGui::Text("Text Editor");
						if (open)
						{
							bool visible = false;
							if (KeyBindInputButton(Preferences::Keymap::TextEditorDuplicate)) visible = true;
							if (KeyBindInputButton(Preferences::Keymap::TextEditorSwapLineUp)) visible = true;
							if (KeyBindInputButton(Preferences::Keymap::TextEditorSwapLineDown)) visible = true;
							if (KeyBindInputButton(Preferences::Keymap::TextEditorSwitchHeader)) visible = true;
							vis = visible;
							if (m_KeyBindSearchBar == "") ImGui::TreePop();
						}
					}

					{
						bool open = true;
						static bool vis = true;
						if (m_KeyBindSearchBar == "")
						{
							open = ImGui::TreeNodeEx("Interface", treeNodeFlags); vis = false;
						}
						else if (vis) ImGui::Text("Interface");
						if (open)
						{
							bool visible = false;
							if (KeyBindInputButton(Preferences::Keymap::ClosePopupBind)) visible = true;
							vis = visible;
							if (m_KeyBindSearchBar == "") ImGui::TreePop();
						}
					}

					ImGui::EndChild();
				}
				else if (m_CurrentCategory == System)
				{
				}
				else if (m_CurrentCategory == SaveLoad)
				{
					ImGui::SliderInt(CHARACTER_ICON_RECENT " Recent Files", &Preferences::GetData().RecentFileCount, 0, 30);
					bool autosave = ImGui::TreeNodeEx(FA_ALARM_CLOCK " Auto Save", treeNodeFlags);
					ImGui::SameLine();
					ImGui::Checkbox("##AutosavePrefCheckbox", &Preferences::GetData().AutosaveEnabled);
					if (autosave)
					{
						ImGui::SliderInt("Timer (minutes)", &Preferences::GetData().AutosaveTime, 1, 60);
						ImGui::TreePop();
					}
				}
				else if (m_CurrentCategory == FilePaths)
				{
					if (ImGui::TreeNodeEx(CHARACTER_ICON_VISUAL_STUDIO " Development Environment", treeNodeFlags | ImGuiTreeNodeFlags_DefaultOpen))
					{
						ImGui::Text("Devenv Path Detection");

						const char* options[] = { "Automatic", "Manual" };
						int option = Preferences::GetData().ManualDevenv;
						if (ImGui::SwitchButtonEx("##DevenvDetectionMethod", options, 2, &option, ImVec2(ImGui::GetContentRegionAvailWidth(), 30.0f)))
							Preferences::GetData().ManualDevenv = option;

						if (Preferences::GetData().ManualDevenv)
						{
							char buffer[256];
							memset(buffer, 0, sizeof(buffer));
							std::strncpy(buffer, Preferences::GetData().DevenvPath.c_str(), sizeof(buffer));
							if (ImGui::InputText("##DevenvPathInput", buffer, sizeof(buffer)))
								Preferences::GetData().DevenvPath = std::string(buffer);
							ImGui::SameLine();
							ImGui::Text("Devenv Path");
						}
						ImGui::TreePop();
					}

					if (ImGui::TreeNodeEx(FA_CODE_BRANCH " Source Control", treeNodeFlags | ImGuiTreeNodeFlags_DefaultOpen))
					{
						char buffer[256];
						memset(buffer, 0, sizeof(buffer));
						std::strncpy(buffer, Preferences::GetData().GitExecutablePath.c_str(), sizeof(buffer));
						if (ImGui::InputText("##GitExecutablePath", buffer, sizeof(buffer)))
							Preferences::GetData().GitExecutablePath = std::string(buffer);
						ImGui::SameLine();
						ImGui::Text("Git Executable Path");
						ImGui::TreePop();
					}

					if (ImGui::TreeNodeEx(FA_WINDOW_FLIP " Default Applications", treeNodeFlags | ImGuiTreeNodeFlags_DefaultOpen))
					{
						auto& defaultApplications = Preferences::GetData().DefaultApplications;

						ImGui::Text("Extension ");
						ImGui::SameLine();
						
						char buffer[256];
						memset(buffer, 0, sizeof(buffer));
						ImGui::SetNextItemWidth(-1);
						ImGui::InputText("##DefaultApplicationsExtensionInput", buffer, sizeof(buffer));
						
						if ((ImGui::IsItemActivePreviousFrame() && !ImGui::IsItemActive()) || (ImGui::IsItemActive() && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter))))
							if (!std::string(buffer).empty()) 
								defaultApplications[buffer] = "";

						ImGui::Separator();

						const float lineHeight = ImGui::GetTextLineHeightWithSpacing();

						for (auto& [extension, application] : defaultApplications)
						{
							ImGui::PushID(extension.c_str());

							ImGui::Text(extension.c_str());
							ImGui::SameLine();

							std::string applicationName = application.string();
							for (auto& details : s_ApplicationInstallDetails)
							{
								if (details.Path.lexically_normal() == application.lexically_normal())
								{
									ImGui::Image((ImTextureID)details.Icon->GetRendererID(), ImVec2(lineHeight, lineHeight));
									ImGui::SameLine();

									applicationName = details.Name;
									break;
								}
							}
							
							char buffer[256];
							memset(buffer, 0, sizeof(buffer));
							std::strncpy(buffer, applicationName.c_str(), sizeof(buffer));
							ImGui::SetNextItemWidth(-40.0f - style.FramePadding.x * 3.0f);
							ImGui::InputText("##DefaultApplicationInput", buffer, sizeof(buffer));
							
							if ((ImGui::IsItemActivePreviousFrame() && !ImGui::IsItemActive()) || (ImGui::IsItemActive() && ImGui::IsKeyPressed(ImGuiKey_Enter)))
							{
								bool found = false;
								for (auto& details : s_ApplicationInstallDetails)
								{
									if (String::ToLower(details.Name) == String::ToLower(std::string(buffer)))
									{
										application = details.Path;
										found = true;
										break;
									}
								}

								if (!found)
									application = std::string(buffer);
							}

							ImVec2 inputPosition = ImVec2(ImGui::GetItemRectMin().x, ImGui::GetItemRectMax().y);
							if (ImGui::IsItemActive() || ImGui::IsPopupOpen("##DefaultApplicationSearchPopup"))
							{
								ImGui::OpenPopup("##DefaultApplicationSearchPopup");
								ImGui::SetNextWindowSizeConstraints(ImVec2(0.0f, 0.0f), ImVec2(FLT_MAX, 200.0f));
								if (ImGui::BeginPopup("##DefaultApplicationSearchPopup", ImGuiWindowFlags_NoFocusOnAppearing))
								{
									ImGui::SetWindowPos(inputPosition);

									bool found = false;
									for (auto& installedApplication : s_ApplicationInstallDetails)
									{
										if (installedApplication.Path.lexically_normal() == application.lexically_normal())
											continue;

										if (String::ToLower(installedApplication.Name).find(String::ToLower(std::string(buffer))) != std::string::npos)
										{
											found = true;
											ImGui::Image((ImTextureID)installedApplication.Icon->GetRendererID(), ImVec2(lineHeight, lineHeight));
											ImGui::SameLine();
											if (ImGui::MenuItem(installedApplication.Name.c_str()))
												application = installedApplication.Path;
										}
									}

									if (!found)
										ImGui::TextDisabled("No Installed Applications Found");

									ImGui::EndPopup();

									if (ImGui::IsWindowFocused())
										ImGui::CloseCurrentPopup();
								}
							}

							ImGui::SameLine();
							
							if (ImGui::Button(FA_TRASH, ImVec2(40.0f, 0)))
								defaultApplications.erase(extension);

							ImGui::PopID();
						}

						ImGui::TreePop();
					}
				}
				ImGui::EndChild();

				ImGui::EndTable();
			}


			ImGui::End();
		}
	}

	bool PreferencesPanel::KeyBindInputButton(Preferences::Keymap::KeyBindEvent event)
	{
		auto& style = ImGui::GetStyle();
		auto& keybind = Preferences::Keymap::GetKeymap()[event];

		std::string bind_string;
		if (keybind.Ctrl) bind_string += "Ctrl ";
		if (keybind.Shift) bind_string += "Shift ";
		if (keybind.Alt) bind_string += "Alt ";
		bind_string += keybind.BindCategory == Preferences::Keymap::Keyboard ? Preferences::Keymap::GetKeyName(keybind.KeyCode) : Preferences::Keymap::GetMouseButtonName(keybind.MouseCode);

		bool visible = true;
		if (!m_KeyBindSearchBar.empty())
		 visible = (m_SearchByNameKey ? ((String::ToLower(Preferences::Keymap::GetBindEventName(event))).find(String::ToLower(m_KeyBindSearchBar)) != std::string::npos) : ((String::ToLower(bind_string)).find(String::ToLower(m_KeyBindSearchBar)) != std::string::npos));

		if (visible)
		{
			ImGui::PushID(event);
			bool open = ImGui::TreeNodeEx("##KeybindTreeNode", ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding);
			ImGui::SameLine();
			ImGui::Checkbox("##EnabledCheckbox", &keybind.Enabled);
			ImGui::SameLine();
			ImGui::Text(Preferences::Keymap::GetBindEventName(event).c_str());
			ImGui::SameLine(ImGui::GetContentRegionAvail().x - 155);

			if (m_ButtonActive == event)
			{
				bind_string.clear();

				if (Input::IsKeyPressed(Key::LeftControl) || Input::IsKeyPressed(Key::RightControl)) bind_string += "Ctrl ";
				if (Input::IsKeyPressed(Key::LeftShift) || Input::IsKeyPressed(Key::RightShift)) bind_string += "Shift ";
				if (Input::IsKeyPressed(Key::LeftAlt) || Input::IsKeyPressed(Key::RightAlt)) bind_string += "Alt ";
				if (bind_string.empty()) bind_string = "Enter Combination";
			}

			if (ImGui::Button(bind_string.c_str(), ImVec2{ 150, 23 }))
				m_ButtonActive = event;

			if (m_ButtonActive == event && !ImGui::IsItemHovered())
				m_ButtonActive = Preferences::Keymap::INVALID_BIND;

			if (open)
			{
				ImGui::ToggleButton("Ctrl", &keybind.Ctrl, ImVec2{ 45, 23 });
				ImGui::SameLine();
				ImGui::ToggleButton("Shift", &keybind.Shift, ImVec2{ 45, 23 });
				ImGui::SameLine();
				ImGui::ToggleButton("Alt", &keybind.Alt, ImVec2{ 45, 23 });
				ImGui::SameLine();

				const float width = ImGui::GetContentRegionAvailWidth() * 0.5f - style.FramePadding.x * 2.0f;

				ImGui::SetNextItemWidth(width);
				if (ImGui::BeginCombo("##BindCategoryCombo", Preferences::Keymap::GetBindCategoryName(keybind.BindCategory).c_str()))
				{
					if (ImGui::MenuItem("Keyboard")) keybind.BindCategory = Preferences::Keymap::Keyboard;
					if (ImGui::MenuItem("Mouse Button")) keybind.BindCategory = Preferences::Keymap::MouseButton;
					ImGui::EndCombo();
				}

				ImGui::SameLine();

				ImGui::SetNextItemWidth(keybind.BindCategory == Preferences::Keymap::Keyboard ? (width - 100.0f) : width);
				if (ImGui::BeginCombo("##KeyMouseCombo", (keybind.BindCategory == Preferences::Keymap::Keyboard ? Preferences::Keymap::GetKeyName(keybind.KeyCode) : Preferences::Keymap::GetMouseButtonName(keybind.MouseCode)).c_str()))
				{
					if (keybind.BindCategory == Preferences::Keymap::Keyboard)
					{
						for (auto& key : Preferences::Keymap::GetAllKeys())
							if (ImGui::MenuItem(Preferences::Keymap::GetKeyName(key).c_str())) keybind.KeyCode = key;
					}
					else
					{
						for (auto& mouseButton : Preferences::Keymap::GetAllMouseButtons())
							if (ImGui::MenuItem(Preferences::Keymap::GetMouseButtonName(mouseButton).c_str())) keybind.MouseCode = mouseButton;
					}
					ImGui::EndCombo();
				}

				if (keybind.BindCategory == Preferences::Keymap::Keyboard)
				{
					ImGui::SameLine();
					ImGui::Checkbox("##KeyRepeats", &keybind.Repeats);
					ImGui::SameLine();
					ImGui::Text("Repeats");
				}

				ImGui::TreePop();
			}

			ImGui::PopID();
		}
		return visible;
	}

	void PreferencesPanel::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);

		dispatcher.Dispatch<KeyPressedEvent>(DY_BIND_EVENT_FN(PreferencesPanel::OnKeyPressed));
		dispatcher.Dispatch<MouseButtonPressedEvent>(DY_BIND_EVENT_FN(PreferencesPanel::OnMouseButtonPressed));
	}

	bool PreferencesPanel::OnKeyPressed(KeyPressedEvent& e)
	{
		if (m_ButtonActive != Preferences::Keymap::INVALID_BIND)
		{
			auto code = e.GetKeyCode();
			if (code != Key::LeftControl && code != Key::RightControl && code != Key::LeftShift && code != Key::RightShift && code != Key::LeftAlt && code != Key::RightAlt)
			{
				auto& keybind = Preferences::Keymap::GetKeyBind(m_ButtonActive);
				keybind.KeyCode = code;
				keybind.Ctrl = Input::IsKeyPressed(Key::LeftControl) || Input::IsKeyPressed(Key::RightControl);
				keybind.Shift = Input::IsKeyPressed(Key::LeftShift) || Input::IsKeyPressed(Key::RightShift);
				keybind.Alt = Input::IsKeyPressed(Key::LeftAlt) || Input::IsKeyPressed(Key::RightAlt);
				m_ButtonActive = Preferences::Keymap::INVALID_BIND;
			}
		}

		return false;
	}

	bool PreferencesPanel::OnMouseButtonPressed(MouseButtonPressedEvent& e)
	{
		if (m_ButtonActive != Preferences::Keymap::INVALID_BIND)
		{
			auto& keybind = Preferences::Keymap::GetKeyBind(m_ButtonActive);
			keybind.KeyCode = e.GetMouseButton();
			keybind.Ctrl = Input::IsKeyPressed(Key::LeftControl) || Input::IsKeyPressed(Key::RightControl);
			keybind.Shift = Input::IsKeyPressed(Key::LeftShift) || Input::IsKeyPressed(Key::RightShift);
			keybind.Alt = Input::IsKeyPressed(Key::LeftAlt) || Input::IsKeyPressed(Key::RightAlt);
			m_ButtonActive = Preferences::Keymap::INVALID_BIND;
		}

		return false;
	}

	void PreferencesPanel::ImportTheme()
	{
		std::string filepath = FileDialogs::OpenFile("Dymatic Theme (*.dytheme)\0*.dytheme\0");
		if (!filepath.empty())
			Preferences::LoadTheme(filepath);
	}

	void PreferencesPanel::ExportTheme()
	{
		std::string filepath = FileDialogs::SaveFile("Dymatic Theme (*.dytheme)\0*.dytheme\0");
		if (!filepath.empty())
			Preferences::SaveTheme(filepath);
	}

	void PreferencesPanel::ImportKeymap()
	{
		std::string filepath = FileDialogs::OpenFile("Keymap (*.keymap)\0*.keymap\0");
		if (!filepath.empty())
			Preferences::LoadKeymap(filepath);
	}

	void PreferencesPanel::ExportKeymap()
	{
		std::string filepath = FileDialogs::SaveFile("Keymap (*.keymap)\0*.keymap\0");
		if (!filepath.empty())
			Preferences::SaveKeymap(filepath);
	}

	void PreferencesPanel::ImportPreferences()
	{
		std::string filepath = FileDialogs::OpenFile("Preferences (*.prefs)\0*.prefs\0");
		if (!filepath.empty())
			Preferences::LoadPreferences(filepath);
	}

	void PreferencesPanel::ExportPreferences()
	{
		std::string filepath = FileDialogs::SaveFile("Preferences (*.prefs)\0*.prefs\0");
		if (!filepath.empty())
			Preferences::SavePreferences(filepath);
	}

	static void PopulatePresets(const std::filesystem::path& manifestPath, std::vector<PreferencesPanel::PreferencesPreset>& presets)
	{
		YAML::Node data;
		try
		{
			data = YAML::LoadFile(manifestPath.string());
		}
		catch (YAML::ParserException e)
		{
			DY_CORE_ERROR("Failed to load preset manifest file '{}'\n     {}", manifestPath.string(), e.what());
			return;
		}

		auto manifest = data["Manifest"];

		if (!manifest)
			return;

		for (auto item : manifest)
		{
			PreferencesPanel::PreferencesPreset preset;
			preset.Name = item.first.as<std::string>();
			preset.Path = manifestPath.parent_path() / item.second.as<std::string>();
			presets.push_back(preset);
		}
	}

	void PreferencesPanel::LoadAvailablePresets()
	{
		m_ThemePresets.clear();
		m_KeymapPresets.clear();
		
		PopulatePresets("saved/presets/themes/Themes.manifest", m_ThemePresets);
		PopulatePresets("saved/presets/keymaps/Keymaps.manifest", m_KeymapPresets);
	}

	void PreferencesPanel::RefreshPlugins()
	{
		m_PluginInfo.clear();
		for (auto& file : std::filesystem::directory_iterator("Resources/Plugins"))
			if (file.is_directory())
			{
#ifdef DY_DEBUG
				auto path = file.path() / "Debug";
#else
				auto path = file.path() / "Release";
#endif
					if (std::filesystem::exists(path))
					{
						for (auto& file : std::filesystem::directory_iterator(path))
							if (file.path().extension().string() == ".dll")
							{
								PluginInfo info;
								if (PluginLoader::GetPluginInfo(file.path(), info))
									m_PluginInfo.push_back(info);
							}
					}
			}
		LoadPluginManifest();
	}

	void PreferencesPanel::LoadPluginManifest()
	{
		if (std::filesystem::exists("Resources/Plugins/PluginsManifest"))
		{
			std::ifstream file("Resources/Plugins/PluginsManifest");
			std::string data;

			while (getline(file, data, '|'))
			{
				bool found = false;
				if (!data.empty())
				{
					for (auto& plugin : m_PluginInfo)
						if (plugin.path.filename() == data)
						{
							found = true;

							getline(file, data, '|');
							getline(file, data, '|');
							plugin.enabled = data == "1";
							getline(file, data, '|');
							while (data != "_END_PLUGIN")
							{
								std::string type;
								getline(file, type, '|');

								for (auto& param : plugin.params)
								{
									if (param.name == data)
									{
										switch (param.type)
										{
										case PluginInfo::PluginParam::Bool:
											param.data.Bool = stoi(type);
											break;
										case PluginInfo::PluginParam::Int:
											param.data.Int = stoi(type);
											break;
										case PluginInfo::PluginParam::Float:
											param.data.Float = stof(type);
											break;
										case PluginInfo::PluginParam::String:
											param.data.String = type.c_str();
											break;
										}
									}
								}

								getline(file, data, '|');
							}
						}
				}

				if (!found)
				{
					while (getline(file, data, '|'))
					{
						if (data == "_END_PLUGIN")
							break;
					}
				}
			}
			file.close();
		}
	}

	void PreferencesPanel::WritePluginManifest()
	{
		std::ofstream manifest;
		manifest.open("Resources/Plugins/PluginsManifest");
		for (auto& plugin : m_PluginInfo)
		{
			manifest << plugin.path.filename().string() << "|";
			{
				auto path = plugin.path.string();
				String::ReplaceAll(path, '\\', '/');
				const char* loc = "Resources/Plugins/";
				int index = path.find(loc);
				if (index != std::string::npos)
				{
					path.erase(0, index + strlen(loc));
					path.erase(path.find("/"));
					manifest << path << "|";
				}
			}
			manifest << plugin.enabled << "|";

			for (auto& param : plugin.params)
			{
				switch (param.type)
				{
				case PluginInfo::PluginParam::Bool:
					if (param.data.Bool != param.defaultValue.Bool)
					{
						manifest << param.name << "|";
						manifest << param.data.Bool << "|";
					}
					break;
				case PluginInfo::PluginParam::Int:
					if (param.data.Int != param.defaultValue.Int)
					{
						manifest << param.name << "|";
						manifest << param.data.Int << "|";
					}
					break;
				case PluginInfo::PluginParam::Float:
					if (param.data.Float != param.defaultValue.Float)
					{
						manifest << param.name << "|";
						manifest << param.data.Float << "|";
					}
					break;
				case PluginInfo::PluginParam::String:
					if (param.data.String != param.defaultValue.String)
					{
						manifest << param.name << "|";
						manifest << param.data.String << "|";
					}
					break;
				}

			}
			manifest << "_END_PLUGIN|";
		}
		manifest.close();
	}

}