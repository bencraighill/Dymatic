#pragma once

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <array>

namespace ImGui::Custom {

	enum ImGuiCol_
	{
		ImGuiCol_CheckboxBg,
		ImGuiCol_CheckboxBgHovered,
		ImGuiCol_CheckboxBgActive,
		ImGuiCol_CheckboxBgTicked,
		ImGuiCol_CheckboxBgHoveredTicked,
		ImGuiCol_FileBackground,
		ImGuiCol_FileIcon,
		ImGuiCol_FileSelected,
		ImGuiCol_FileHovered,
		IMGUI_CUSTOM_COLOR_SIZE
	};

	ImVec4& GetImGuiCustomColorValue(ImGuiCol_ col);

	bool Splitter(bool split_vertically, float thickness, float* size1, float* size2, float min_size1, float min_size2, float splitter_long_axis_size = -1.0f);
	bool ButtonCornersEx(const char* label, const ImVec2& size_arg, ImGuiButtonFlags flags, float rounding, ImDrawCornerFlags cornerFlags, bool active = false, ImVec2 frameOffsetMin = ImVec2{ 0, 0 }, ImVec2 frameOffsetMax = ImVec2{ 0, 0 });
	bool Checkbox(const char* label, bool* v);

	
	
}
