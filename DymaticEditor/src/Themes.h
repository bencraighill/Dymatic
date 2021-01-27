#pragma once
#include "imgui/imgui.h"

namespace Dymatic {

	enum ColorSchemeType
	{
		//---
		Text,
		TextDisabled,
		TextSelectedBg,
		//---//
		MenuBarBg,
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
		//-------//
		DYMATIC_COLOR_SCHEME_SIZE
	};

	struct ColorScheme
	{
		ColorSchemeType colorSchemeType;
		//ImVec4 ImGuiCustomColors[IMGUI_CUSTOM_COLOR_SIZE] = { ImVec4{1.0f, 1.0f, 1.0f, 1.0f} };
		std::array<ImVec4, DYMATIC_COLOR_SCHEME_SIZE>&& colorSchemeValues = { ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f},  ImVec4{1.0f, 1.0f, 1.0f, 1.0f},  ImVec4{1.0f, 1.0f, 1.0f, 1.0f},  ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, 
			ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f},
			ImVec4{1.0f, 1.0f, 1.0f, 1.0f} , ImVec4{1.0f, 1.0f, 1.0f, 1.0f} , ImVec4{1.0f, 1.0f, 1.0f, 1.0f} , ImVec4{1.0f, 1.0f, 1.0f, 1.0f} , ImVec4{1.0f, 1.0f, 1.0f, 1.0f} , ImVec4{1.0f, 1.0f, 1.0f, 1.0f} , ImVec4{1.0f, 1.0f, 1.0f, 1.0f} , ImVec4{1.0f, 1.0f, 1.0f, 1.0f} };
	};


}
