#pragma once
#include "imgui/imgui.h"
#include <string>
#include "Dymatic/Core/Base.h"

namespace Dymatic {

	enum ColorSchemeType
	{
		//---
		Text,
		TextDisabled,
		TextSelectedBg,
		//---//
		MenuBarBg,
		MenuBarGrip,
		MenuBarGripBorder,
		WindowBg,
		Header,
		HeaderHovered,
		HeaderActive,
		Tab,
		TabHovered,
		TabActive,
		TabUnfocused,
		TabUnfocusedActive,
		TitleBg,
		TitleBgActive,
		TitleBgCollapsed,
		Button,
		ButtonHovered,
		ButtonActive,
		ButtonToggled,
		ButtonToggledHovered,
		//---
		PopupBg,
		ModalWindowDimBg,
		Border,
		BorderShadow,
		//---//
		FrameBg,
		FrameBgHovered,
		FrameBgActive,
		//---
		ScrollbarBg,
		ScrollbarGrab,
		ScrollbarGrabHovered,
		ScrollbarGrabActive,
		ScrollbarDots,
		//----
		ProgressBarBg,
		ProgressBarBorder,
		ProgressBarFill,
		//----
		SliderGrab,
		SliderGrabActive,
		Separator,
		SeparatorHovered,
		SeparatorActive,
		ResizeGrip,
		ResizeGripHovered,
		ResizeGripActive,
		//---
		FileBackground,
		FileIcon,
		FileHovered,
		FileSelected,
		//---//
		
		CheckMark,
		Checkbox,
		CheckboxHovered,
		CheckboxActive,
		CheckboxTicked,
		CheckboxHoveredTicked,
		//---
		DockingPreview,
		DockingEmptyBg,
		PlotLines,
		PlotLinesHovered,
		PlotHistogram,
		PlotHistogramHovered,
		DragDropTarget,
		NavHighlight,
		NavWindowingHighlight,
		NavWindowingDimBg,
		//---
		TextEditorDefault,
		TextEditorKeyword,
		TextEditorSpecialKeyword,
		TextEditorNumber,
		TextEditorString,
		TextEditorCharLiteral,
		TextEditorPunctuation,
		TextEditorPreprocessor,
		TextEditorIdentifier,
		TextEditorComment,
		TextEditorMultiLineComment,
		TextEditorLineNumber,
		TextEditorCurrentLineFill,
		TextEditorCurrentLineFillInactive,
		TextEditorCurrentLineEdge,
		//-------//
		DYMATIC_COLOR_SCHEME_SIZE
	};

	std::string GetStringFromTheme(ColorSchemeType theme);
	ColorSchemeType GetThemeFromString(std::string string);

	struct ColorScheme
	{
		ColorSchemeType colorSchemeType;
		std::array<ImVec4, DYMATIC_COLOR_SCHEME_SIZE>&& colorSchemeValues = { ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f},  ImVec4{1.0f, 1.0f, 1.0f, 1.0f},  ImVec4{1.0f, 1.0f, 1.0f, 1.0f},  ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, 
			ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f},
			ImVec4{1.0f, 1.0f, 1.0f, 1.0f} , ImVec4{1.0f, 1.0f, 1.0f, 1.0f} , ImVec4{1.0f, 1.0f, 1.0f, 1.0f} , ImVec4{1.0f, 1.0f, 1.0f, 1.0f} , ImVec4{1.0f, 1.0f, 1.0f, 1.0f} , ImVec4{1.0f, 1.0f, 1.0f, 1.0f} , ImVec4{1.0f, 1.0f, 1.0f, 1.0f} , ImVec4{1.0f, 1.0f, 1.0f, 1.0f} };
	};


}
