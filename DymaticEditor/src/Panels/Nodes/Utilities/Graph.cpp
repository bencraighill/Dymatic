#include "Panels/Nodes/Utilities/Graph.h"

#include "EditorResources.h"
#include "Fonts.h"
#include "TextSymbols.h"

#include <imgui_stdlib.h>
#include <imgui_node_editor.h>

#include <glm/gtc/type_ptr.hpp>

namespace Dymatic::UI {

	namespace ed = ax::NodeEditor;

	// TODO: Move to styling
	static const float s_CommentAlpha = 0.75f;

	static const float s_SearchWindowSize = 400.0f;

	ImRect GetItemRect()
	{
		return ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
	}

	ImRect ExpandedRect(const ImRect& rect, float x, float y)
	{
		auto result = rect;
		result.Min.x -= x;
		result.Min.y -= y;
		result.Max.x += x;
		result.Max.y += y;
		return result;
	}

	void DrawLabel(const char* label, ImColor color)
	{
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() - ImGui::GetTextLineHeight());
		auto size = ImGui::CalcTextSize(label);

		auto padding = ImGui::GetStyle().FramePadding;
		auto spacing = ImGui::GetStyle().ItemSpacing;

		ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(spacing.x, -spacing.y));

		auto rectMin = ImGui::GetCursorScreenPos() - padding;
		auto rectMax = ImGui::GetCursorScreenPos() + size + padding;

		auto drawList = ImGui::GetWindowDrawList();
		drawList->AddRectFilled(rectMin, rectMax, color, size.y * 0.15f);
		ImGui::TextUnformatted(label);
	}

	bool BeginDockedGraphWindow(const char* label, AssetHandle handle, const ImGuiWindowFlags windowFlags)
	{
		return ImGui::Begin(UI::GetFixedWindowNameWithID(label, handle).c_str(), nullptr, ImGuiWindowFlags_NoNavFocus | windowFlags);
	}

	void DrawGraphOverlay(const char* overlayText)
	{
		UI::PushFont(FontType::ExtraLarge);
		ImGui::GetWindowDrawList()->AddText(ImGui::GetWindowPos() + ImGui::GetWindowSize() - ImGui::CalcTextSize(overlayText) - ImVec2(20.0f, 20.0f), IM_COL32(255, 255, 255, 50), overlayText);
		UI::PopFont();
		UI::DrawWindowInnerShadows(ImGui::GetStyleColorVec4(ImGuiCol_BorderShadow), 75.0f);
	}

	void DrawErrorMessage(const float minX, const float maxX, const float maxY)
	{
		const float fontSize = ImGui::GetFontSize();
		const ImGuiStyle& style = ImGui::GetStyle();

		const char* message = "ERROR!";
		ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(minX + style.FramePadding.x, maxY - fontSize), ImVec2(maxX - style.FramePadding.x, maxY), ImColor(255, 0, 0), 5.0f);
		ImGui::GetWindowDrawList()->AddText(ImVec2(minX + (maxX - minX - ImGui::CalcTextSize(message).x) * 0.5f, maxY - fontSize), ImColor(255, 255, 255), message);
	}

	void DrawNodeErrorMessage(uint64_t id)
	{
		const ImVec2 nodeMin = ed::GetNodePosition(id);
		const ImVec2 nodeMax = nodeMin + ed::GetNodeSize(id);

		DrawErrorMessage(nodeMin.x, nodeMax.x, nodeMax.y);
	}

	void DrawNodeShadow(uint64_t id)
	{
		auto& pos = ed::GetNodePosition(id) - ImVec2(10.0f, 10.0f);
		auto& size = ed::GetNodeSize(id) + ImVec2(20.0f, 20.0f);
		auto drawList = ImGui::GetWindowDrawList();

		const int vert_start_idx = drawList->VtxBuffer.Size;
		drawList->AddRect(pos, pos + size, ImColor(0, 0, 0, 75), ed::GetStyle().NodeRounding + 5.0f, ImDrawFlags_RoundCornersAll, 20.0f);
		const int vert_end_idx = drawList->VtxBuffer.Size;
		ImDrawVert* vert_start = drawList->VtxBuffer.Data + vert_start_idx;
		ImDrawVert* vert_end = drawList->VtxBuffer.Data + vert_end_idx;
		for (ImDrawVert* vert = vert_start; vert < vert_end; vert++)
			if (vert->pos.x < pos.x || vert->pos.y < pos.y || vert->pos.x > pos.x + size.x || vert->pos.y > pos.y + size.y || (pos.y + size.y - vert->pos.y) > size.y * 0.5f)
				vert->col = ImGui::ColorConvertFloat4ToU32(ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
	}

	static uint64_t s_HoveredCommentID = 0;
	static float s_CommentOpacity = 0.0f;

	void DrawNodeComment(uint64_t id, bool& enabled, bool& pinned, std::string& comment, const glm::vec3& color)
	{
		ImGui::PushID(id);
		if (enabled)
		{
			ImVec4 commentColor = glm::vec4(color, 1.0f);
			const float bgAlpha = ImGui::GetStyle().Alpha;

			auto zoom = pinned ? ed::GetCurrentZoom() : 1.0f;
			auto padding = ImVec2(2.5f, 2.5f);

			auto min = ed::GetNodePosition(id) - ImVec2(0, padding.y * zoom * 2.0f);

			auto originalScale = ImGui::GetFont()->Scale;
			ImGui::GetFont()->Scale = zoom;
			ImGui::PushFont(ImGui::GetFont());

			ImGui::SetCursorScreenPos(min - ImVec2(-8, ImGui::GetTextLineHeightWithSpacing() + 4));
			ImGui::BeginGroup();

			ImGui::SelectableInput("##NodeComment", comment.empty() ? 1.0f : ImGui::CalcTextSize(comment.c_str()).x, false, 0, &comment, nullptr, ImGuiInputTextFlags_NoHorizontalScroll);

			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4());
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4());
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f * zoom);

			ImGui::SameLine();

			if (ImGui::ImageButton((ImTextureID)(pinned ? EditorResources::PinnedIcon : EditorResources::PinIcon)->GetRendererID(), ImVec2(15.0f, 15.0f) * zoom, { 0, 1 }, { 1, 0 }, 2 * zoom))
				pinned = !pinned;

			ImGui::SameLine();

			if (ImGui::ImageButton((ImTextureID)EditorResources::CommentIcon->GetRendererID(), ImVec2(15.0f, 15.0f) * zoom, { 0, 1 }, { 1, 0 }, 2 * zoom))
				enabled = false;

			ImGui::PopStyleVar();
			ImGui::PopStyleColor(3);

			ImGui::EndGroup();

			ImGui::GetFont()->Scale = originalScale;
			ImGui::PopFont();

			auto drawList = ImGui::GetWindowDrawList();

			const ImRect hintBounds = GetItemRect();
			const ImRect hintFrameBounds = ExpandedRect(hintBounds, 8, 4);

			commentColor.w = (64.0f / 255.0f) * bgAlpha;
			drawList->AddRectFilled(
				hintFrameBounds.GetTL() - padding * zoom,
				hintFrameBounds.GetBR() + padding * zoom,
				ImGui::GetColorU32(commentColor), 4.0f * zoom);

			commentColor.w = (128.0f / 255.0f) * bgAlpha;
			drawList->AddRect(
				hintFrameBounds.GetTL() - padding * zoom,
				hintFrameBounds.GetBR() + padding * zoom,
				ImGui::GetColorU32(commentColor), 4.0f * zoom, ImDrawFlags_RoundCornersAll, zoom);
		}
		else if (ImGui::IsItemHovered() || (s_CommentOpacity > -1.0f && s_HoveredCommentID == id))
		{
			bool nodeHovered = ImGui::IsItemHovered();
			if (nodeHovered)
				s_HoveredCommentID = id;

			auto zoom = pinned ? ed::GetCurrentZoom() : 1.0f;
			const ImVec2 padding = ImVec2(2.5f, 2.5f);

			const ImVec2 min = ed::GetNodePosition(id) - ImVec2(0, padding.y * zoom * 2.0f);

			ImGui::SetCursorScreenPos(min - ImVec2(-8, ImGui::GetTextLineHeightWithSpacing() + 4));
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, s_CommentOpacity);
			if (ImGui::ImageButton((ImTextureID)EditorResources::CommentIcon->GetRendererID(), ImVec2(15.0f, 15.0f) * zoom, { 0, 1 }, { 1, 0 }, 2 * zoom))
			{
				s_CommentOpacity = -2.5f;
				enabled = true;
			}

			ImGui::PopStyleVar();

			s_CommentOpacity = ImGui::IsItemHovered() || nodeHovered ? std::min(s_CommentOpacity + 0.1f, 1.0f) : std::max(s_CommentOpacity - 0.1f, -2.5f);
		}

		ImGui::PopID();
	}

	void DrawCommentNode(const uint64_t id, std::string& comment, glm::vec2& size, const glm::vec3& color)
	{
		ImGui::PushID(id);

		ImVec4 commentColor = glm::vec4(color, 64.0f / 255.0f);

		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, s_CommentAlpha);
		ed::PushStyleColor(ed::StyleColor_NodeBg, commentColor);
		ed::PushStyleColor(ed::StyleColor_NodeBorder, commentColor);

		ed::BeginNode(id);
		ImGui::BeginVertical("content");
		ImGui::BeginHorizontal("horizontal");
		ImGui::Spring(1);

		ImGui::SelectableInput("##CommentName", comment.empty() ? 1.0f : ImGui::CalcTextSize(comment.c_str()).x, false, 0, &comment, nullptr, ImGuiInputTextFlags_NoHorizontalScroll);

		ImGui::Spring(1);
		ImGui::EndHorizontal();
		ed::Group(size);
		ImGui::EndVertical();
		ed::EndNode();

		if (ed::BeginGroupHint(id))
		{
			const float bgAlpha = ImGui::GetStyle().Alpha;

			auto min = ed::GetGroupMin();
			ImGui::SetCursorScreenPos(min - ImVec2(-8, ImGui::GetTextLineHeightWithSpacing() + 4));
			ImGui::BeginGroup();
			ImGui::TextUnformatted(comment.c_str());
			ImGui::EndGroup();

			auto drawList = ed::GetHintBackgroundDrawList();

			const ImRect hintBounds = UI::GetItemRect();
			const ImRect hintFrameBounds = UI::ExpandedRect(hintBounds, 8, 4);

			commentColor.w = (64.0f / 255.0f) * bgAlpha;
			drawList->AddRectFilled(
				hintFrameBounds.GetTL(),
				hintFrameBounds.GetBR(),
				ImGui::GetColorU32(commentColor), 4.0f);

			commentColor.w = (128.0f / 255.0f) * bgAlpha;
			drawList->AddRect(
				hintFrameBounds.GetTL(),
				hintFrameBounds.GetBR(),
				ImGui::GetColorU32(commentColor), 4.0f);
		}
		ed::EndGroupHint();
		ed::PopStyleColor(2);
		ImGui::PopStyleVar();

		size = ed::GetNodeSize(id);

		ImGui::PopID();

		// Draw shadows beneath nodes
		UI::DrawNodeShadow(id);
	}

	void DrawNodeDefaultValueInput(const ValueInputType type, void* data, const AssetType assetType, const bool spring)
	{
		// Guard to ensure that a valid type was given
		bool drawn = true;

		if (type == ValueInputType::Bool) ImGui::Checkbox("##NodeEditorCheckbox", (bool*)data);
		else if (type == ValueInputType::Int) ImGui::DragInt("##NodeEditorDragInt", (int*)data, 0.1f);
		else if (type == ValueInputType::Float) ImGui::DragFloat("##NodeEditorDragFloat", (float*)data, 0.1f);
		else if (type == ValueInputType::Vector2) ImGui::DragFloat2("##NodeEditorDragFloat2", (float*)data, 0.1f);
		else if (type == ValueInputType::Vector3) ImGui::DragFloat3("##NodeEditorDragFloat3", (float*)data, 0.1f);
		else if (type == ValueInputType::Vector4) ImGui::DragFloat4("##NodeEditorDragFloat4", (float*)data, 0.1f);
		else if (type == ValueInputType::Transform) UI::DrawTransformControl("##NodeEditorTransformControl", *(Transform*)data);
		else if (type == ValueInputType::Handle)
		{
			UI::DrawAssetSelectionDropdown(assetType, *(AssetHandle*)data, [data](UUID handle)
			{
				*(AssetHandle*)data = handle;
			});
		}
		else if (type == ValueInputType::String) ImGui::InputText("##NodeEditorInputText", (std::string*)data);
		else if (type == ValueInputType::StringMultiline)
		{
			std::string* string = (std::string*)data;

			const float height = std::min(ImGui::CalcTextSize(string->c_str()).y + ImGui::GetTextLineHeight() * 2.0f, 250.0f);
			const ImVec2 size = ImVec2(spring ? 0.0f : ImGui::GetContentRegionAvailWidth(), height);
			ImGui::InputTextMultiline("##MaterialEditorInputTextMultiline", string, size, ImGuiInputTextFlags_AllowTabInput);
		}
		else
		{
			DY_CORE_ASSERT(false, "Unknown ValueInputType for DefaultValueInput control");
			drawn = false;
		}

		if (spring && drawn)
			ImGui::Spring(0);
	}

	static uint64_t DrawSearchTree(const std::string& name, const NodeSearchTree& tree, bool setOpen, bool emptySearch)
	{
		uint64_t node = 0;

		if (setOpen)
			ImGui::SetNextItemOpen(!emptySearch);

		const bool isRoot = name.empty();

		ImGui::PushItemFlag(ImGuiItemFlags_NoNav, !emptySearch);
		const bool open = isRoot ? true : ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_SpanAvailWidth);
		ImGui::PopItemFlag();

		if (open)
		{
			for (const auto& [name, subtree] : tree.Subtrees)
			{
				uint64_t newNodeId = DrawSearchTree(name, subtree, setOpen, emptySearch);
				if (newNodeId != 0)
					node = newNodeId;
			}

			for (const auto& [name, result] : tree.Results)
			{
				if (ImGui::MenuItem(name.c_str()) && result.Callback)
					node = result.Callback();
			}

			if (!isRoot)
				ImGui::TreePop();
		}

		return node;
	}

	uint64_t DrawSearchWindow(const char* graphName, const NodeSearchTree& tree, std::string& searchBuffer, bool& contextSensitive, std::function<void()> onUpdate)
	{
		ImGui::SetNextWindowSize(ImVec2(s_SearchWindowSize, s_SearchWindowSize));
		if (!ImGui::BeginPopup("Create New Node"))
			return 0;

		ImGui::Text(fmt::format(FA_SITEMAP " All Actions for this {}", graphName).c_str());
		ImGui::SameLine();
		ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvailWidth() - 140.0f, 0.0f));
		ImGui::SameLine();

		if (ImGui::Checkbox("##NodeSearchPopupContextSensitiveCheckbox", &contextSensitive))
			onUpdate();

		ImGui::SameLine();
		ImGui::Text("Context Sensitive");

		bool edited = false;

		if (ImGui::IsWindowAppearing())
		{
			// Reset the search area and trigger a full update
			ImGui::SetKeyboardFocusHere();
			searchBuffer = std::string();

			edited = true;
		}

		ImGui::SetNextItemWidth(-1);
		if (ImGui::InputTextWithHint("##NodeEditorSearchBar", FA_MAGNIFYING_GLASS " Search:", &searchBuffer))
			edited = true;

		if (edited)
			onUpdate();

		if (tree.Results.empty() && tree.Subtrees.empty())
			ImGui::TextDisabled("No node results found.");

		const uint64_t node = DrawSearchTree(std::string(), tree, edited, searchBuffer.empty());

		ImGui::EndPopup();

		return node;
	}

	static ImGuiCol GetCompileMessageColor(CompilerMessageType type)
	{
		switch (type)
		{
		case CompilerMessageType::Compile:	return ImGuiCol_LogTrace;
		case CompilerMessageType::Info:		return ImGuiCol_LogInfo;
		case CompilerMessageType::Warning:	return ImGuiCol_LogWarn;
		case CompilerMessageType::Error:	return ImGuiCol_LogError;
		}

		return ImGuiCol_LogInfo;
	}

	static const char* GetCompileMessageIcon(CompilerMessageType type)
	{
		switch (type)
		{
		case CompilerMessageType::Compile:	return FA_GEARS;
		case CompilerMessageType::Info:		return FA_CIRCLE_INFO;
		case CompilerMessageType::Warning:	return FA_TRIANGLE_EXCLAMATION;
		case CompilerMessageType::Error:	return FA_OCTAGON_XMARK;
		}

		return FA_QUESTION;
	}

	void DrawCompilerResultWindow(AssetHandle handle, const CompilerResult& result)
	{
		ImGui::Begin(UI::GetFixedWindowNameWithID(FA_TERMINAL " Compiler Results", handle).c_str(), nullptr, ImGuiWindowFlags_NoNavFocus);

		auto drawList = ImGui::GetWindowDrawList();

		for (const auto& message : result.Messages)
		{
			const float fontSize = ImGui::GetFontSize();

			ImGui::TextColored(ImGui::GetStyleColorVec4(GetCompileMessageColor(message.Type)), GetCompileMessageIcon(message.Type));
			ImGui::SameLine();
			ImGui::Selectable(message.Message.c_str());
			if (ImGui::IsItemClicked() && ImGui::IsMouseDoubleClicked(0) && message.Handle != 0)
			{
				ed::SelectNode(message.Handle);
				ed::NavigateToSelection();
			}
		}

		ImGui::End();
	}

}
