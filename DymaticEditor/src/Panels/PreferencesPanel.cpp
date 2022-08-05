#include "PreferencesPanel.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <glm/gtc/type_ptr.hpp>

#include "../Preferences.h"

#include "../TextSymbols.h"
#include "Dymatic/Math/Math.h"
#include "Dymatic/Utils/PlatformUtils.h"
#include "Dymatic/Math/StringUtils.h"


namespace Dymatic {

	PreferencesPannel::PreferencesPannel()
	{
		Preferences::LoadPreferences("saved/presets/DefaultPreferences.prefs");
		Preferences::LoadPreferences("saved/SavedPreferences.prefs");

		Preferences::LoadTheme("saved/presets/themes/DymaticDark.dytheme");
		Preferences::LoadTheme("saved/SavedTheme.dytheme");

		Preferences::LoadKeymap("saved/presets/keymaps/DymaticDefault.keymap");
		Preferences::LoadKeymap("saved/SavedKeymap.keymap");

		LoadPresetLayout();

		RefreshPlugins();
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
				ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
				ImGui::Text(tooltip);
				ImGui::PopFont();
				ImGui::EndTooltip();
			}
		}
		ImGui::PopID();
	}

	void PreferencesPannel::OnImGuiRender()
	{
		static bool previousPreferencesShowWindow = m_PreferencesPanelVisible;

		if ((previousPreferencesShowWindow && !m_PreferencesPanelVisible) && Preferences::GetData().AutosavePreferences)
		{
			Preferences::SavePreferences("saved/SavedPreferences.prefs");
		}

		previousPreferencesShowWindow = m_PreferencesPanelVisible;

		if (m_PreferencesPanelVisible)
		{
			ImGui::Begin((std::string(CHARACTER_WINDOW_ICON_PREFERENCES) + " Preferences").c_str(), &m_PreferencesPanelVisible, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

			if (ImGui::BeginTable("##PreferencesSplitterTable", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchSame))
			{
				ImGui::TableNextColumn();

				const char* categoriesA[4] = { "Interface", "Themes", "Viewport", "Editing" };
				const char* categoriesB[3] = { "Input", "Navigation", "Keymap" };
				const char* categoriesC[1] = { "Plugins" };
				const char* categoriesD[3] = { "System", "Save & Load", "File Paths" };
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
				if (ImGui::ImageButton(reinterpret_cast<void*>(m_IconMoreOptions->GetRendererID()), ImVec2{ 15, 15 }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 }))
				{
					ImGui::OpenPopup("##PreferencesOptionsPopup");
				}

				if (ImGui::BeginPopup("##PreferencesOptionsPopup", ImGuiWindowFlags_NoMove))
				{
					if (ImGui::MenuItem("Load Factory Defaults")) { Preferences::LoadPreferences("saved/presets/DefaultPreferences.prefs"); }
					if (ImGui::MenuItem("Restore Saved Preferences ")) { Preferences::LoadPreferences("saved/SavedPreferences.prefs"); }
					if (ImGui::MenuItem("Import Preferences ")) { ImportPreferences(); }
					if (ImGui::MenuItem("Export Preferences ")) { ExportPreferences(); }
					if (ImGui::MenuItem("Save Preferences ")) { Preferences::SavePreferences("saved/SavedPreferences.prefs"); }
					ImGui::Checkbox("##PreferencesAutosaveCheckbox", &Preferences::GetData().AutosavePreferences);
					ImGui::SameLine();
					ImGui::Text("Auto-Save Preferences");
					ImGui::EndPopup();
				}

				ImGui::TableNextColumn();

				ImGui::BeginChild("##PreferencesPannelContent", {}, false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
				const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
				if (m_CurrentCategory == Interface)
				{
					if (ImGui::TreeNodeEx("Display", treeNodeFlags | ImGuiTreeNodeFlags_DefaultOpen))
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
					if (ImGui::BeginCombo("##ThemeSelectCombo", m_SelectableThemeNames.empty() ? "Unknown Value" : m_SelectableThemeNames[selectedIndex].c_str()))
					{
						if (selectedIndex > m_SelectableThemeNames.size() - 1)
						{
							selectedIndex = 0;
						}
						if (!m_SelectableThemeNames.empty())
						{
							for (int i = 0; i < m_SelectableThemeNames.size(); i++)
							{
								if (ImGui::Selectable(m_SelectableThemeNames[i].c_str()))
								{
									selectedIndex = i;
									Preferences::LoadTheme(m_SelectableThemePaths[i]);
								}
							}
						}
						ImGui::EndCombo();
					}

					ImGui::SameLine();
					if (ImGui::Button("Import", ImVec2{ widthAvalOver / 7, 23 })) { ImportTheme(); }
					ImGui::SameLine();
					if (ImGui::Button("Export", ImVec2{ widthAvalOver / 7, 23 })) { ExportTheme(); }
					ImGui::SameLine();
					if (ImGui::Button("Restore Theme", ImVec2{ widthAvalOver / 7 * 1.5f, 23 })) { Preferences::LoadTheme("saved/SavedTheme.dytheme"); }
					ImGui::SameLine();
					if (ImGui::Button("Save Theme", ImVec2{ widthAvalOver / 7, 23 })) { Preferences::SaveTheme("saved/SavedTheme.dytheme"); }

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
						EditThemeColor(ImGuiCol_MainWindowBorder);
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

				}
				else if (m_CurrentCategory == Editing)
				{

				}
				else if (m_CurrentCategory == Plugins)
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
				}
				else if (m_CurrentCategory == Input)
				{
					if (ImGui::TreeNodeEx("Keyboard", treeNodeFlags | ImGuiTreeNodeFlags_DefaultOpen))
					{
						ImGui::Text("Emulate Numpad");
						ImGui::SameLine();
						ImGui::Checkbox("##KeyboardEmulateNumpadCheckbox", &Preferences::GetData().EmulateNumpad);
						ImGui::TreePop();
					}
					if (ImGui::TreeNodeEx("Mouse", treeNodeFlags | ImGuiTreeNodeFlags_DefaultOpen))
					{
						ImGui::Text("Double Click Speed");
						ImGui::SameLine();
						if (ImGui::SliderInt("##MouseDoubleClickSpeedSlider", &Preferences::GetData().DoubleClickSpeed, 1, 1000))
						{
							ImGui::GetIO().MouseDoubleClickTime = Preferences::GetData().DoubleClickSpeed / 1000.0f;
						}
						ImGui::TreePop();
					}
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
					if (ImGui::BeginCombo("##KeybindSelectCombo", m_SelectableKeybindNames.empty() ? "Unknown Value" : m_SelectableKeybindNames[selectedIndex].c_str()))
					{
						if (selectedIndex > m_SelectableKeybindNames.size() - 1)
						{
							selectedIndex = 0;
						}
						if (!m_SelectableKeybindNames.empty())
						{
							for (int i = 0; i < m_SelectableKeybindNames.size(); i++)
							{
								if (ImGui::Selectable(m_SelectableKeybindNames[i].c_str()))
								{
									selectedIndex = i;
									Preferences::LoadKeymap(m_SelectableKeybindPaths[i]);
								}
							}
						}
						ImGui::EndCombo();
					}

					ImGui::SameLine();

					if (ImGui::Button("Import", ImVec2{ widthAvalOver / 7, 23 })) { ImportKeymap(); }
					ImGui::SameLine();
					if (ImGui::Button("Export", ImVec2{ widthAvalOver / 7, 23 })) { ExportKeymap(); }
					ImGui::SameLine();
					if (ImGui::Button("Restore Binds", ImVec2{ widthAvalOver / 7 * 1.5f, 23 })) { Preferences::LoadKeymap("saved/SavedKeymap.keymap"); }
					ImGui::SameLine();
					if (ImGui::Button("Save Binds", ImVec2{ widthAvalOver / 7, 23 })) { Preferences::SaveKeymap("saved/SavedKeymap.keymap"); }

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
							open = ImGui::TreeNodeEx("Window", treeNodeFlags); vis = false;
						}
						else if (vis) ImGui::Text("Window");
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
							open = ImGui::TreeNodeEx("3D View", treeNodeFlags); vis = false;
						}
						else if (vis) ImGui::Text("3D View");
						if (open)
						{
							bool visible = false;
							if (KeyBindInputButton(Preferences::Keymap::SelectObjectBind)) visible = true;
							if (KeyBindInputButton(Preferences::Keymap::SceneStartBind)) visible = true;
							if (KeyBindInputButton(Preferences::Keymap::SceneStopBind)) visible = true;
							if (KeyBindInputButton(Preferences::Keymap::GizmoNoneBind)) visible = true;
							if (KeyBindInputButton(Preferences::Keymap::GizmoTranslateBind)) visible = true;
							if (KeyBindInputButton(Preferences::Keymap::GizmoRotateBind)) visible = true;
							if (KeyBindInputButton(Preferences::Keymap::GizmoScaleBind)) visible = true;
							if (KeyBindInputButton(Preferences::Keymap::CreateBind)) visible = true;
							if (KeyBindInputButton(Preferences::Keymap::DuplicateBind)) visible = true;
							if (KeyBindInputButton(Preferences::Keymap::DeleteBind)) visible = true;
							if (KeyBindInputButton(Preferences::Keymap::ShadingTypeWireframeBind)) visible = true;
							if (KeyBindInputButton(Preferences::Keymap::ShadingTypeUnlitBind)) visible = true;
							if (KeyBindInputButton(Preferences::Keymap::ShadingTypeSolidBind)) visible = true;
							if (KeyBindInputButton(Preferences::Keymap::ShadingTypeRenderedBind)) visible = true;
							if (KeyBindInputButton(Preferences::Keymap::ToggleShadingTypeBind)) visible = true;
							if (KeyBindInputButton(Preferences::Keymap::ViewFrontBind)) visible = true;
							if (KeyBindInputButton(Preferences::Keymap::ViewSideBind)) visible = true;
							if (KeyBindInputButton(Preferences::Keymap::ViewTopBind)) visible = true;
							if (KeyBindInputButton(Preferences::Keymap::ViewFlipBind)) visible = true;
							if (KeyBindInputButton(Preferences::Keymap::ViewProjectionBind)) visible = true;
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
					ImGui::SliderInt("Recent Files", &Preferences::GetData().RecentFileCount, 0, 30);
					bool autosave = ImGui::TreeNodeEx("Auto Save", treeNodeFlags);
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
					if (ImGui::TreeNodeEx("Development Environment", treeNodeFlags | ImGuiTreeNodeFlags_DefaultOpen))
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
				}
				ImGui::EndChild();

				ImGui::EndTable();
			}


			ImGui::End();
		}
	}

	bool PreferencesPannel::KeyBindInputButton(Preferences::Keymap::KeyBindEvent event)
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

	void PreferencesPannel::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);

		dispatcher.Dispatch<KeyPressedEvent>(DY_BIND_EVENT_FN(PreferencesPannel::OnKeyPressed));
		dispatcher.Dispatch<MouseButtonPressedEvent>(DY_BIND_EVENT_FN(PreferencesPannel::OnMouseButtonPressed));
	}

	bool PreferencesPannel::OnKeyPressed(KeyPressedEvent& e)
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

	bool PreferencesPannel::OnMouseButtonPressed(MouseButtonPressedEvent& e)
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

	void PreferencesPannel::ImportTheme()
	{
		std::string filepath = FileDialogs::OpenFile("Dymatic Theme (*.dytheme)\0*.dytheme\0");
		if (!filepath.empty())
			Preferences::LoadTheme(filepath);
	}

	void PreferencesPannel::ExportTheme()
	{
		std::string filepath = FileDialogs::SaveFile("Dymatic Theme (*.dytheme)\0*.dytheme\0");
		if (!filepath.empty())
			Preferences::SaveTheme(filepath);
	}

	void PreferencesPannel::ImportKeymap()
	{
		std::string filepath = FileDialogs::OpenFile("Keymap (*.keymap)\0*.keymap\0");
		if (!filepath.empty())
			Preferences::LoadKeymap(filepath);
	}

	void PreferencesPannel::ExportKeymap()
	{
		std::string filepath = FileDialogs::SaveFile("Keymap (*.keymap)\0*.keymap\0");
		if (!filepath.empty())
			Preferences::SaveKeymap(filepath);
	}

	void PreferencesPannel::ImportPreferences()
	{
		std::string filepath = FileDialogs::OpenFile("Preferences (*.prefs)\0*.prefs\0");
		if (!filepath.empty())
			Preferences::LoadPreferences(filepath);
	}

	void PreferencesPannel::ExportPreferences()
	{
		std::string filepath = FileDialogs::SaveFile("Preferences (*.prefs)\0*.prefs\0");
		if (!filepath.empty())
			Preferences::SavePreferences(filepath);
	}

	void PreferencesPannel::LoadPresetLayout()
	{
		// Load Themes
		{
			std::string result;
			std::ifstream in("saved/presets/themes/ThemePresets.txt", std::ios::in | std::ios::binary); // ifstream closes itself due to RAII

			if (in)
			{
				in.seekg(0, std::ios::end);
				size_t size = in.tellg();
				if (size != -1)
				{
					result.resize(size);
					in.seekg(0, std::ios::beg);
					in.read(&result[0], size);
				}
			}

			while (result.find_first_of("\n") != -1)
			{
				result = result.erase(result.find_first_of("\n"), 1);
			}

			while (result.find_first_of("\r") != -1)
			{
				result = result.erase(result.find_first_of("\r"), 1);
			}

			bool openValueName = false;
			bool openValue = false;
			std::string CurrentValueName = "";
			std::string CurrentValue = "";
			for (int i = 0; i < result.length(); i++)
			{
				std::string character = result.substr(i, 1);

				if (character == ">") { openValueName = false; }
				if (openValueName) { CurrentValueName = CurrentValueName + character; }
				if (character == "<") { openValueName = true; CurrentValueName = ""; }

				if (character == "}") { openValue = false; }
				if (openValue) { CurrentValue = CurrentValue + character; }
				if (character == "{") { openValue = true; CurrentValue = ""; }

				if (character == "}" && CurrentValue != "")
				{
					m_SelectableThemeNames.push_back(CurrentValueName);
					m_SelectableThemePaths.push_back(CurrentValue);
				}
			}
		}

		// Load Keymaps
		{
			std::string result;
			std::ifstream in("saved/presets/keymaps/KeymapPresets.txt", std::ios::in | std::ios::binary); // ifstream closes itself due to RAII

			if (in)
			{
				in.seekg(0, std::ios::end);
				size_t size = in.tellg();
				if (size != -1)
				{
					result.resize(size);
					in.seekg(0, std::ios::beg);
					in.read(&result[0], size);
				}
			}

			while (result.find_first_of("\n") != -1)
			{
				result = result.erase(result.find_first_of("\n"), 1);
			}

			while (result.find_first_of("\r") != -1)
			{
				result = result.erase(result.find_first_of("\r"), 1);
			}

			bool openValueName = false;
			bool openValue = false;
			std::string CurrentValueName = "";
			std::string CurrentValue = "";
			for (int i = 0; i < result.length(); i++)
			{
				std::string character = result.substr(i, 1);

				if (character == ">") { openValueName = false; }
				if (openValueName) { CurrentValueName = CurrentValueName + character; }
				if (character == "<") { openValueName = true; CurrentValueName = ""; }

				if (character == "}") { openValue = false; }
				if (openValue) { CurrentValue = CurrentValue + character; }
				if (character == "{") { openValue = true; CurrentValue = ""; }

				if (character == "}" && CurrentValue != "")
				{
					m_SelectableKeybindNames.push_back(CurrentValueName);
					m_SelectableKeybindPaths.push_back(CurrentValue);
				}
			}
		}
	}

	void PreferencesPannel::RefreshPlugins()
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

	void PreferencesPannel::LoadPluginManifest()
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

	void PreferencesPannel::WritePluginManifest()
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