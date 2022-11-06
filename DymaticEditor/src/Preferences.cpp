#include "Preferences.h"

#include <imgui/imgui.h>
#include <yaml-cpp/yaml.h>

namespace YAML
{
	template<>
	struct convert<ImVec4>
	{
		static Node encode(const ImVec4& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			node.push_back(rhs.w);
			node.SetStyle(EmitterStyle::Flow);
			return node;
		}

		static bool decode(const Node& node, ImVec4& rhs)
		{
			if (!node.IsSequence() || node.size() != 4)
				return false;

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			rhs.z = node[2].as<float>();
			rhs.w = node[3].as<float>();
			return true;
		}
	};

	Emitter& operator<<(Emitter& out, const ImVec4& v)
	{
		out << Flow;
		out << BeginSeq << v.x << v.y << v.z << v.w << EndSeq;
		return out;
	}
}

namespace Dymatic {

	static Preferences::PreferencesData s_PreferencesData;
	static std::array<Preferences::Preferences::Keymap::KeyBindData, Preferences::Keymap::BIND_EVENT_SIZE> s_Keymap;
	
	namespace
	{
		using namespace Key;
		static const std::array<KeyCode, 120> s_AllKeys
		{
			Space, Apostrophe, Comma, Minus, Period, Slash,
			D0, D1, D2, D3, D4, D5, D6, D7, D8, D9,
			Semicolon, Equal,
			A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
			LeftBracket, Backslash, RightBracket, GraveAccent,
			World1, World2,
			Escape, Enter, Tab, Backspace, Insert, Delete, Right, Left, Down, Up, PageUp, PageDown, Home, End, CapsLock, ScrollLock, NumLock, PrintScreen, Pause, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15, F16, F17, F18, F19, F20, F21, F22, F23, F24, F25,
			KP0, KP1, KP2, KP3, KP4, KP5, KP6, KP7, KP8, KP9, KPDecimal, KPDivide, KPMultiply, KPSubtract, KPAdd, KPEnter, KPEqual,
			LeftShift, LeftControl, LeftAlt, LeftSuper, RightShift, RightControl, RightAlt, RightSuper, Menu
		};
	
		using namespace Mouse;
		static const std::array<MouseCode, 12> s_AllMouseButtons
		{
			Button0, Button1, Button2, Button3, Button4, Button5, Button6, Button7,
			ButtonLast, ButtonLeft, ButtonRight, ButtonMiddle
		};
	}

	Preferences::PreferencesData::PreferencesData()
	{
		for (size_t i = 0; i < 6; i++)
			Filters[i] = true;
	}

	std::string Preferences::GetThemeColorName(uint8_t idx)
	{
		switch (idx)
		{
		case ImGuiCol_Text:  return "Text";
		case ImGuiCol_TextDisabled:  return "Text Disabled";
		case ImGuiCol_TextSelectedBg:  return "Text Selected Background";
		case ImGuiCol_WindowBg:  return "Window Background";
		case ImGuiCol_ChildBg:  return "Child Background";
		case ImGuiCol_MenuBarBg:  return "Menu Bar Background";
		case ImGuiCol_MenuBarGrip:  return "Menu Bar Grip";
		case ImGuiCol_MenuBarGripBorder:  return "Menu Bar Grip Border";
		case ImGuiCol_Header:  return "Header";
		case ImGuiCol_HeaderHovered:  return "Header Hovered";
		case ImGuiCol_HeaderActive:  return "Header Active";
		case ImGuiCol_Tab:  return "Tab";
		case ImGuiCol_TabHovered:  return "Tab Hovered";
		case ImGuiCol_TabActive:  return "Tab Active";
		case ImGuiCol_TabUnfocused:  return "Tab Unfocused";
		case ImGuiCol_TabUnfocusedActive:  return "Tab Unfocused Active";
		case ImGuiCol_TitleBg:  return "Title Background";
		case ImGuiCol_TitleBgActive:  return "Title Background Active";
		case ImGuiCol_TitleBgCollapsed:  return "Title Background Collapsed";
		case ImGuiCol_Button:  return "Button";
		case ImGuiCol_ButtonHovered:  return "Button Hovered";
		case ImGuiCol_ButtonActive:  return "Button Active";
		case ImGuiCol_ButtonToggled:  return "Button Toggled";
		case ImGuiCol_ButtonToggledHovered:  return "Button Toggled & Hovered";
		case ImGuiCol_PopupBg:  return "Popup Background";
		case ImGuiCol_ModalWindowDimBg:  return "Modal Window Dim Background";
		case ImGuiCol_MainWindowBorderEdit:  return "Main Window Border Edit";
		case ImGuiCol_MainWindowBorderPlay:  return "Main Window Border Play";
		case ImGuiCol_MainWindowBorderSimulate:  return "Main Window Border Simulate";
		case ImGuiCol_Border:  return "Border";
		case ImGuiCol_BorderShadow:  return "Border Shadow";
		case ImGuiCol_FrameBg:  return "Frame Background";
		case ImGuiCol_FrameBgHovered:  return "Frame Background Hovered";
		case ImGuiCol_FrameBgActive:  return "Frame Background Active";
		case ImGuiCol_ScrollbarBg:  return "Scroll Bar Background";
		case ImGuiCol_ScrollbarGrab:  return "Scroll Bar Grab";
		case ImGuiCol_ScrollbarGrabHovered:  return "Scroll Bar Grab Hovered";
		case ImGuiCol_ScrollbarGrabActive:  return "Scroll Bar Grab Active";
		case ImGuiCol_ScrollbarDots:  return "Scroll Bar Dots";
		case ImGuiCol_ProgressBarBg:  return "Progress Bar Background";
		case ImGuiCol_ProgressBarBorder:  return "Progress Bar Border";
		case ImGuiCol_ProgressBarFill:  return "Progress Bar Fill";
		case ImGuiCol_SliderGrab:  return "Slider Grab";
		case ImGuiCol_SliderGrabActive:  return "Slider Grab Active";
		case ImGuiCol_Separator:  return "Separator";
		case ImGuiCol_SeparatorHovered:  return "Separator Hovered";
		case ImGuiCol_SeparatorActive:  return "Separator Active";
		case ImGuiCol_ResizeGrip:  return "Resize Grip";
		case ImGuiCol_ResizeGripHovered:  return "Resize Grip Hovered";
		case ImGuiCol_ResizeGripActive:  return "Resize Grip Active";
		case ImGuiCol_TextEditorDefault:  return "Default";
		case ImGuiCol_TextEditorKeyword:  return "Keyword";
		case ImGuiCol_TextEditorSpecialKeyword:  return "Special Keyword";
		case ImGuiCol_TextEditorNumber:  return "Number";
		case ImGuiCol_TextEditorString:  return "String";
		case ImGuiCol_TextEditorCharLiteral:  return "Character Literal";
		case ImGuiCol_TextEditorPunctuation:  return "Punctuation";
		case ImGuiCol_TextEditorPreprocessor:  return "Preprocessor";
		case ImGuiCol_TextEditorIdentifier:  return "Identifier";
		case ImGuiCol_TextEditorComment:  return "Comment";
		case ImGuiCol_TextEditorMultiLineComment:  return "Multiline Comment";
		case ImGuiCol_TextEditorLineNumber:  return "Line Number";
		case ImGuiCol_TextEditorCurrentLineFill:  return "Current Line Fill";
		case ImGuiCol_TextEditorCurrentLineFillInactive:  return "Current Line Fill Inactive";
		case ImGuiCol_TextEditorCurrentLineEdge:  return "Current Line Edge";
		case ImGuiCol_CheckMark:  return "Check Mark";
		case ImGuiCol_Checkbox:  return "Checkbox";
		case ImGuiCol_CheckboxHovered:  return "Checkbox Hovered";
		case ImGuiCol_CheckboxActive:  return "Checkbox Active";
		case ImGuiCol_CheckboxTicked:  return "Checkbox Ticked";
		case ImGuiCol_CheckboxHoveredTicked:  return "Check Box Hovered Ticked";
		case ImGuiCol_DockingPreview:  return "Docking Preview";
		case ImGuiCol_DockingEmptyBg:  return "Docking Empty Background";
		case ImGuiCol_PlotLines:  return "Plot Lines";
		case ImGuiCol_PlotLinesHovered:  return "Plot Lines Hovered";
		case ImGuiCol_PlotHistogram:  return "Plot Histogram";
		case ImGuiCol_PlotHistogramHovered:  return "Plot Histogram Hovered";
		case ImGuiCol_TableHeaderBg:  return "Table Header Background";
		case ImGuiCol_TableBorderStrong:  return "Table Border Strong";
		case ImGuiCol_TableBorderLight:  return "Table Border Light";
		case ImGuiCol_TableRowBg:  return "Table Row Background";
		case ImGuiCol_TableRowBgAlt:  return "Table Row Background Alternate";
		case ImGuiCol_DragDropTarget:  return "Drag Drop Target";
		case ImGuiCol_NavHighlight:  return "Nav Highlight";
		case ImGuiCol_NavWindowingHighlight:  return "Nav Windowing Highlight";
		case ImGuiCol_NavWindowingDimBg:  return "Nav Windowing Dim Background";
		case ImGuiCol_WindowShadow:  return "Window Shadow";
		}

		DY_CORE_ASSERT(false, "Theme color variable does not exist.");
		return "";
	}

	uint8_t Preferences::GetThemeColor(std::string name)
	{
		if (name == "Text") { return ImGuiCol_Text; }
		else if (name == "Text Disabled") { return ImGuiCol_TextDisabled; }
		else if (name == "Text Selected Background") { return ImGuiCol_TextSelectedBg; }
		else if (name == "Window Background") { return ImGuiCol_WindowBg; }
		else if (name == "Child Background") { return ImGuiCol_ChildBg; }
		else if (name == "Menu Bar Background") { return ImGuiCol_MenuBarBg; }
		else if (name == "Menu Bar Grip") { return ImGuiCol_MenuBarGrip; }
		else if (name == "Menu Bar Grip Border") { return ImGuiCol_MenuBarGripBorder; }
		else if (name == "Header") { return ImGuiCol_Header; }
		else if (name == "Header Hovered") { return ImGuiCol_HeaderHovered; }
		else if (name == "Header Active") { return ImGuiCol_HeaderActive; }
		else if (name == "Tab") { return ImGuiCol_Tab; }
		else if (name == "Tab Hovered") { return ImGuiCol_TabHovered; }
		else if (name == "Tab Active") { return ImGuiCol_TabActive; }
		else if (name == "Tab Unfocused") { return ImGuiCol_TabUnfocused; }
		else if (name == "Tab Unfocused Active") { return ImGuiCol_TabUnfocusedActive; }
		else if (name == "Title Background") { return ImGuiCol_TitleBg; }
		else if (name == "Title Background Active") { return ImGuiCol_TitleBgActive; }
		else if (name == "Title Background Collapsed") { return ImGuiCol_TitleBgCollapsed; }
		else if (name == "Button") { return ImGuiCol_Button; }
		else if (name == "Button Hovered") { return ImGuiCol_ButtonHovered; }
		else if (name == "Button Active") { return ImGuiCol_ButtonActive; }
		else if (name == "Button Toggled") { return ImGuiCol_ButtonToggled; }
		else if (name == "Button Toggled & Hovered") { return ImGuiCol_ButtonToggledHovered; }
		else if (name == "Popup Background") { return ImGuiCol_PopupBg; }
		else if (name == "Modal Window Dim Background") { return ImGuiCol_ModalWindowDimBg; }
		else if (name == "Main Window Border Edit") { return ImGuiCol_MainWindowBorderEdit; }
		else if (name == "Main Window Border Play") { return ImGuiCol_MainWindowBorderPlay; }
		else if (name == "Main Window Border Simulate") { return ImGuiCol_MainWindowBorderSimulate; }
		else if (name == "Border") { return ImGuiCol_Border; }
		else if (name == "Border Shadow") { return ImGuiCol_BorderShadow; }
		else if (name == "Frame Background") { return ImGuiCol_FrameBg; }
		else if (name == "Frame Background Hovered") { return ImGuiCol_FrameBgHovered; }
		else if (name == "Frame Background Active") { return ImGuiCol_FrameBgActive; }
		else if (name == "Scroll Bar Background") { return ImGuiCol_ScrollbarBg; }
		else if (name == "Scroll Bar Grab") { return ImGuiCol_ScrollbarGrab; }
		else if (name == "Scroll Bar Grab Hovered") { return ImGuiCol_ScrollbarGrabHovered; }
		else if (name == "Scroll Bar Grab Active") { return ImGuiCol_ScrollbarGrabActive; }
		else if (name == "Scroll Bar Dots") { return ImGuiCol_ScrollbarDots; }
		else if (name == "Progress Bar Background") { return ImGuiCol_ProgressBarBg; }
		else if (name == "Progress Bar Border") { return ImGuiCol_ProgressBarBorder; }
		else if (name == "Progress Bar Fill") { return ImGuiCol_ProgressBarFill; }
		else if (name == "Slider Grab") { return ImGuiCol_SliderGrab; }
		else if (name == "Slider Grab Active") { return ImGuiCol_SliderGrabActive; }
		else if (name == "Separator") { return ImGuiCol_Separator; }
		else if (name == "Separator Hovered") { return ImGuiCol_SeparatorHovered; }
		else if (name == "Separator Active") { return ImGuiCol_SeparatorActive; }
		else if (name == "Resize Grip") { return ImGuiCol_ResizeGrip; }
		else if (name == "Resize Grip Hovered") { return ImGuiCol_ResizeGripHovered; }
		else if (name == "Resize Grip Active") { return ImGuiCol_ResizeGripActive; }
		else if (name == "Default") { return ImGuiCol_TextEditorDefault; }
		else if (name == "Keyword") { return ImGuiCol_TextEditorKeyword; }
		else if (name == "Special Keyword") { return ImGuiCol_TextEditorSpecialKeyword; }
		else if (name == "Number") { return ImGuiCol_TextEditorNumber; }
		else if (name == "String") { return ImGuiCol_TextEditorString; }
		else if (name == "Character Literal") { return ImGuiCol_TextEditorCharLiteral; }
		else if (name == "Punctuation") { return ImGuiCol_TextEditorPunctuation; }
		else if (name == "Preprocessor") { return ImGuiCol_TextEditorPreprocessor; }
		else if (name == "Identifier") { return ImGuiCol_TextEditorIdentifier; }
		else if (name == "Comment") { return ImGuiCol_TextEditorComment; }
		else if (name == "Multiline Comment") { return ImGuiCol_TextEditorMultiLineComment; }
		else if (name == "Line Number") { return ImGuiCol_TextEditorLineNumber; }
		else if (name == "Current Line Fill") { return ImGuiCol_TextEditorCurrentLineFill; }
		else if (name == "Current Line Fill Inactive") { return ImGuiCol_TextEditorCurrentLineFillInactive; }
		else if (name == "Current Line Edge") { return ImGuiCol_TextEditorCurrentLineEdge; }
		else if (name == "Check Mark") { return ImGuiCol_CheckMark; }
		else if (name == "Checkbox") { return ImGuiCol_Checkbox; }
		else if (name == "Checkbox Hovered") { return ImGuiCol_CheckboxHovered; }
		else if (name == "Checkbox Active") { return ImGuiCol_CheckboxActive; }
		else if (name == "Checkbox Ticked") { return ImGuiCol_CheckboxTicked; }
		else if (name == "Check Box Hovered Ticked") { return ImGuiCol_CheckboxHoveredTicked; }
		else if (name == "Docking Preview") { return ImGuiCol_DockingPreview; }
		else if (name == "Docking Empty Background") { return ImGuiCol_DockingEmptyBg; }
		else if (name == "Plot Lines") { return ImGuiCol_PlotLines; }
		else if (name == "Plot Lines Hovered") { return ImGuiCol_PlotLinesHovered; }
		else if (name == "Plot Histogram") { return ImGuiCol_PlotHistogram; }
		else if (name == "Plot Histogram Hovered") { return ImGuiCol_PlotHistogramHovered; }
		else if (name == "Table Header Background") { return ImGuiCol_TableHeaderBg; }
		else if (name == "Table Border Strong") { return ImGuiCol_TableBorderStrong; }
		else if (name == "Table Border Light") { return ImGuiCol_TableBorderLight; }
		else if (name == "Table Row Background") { return ImGuiCol_TableRowBg; }
		else if (name == "Table Row Background Alternate") { return ImGuiCol_TableRowBgAlt; }
		else if (name == "Drag Drop Target") { return ImGuiCol_DragDropTarget; }
		else if (name == "Nav Highlight") { return ImGuiCol_NavHighlight; }
		else if (name == "Nav Windowing Highlight") { return ImGuiCol_NavWindowingHighlight; }
		else if (name == "Nav Windowing Dim Background") { return ImGuiCol_NavWindowingDimBg; }
		else if (name == "Window Shadow") { return ImGuiCol_WindowShadow; }

		DY_CORE_ASSERT(false, "Theme color variable does not exist.");
		return ImGuiCol_COUNT;
	}

	bool Preferences::LoadTheme(const std::filesystem::path& filepath)
	{
		if (!std::filesystem::exists(filepath))
			return false;

		YAML::Node theme;
		try
		{
			theme = YAML::LoadFile(filepath.string());
		}
		catch (YAML::ParserException e)
		{
			return false;
		}

		auto& style = ImGui::GetStyle();
		if (theme)
		{
			auto colors = theme["Colors"];
			if (colors)
			{
				for (auto& color : colors)
				{
					style.Colors[GetThemeColor(color.first.as<std::string>())] = color.second.as<ImVec4>();
				}
			}
		}
	}

	void Preferences::SaveTheme(const std::filesystem::path& filepath)
	{
		auto style = ImGui::GetStyle();

		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Colors" << YAML::Value << YAML::BeginMap; // Colors
		for (size_t i = 0; i < ImGuiCol_COUNT; i++)
			out << YAML::Key << GetThemeColorName(i) << YAML::Value << style.Colors[i];
		out << YAML::EndMap; // Colors
		out << YAML::EndMap;

		std::ofstream fout(filepath);
		fout << out.c_str();
	}

	bool Preferences::LoadKeymap(const std::filesystem::path& filepath)
	{
		if (!std::filesystem::exists(filepath))
			return false;

		YAML::Node keymaps;
		try
		{
			keymaps = YAML::LoadFile(filepath.string());
		}
		catch (YAML::ParserException e)
		{
			return false;
		}

		if (keymaps)
		{
			for (auto keymap : keymaps)
			{
				auto bind = keymap["Bind"];
				if (bind)
				{
					auto& value = s_Keymap[Preferences::Keymap::GetBindEventByName(bind.as<std::string>())];

					auto enabled = keymap["Enabled"];
					if (enabled)
						value.Enabled = enabled.as<bool>();

					auto key = keymap["Key"];
					if (key)
					{
						value.KeyCode = Preferences::Keymap::GetKeyByName(key.as<std::string>());

						value.BindCategory = Preferences::Keymap::BindCategory::Keyboard;

						auto repeats = keymap["Repeats"];
						if (repeats)
							value.Repeats = repeats.as<bool>();
					}

					auto mouse = keymap["Mouse Button"];
					if (mouse)
					{
						value.MouseCode = Preferences::Keymap::GetMouseButtonByName(mouse.as<std::string>());

						value.BindCategory = Preferences::Keymap::BindCategory::MouseButton;
					}

					auto ctrl = keymap["Ctrl"];
					if (ctrl)
						value.Ctrl = ctrl.as<bool>();

					auto alt = keymap["Alt"];
					if (alt)
						value.Alt = alt.as<bool>();

					auto shift = keymap["Shift"];
					if (shift)
						value.Shift = shift.as<bool>();
				}
			}
		}
	}

	void Preferences::SaveKeymap(const std::filesystem::path& filepath)
	{
		YAML::Emitter out;
		out << YAML::BeginSeq;
		for (size_t i = 0; i < Preferences::Keymap::BIND_EVENT_SIZE; i++)
		{
			auto& keybind = s_Keymap[i];

			out << YAML::BeginMap;
			out << YAML::Key << "Bind" << YAML::Value << Preferences::Keymap::GetBindEventName((Preferences::Keymap::KeyBindEvent)i);
			out << YAML::Key << "Enabled" << YAML::Value << keybind.Enabled;
			if (s_Keymap[i].BindCategory == Preferences::Keymap::BindCategory::Keyboard)
			{
				out << YAML::Key << "Key" << YAML::Value << Preferences::Keymap::GetKeyName(keybind.KeyCode);
				out << YAML::Key << "Repeats" << YAML::Value << keybind.Repeats;
			}
			else
				out << YAML::Key << "Mouse Button" << YAML::Value << Preferences::Keymap::GetMouseButtonName(keybind.MouseCode);
			out << YAML::Key << "Ctrl" << YAML::Value << keybind.Ctrl;
			out << YAML::Key << "Alt" << YAML::Value << keybind.Alt;
			out << YAML::Key << "Shift" << YAML::Value << keybind.Shift;

			out << YAML::EndMap;
		}
		out << YAML::EndSeq;

		std::ofstream fout(filepath);
		fout << out.c_str();
	}

	bool Preferences::LoadPreferences(const std::filesystem::path& filepath)
	{
		if (!std::filesystem::exists(filepath))
			return false;

		YAML::Node prefs;
		try
		{
			prefs = YAML::LoadFile(filepath.string());
		}
		catch (YAML::ParserException e)
		{
			return false;
		}

		if (prefs)
		{
			auto autosavePreferences = prefs["Autosave Preferences"];
			if (autosavePreferences)
				GetData().AutosavePreferences = autosavePreferences.as<bool>();

			auto autosaveEnabled = prefs["Autosave Enabled"];
			if (autosaveEnabled)
				GetData().AutosaveEnabled = autosaveEnabled.as<bool>();

			auto autosaveTime = prefs["Autosave Time"];
			if (autosaveTime)
				GetData().AutosaveTime = autosaveTime.as<int>();

			auto recentFiles = prefs["Recent Files"];
			if (recentFiles)
				GetData().RecentFileCount = recentFiles.as<int>();

			auto showSplash = prefs["Show Splash"];
			if (showSplash)
				GetData().ShowSplashStartup = showSplash.as<bool>();

			auto doubleClickSpeed = prefs["Double Click Speed"];
			if (doubleClickSpeed)
			{
				GetData().DoubleClickSpeed = doubleClickSpeed.as<int>();
				ImGui::GetIO().MouseDoubleClickTime = GetData().DoubleClickSpeed / 1000.0f;
			}

			auto emulateNumpad = prefs["Emulate Numpad"];
			if (emulateNumpad)
				GetData().EmulateNumpad = emulateNumpad.as<bool>();

			auto manualDevenv = prefs["Manual Devenv"];
			if (manualDevenv)
				GetData().ManualDevenv = manualDevenv.as<bool>();

			auto devenvPath = prefs["Devenv Path"];
			if (devenvPath)
				GetData().DevenvPath = devenvPath.as<std::string>();
		}
	}

	void Preferences::SavePreferences(const std::filesystem::path& filepath)
	{
		YAML::Emitter out;
		out << YAML::BeginMap;

		out << YAML::Key << "Autosave Preferences" << YAML::Value << GetData().AutosavePreferences;
		out << YAML::Key << "Autosave Enabled" << YAML::Value << GetData().AutosaveEnabled;
		out << YAML::Key << "Autosave Time" << YAML::Value << GetData().AutosaveTime;
		out << YAML::Key << "Recent Files" << YAML::Value << GetData().RecentFileCount;
		out << YAML::Key << "Show Splash" << YAML::Value << GetData().ShowSplashStartup;
		out << YAML::Key << "Double Click Speed" << YAML::Value << GetData().DoubleClickSpeed;
		out << YAML::Key << "Emulate Numpad" << YAML::Value << GetData().EmulateNumpad;
		out << YAML::Key << "Manual Devenv" << YAML::Value << GetData().ManualDevenv;
		out << YAML::Key << "Devenv Path" << YAML::Value << GetData().DevenvPath;

		out << YAML::EndMap;

		std::ofstream fout(filepath);
		fout << out.c_str();
	}

	Preferences::PreferencesData& Preferences::GetData()
	{
		return s_PreferencesData;
	}

	std::vector<Preferences::Keymap::KeyBindEvent> Preferences::Keymap::CheckKey(KeyPressedEvent event)
	{
		std::vector<Preferences::Keymap::KeyBindEvent> events;
		size_t index = 0;
		auto code = event.GetKeyCode();
		for (auto& bind : s_Keymap)
		{
			if (bind.BindCategory == Keyboard && bind.KeyCode == code)
			{
				if ((bind.Ctrl == Input::IsKeyPressed(Key::LeftControl) || bind.Ctrl == Input::IsKeyPressed(Key::RightControl)) &&
					(bind.Shift == Input::IsKeyPressed(Key::LeftShift) || bind.Shift == Input::IsKeyPressed(Key::RightShift)) &&
					(bind.Alt == Input::IsKeyPressed(Key::LeftAlt) || bind.Alt == Input::IsKeyPressed(Key::RightAlt))
					)
				{
					if (bind.Enabled && (bind.Repeats || !event.IsRepeat()))
						events.push_back((Preferences::Keymap::KeyBindEvent)index);
				}
			}
			index++;
		}

		return events;
	}


	std::vector<Preferences::Keymap::KeyBindEvent> Preferences::Keymap::CheckMouseButton(MouseButtonPressedEvent event)
	{
		std::vector<Preferences::Keymap::KeyBindEvent> events;
		size_t index = 0;
		auto code = event.GetMouseButton();
		for (auto& bind : s_Keymap)
		{
			if (bind.BindCategory == MouseButton && bind.MouseCode == code)
			{
				if ((bind.Ctrl == Input::IsKeyPressed(Key::LeftControl) || bind.Ctrl == Input::IsKeyPressed(Key::RightControl)) &&
					(bind.Shift == Input::IsKeyPressed(Key::LeftShift) || bind.Shift == Input::IsKeyPressed(Key::RightShift)) &&
					(bind.Alt == Input::IsKeyPressed(Key::LeftAlt) || bind.Alt == Input::IsKeyPressed(Key::RightAlt))
					)
				{
					if (bind.Enabled)
						events.push_back((Preferences::Keymap::KeyBindEvent)index);
				}
			}
			index++;
		}

		return events;
	}


	std::string Preferences::Keymap::GetBindString(KeyBindEvent event)
	{
		auto& keybind = s_Keymap[event];
		std::string string;
		if (keybind.Ctrl) string += "Ctrl ";
		if (keybind.Shift) string += string.empty() ? "Shift " : "+ Shift ";
		if (keybind.Alt) string += string.empty() ? "Alt " : "+ Alt ";
		if (keybind.BindCategory == Keyboard)
			string += Preferences::Keymap::GetKeyName(keybind.KeyCode);
		else
			string += Preferences::Keymap::GetMouseButtonName(keybind.MouseCode);

		return string;
	}

	Preferences::Preferences::Keymap::KeyBindData& Preferences::Keymap::GetKeyBind(KeyBindEvent event)
	{
		return s_Keymap[event];
	}

	std::array<Preferences::Preferences::Keymap::KeyBindData, Preferences::Keymap::BIND_EVENT_SIZE>& Preferences::Keymap::GetKeymap()
	{
		return s_Keymap;
	}

	std::string Preferences::Keymap::GetBindEventName(KeyBindEvent event)
	{
		if (event == NewSceneBind) return "New Scene";
		else if (event == OpenSceneBind) return "Open Scene";
		else if (event == SaveSceneBind) return "Save Scene";
		else if (event == SaveSceneAsBind) return "Save Scene As";
		else if (event == QuitBind) return "Quit";
		else if (event == SelectObjectBind) return "Mouse Picking";
		else if (event == SceneStartBind) return "Scene Start";
		else if (event == SceneStopBind) return "Scene Stop";
		else if (event == GizmoNoneBind) return "Gizmo None";
		else if (event == GizmoTranslateBind) return "Gizmo Translate";
		else if (event == GizmoRotateBind) return "Gizmo Rotate";
		else if (event == GizmoScaleBind) return "Gizmo Scale";
		else if (event == CreateBind) return "Create";
		else if (event == DuplicateBind) return "Duplicate";
		else if (event == DeleteBind) return "Delete";
		else if (event == ShadingTypeWireframeBind) return "Shading Type Wireframe";
		else if (event == ShadingTypeUnlitBind) return "Shading Type Unlit";
		else if (event == ShadingTypeSolidBind) return "Shading Type Solid";
		else if (event == ShadingTypeRenderedBind) return "Shading Type Rendered";
		else if (event == ToggleShadingTypeBind) return "Toggle Shading Type";
		else if (event == ViewFrontBind) return "View Front";
		else if (event == ViewSideBind) return "View Side";
		else if (event == ViewTopBind) return "View Top";
		else if (event == ViewFlipBind) return "View Flip";
		else if (event == ViewProjectionBind) return "View Projection";
		else if (event == RenameBind) return "Rename";
		else if (event == ClosePopupBind) return "Close Popup";
		else if (event == TextEditorDuplicate) return "Text Editor Duplicate";
		else if (event == TextEditorSwapLineUp) return "Text Editor Swap Line Up";
		else if (event == TextEditorSwapLineDown) return "Text Editor Swap Line Down";
		else if (event == TextEditorSwitchHeader) return "Text Editor Switch Header";

		else return "INVALID";
	}

	Preferences::Keymap::KeyBindEvent Preferences::Keymap::GetBindEventByName(const std::string& name)
	{
		if (name == "New Scene") return Preferences::Keymap::NewSceneBind;
		else if (name == "Open Scene") return Preferences::Keymap::OpenSceneBind;
		else if (name == "Save Scene") return Preferences::Keymap::SaveSceneBind;
		else if (name == "Save Scene As") return Preferences::Keymap::SaveSceneAsBind;
		else if (name == "Quit") return Preferences::Keymap::QuitBind;
		else if (name == "Mouse Picking") return Preferences::Keymap::SelectObjectBind;
		else if (name == "Scene Start") return Preferences::Keymap::SceneStartBind;
		else if (name == "Scene Stop") return Preferences::Keymap::SceneStopBind;
		else if (name == "Gizmo None") return Preferences::Keymap::GizmoNoneBind;
		else if (name == "Gizmo Translate") return Preferences::Keymap::GizmoTranslateBind;
		else if (name == "Gizmo Rotate") return Preferences::Keymap::GizmoRotateBind;
		else if (name == "Gizmo Scale") return Preferences::Keymap::GizmoScaleBind;
		else if (name == "Create") return Preferences::Keymap::CreateBind;
		else if (name == "Duplicate") return Preferences::Keymap::DuplicateBind;
		else if (name == "Delete") return Preferences::Keymap::DeleteBind;
		else if (name == "Shading Type Wireframe") return Preferences::Keymap::ShadingTypeWireframeBind;
		else if (name == "Shading Type Unlit") return Preferences::Keymap::ShadingTypeUnlitBind;
		else if (name == "Shading Type Solid") return Preferences::Keymap::ShadingTypeSolidBind;
		else if (name == "Shading Type Rendered") return Preferences::Keymap::ShadingTypeRenderedBind;
		else if (name == "Toggle Shading Type") return Preferences::Keymap::ToggleShadingTypeBind;
		else if (name == "View Front") return Preferences::Keymap::ViewFrontBind;
		else if (name == "View Side") return Preferences::Keymap::ViewSideBind;
		else if (name == "View Top") return Preferences::Keymap::ViewTopBind;
		else if (name == "View Flip") return Preferences::Keymap::ViewFlipBind;
		else if (name == "View Projection") return Preferences::Keymap::ViewProjectionBind;
		else if (name == "Rename") return Preferences::Keymap::RenameBind;
		else if (name == "Close Popup") return Preferences::Keymap::ClosePopupBind;
		else if (name == "Text Editor Duplicate") return Preferences::Keymap::TextEditorDuplicate;
		else if (name == "Text Editor Swap Line Up") return Preferences::Keymap::TextEditorSwapLineUp;
		else if (name == "Text Editor Swap Line Down") return Preferences::Keymap::TextEditorSwapLineDown;
		else if (name == "Text Editor Switch Header") return Preferences::Keymap::TextEditorSwitchHeader;

		else return Preferences::Keymap::INVALID_BIND;
	}

	std::string Preferences::Keymap::GetKeyName(KeyCode key)
	{
		if (key == Key::Space) return "Space";
		else if (key == Key::Apostrophe) return "Apostrophe";
		else if (key == Key::Comma) return "Comma";
		else if (key == Key::Minus) return "Minus";
		else if (key == Key::Period) return "Period";
		else if (key == Key::Slash) return "Slash";

		else if (key == Key::D0) return "Number 0";
		else if (key == Key::D1) return "Number 1";
		else if (key == Key::D2) return "Number 2";
		else if (key == Key::D3) return "Number 3";
		else if (key == Key::D4) return "Number 4";
		else if (key == Key::D5) return "Number 5";
		else if (key == Key::D6) return "Number 6";
		else if (key == Key::D7) return "Number 7";
		else if (key == Key::D8) return "Number 8";
		else if (key == Key::D9) return "Number 9";

		else if (key == Key::Semicolon) return "Semicolon";
		else if (key == Key::Equal) return "Equal";

		else if (key == Key::A) return "A";
		else if (key == Key::B) return "B";
		else if (key == Key::C) return "C";
		else if (key == Key::D) return "D";
		else if (key == Key::E) return "E";
		else if (key == Key::F) return "F";
		else if (key == Key::G) return "G";
		else if (key == Key::H) return "H";
		else if (key == Key::I) return "I";
		else if (key == Key::J) return "J";
		else if (key == Key::K) return "K";
		else if (key == Key::L) return "L";
		else if (key == Key::M) return "M";
		else if (key == Key::N) return "N";
		else if (key == Key::O) return "O";
		else if (key == Key::P) return "P";
		else if (key == Key::Q) return "Q";
		else if (key == Key::R) return "R";
		else if (key == Key::S) return "S";
		else if (key == Key::T) return "T";
		else if (key == Key::U) return "U";
		else if (key == Key::V) return "V";
		else if (key == Key::W) return "W";
		else if (key == Key::X) return "X";
		else if (key == Key::Y) return "Y";
		else if (key == Key::Z) return "Z";

		else if (key == Key::LeftBracket) return "Left Bracket";
		else if (key == Key::Backslash) return "Backslash";
		else if (key == Key::RightBracket) return "Right Bracket";
		else if (key == Key::GraveAccent) return "Grave Accent";

		else if (key == Key::World1) return "World 1";
		else if (key == Key::World2) return "World 2";

		/* Function keys */
		else if (key == Key::Escape) return "Escape";
		else if (key == Key::Enter) return "Enter";
		else if (key == Key::Tab) return "Tab";
		else if (key == Key::Backspace) return "Backspace";
		else if (key == Key::Insert) return "Insert";
		else if (key == Key::Delete) return "Delete";
		else if (key == Key::Right) return "Right";
		else if (key == Key::Left) return "Left";
		else if (key == Key::Down) return "Down";
		else if (key == Key::Up) return "Up";
		else if (key == Key::PageUp) return "Page Up";
		else if (key == Key::PageDown) return "Page Down";
		else if (key == Key::Home) return "Home";
		else if (key == Key::End) return "End";
		else if (key == Key::CapsLock) return "Caps Lock";
		else if (key == Key::ScrollLock) return "Scroll Lock";
		else if (key == Key::NumLock) return "Num Lock";
		else if (key == Key::PrintScreen) return "Print Screen";
		else if (key == Key::Pause) return "Pause";
		else if (key == Key::F1) return "F1";
		else if (key == Key::F2) return "F2";
		else if (key == Key::F3) return "F3";
		else if (key == Key::F4) return "F4";
		else if (key == Key::F5) return "F5";
		else if (key == Key::F6) return "F6";
		else if (key == Key::F7) return "F7";
		else if (key == Key::F8) return "F8";
		else if (key == Key::F9) return "F9";
		else if (key == Key::F10) return "F10";
		else if (key == Key::F11) return "F11";
		else if (key == Key::F12) return "F12";
		else if (key == Key::F13) return "F13";
		else if (key == Key::F14) return "F14";
		else if (key == Key::F15) return "F15";
		else if (key == Key::F16) return "F16";
		else if (key == Key::F17) return "F17";
		else if (key == Key::F18) return "F18";
		else if (key == Key::F19) return "F19";
		else if (key == Key::F20) return "F20";
		else if (key == Key::F21) return "F21";
		else if (key == Key::F22) return "F22";
		else if (key == Key::F23) return "F23";
		else if (key == Key::F24) return "F24";
		else if (key == Key::F25) return "F25";

		/* Keypad */
		else if (key == Key::KP0) return "Key Pad 0";
		else if (key == Key::KP1) return "Key Pad 1";
		else if (key == Key::KP2) return "Key Pad 2";
		else if (key == Key::KP3) return "Key Pad 3";
		else if (key == Key::KP4) return "Key Pad 4";
		else if (key == Key::KP5) return "Key Pad 5";
		else if (key == Key::KP6) return "Key Pad 6";
		else if (key == Key::KP7) return "Key Pad 7";
		else if (key == Key::KP8) return "Key Pad 8";
		else if (key == Key::KP9) return "Key Pad 9";
		else if (key == Key::KPDecimal) return "Key Pad Decimal";
		else if (key == Key::KPDivide) return "Key Pad Divide";
		else if (key == Key::KPMultiply) return "Key Pad Multiply";
		else if (key == Key::KPSubtract) return "Key Pad Subtract";
		else if (key == Key::KPAdd) return "Key Pad Add";
		else if (key == Key::KPEnter) return "Key Pad Enter";
		else if (key == Key::KPEqual) return "Key Pad Equal";

		else if (key == Key::LeftShift) return "Left Shift";
		else if (key == Key::LeftControl) return "Left Control";
		else if (key == Key::LeftAlt) return "Left Alt";
		else if (key == Key::LeftSuper) return "Left Super";
		else if (key == Key::RightShift) return "Right Shift";
		else if (key == Key::RightControl) return "Right Control";
		else if (key == Key::RightAlt) return "Right Alt";
		else if (key == Key::RightSuper) return "Right Super";
		else if (key == Key::Menu) return "Menu";

		return "INVALID";
	}

	KeyCode Preferences::Keymap::GetKeyByName(const std::string& name)
	{
		if (name == "Space") return Key::Space;
		else if (name == "Apostrophe") return Key::Apostrophe;
		else if (name == "Comma") return Key::Comma;
		else if (name == "Minus") return Key::Minus;
		else if (name == "Period") return Key::Period;
		else if (name == "Slash") return Key::Slash;

		else if (name == "Number 0") return Key::D0;
		else if (name == "Number 1") return Key::D1;
		else if (name == "Number 2") return Key::D2;
		else if (name == "Number 3") return Key::D3;
		else if (name == "Number 4") return Key::D4;
		else if (name == "Number 5") return Key::D5;
		else if (name == "Number 6") return Key::D6;
		else if (name == "Number 7") return Key::D7;
		else if (name == "Number 8") return Key::D8;
		else if (name == "Number 9") return Key::D9;

		else if (name == "Semicolon") return Key::Semicolon;
		else if (name == "Equal") return Key::Equal;

		else if (name == "A") return Key::A;
		else if (name == "B") return Key::B;
		else if (name == "C") return Key::C;
		else if (name == "D") return Key::D;
		else if (name == "E") return Key::E;
		else if (name == "F") return Key::F;
		else if (name == "G") return Key::G;
		else if (name == "H") return Key::H;
		else if (name == "I") return Key::I;
		else if (name == "J") return Key::J;
		else if (name == "K") return Key::K;
		else if (name == "L") return Key::L;
		else if (name == "M") return Key::M;
		else if (name == "N") return Key::N;
		else if (name == "O") return Key::O;
		else if (name == "P") return Key::P;
		else if (name == "Q") return Key::Q;
		else if (name == "R") return Key::R;
		else if (name == "S") return Key::S;
		else if (name == "T") return Key::T;
		else if (name == "U") return Key::U;
		else if (name == "V") return Key::V;
		else if (name == "W") return Key::W;
		else if (name == "X") return Key::X;
		else if (name == "Y") return Key::Y;
		else if (name == "Z") return Key::Z;

		else if (name == "Left Bracket") return Key::LeftBracket;
		else if (name == "Backslash") return Key::Backslash;
		else if (name == "Right Bracket") return Key::RightBracket;
		else if (name == "Grave Accent") return Key::GraveAccent;

		else if (name == "World 1") return Key::World1;
		else if (name == "World 2") return Key::World2;

		/* Function keys */
		else if (name == "Escape") return Key::Escape;
		else if (name == "Enter") return Key::Enter;
		else if (name == "Tab") return Key::Tab;
		else if (name == "Backspace") return Key::Backspace;
		else if (name == "Insert") return Key::Insert;
		else if (name == "Delete") return Key::Delete;
		else if (name == "Right") return Key::Right;
		else if (name == "Left") return Key::Left;
		else if (name == "Down") return Key::Down;
		else if (name == "Up") return Key::Up;
		else if (name == "Page Up") return Key::PageUp;
		else if (name == "Page Down") return Key::PageDown;
		else if (name == "Home") return Key::Home;
		else if (name == "End") return Key::End;
		else if (name == "Caps Lock") return Key::CapsLock;
		else if (name == "Scroll Lock") return Key::ScrollLock;
		else if (name == "Num Lock") return Key::NumLock;
		else if (name == "Print Screen") return Key::PrintScreen;
		else if (name == "Pause") return Key::Pause;
		else if (name == "F1") return Key::F1;
		else if (name == "F2") return Key::F2;
		else if (name == "F3") return Key::F3;
		else if (name == "F4") return Key::F4;
		else if (name == "F5") return Key::F5;
		else if (name == "F6") return Key::F6;
		else if (name == "F7") return Key::F7;
		else if (name == "F8") return Key::F8;
		else if (name == "F9") return Key::F9;
		else if (name == "F10") return Key::F10;
		else if (name == "F11") return Key::F11;
		else if (name == "F12") return Key::F12;
		else if (name == "F13") return Key::F13;
		else if (name == "F14") return Key::F14;
		else if (name == "F15") return Key::F15;
		else if (name == "F16") return Key::F16;
		else if (name == "F17") return Key::F17;
		else if (name == "F18") return Key::F18;
		else if (name == "F19") return Key::F19;
		else if (name == "F20") return Key::F20;
		else if (name == "F21") return Key::F21;
		else if (name == "F22") return Key::F22;
		else if (name == "F23") return Key::F23;
		else if (name == "F24") return Key::F24;
		else if (name == "F25") return Key::F25;

		/* Keypad */
		else if (name == "Key Pad 0") return Key::KP0;
		else if (name == "Key Pad 1") return Key::KP1;
		else if (name == "Key Pad 2") return Key::KP2;
		else if (name == "Key Pad 3") return Key::KP3;
		else if (name == "Key Pad 4") return Key::KP4;
		else if (name == "Key Pad 5") return Key::KP5;
		else if (name == "Key Pad 6") return Key::KP6;
		else if (name == "Key Pad 7") return Key::KP7;
		else if (name == "Key Pad 8") return Key::KP8;
		else if (name == "Key Pad 9") return Key::KP9;
		else if (name == "Key Pad Decimal") return Key::KPDecimal;
		else if (name == "Key Pad Divide") return Key::KPDivide;
		else if (name == "Key Pad Multiply") return Key::KPMultiply;
		else if (name == "Key Pad Subtract") return Key::KPSubtract;
		else if (name == "Key Pad Add") return Key::KPAdd;
		else if (name == "Key Pad Enter") return Key::KPEnter;
		else if (name == "Key Pad Equal") return Key::KPEqual;

		else if (name == "Left Shift") return Key::LeftShift;
		else if (name == "Left Control") return Key::LeftControl;
		else if (name == "Left Alt") return Key::LeftAlt;
		else if (name == "Left Super") return Key::LeftSuper;
		else if (name == "Right Shift") return Key::RightShift;
		else if (name == "Right Control") return Key::RightControl;
		else if (name == "Right Alt") return Key::RightAlt;
		else if (name == "Right Super") return Key::RightSuper;
		else if (name == "Menu") return Key::Menu;

		return Key::Invalid;
	}

	std::string Preferences::Keymap::GetMouseButtonName(MouseCode button)
	{
		if (button == Mouse::ButtonLeft) return "Button Left";
		else if (button == Mouse::ButtonRight) return "Button Right";
		else if (button == Mouse::ButtonMiddle) return "Button Middle";
		else if (button == Mouse::ButtonLast) return "Button Last";
		else if (button == Mouse::Button0) return "Button 0";
		else if (button == Mouse::Button1) return "Button 1";
		else if (button == Mouse::Button2) return "Button 2";
		else if (button == Mouse::Button3) return "Button 3";
		else if (button == Mouse::Button4) return "Button 4";
		else if (button == Mouse::Button5) return "Button 5";
		else if (button == Mouse::Button6) return "Button 6";
		else if (button == Mouse::Button7) return "Button 7";

		return "INVALID";
	}

	MouseCode Preferences::Keymap::GetMouseButtonByName(const std::string& name)
	{
		if (name == "Button Left") return Mouse::ButtonLeft;
		else if (name == "Button Right") return Mouse::ButtonRight;
		else if (name == "Button Middle") return Mouse::ButtonMiddle;
		else if (name == "Button Last") return Mouse::ButtonLast;
		else if (name == "Button 0") return Mouse::Button0;
		else if (name == "Button 1") return Mouse::Button1;
		else if (name == "Button 2") return Mouse::Button2;
		else if (name == "Button 3") return Mouse::Button3;
		else if (name == "Button 4") return Mouse::Button4;
		else if (name == "Button 5") return Mouse::Button5;
		else if (name == "Button 6") return Mouse::Button6;
		else if (name == "Button 7") return Mouse::Button7;

		return Mouse::Invalid;
	}

	std::string Preferences::Keymap::GetBindCategoryName(BindCategory category)
	{
		if (category == Keyboard) return "Keyboard";
		else if (category == MouseButton) return "Mouse Button";

		return "INVALID";
	}

	Preferences::Keymap::BindCategory Preferences::Keymap::GetBindCategoryByName(const std::string& name)
	{
		if (name == "Keyboard") return Keyboard;
		else if (name == "Mouse Button") return MouseButton;

		return Invalid;
	}

	const std::array<KeyCode, 120>& Preferences::Keymap::GetAllKeys()
	{
		return s_AllKeys;
	}

	const std::array<MouseCode, 12>& Preferences::Keymap::GetAllMouseButtons()
	{
		return s_AllMouseButtons;
	}

}