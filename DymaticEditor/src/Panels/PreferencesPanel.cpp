#include "PreferencesPanel.h"
#include "ImGuiCustom.h"
#define IMGUI_DEFINE_MATH_OPERATORS

#include "Dymatic/Math/Math.h"

#include <glm/gtc/type_ptr.hpp>

namespace Dymatic {



	PreferencesPannel::PreferencesPannel()
	{
		OpenPreferencesByFilepath("saved/presets/DefaultPreferences.prefs");
		OpenPreferencesByFilepath("saved/SavedPreferences.prefs");

		OpenThemeByPath("saved/presets/themes/DymaticDark.dytheme");
		OpenThemeByPath("saved/SavedTheme.dytheme");

		OpenKeyBindsByFilepath("saved/presets/keybinds/DymaticDefault.keybind");
		OpenKeyBindsByFilepath("saved/SavedKeyBinds.keybind");

	}

	void PreferencesPannel::OnImGuiRender()
	{
		static bool previousPreferencesShowWindow = m_ShowWindow;

		if ((previousPreferencesShowWindow && !m_ShowWindow) && GetPreferences().m_PreferenceData.autosavePreferences)
		{
			SavePreferencesByFilepath("saved/SavedPreferences.prefs");
		}

		previousPreferencesShowWindow = m_ShowWindow;

		if (m_ShowWindow)
		{
			ImGui::Begin("Preferences", &m_ShowWindow);

			float h = ImGui::GetContentRegionAvail().y;
			float minSize = 20.0f;
			float sz1 = ImGui::GetContentRegionAvail().x / 4 - variation1;
			float sz2 = ImGui::GetContentRegionAvail().x / 4 * 3 - variation2;

			ImGui::Custom::Splitter(true, 2.0f, &sz1, &sz2, minSize, minSize, h);
			variation1 = (ImGui::GetContentRegionAvail().x / 4) - sz1;
			variation2 = (ImGui::GetContentRegionAvail().x / 4 * 3) - sz2;

			ImGui::BeginChild("##Tabs", ImVec2(sz1, h), false);

			std::string clickedCatagory = (DrawRoundedButtonStack({ "System", "Save & Load", "File Paths" }) +
				DrawRoundedButtonStack({ "Input", "Navigation", "Keymap" }) +
				DrawRoundedButtonStack({ "Plugins" }) +
				DrawRoundedButtonStack({ "Interface", "Themes", "Viewport", "Editing" }));

			if (clickedCatagory != m_CurrentCatagory && clickedCatagory != "")
			{
				m_CurrentCatagory = clickedCatagory;
			}

			ImGui::Dummy(ImVec2{ 0, ImGui::GetContentRegionAvail().y - 25 });
			if (ImGui::ImageButton(reinterpret_cast<void*>(m_IconMoreOptions->GetRendererID()), ImVec2{ 15, 15 }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 }))
			{
				ImGui::OpenPopup("##PreferencesOptionsPopup");
			}

			if (ImGui::BeginPopup("##PreferencesOptionsPopup", ImGuiWindowFlags_NoMove))
			{
				if (ImGui::MenuItem("Load Factory Defaults")) { OpenPreferencesByFilepath("saved/presets/DefaultPreferences.prefs"); }
				if (ImGui::MenuItem("Restore Saved Preferences ")) { OpenPreferencesByFilepath("saved/SavedPreferences.prefs"); }
				if (ImGui::MenuItem("Import Preferences ")) { ImportPreferences(); }
				if (ImGui::MenuItem("Export Preferences ")) { ExportPreferences(); }
				if (ImGui::MenuItem("Save Preferences ")) { SavePreferencesByFilepath("saved/SavedPreferences.prefs"); }
				ImGui::Custom::Checkbox("##PreferencesAutosaveCheckbox", &GetPreferences().m_PreferenceData.autosavePreferences);
				ImGui::SameLine();
				ImGui::Text("Auto-Save Preferences");
				ImGui::EndPopup();
			}

			ImGui::EndChild();

			ImGui::SameLine();

			ImGui::BeginChild("##PathsViewerWindow", ImVec2(sz2, h), false);
			const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
			auto& preferencesData = GetPreferences().m_PreferenceData;
			auto& values = preferencesData.colorScheme.colorSchemeValues;
			if (m_CurrentCatagory == "Interface")
			{
				if (ImGui::TreeNodeEx("Display", treeNodeFlags | ImGuiTreeNodeFlags_DefaultOpen))
				{
					ImGui::Custom::Checkbox("Show Splash", &preferencesData.showSplash);
					ImGui::TreePop();
				}
				if (ImGui::TreeNodeEx("File Colors", treeNodeFlags))
				{
					auto& fileColors = GetPreferences().m_PreferenceData.fileColors;

					if (ImGui::Button("New"))
					{
						FileColor newFileColor;
						newFileColor.Color = ImGui::Custom::GetImGuiCustomColorValue(ImGui::Custom::ImGuiCol_FileBackground);
						fileColors.push_back(newFileColor);
					}
					ImGui::SameLine();
					if (ImGui::Button("Clear All"))
					{
						fileColors.clear();
					}
					if (!fileColors.empty())
					{
						for (int i = 0; i < GetPreferences().m_PreferenceData.fileColors.size(); i++)
						{
							ImGui::InputTextWithHint(("##fileColor" + std::to_string(i)).c_str(), "Extension: ", fileColors[i].Extension, sizeof(fileColors[i].Extension));
							ImGui::SameLine();
							ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 35);
							ImGui::ColorEdit3(("##fileColorPicker" + std::to_string(i)).c_str(), (float*)&fileColors[i].Color);
							ImGui::SameLine();
							if (ImGui::Button(("X##PreferencesFileColVal" + std::to_string(i)).c_str(), ImVec2{ 23, 23 })) { fileColors.erase(fileColors.begin() + i); };
						}
					}
					ImGui::TreePop();
				}
			}
			else if (m_CurrentCatagory == "Themes")
			{
				float widthAvalOver = ImGui::GetContentRegionAvail().x - 30;

				static const char* items[]{ "Dymatic Dark", "Dymatic Light", "Sunset Gold" };
				static int selectedItem = 0;

				ImGui::SetNextItemWidth(widthAvalOver / 6 * 3);
				if (ImGui::Combo("##Preset", &selectedItem, items, IM_ARRAYSIZE(items)))
				{
					switch (selectedItem)
					{

					case 0:
					{
						OpenThemeByPath("saved/presets/themes/DymaticDark.dytheme");
						break;
					}

					case 1:
					{
						OpenThemeByPath("saved/presets/themes/DymaticLight.dytheme");
						break;
					}

					case 2:
					{
						OpenThemeByPath("saved/presets/themes/SunsetGold.dytheme");
						break;
					}

					}
				}

				ImGui::SameLine();
				if (ImGui::Button("Import", ImVec2{ widthAvalOver / 6, 23 })) { ImportTheme(); }
				ImGui::SameLine();
				if (ImGui::Button("Export", ImVec2{ widthAvalOver / 6, 23 })) { ExportTheme(); }
				ImGui::SameLine();
				if (ImGui::Button("Save Theme", ImVec2{widthAvalOver / 6, 23})) { SaveThemeByPath("saved/SavedTheme.dytheme"); }

				ImGui::BeginChild("##ThemePrefsChild");

				if (ImGui::TreeNodeEx("Text", treeNodeFlags))
				{
					EditThemeColor(Text, "Plain text used on all buttons, popups and windows throughout Dymatic.");
					EditThemeColor(TextDisabled, "Text that is unavalible to the user or cannot be interacted with. (Used for shortcut hints)");
					EditThemeColor(TextSelectedBg, "Highlight color behind text when selected with cursor.");
					ImGui::TreePop();
				}

				if (ImGui::TreeNodeEx("Windows", treeNodeFlags))
				{
					EditThemeColor(MenuBarBg, "Main menu bar, located at the top of the main window.");
					EditThemeColor(WindowBg, "Default background color for all windows.");
					ImGui::TreePop();
				}

				if (ImGui::TreeNodeEx("Tabs", treeNodeFlags))
				{
					EditThemeColor(Header, "Header colors are used for CollapsingHeader, TreeNode, selectable and Menu Items");
					EditThemeColor(HeaderHovered);
					EditThemeColor(HeaderActive);
					ImGui::Separator();
					EditThemeColor(Tab);
					EditThemeColor(TabHovered);
					EditThemeColor(TabActive);
					EditThemeColor(TabUnfocused);
					EditThemeColor(TabUnfocusedActive);
					ImGui::Separator();
					EditThemeColor(TitleBg);
					EditThemeColor(TitleBgActive);
					EditThemeColor(TitleBgCollapsed);
					ImGui::TreePop();
				}

				if (ImGui::TreeNodeEx("Buttons", treeNodeFlags))
				{
					EditThemeColor(Button);
					EditThemeColor(ButtonHovered);
					EditThemeColor(ButtonActive);
					EditThemeColor(ButtonToggled);
					EditThemeColor(ButtonToggledHovered);
					ImGui::TreePop();
				}

				if (ImGui::TreeNodeEx("Popups & Modals", treeNodeFlags))
				{
					EditThemeColor(PopupBg, "Background of popups, menus, tooltips windows.");
					EditThemeColor(ModalWindowDimBg);
					ImGui::TreePop();
				}
				if (ImGui::TreeNodeEx("Borders", treeNodeFlags))
				{
					EditThemeColor(Border);
					EditThemeColor(BorderShadow);
					ImGui::TreePop();
				}

				if (ImGui::TreeNodeEx("Frames", treeNodeFlags))
				{
					EditThemeColor(FrameBg, "Background of radio button, plot, slider and text input.");
					EditThemeColor(FrameBgHovered);
					EditThemeColor(FrameBgActive);
					ImGui::TreePop();
				}

				if (ImGui::TreeNodeEx("Scrollbars", treeNodeFlags))
				{
					EditThemeColor(ScrollbarBg);
					EditThemeColor(ScrollbarGrab);
					EditThemeColor(ScrollbarGrabHovered);
					EditThemeColor(ScrollbarGrabActive);
					EditThemeColor(ScrollbarDots);
					ImGui::TreePop();
				}

				if (ImGui::TreeNodeEx("Slider", treeNodeFlags))
				{
					EditThemeColor(SliderGrab);
					EditThemeColor(SliderGrabActive);
					ImGui::TreePop();
				}

				if (ImGui::TreeNodeEx("Separator", treeNodeFlags))
				{
					EditThemeColor(Separator);
					EditThemeColor(SeparatorHovered);
					EditThemeColor(SeparatorActive);
					ImGui::TreePop();
				}

				if (ImGui::TreeNodeEx("Resize", treeNodeFlags))
				{
					EditThemeColor(ResizeGrip);
					EditThemeColor(ResizeGripHovered);
					EditThemeColor(ResizeGripActive);
					ImGui::TreePop();
				}

				if (ImGui::TreeNodeEx("Content Browser", treeNodeFlags))
				{
					EditThemeColor(FileBackground);
					EditThemeColor(FileIcon);
					EditThemeColor(FileHovered);
					EditThemeColor(FileSelected);
					ImGui::TreePop();
				}

				if (ImGui::TreeNodeEx("Checkboxes", treeNodeFlags))
				{
					EditThemeColor(CheckMark);
					EditThemeColor(Checkbox);
					EditThemeColor(CheckboxHovered);
					EditThemeColor(CheckboxActive);
					EditThemeColor(CheckboxTicked);
					EditThemeColor(CheckboxHoveredTicked);
					ImGui::TreePop();
				}

				if (ImGui::TreeNodeEx("Docking", treeNodeFlags))
				{
					EditThemeColor(DockingPreview, "Preview overlay color when about to docking something.");
					EditThemeColor(DockingEmptyBg, "Background color for empty node.");
					ImGui::TreePop();
				}

				if (ImGui::TreeNodeEx("Plots", treeNodeFlags))
				{
					EditThemeColor(PlotLines);
					EditThemeColor(PlotLinesHovered);
					EditThemeColor(PlotHistogram);
					EditThemeColor(PlotHistogramHovered);
					ImGui::TreePop();
				}

				if (ImGui::TreeNodeEx("Drag & Drop", treeNodeFlags))
				{
					EditThemeColor(DragDropTarget);
					ImGui::TreePop();
				}

				if (ImGui::TreeNodeEx("Nav", treeNodeFlags))
				{
					EditThemeColor(NavHighlight, "Current highlighted item with keyboard.");
					EditThemeColor(NavWindowingHighlight, "Highlight window when using CTRL + TAB");
					EditThemeColor(NavWindowingDimBg, "Darken/colorize entire screen behind the window list, when active");
					ImGui::TreePop();
				}




				ImGui::EndChild();
			}
			else if (m_CurrentCatagory == "Viewport")
			{

			}
			else if (m_CurrentCatagory == "Editing")
			{

			}
			else if (m_CurrentCatagory == "Plugins")
			{

			}
			else if (m_CurrentCatagory == "Input")
			{
				if (ImGui::TreeNodeEx("Mouse", treeNodeFlags | ImGuiTreeNodeFlags_DefaultOpen))
				{
					ImGui::Text("Double Click Speed");
					ImGui::SameLine();
					ImGui::SliderInt("##MouseDoubleClickSpeedSlider", &GetPreferences().m_PreferenceData.doubleClickSpeed, 1, 1000);
					ImGui::TreePop();
				}
			}
			else if (m_CurrentCatagory == "Navigation")
			{
				//Emulate numpad
			}
			else if (m_CurrentCatagory == "Keymap")
			{
			float widthAvalOver = ImGui::GetContentRegionAvail().x - 30;

			auto& keyBinds = GetPreferences().m_PreferenceData.keyBinds;

				static const char* items[]{ "Dymatic Default" };
				static int selectedItem = 0;

				ImGui::SetNextItemWidth(widthAvalOver / 6 * 3);
				if (ImGui::Combo("##PresetKeymap", &selectedItem, items, IM_ARRAYSIZE(items)))
				{
					switch (selectedItem)
					{

					case 0:
					{
						OpenKeyBindsByFilepath("saved/presets/keybinds/DymaticDefault.keybind");
						break;
					}
					}
				}

				ImGui::SameLine();

				if (ImGui::Button("Import", ImVec2{ widthAvalOver / 6, 23})) { ImportKeyBinds(); }
				ImGui::SameLine();
				if (ImGui::Button("Export", ImVec2{ widthAvalOver / 6, 23 })) { ExportKeyBinds(); }
				ImGui::SameLine();
				if (ImGui::Button("Save Binds", ImVec2{ widthAvalOver / 6, 23 })) { SaveKeyBindsByFilepath("saved/SavedKeyBinds.keybind"); }

				static int currentKeyBindSearchItem = 0;
				const char* SearchTypes[2] = { "Name", "Key Binding"};
				switch (ImGui::SwitchButtonEx("KeyBindsSearchBarButtonSwitch", SearchTypes, 2, currentKeyBindSearchItem, ImVec2{ 200, 23 }))
				{
				case 0: {currentKeyBindSearchItem = 0; SearchByNameKey = true; break; }
				case 1: {currentKeyBindSearchItem = 1; SearchByNameKey = false; break; }
				}

				ImGui::SameLine();
				
				static char KeybindsSearchBuffer[200] = {};

				ImGui::PushID("KeyBindsSearchBar");
				ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - 35);
				if (ImGui::InputTextWithHint("##SearchBarIDs", "Search:", KeybindsSearchBuffer, sizeof(KeybindsSearchBuffer)))
				{
					KeyBindSearchBar = KeybindsSearchBuffer;
				}
				ImGui::PopItemWidth();
				ImGui::SameLine();
				if (ImGui::Button("X", ImVec2{ 23, 23 }))
				{
					memset(KeybindsSearchBuffer, 0, sizeof(KeybindsSearchBuffer));
					KeyBindSearchBar = "";
				}
				ImGui::PopID();

				ImGui::BeginChild("##KeymapList");

				ImGui::Dummy(ImVec2{ 0, 10 });
				auto h = ImGui::GetStyleColorVec4(ImGuiCol_Header);
				ImGui::PushStyleColor(ImGuiCol_Header, ImVec4{ h.x + 0.1f, h.y + 0.1f, h.z + 0.1f, h.w + 0.1f });
				if (ImGui::TreeNodeEx("Preferences", treeNodeFlags | ImGuiTreeNodeFlags_DefaultOpen))
				{
					static const char* b[2] = { "Mouse Left", "Mouse Right" };
					ImGui::PushID("SwitchButton");
					switch (ImGui::SwitchButtonEx("Switch1", b, 2, keyBinds.GetKeyBind(SelectObjectBind).mouseCode == Mouse::ButtonRight ? 1 : 0, ImVec2{ ImGui::GetContentRegionAvail().x, 23 }))
					{
					case 0: {keyBinds.GetKeyBind(SelectObjectBind).mouseCode = Mouse::ButtonLeft;  keyBinds.GetKeyBind(SelectObjectBind).bindCatagory = MouseButton; break; }
					case 1: {keyBinds.GetKeyBind(SelectObjectBind).mouseCode = Mouse::ButtonRight; keyBinds.GetKeyBind(SelectObjectBind).bindCatagory = MouseButton; break; }
					}
					ImGui::PopID();
					ImGui::TreePop();
				}
				ImGui::PopStyleColor();

				ImGui::Dummy(ImVec2{ 0, 30 });

				bool WindowKeyBindPrefsOpen = true;
				static bool WindowBindPrefVis = true;
				if (KeyBindSearchBar == "")
				{
					WindowKeyBindPrefsOpen = ImGui::TreeNodeEx("Window", treeNodeFlags); WindowBindPrefVis = false;
				}
				else if (WindowBindPrefVis) ImGui::Text("Window");
				if (WindowKeyBindPrefsOpen)
				{
					bool vis = false;
					if (KeyBindInputButton(NewSceneBind)) vis = true;
					if (KeyBindInputButton(OpenSceneBind)) vis = true;
					if (KeyBindInputButton(SaveSceneBind)) vis = true;
					if (KeyBindInputButton(SaveSceneAsBind)) vis = true;
					if (KeyBindInputButton(QuitBind)) vis = true;
					if (KeyBindInputButton(RenameBind)) vis = true;
					WindowBindPrefVis = vis;
					if (KeyBindSearchBar == "") ImGui::TreePop();
				}

				bool ThreeDViewKeyBindPrefsOpen = true;
				static bool ThreeDViewBindPrefVis = true;
				if (KeyBindSearchBar == "")
				{
					ThreeDViewKeyBindPrefsOpen = ImGui::TreeNodeEx("3D View", treeNodeFlags); ThreeDViewBindPrefVis = false;
				}
				else if (ThreeDViewBindPrefVis) ImGui::Text("3D View");
				if (ThreeDViewKeyBindPrefsOpen)
				{
					bool vis = false;
					if (KeyBindInputButton(SelectObjectBind)) vis = true;
					if (KeyBindInputButton(GizmoNoneBind)) vis = true;
					if (KeyBindInputButton(GizmoTranslateBind)) vis = true;
					if (KeyBindInputButton(GizmoRotateBind)) vis = true;
					if (KeyBindInputButton(GizmoScaleBind)) vis = true;
					if (KeyBindInputButton(DuplicateBind)) vis = true;
					ThreeDViewBindPrefVis = vis;
					if (KeyBindSearchBar == "") ImGui::TreePop();
				}

				ImGui::EndChild();
			}
			else if (m_CurrentCatagory == "System")
			{
				if (ImGui::TreeNodeEx("Notifications", treeNodeFlags | ImGuiTreeNodeFlags_DefaultOpen))
				{
					static int b = GetPreferences().m_PreferenceData.NotificationPreset;
					static const char* a[] = { "Disabled", "Program Inactive", "Always", "Custom" };
					if (ImGui::Combo("Toast Preset", &b, a, IM_ARRAYSIZE(a)))
					{
						GetPreferences().m_PreferenceData.NotificationPreset = b;
					}

					bool PrefAutoEnabled = b == 3;

					NotificationEdit("Auto-Save Pending", 0, PrefAutoEnabled);
					NotificationEdit("Auto-Save Complete", 1, PrefAutoEnabled);
					ImGui::TreePop();
				}
			}
			else if (m_CurrentCatagory == "Save & Load")
			{
				bool autosave = ImGui::TreeNodeEx("Auto Save", treeNodeFlags);
				ImGui::SameLine();
				ImGui::Custom::Checkbox("##AutosavePrefCheckbox", &m_Preferences.m_PreferenceData.autosaveEnabled);
				if (autosave)
				{
					ImGui::SliderInt("Timer (minutes)", &m_Preferences.m_PreferenceData.autosaveTime, 1, 60);
					ImGui::TreePop();
				}
			}
			else if (m_CurrentCatagory == "File Paths")
			{

			}
			ImGui::EndChild();


			ImGui::End();
		}
	}

	std::string PreferencesPannel::ColorVariableToText(ColorSchemeType colorVariable)
	{
		switch (colorVariable)
		{
		case Text: {return "Text"; }
		case TextDisabled: {return "Text Disabled"; }
		case TextSelectedBg: {return "Text Selected Background"; }
		case MenuBarBg: {return "Menu Bar Background"; }
		case WindowBg: {return "Window Background"; }
		case Header: {return "Header"; }
		case HeaderHovered: {return "Header Hovered"; }
		case HeaderActive: {return "Header Active"; }
		case Tab: {return "Tab"; }
		case TabHovered: {return "Tab Hovered"; }
		case TabActive: {return "Tab Active"; }
		case TabUnfocused: {return "Tab Unfocused"; }
		case TabUnfocusedActive: {return "Tab Unfocused Active"; }
		case TitleBg: {return "Title Background"; }
		case TitleBgActive: {return "Title Background Active"; }
		case TitleBgCollapsed: {return "Title Background Collapsed"; }
		case Button: {return "Button"; }
		case ButtonHovered: {return "Button Hovered"; }
		case ButtonActive: {return "Button Active"; }
		case ButtonToggled: {return "Button Toggled"; }
		case ButtonToggledHovered: {return "Button Toggled & Hovered"; }
		case PopupBg: {return "Popup Background"; }
		case ModalWindowDimBg: {return "Modal Window Dim Background"; }
		case Border: {return "Border"; }
		case BorderShadow: {return "Border Shadow"; }
		case FrameBg: {return "Frame Background"; }
		case FrameBgHovered: {return "Frame Background Hovered"; }
		case FrameBgActive: {return "Frame Background Active"; }
		case ScrollbarBg: {return "Scroll Bar Background"; }
		case ScrollbarGrab: {return "Scroll Bar Grab"; }
		case ScrollbarGrabHovered: {return "Scroll Bar Grab Hovered"; }
		case ScrollbarGrabActive: {return "Scroll Bar Grab Active"; }
		case ScrollbarDots: {return "Scroll Bar Dots"; }
		case SliderGrab: {return "Slider Grab"; }
		case SliderGrabActive: {return "Slider Grab Active"; }
		case Separator: {return "Separator"; }
		case SeparatorHovered: {return "Separator Hovered"; }
		case SeparatorActive: {return "Separator Active"; }
		case ResizeGrip: {return "Resize Grip"; }
		case ResizeGripHovered: {return "Resize Grip Hovered"; }
		case ResizeGripActive: {return "Resize Grip Active"; }
		case FileBackground: {return "File Background"; }
		case FileIcon: {return "File Icon"; }
		case FileHovered: {return "File Hovered"; }
		case FileSelected: {return "File Selected"; }
		case CheckMark: {return "Check Mark"; }
		case Checkbox: {return "Checkbox"; }
		case CheckboxHovered: {return "Checkbox Hovered"; }
		case CheckboxActive: {return "Checkbox Active"; }
		case CheckboxTicked: {return "Checkbox Ticked"; }
		case CheckboxHoveredTicked: {return "Check Box Hovered Ticked"; }
		case DockingPreview: {return "Docking Preview"; }
		case DockingEmptyBg: {return "Docking Empty Background"; }
		case PlotLines: {return "Plot Lines"; }
		case PlotLinesHovered: {return "Plot Lines Hovered"; }
		case PlotHistogram: {return "Plot Histogram"; }
		case PlotHistogramHovered: {return "Plot Histogram Hovered"; }
		case DragDropTarget: {return "Drag Drop Target"; }
		case NavHighlight: {return "Nav Highlight"; }
		case NavWindowingHighlight: {return "Nav Windowing Highlight"; }
		case NavWindowingDimBg: {return "Nav Windowing Dim Background"; }
		}
		DY_CORE_ASSERT(false, "Theme color variable, does not exist");
		return "";
	}

	void PreferencesPannel::EditThemeColor(ColorSchemeType colorSchemeType, std::string tooltip)
	{
		auto& values = GetPreferences().m_PreferenceData.colorScheme.colorSchemeValues;
		if (ImGui::ColorEdit4(("##" + ColorVariableToText(colorSchemeType)).c_str(), (float*)&values[colorSchemeType])) { UpdateThemePreferences(); }

		ImGui::SameLine();

		std::string message = ColorVariableToText(colorSchemeType);
		ImGui::Text(message.c_str());
		
		if (tooltip != "")
		{
			if (ImGui::IsItemHovered())
			{
				ImGui::BeginTooltip();
				ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
				ImGui::Text(tooltip.c_str());
				ImGui::PopFont();
				ImGui::EndTooltip();
			}
		}
	}

	bool PreferencesPannel::KeyBindInputButton(KeyBindEvent keyBindEvent)
	{
		auto& keyBinds = GetPreferences().m_PreferenceData.keyBinds;

		std::string ctrl = keyBinds.GetKeyBind(keyBindEvent).Ctrl ? "Ctrl " : "";
		std::string shift = keyBinds.GetKeyBind(keyBindEvent).Shift ? "Shift " : "";
		std::string alt = keyBinds.GetKeyBind(keyBindEvent).Alt ? "Alt " : "";
		std::string combo = ctrl + shift + alt + (keyBinds.GetKeyBind(keyBindEvent).bindCatagory == Keyboard ? keyBinds.GetStringFromKey(keyBinds.GetKeyBind(keyBindEvent).keyCode) : keyBinds.GetStringFromMouseButton(keyBinds.GetKeyBind(keyBindEvent).mouseCode));

		bool visible = KeyBindSearchBar == "" || (SearchByNameKey ? ((ToLower(keyBinds.GetStringFromEvent(keyBindEvent))).find(ToLower(KeyBindSearchBar)) != std::string::npos) : ((ToLower(combo)).find(ToLower(KeyBindSearchBar)) != std::string::npos));

		if (visible)
		{			
			bool clicked = false;

			ImGui::PushID(("TopBarLayer" + std::to_string(keyBindEvent)).c_str());
			bool open = ImGui::TreeNodeEx("", ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding);
			ImGui::PopID();
			ImGui::SameLine();
			ImGui::PushID(("EnableCheckBoxID" + std::to_string(keyBindEvent)).c_str());
			ImGui::Custom::Checkbox("##EnabledCheckbox", &keyBinds.GetKeyBind(keyBindEvent).enabled);
			ImGui::PopID();
			ImGui::SameLine();
			ImGui::Text(keyBinds.GetStringFromEvent(keyBindEvent).c_str());
			ImGui::SameLine(ImGui::GetContentRegionAvail().x - 155);


			if (m_ButtonActive == keyBindEvent)
			{
				combo = "";

				bool ctrl = Input::IsKeyPressed(Key::LeftControl) || Input::IsKeyPressed(Key::RightControl);
				bool shift = Input::IsKeyPressed(Key::LeftShift) || Input::IsKeyPressed(Key::RightShift);
				bool alt = Input::IsKeyPressed(Key::LeftAlt) || Input::IsKeyPressed(Key::RightAlt);

				if (ctrl) { combo = combo + "Ctrl "; }
				if (shift) { combo = combo + "Shift "; }
				if (alt) { combo = combo + "Alt "; }
				if (!ctrl && ! shift && !alt) { combo = "Enter Combitination"; }

				ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
				ImGui::PushID(("##KeyComboInput" + std::to_string(keyBindEvent)).c_str());
				clicked = ImGui::Button(combo.c_str(), ImVec2{ 150, 23 });
				ImGui::PopID();
				ImGui::PopStyleColor();
			}
			else
			{
				ImGui::PushID(("##KeyComboInput" + std::to_string(keyBindEvent)).c_str());
				clicked = ImGui::Button(combo.c_str(), ImVec2{ 150, 23 });
				ImGui::PopID();
			}

			if (m_ButtonActive == keyBindEvent && !ImGui::IsItemHovered())
			{
				m_ButtonActive = INVALID_BIND;
			}


			if (clicked)
			{
				m_ButtonActive = keyBindEvent;
			}
			if (m_ButtonActive == keyBindEvent)
			{
				bool control = Input::IsKeyPressed(Key::LeftControl) || Input::IsKeyPressed(Key::RightControl);
				bool shift = Input::IsKeyPressed(Key::LeftShift) || Input::IsKeyPressed(Key::RightShift);
				bool alt = Input::IsKeyPressed(Key::LeftAlt) || Input::IsKeyPressed(Key::RightAlt);


				for (KeyCode key : keyBinds.GetAllKeys())
				{

					if (Input::IsKeyPressed(key) && key != Key::LeftControl && key != Key::RightControl && key != Key::LeftShift && key != Key::RightShift && key != Key::LeftAlt && key != Key::RightAlt)
					{
						keyBinds.SetKeyBind(keyBindEvent, key, NULL, Keyboard, control, shift, alt);
						m_ButtonActive = INVALID_BIND;
					}
				}
				for (MouseCode mouseButton : keyBinds.GetAllMouseButtons())
				{
					if (Input::IsMouseButtonPressed(mouseButton))
					{
						keyBinds.SetKeyBind(keyBindEvent, NULL, mouseButton, MouseButton, control, shift, alt);
						m_ButtonActive = INVALID_BIND;
					}
				}
			}



			if (open)
			{
				ImGui::TreePop();
				ImGui::PushID(("CtrlBind" + std::to_string(keyBindEvent)).c_str());
				ImGui::ToggleButton("Ctrl", &keyBinds.GetKeyBind(keyBindEvent).Ctrl, ImVec2{ 45, 23 });
				ImGui::PopID();
				ImGui::SameLine();
				ImGui::PushID(("ShiftBind" + std::to_string(keyBindEvent)).c_str());
				ImGui::ToggleButton("Shift", &keyBinds.GetKeyBind(keyBindEvent).Shift, ImVec2{ 45, 23 });
				ImGui::PopID();
				ImGui::SameLine();
				ImGui::PushID(("AltBind" + std::to_string(keyBindEvent)).c_str());
				ImGui::ToggleButton("Alt", &keyBinds.GetKeyBind(keyBindEvent).Alt, ImVec2{ 45, 23 });
				ImGui::PopID();
				ImGui::SameLine();
				if (ImGui::Button((keyBinds.GetStringFromBindCatagory(keyBinds.GetKeyBind(keyBindEvent).bindCatagory) + "##" + keyBinds.GetStringFromEvent(keyBindEvent)).c_str(), ImVec2{ 100, 23 }))
				{
					ImGui::OpenPopup("##AllBindSelect" + keyBindEvent);
					m_KeySelectReturn = keyBindEvent;
				}
				ImGui::SameLine();
				auto keycodeTemp = keyBinds.GetStringFromKey(keyBinds.GetKeyBind(keyBindEvent).keyCode);
				auto mouseCodeTemp = keyBinds.GetStringFromMouseButton(keyBinds.GetKeyBind(keyBindEvent).mouseCode);
				if (ImGui::Button(((keyBinds.GetKeyBind(keyBindEvent).bindCatagory == Keyboard ? keycodeTemp : mouseCodeTemp) + "##" + keyBinds.GetStringFromEvent(keyBindEvent)).c_str(), ImVec2{ 100, 23 }))
				{
					ImGui::OpenPopup("##AllKeySelect" + keyBindEvent);
					m_KeySelectReturn = keyBindEvent;
				}
				if (keyBinds.GetKeyBind(keyBindEvent).bindCatagory == Keyboard)
				{
					ImGui::SameLine();
					ImGui::PushID(("RepeatsBind" + std::to_string(keyBindEvent)).c_str());
					ImGui::Custom::Checkbox("##KeyRepeats", &keyBinds.GetKeyBind(keyBindEvent).repeats);
					ImGui::SameLine();
					ImGui::Text("Repeats");
					ImGui::PopID();
				}
			}

			if (m_KeySelectReturn == keyBindEvent)
			{
				ImGui::SetNextWindowSize(ImVec2{ 150, 500 });
				if (ImGui::BeginPopup("##AllKeySelect" + keyBindEvent))
				{
					if (keyBinds.GetKeyBind(keyBindEvent).bindCatagory == Keyboard)
					{
						for (KeyCode key : keyBinds.GetAllKeys())
						{
							if (ImGui::MenuItem(keyBinds.GetStringFromKey(key).c_str()))
							{
								keyBinds.GetKeyBind(keyBindEvent).keyCode = key;
								m_KeySelectReturn = INVALID_BIND;
								ImGui::CloseCurrentPopup();
							}
						}
					}
					else if (keyBinds.GetKeyBind(keyBindEvent).bindCatagory == MouseButton)
					{
						for (MouseCode mouseButton : keyBinds.GetAllMouseButtons())
						{
							if (ImGui::MenuItem(keyBinds.GetStringFromMouseButton(mouseButton).c_str()))
							{
								keyBinds.GetKeyBind(keyBindEvent).mouseCode = mouseButton;
								m_KeySelectReturn = INVALID_BIND;
								ImGui::CloseCurrentPopup();
							}
						}
					}
					ImGui::EndPopup();
				}
				ImGui::SetNextWindowSize(ImVec2{ 150, 100 });
				if (ImGui::BeginPopup("##AllBindSelect" + keyBindEvent))
				{
					if (ImGui::MenuItem("Keyboard")) { keyBinds.GetKeyBind(keyBindEvent).bindCatagory = Keyboard; }
					else if (ImGui::MenuItem("Mouse Button")) { keyBinds.GetKeyBind(keyBindEvent).bindCatagory = MouseButton; }
					ImGui::EndPopup();
				}
			}
		}
		return visible;
	}

	void PreferencesPannel::NotificationEdit(std::string NotificationName, int varIndex, bool enabled)
	{
		auto& prefs = GetPreferences().m_PreferenceData;
		ImGui::Custom::Checkbox(("##NotificationPreference" + NotificationName).c_str(), &prefs.NotificationEnabled[varIndex]);
		ImGui::SameLine();
		ImGui::Text(NotificationName.c_str());
		ImGui::SameLine();
		ImGui::Dummy(ImVec2{ 200 - ImGui::CalcTextSize(NotificationName.c_str()).x, 0 });
		ImGui::SameLine();

		if (!prefs.NotificationEnabled[varIndex])
			enabled = false;
		
		static const char* b[3] = { "Disabled", "Program Inactive", "Always" };

		ImVec2 prePos = ImGui::GetCursorScreenPos();
		float switchValueLength = ImGui::GetContentRegionAvail().x;

		ImGui::PushID(("NotificationPreferenceSwitchButton" + NotificationName).c_str());
		ImGui::PushItemFlag(ImGuiItemFlags_Disabled, !enabled);
		auto switchValue = ImGui::SwitchButtonEx(("NotificationPreferenceToastSwitch" + NotificationName).c_str(), b, 3, prefs.NotificationToastEnabled[varIndex], ImVec2{ switchValueLength, 23 });
		ImGui::PopItemFlag();
		ImGui::PopID();
		if (!enabled)
			ImGui::GetWindowDrawList()->AddRectFilled(prePos, ImVec2{ prePos.x + switchValueLength, prePos.y + 23 }, ImGui::ColorConvertFloat4ToU32(ImVec4{ 0.0f, 0.0f, 0.0f, 0.7f }), GImGui->Style.FrameRounding);
		if (switchValue != -1)
			prefs.NotificationToastEnabled[varIndex] = switchValue;
	}

	//Import and Export with Dialogue versions

	void PreferencesPannel::ImportTheme()
	{
		std::optional<std::string> filepath = FileDialogs::OpenFile("Dymatic Theme (*.dytheme)\0*.dytheme\0");
		if (filepath)
		{			
			OpenThemeByPath(*filepath);
		}
	}

	void PreferencesPannel::ExportTheme()
	{
		std::optional<std::string> filepath = FileDialogs::SaveFile("Dymatic Theme (*.dytheme)\0*.dytheme\0");
		if (filepath)
		{
			SaveThemeByPath(*filepath);
		}
	}

	void PreferencesPannel::RetryDythemeFile()
	{
		if (m_RecentThemePath != "")
		{
			OpenThemeByPath(m_RecentThemePath);
		}
	}

	//Filepath Versions

	void PreferencesPannel::OpenThemeByPath(std::string filepath)
	{
		m_RecentThemePath = filepath;
		bool fatalError = false;
		auto& theme = GetPreferences().m_PreferenceData.colorScheme.colorSchemeValues;

		std::string result;
		std::ifstream in(filepath, std::ios::in | std::ios::binary); // ifstream closes itself due to RAII
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
			else
			{
				DY_CORE_ERROR("Could not read from file '{0}'", filepath);
				fatalError = true;
				m_PreferencesMessage.title = "Dytheme Read Error";
				m_PreferencesMessage.message = "Dytheme file:\n" + filepath + "\nCould not read from Dytheme file\n";
				m_PreferencesMessage.buttons = { "Retry", "Ok" };
				return;
			}
		}

		//Removed due to producing error on first startup when no Dytheme file existed.
		//else
		//{
		//	DY_CORE_ERROR("Could not open file '{0}'", filepath);
		//}

		if (result != "" && fatalError == false)
		{
			//while (result.find_first_of(" ") != -1)
			//{
			//	result = result.erase(result.find_first_of(" "), 1);
			//}
			

			while (result.find_first_of("\n") != -1)
			{
				result = result.erase(result.find_first_of("\n"), 1);
			}

			while (result.find_first_of("\r") != -1)
			{
				result = result.erase(result.find_first_of("\r"), 1);
			}


			while (result.find_first_of("<") != -1)
			{
				std::string tempstring = result;
				if (result.find_first_of(">") == -1 || (tempstring.erase(tempstring.find_first_of("<"), 1).find_first_of("<") < tempstring.find_first_of(">")))
				{
					fatalError = true;
					m_PreferencesMessage.title = "Dytheme Read Error";
					m_PreferencesMessage.message = "Dytheme file:\n" + filepath + "\nOpens comment (<), but never closes it, (>)\n";
					m_PreferencesMessage.buttons = { "Retry", "Ok" };
					return;
				}
				result = result.erase(result.find_first_of("<"), result.find_first_of(">") + 1 - result.find_first_of("<"));
			}

			for (int i = 0; i < result.length(); i++)
			{
				std::string character = result.substr(i, 1);
				if (!isdigit(character[0]) && character != "." && character != ",")
				{
					result = result.erase(i, 1);
					i--;
				}
			}

			if (fatalError == false)
			{
				for (int i = 0; result.find_first_of(",") != std::string::npos; i++)
				{
					theme[i].x = std::stof(result.substr(0, result.find_first_of(",")));
					result = result.substr(result.find_first_of(",") + 1, result.length() - result.find_first_of(","));
					theme[i].y = std::stof(result.substr(0, result.find_first_of(",")));
					result = result.substr(result.find_first_of(",") + 1, result.length() - result.find_first_of(","));
					theme[i].z = std::stof(result.substr(0, result.find_first_of(",")));
					result = result.substr(result.find_first_of(",") + 1, result.length() - result.find_first_of(","));
					theme[i].w = std::stof(result.substr(0, result.find_first_of(",")));
					result = result.substr(result.find_first_of(",") + 1, result.length() - result.find_first_of(","));

				}

				UpdateThemePreferences();
			}
		}

	}

	std::string PreferencesPannel::RoundFloatString(std::string stringValue)
	{
		if (stringValue.find_last_of("0") == stringValue.length() - 1)
		{
			stringValue = stringValue.erase(stringValue.find_last_not_of("0") + 1, stringValue.find_last_of("0") - stringValue.find_last_not_of("0") + 1);
		}
		if (stringValue.find_first_of(".") == stringValue.length() - 1)
		{
			stringValue = stringValue + "0";
		}
		return stringValue;
	}

	void PreferencesPannel::SaveThemeByPath(std::string filepath)
	{
		auto theme = GetPreferences().m_PreferenceData.colorScheme.colorSchemeValues;
		std::string out = "";
		for (int i = 0; i < theme.size(); i++)
		{
			out = out + "< " + ColorVariableToText(static_cast<ColorSchemeType>(i)) + " >\r";
			out = out + RoundFloatString(std::to_string(theme[i].x)) + "," + RoundFloatString(std::to_string(theme[i].y)) + "," + RoundFloatString(std::to_string(theme[i].z)) + "," + RoundFloatString(std::to_string(theme[i].w)) + ",\r\r";
		}


		std::ofstream fout(filepath);
		fout << out.c_str();
	}

	void PreferencesPannel::SetThemePreferences(ColorScheme colorScheme)
	{
		GetPreferences().m_PreferenceData.colorScheme.colorSchemeValues = colorScheme.colorSchemeValues;
		UpdateThemePreferences();
	}

	void PreferencesPannel::UpdateThemePreferences()
	{
		auto& colors = ImGui::GetStyle().Colors;
		auto& theme = GetPreferences().m_PreferenceData.colorScheme.colorSchemeValues;

		//Text
		colors[ImGuiCol_Text] = theme[Text];
		colors[ImGuiCol_TextDisabled] = theme[TextDisabled];
		colors[ImGuiCol_TextSelectedBg] = theme[TextSelectedBg];

		//Window
		colors[ImGuiCol_MenuBarBg] = theme[MenuBarBg];
		colors[ImGuiCol_WindowBg] = theme[WindowBg];

		// Headers
		colors[ImGuiCol_Header] = theme[Header];
		colors[ImGuiCol_HeaderHovered] = theme[HeaderHovered];
		colors[ImGuiCol_HeaderActive] = theme[HeaderActive];

		// Tabs
		colors[ImGuiCol_Tab] = theme[Tab];
		colors[ImGuiCol_TabHovered] = theme[TabHovered];
		colors[ImGuiCol_TabActive] = theme[TabActive];
		colors[ImGuiCol_TabUnfocused] = theme[TabUnfocused];
		colors[ImGuiCol_TabUnfocusedActive] = theme[TabUnfocusedActive];

		// Title
		colors[ImGuiCol_TitleBg] = theme[TitleBg];
		colors[ImGuiCol_TitleBgActive] = theme[TitleBgActive];
		colors[ImGuiCol_TitleBgCollapsed] = theme[TitleBgCollapsed];

		// Buttons
		colors[ImGuiCol_Button] = theme[Button];
		colors[ImGuiCol_ButtonHovered] = theme[ButtonHovered];
		colors[ImGuiCol_ButtonActive] = theme[ButtonActive];
		colors[ImGuiCol_ButtonToggled] = theme[ButtonToggled];
		colors[ImGuiCol_ButtonToggledHovered] = theme[ButtonToggledHovered];

		//Popup and Modal
		colors[ImGuiCol_PopupBg] = theme[PopupBg];
		colors[ImGuiCol_ModalWindowDimBg] = theme[ModalWindowDimBg];

		//Borders
		colors[ImGuiCol_Border] = theme[Border];
		colors[ImGuiCol_BorderShadow] = theme[BorderShadow];

		// Frame BG
		colors[ImGuiCol_FrameBg] = theme[FrameBg];
		colors[ImGuiCol_FrameBgHovered] = theme[FrameBgHovered];
		colors[ImGuiCol_FrameBgActive] = theme[FrameBgHovered];

		//Scrollbar
		colors[ImGuiCol_ScrollbarBg] = theme[ScrollbarBg];
		colors[ImGuiCol_ScrollbarGrab] = theme[ScrollbarGrab];
		colors[ImGuiCol_ScrollbarGrabHovered] = theme[ScrollbarGrabHovered];
		colors[ImGuiCol_ScrollbarGrabActive] = theme[ScrollbarGrabActive];
		colors[ImGuiCol_ScrollbarDots] = theme[ScrollbarDots];
		
		//Slider
		colors[ImGuiCol_SliderGrab] = theme[SliderGrab];
		colors[ImGuiCol_SliderGrabActive] = theme[SliderGrabActive];

		//Separator
		colors[ImGuiCol_Separator] = theme[Separator];
		colors[ImGuiCol_SeparatorHovered] = theme[SeparatorHovered];
		colors[ImGuiCol_SeparatorActive] = theme[SeparatorActive];

		//Resize
		colors[ImGuiCol_ResizeGrip] = theme[ResizeGrip];
		colors[ImGuiCol_ResizeGripHovered] = theme[ResizeGripHovered];
		colors[ImGuiCol_ResizeGripActive] = theme[ResizeGripActive];

		//Content Browser -- In Custom

		//Checkbox -- In Custom
		colors[ImGuiCol_CheckMark] = theme[CheckMark];

		//Docking
		colors[ImGuiCol_DockingPreview] = theme[DockingPreview];
		colors[ImGuiCol_DockingEmptyBg] = theme[DockingEmptyBg];

		//Plotlines
		colors[ImGuiCol_PlotLines] = theme[PlotLines];
		colors[ImGuiCol_PlotLinesHovered] = theme[PlotLinesHovered];
		colors[ImGuiCol_PlotHistogram] = theme[PlotHistogram];
		colors[ImGuiCol_PlotHistogramHovered] = theme[PlotHistogramHovered];

		//Drag and Drop
		colors[ImGuiCol_DragDropTarget] = theme[DragDropTarget];

		//Nav
		colors[ImGuiCol_NavHighlight] = theme[NavHighlight];
		colors[ImGuiCol_NavWindowingHighlight] = theme[NavWindowingHighlight];
		colors[ImGuiCol_NavWindowingDimBg] = theme[NavWindowingDimBg];

		//------------------------Custom--------------------------//

		//Checkbox
		
		GetImGuiCustomColorValue(ImGui::Custom::ImGuiCol_CheckboxBg) = theme[Checkbox];
		GetImGuiCustomColorValue(ImGui::Custom::ImGuiCol_CheckboxBgHovered) = theme[CheckboxHovered];
		GetImGuiCustomColorValue(ImGui::Custom::ImGuiCol_CheckboxBgActive) = theme[CheckboxActive];
		GetImGuiCustomColorValue(ImGui::Custom::ImGuiCol_CheckboxBgTicked) = theme[CheckboxTicked];
		GetImGuiCustomColorValue(ImGui::Custom::ImGuiCol_CheckboxBgHoveredTicked) = theme[CheckboxHoveredTicked];

		//Content Browser
		GetImGuiCustomColorValue(ImGui::Custom::ImGuiCol_FileBackground) = theme[FileBackground];
		GetImGuiCustomColorValue(ImGui::Custom::ImGuiCol_FileIcon) = theme[FileIcon];
		GetImGuiCustomColorValue(ImGui::Custom::ImGuiCol_FileHovered) = theme[FileHovered];
		GetImGuiCustomColorValue(ImGui::Custom::ImGuiCol_FileSelected) = theme[FileSelected];
	}

	void PreferencesPannel::ImportKeyBinds()
	{
		std::optional<std::string> filepath = FileDialogs::OpenFile("Key Bind (*.keybind)\0*.keybind\0");
		if (filepath)
		{
			OpenKeyBindsByFilepath(*filepath);
		}
	}

	void PreferencesPannel::ExportKeyBinds()
	{
		std::optional<std::string> filepath = FileDialogs::SaveFile("Key Bind (*.keybind)\0*.keybind\0");
		if (filepath)
		{
			SaveKeyBindsByFilepath(*filepath);
		}
	}

	void PreferencesPannel::OpenKeyBindsByFilepath(std::string filepath)
	{
		bool fatalError = false;

		std::string result;
		std::ifstream in(filepath, std::ios::in | std::ios::binary); // ifstream closes itself due to RAII
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
			else
			{
				DY_CORE_ERROR("Could not read from file '{0}'", filepath);
				fatalError = true;
				m_PreferencesMessage.title = "Import Key Bind Error";
				m_PreferencesMessage.message = "Bind file:\n" + filepath + "\nCould not read from Key Binds file\n";
				m_PreferencesMessage.buttons = { "Ok" };
				return;
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

		bool openName = false;
		bool openValueName = false;
		bool openValue = false;
		std::string CurrentName = "";
		std::string CurrentValueName = "";
		std::string CurrentValue = "";
		for (int i = 0; i < result.length(); i++)
		{
			std::string character = result.substr(i, 1);

			if (character == ")") { openName = false; }
			if (openName) { CurrentName = CurrentName + character; }
			if (character == "(") { openName = true; CurrentName = ""; }

			if (character == ">") { openValueName = false; }
			if (openValueName) { CurrentValueName = CurrentValueName + character; }
			if (character == "<") { openValueName = true; CurrentValueName = ""; }

			if (character == "}") { openValue = false; }
			if (openValue) { CurrentValue = CurrentValue + character; }
			if (character == "{") { openValue = true; CurrentValue = ""; }

			if (character == "}" && CurrentValue != "")
			{
				m_Preferences.m_PreferenceData.keyBinds.SetKeyStrings(CurrentName, CurrentValueName, CurrentValue);
			}

		}

	}

	void PreferencesPannel::SaveKeyBindsByFilepath(std::string filepath)
	{
		auto& keyBinds = GetPreferences().m_PreferenceData.keyBinds;
		auto& allKeyBinds = keyBinds.GetAllKeyBinds();
		std::string out = "";
		for (int i = 1; i < allKeyBinds.size(); i++)
		{
			out = out + "(" + keyBinds.GetStringFromEvent((KeyBindEvent)i) +")";
			out = out + " <Key> {" + keyBinds.GetStringFromKey(allKeyBinds[i].keyCode) + "}";
			out = out + " <MouseButton> {" + keyBinds.GetStringFromMouseButton(allKeyBinds[i].mouseCode) + "}";
			out = out + " <BindCatagory> {" + keyBinds.GetStringFromBindCatagory(allKeyBinds[i].bindCatagory) + "}";
			out = out + " <Ctrl> {" + (allKeyBinds[i].Ctrl ? "true" : "false") + "}";
			out = out + " <Shift> {" + (allKeyBinds[i].Shift ? "true" : "false") + "}";
			out = out + " <Alt> {" + (allKeyBinds[i].Alt ? "true" : "false") + "}";
			out = out + " <Enabled> {" + (allKeyBinds[i].enabled ? "true" : "false") + "}";
			out = out + " <Repeats> {" + (allKeyBinds[i].repeats ? "true" : "false") + "}";
			out = out + "\r";

			//(Open File) < Key > {O} <MouseButton> {Button Left} <BindCatagory> {Keyboard} <Ctrl> {true} <Shift> {false} <Alt> {false} <Enabled> {true}
		}

		std::ofstream fout(filepath);
		fout << out.c_str();
	}

	void PreferencesPannel::ImportPreferences()
	{
		std::optional<std::string> filepath = FileDialogs::OpenFile("Preferences (*.prefs)\0*.prefs\0");
		if (filepath)
		{
			OpenPreferencesByFilepath(*filepath);
		}
	}

	void PreferencesPannel::ExportPreferences()
	{
		std::optional<std::string> filepath = FileDialogs::SaveFile("Preferences (*.prefs)\0*.prefs\0");
		if (filepath)
		{
			SavePreferencesByFilepath(*filepath);
		}
	}

	void PreferencesPannel::OpenPreferencesByFilepath(std::string filepath)
	{
		bool fatalError = false;

		std::string result;
		std::ifstream in(filepath, std::ios::in | std::ios::binary); // ifstream closes itself due to RAII
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
			else
			{
				DY_CORE_ERROR("Could not read from file '{0}'", filepath);
				fatalError = true;
				m_PreferencesMessage.title = "Import Preferences Error";
				m_PreferencesMessage.message = "Preferences file:\n" + filepath + "\nCould not read from Preferences file\n";
				m_PreferencesMessage.buttons = { "Ok" };
				return;
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

			auto& prefs = GetPreferences().m_PreferenceData;
			if (character == "}" && CurrentValue != "")
			{
				if (CurrentValueName == "AutosavePreferences") { prefs.autosavePreferences = CurrentValue == "true" ? true : false; }
				if (CurrentValueName == "AutosaveEnabled") { prefs.autosaveEnabled = CurrentValue == "true" ? true : false; }
				else if (CurrentValueName == "AutosaveTime") { prefs.autosaveTime = std::stof(CurrentValue); }
				else if (CurrentValueName == "ShowSplash") { prefs.showSplash = CurrentValue == "true" ? true : false; }
				else if (CurrentValueName == "FileColors") { SetFileColorsFromString(CurrentValue); }
				else if (CurrentValueName == "DoubleClickSpeed") { prefs.doubleClickSpeed = std::stof(CurrentValue); }
				else if (CurrentValueName == "NotificationPreset") { prefs.NotificationPreset = std::clamp((int)std::stof(CurrentValue), 0, 3); }
				else if (CurrentValueName == "NotificationToastEnabled") { for (int i = 0; i < CurrentValue.length() && prefs.NotificationToastEnabled.size(); i++) { prefs.NotificationToastEnabled[i] = std::stof(CurrentValue.substr(i, 1)) > 2 || std::stof(CurrentValue.substr(i, 1)) < 0 ? 0 : std::stof(CurrentValue.substr(i, 1)); } }
				else if (CurrentValueName == "NotificationEnabled") { for (int i = 0; i < CurrentValue.length() && prefs.NotificationEnabled.size(); i++) { prefs.NotificationEnabled[i] = CurrentValue.substr(i, 1) == "1" ? 1 : 0; } }

			}

		}
	}

	void PreferencesPannel::SavePreferencesByFilepath(std::string filepath)
	{
		auto& prefs = GetPreferences().m_PreferenceData;
		std::string out = "";

		out = out + "<AutosavePreferences> {" + (prefs.autosavePreferences ? "true" : "false") + "}\r";
		out = out + "<AutosaveEnabled> {" + (prefs.autosaveEnabled ? "true" : "false") + "}\r";
		out = out + "<AutosaveTime> {" + (std::to_string(prefs.autosaveTime)) + "}\r";
		out = out + "<ShowSplash> {" + (prefs.showSplash ? "true" : "false") + "}\r";

		std::string FileColorString = "";
		for (FileColor fileColor : prefs.fileColors) { FileColorString = FileColorString + fileColor.Extension + "," + RoundFloatString(std::to_string(fileColor.Color.x)) + "," + RoundFloatString(std::to_string(fileColor.Color.y)) + "," + RoundFloatString(std::to_string(fileColor.Color.z)) + "," + RoundFloatString(std::to_string(fileColor.Color.w)) + ","; }
		if (FileColorString == "") { FileColorString = ","; }
		out = out + "<FileColors> {" + (FileColorString) + "}\r";

		out = out + "<DoubleClickSpeed> {" + (std::to_string(prefs.doubleClickSpeed)) + "}\r";

		out = out + "<NotificationPreset> {" + (std::to_string(prefs.NotificationPreset)) + "}\r";
		std::string NotificationToastString = "";
		for (int number : prefs.NotificationToastEnabled) { NotificationToastString += std::to_string(number); }
		out = out + "<NotificationToastEnabled> {" + NotificationToastString + "}\r";
		std::string NotificationString = "";
		for (bool number : prefs.NotificationEnabled) { NotificationString += std::to_string(number ? 1 : 0); }
		out = out + "<NotificationEnabled> {" + NotificationString + "}\r";

		std::ofstream fout(filepath);
		fout << out.c_str();
	}

	//ImGui Code

	std::string PreferencesPannel::DrawRoundedButtonStack(std::vector<std::string> buttonIds)
	{
		std::string buttonPressed = "";

		ImGui::Dummy(ImVec2{ 0, 5 });

		if (!buttonIds.empty())
		{
			if (buttonIds.size() == 1)
			{
				bool active = buttonIds[0] == m_CurrentCatagory;
				if (ImGui::Custom::ButtonCornersEx(buttonIds[0].c_str(), ImVec2{ ImGui::GetContentRegionAvail().x - 5.0f, 22.0f }, NULL, 5.0f, ImDrawCornerFlags_All, active, ImVec2{ 0, -2 }, ImVec2{ 0, 2 }))
					buttonPressed = buttonIds[0];
			}

			else
			{
				for (int i = 0; i < buttonIds.size(); i++)
				{
					bool active = buttonIds[i] == m_CurrentCatagory;

					if (i == 0)
					{
						if (ImGui::Custom::ButtonCornersEx(buttonIds[i].c_str(), ImVec2{ ImGui::GetContentRegionAvail().x - 5.0f, 22.0f }, NULL, 5.0f, ImDrawCornerFlags_Top, active, ImVec2{ 0, -2 }, ImVec2{ 0, 2 }))
							buttonPressed = buttonIds[i];
					}
					else if (i == buttonIds.size() - 1)
					{
						if (ImGui::Custom::ButtonCornersEx(buttonIds[i].c_str(), ImVec2{ ImGui::GetContentRegionAvail().x - 5.0f, 22.0f }, NULL, 5.0f, ImDrawCornerFlags_Bot, active, ImVec2{ 0, -2 }, ImVec2{ 0, 2 }))
							buttonPressed = buttonIds[i];
					}
					else
					{
						if (ImGui::Custom::ButtonCornersEx(buttonIds[i].c_str(), ImVec2{ ImGui::GetContentRegionAvail().x - 5.0f, 22.0f }, NULL, 5.0f, ImDrawCornerFlags_None, active, ImVec2{ 0, -2 }, ImVec2{ 0, 2 }))
							buttonPressed = buttonIds[i];
					}
				}
			}
		}
		return buttonPressed;
	}

	void PreferencesPannel::SetFileColorsFromString(std::string colorString)
	{
		GetPreferences().m_PreferenceData.fileColors.clear();
		std::string fileColorsTempString = colorString;

		while (fileColorsTempString.find_first_of(",") != std::string::npos)
		{
			FileColor tempFileColor;
			int successFactor = 0;

			if (fileColorsTempString.find_first_of(",") != std::string::npos)
			{
				std::string a = fileColorsTempString.substr(0, fileColorsTempString.find_first_of(","));
				strcpy_s(tempFileColor.Extension, a.c_str());
				fileColorsTempString = fileColorsTempString.erase(0, fileColorsTempString.find_first_of(",") + 1);
				successFactor++;
			}

			if (fileColorsTempString.find_first_of(",") != std::string::npos)
			{
				tempFileColor.Color.x = std::stof(fileColorsTempString.substr(0, fileColorsTempString.find_first_of(",")));
				fileColorsTempString = fileColorsTempString.erase(0, fileColorsTempString.find_first_of(",") + 1);
				successFactor++;
			}

			if (fileColorsTempString.find_first_of(",") != std::string::npos)
			{
				tempFileColor.Color.y = std::stof(fileColorsTempString.substr(0, fileColorsTempString.find_first_of(",")));
				fileColorsTempString = fileColorsTempString.erase(0, fileColorsTempString.find_first_of(",") + 1);
				successFactor++;
			}

			if (fileColorsTempString.find_first_of(",") != std::string::npos)
			{
				tempFileColor.Color.z = std::stof(fileColorsTempString.substr(0, fileColorsTempString.find_first_of(",")));
				fileColorsTempString = fileColorsTempString.erase(0, fileColorsTempString.find_first_of(",") + 1);
				successFactor++;
			}

			if (fileColorsTempString.find_first_of(",") != std::string::npos)
			{
				tempFileColor.Color.w = std::stof(fileColorsTempString.substr(0, fileColorsTempString.find_first_of(",")));
				fileColorsTempString = fileColorsTempString.erase(0, fileColorsTempString.find_first_of(",") + 1);
				successFactor++;
			}

			if (successFactor == 5) { GetPreferences().m_PreferenceData.fileColors.push_back(tempFileColor); }
		}
	}

	std::string PreferencesPannel::ToLower(std::string inString)
	{
		transform(inString.begin(), inString.end(), inString.begin(), ::tolower);
		return inString;
	}

}