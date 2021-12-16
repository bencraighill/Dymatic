#include "ImGuiCustom.h"

namespace ImGui::Custom {

	extern std::array<ImVec4, IMGUI_CUSTOM_COLOR_SIZE>&& ImGuiCustomColors = { ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, ImVec4{1.0f, 1.0f, 1.0f, 1.0f} };

	ImVec4& GetImGuiCustomColorValue(ImGuiCol_ col)
	{
		return ImGuiCustomColors[col];
	}

	bool ImGui::Custom::Splitter(bool split_vertically, float thickness, float* size1, float* size2, float min_size1, float min_size2, float splitter_long_axis_size /*= -1.0f*/)
	{
		ImGuiContext& g = *GImGui;
		ImGuiWindow* window = g.CurrentWindow;
		ImGuiID id = window->GetID("##Splitter");
		ImRect bb;
		bb.Min = window->DC.CursorPos + (split_vertically ? ImVec2(*size1, 0.0f) : ImVec2(0.0f, *size1));
		bb.Max = bb.Min + CalcItemSize(split_vertically ? ImVec2(thickness, splitter_long_axis_size) : ImVec2(splitter_long_axis_size, thickness), 0.0f, 0.0f);
		return SplitterBehavior(bb, id, split_vertically ? ImGuiAxis_X : ImGuiAxis_Y, size1, size2, min_size1, min_size2, 0.0f);
	}

	bool ImGui::Custom::ButtonCornersEx(const char* label, const ImVec2& size_arg, ImGuiButtonFlags flags, float rounding, ImDrawCornerFlags cornerFlags, bool active, ImVec2 frameOffsetMin, ImVec2 frameOffsetMax)
	{
		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems)
			return false;

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;
		const ImGuiID id = window->GetID(label);
		const ImVec2 label_size = CalcTextSize(label, NULL, true);

		ImVec2 pos = window->DC.CursorPos;
		if ((flags & ImGuiButtonFlags_AlignTextBaseLine) && style.FramePadding.y < window->DC.CurrLineTextBaseOffset) // Try to vertically align buttons that are smaller/have no padding so that text baseline matches (bit hacky, since it shouldn't be a flag)
			pos.y += window->DC.CurrLineTextBaseOffset - style.FramePadding.y;
		ImVec2 size = CalcItemSize(size_arg, label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);

		const ImRect bb(pos, pos + size);
		ItemSize(size, style.FramePadding.y);
		if (!ItemAdd(bb, id))
			return false;

		if (window->DC.ItemFlags & ImGuiItemFlags_ButtonRepeat)
			flags |= ImGuiButtonFlags_Repeat;
		bool hovered, held;
		bool pressed = ButtonBehavior(bb, id, &hovered, &held, flags);
		if (active)
		{
			hovered = true;
			held = true;
		}

		// Render
		const ImU32 col = GetColorU32((held && hovered) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
		RenderNavHighlight(bb, id);


		window->DrawList->AddRectFilled(bb.Min + frameOffsetMin, bb.Max + frameOffsetMax, col, rounding, cornerFlags);
		const float border_size2 = g.Style.FrameBorderSize;
		const float border_size = 0.0f;
		if (true && border_size > 0.0f)
		{
			window->DrawList->AddRect(bb.Min + ImVec2(1, 1) + frameOffsetMin, bb.Max + ImVec2(1, 1) + frameOffsetMax, GetColorU32(ImGuiCol_BorderShadow), rounding, cornerFlags, border_size);
			window->DrawList->AddRect(bb.Min + frameOffsetMin, bb.Max + frameOffsetMax, GetColorU32(/*ImGuiCol_Border*/ImVec4{0.0f, 0.0f, 0.0f, 1.0f}), rounding, cornerFlags, border_size);
		}

		RenderTextClipped(bb.Min + style.FramePadding, bb.Max - style.FramePadding, label, NULL, &label_size, style.ButtonTextAlign, &bb);

		// Automatically close popups
		//if (pressed && !(flags & ImGuiButtonFlags_DontClosePopups) && (window->Flags & ImGuiWindowFlags_Popup))
		//    CloseCurrentPopup();

		IMGUI_TEST_ENGINE_ITEM_INFO(id, label, window->DC.LastItemStatusFlags);
		return pressed;
	}

	bool ImGui::Custom::Checkbox(const char* label, bool* v)
	{
		//Custom Value
		//float rounding = style.FrameRounding;

		float sizeMultiplier = 0.75f;

		float rounding = 3.0f;

		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems)
			return false;

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;
		const ImGuiID id = window->GetID(label);
		const ImVec2 label_size = CalcTextSize(label, NULL, true);

		const float square_sz = GetFrameHeight() * sizeMultiplier;
		const ImVec2 pos = window->DC.CursorPos + ImVec2{ (1.0f - sizeMultiplier) * (square_sz / sizeMultiplier) * 0.5f , (1.0f - sizeMultiplier)* (square_sz / sizeMultiplier) * 0.5f };
		const ImRect total_bb(pos, pos + ImVec2(square_sz + (label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f), label_size.y + style.FramePadding.y * 2.0f));
		ItemSize(total_bb, style.FramePadding.y);
		if (!ItemAdd(total_bb, id))
			return false;

		bool hovered, held;
		bool pressed = ButtonBehavior(total_bb, id, &hovered, &held);
		if (pressed)
		{
			*v = !(*v);
			MarkItemEdited(id);
		}

		const ImRect check_bb(pos , pos + ImVec2(square_sz, square_sz));
		RenderNavHighlight(total_bb, id);
		RenderFrame(check_bb.Min, check_bb.Max, GetColorU32((*v && hovered) ? ImGuiCustomColors[ImGuiCol_CheckboxBgHoveredTicked] : (*v) ? ImGuiCustomColors[ImGuiCol_CheckboxBgTicked] : (held && hovered) ? ImGuiCustomColors[ImGuiCol_CheckboxBgActive] : hovered ? ImGuiCustomColors[ImGuiCol_CheckboxBgHovered] : ImGuiCustomColors[ImGuiCol_CheckboxBg]), true, rounding);
		ImU32 check_col = GetColorU32(ImGuiCol_CheckMark);
		if (window->DC.ItemFlags & ImGuiItemFlags_MixedValue)
		{
			// Undocumented tristate/mixed/indeterminate checkbox (#2644)
			ImVec2 pad(ImMax(1.0f, IM_FLOOR(square_sz / 3.6f)), ImMax(1.0f, IM_FLOOR(square_sz / 3.6f)));
			window->DrawList->AddRectFilled(check_bb.Min + pad, check_bb.Max - pad, check_col, rounding);
		}
		else if (*v)
		{
			const float pad = ImMax(1.0f, IM_FLOOR(square_sz / 6.0f));
			RenderCheckMark(window->DrawList, check_bb.Min + ImVec2(pad, pad), check_col, square_sz - pad * 2.0f);
		}

		if (g.LogEnabled)
			LogRenderedText(&total_bb.Min, *v ? "[x]" : "[ ]");
		if (label_size.x > 0.0f)
			RenderText(ImVec2(check_bb.Max.x + style.ItemInnerSpacing.x, check_bb.Min.y + style.FramePadding.y), label);

		IMGUI_TEST_ENGINE_ITEM_INFO(id, label, window->DC.ItemFlags | ImGuiItemStatusFlags_Checkable | (*v ? ImGuiItemStatusFlags_Checked : 0));
		return pressed;
	}
}

