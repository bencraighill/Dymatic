#pragma once

// Required engine includes
#include "Dymatic/Asset/AssetManager.h"
#include "Dymatic/Renderer/EditorCamera.h"
#include "Dymatic/Renderer/SceneRendererContext.h"

#include "EditorLayer.h"

// ImGui include
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui/imgui.h>

namespace Dymatic::UI {

	// Helpers

	// For windows that have a changing name but fixed ID
	// Warning: Multiple windows with the same ID (regardless of name) will clash. Ensure a string prefix is used to differentiate them.
	// Note: The `GetFixedWindowNameWithID` method can be used as the ID for this function to resolve the warning above.
	template <typename T>
	std::string GetVaryingWindowNameWithID(const std::string& name, T id)
	{
		return fmt::format("{}###{}", name, id);
	}

	// For windows with a fixed name and fixed ID used to differentiate them from other windows with the same name.
	template <typename T>
	std::string GetFixedWindowNameWithID(const std::string& name, T id)
	{
		return fmt::format("{}##{}", name, id);
	}

	// Generic Widgets

	bool DrawTextIconButton(const char* text, const ImVec2 buttonSize);
	bool DrawTextIconButton(const char* text);
	bool DrawTextIconButton(const char* text, bool* value);

	std::string GetAssetHandleLabel(AssetHandle handle);
	void SetShowAssetSelectionCallback(const std::function<void(AssetHandle)>& callback);
	void DrawAssetSelectionDropdown(AssetType type, UUID selectedHandle, std::function<void(UUID)> onSelect);
	bool DrawAssetSelectionDropdown(AssetType type, AssetHandle& handle);

	template<typename T, typename R>
	void DrawAssetSelectionDropdown(AssetType type, const Ref<T>& asset, R onSelect, bool multithreaded = false)
	{
		DrawAssetSelectionDropdown(type, asset == nullptr ? 0 : asset->Handle,
		[&onSelect, multithreaded](UUID handle)
		{
			onSelect(handle == 0 ? nullptr : multithreaded ? AssetManager::RequestAsset<T>(handle) : AssetManager::GetAsset<T>(handle));
		});
	}

	template<typename T>
	bool DrawAssetSelectionDropdown(AssetType type, Ref<T>& asset, bool multithreaded = false)
	{
		bool result = false;

		DrawAssetSelectionDropdown(type, asset, [&](Ref<T> newAsset)
		{
			asset = newAsset;
			result = true;
		}, multithreaded);

		return result;
	}

	template<typename T>
	bool DrawAssetSelectionDropdown(const char* str_id, AssetType type, Ref<T>& asset, bool multithreaded = false)
	{
		ImGui::PushID(str_id);
		bool result = DrawAssetSelectionDropdown<T>(type, asset, multithreaded);
		ImGui::PopID();

		return result;
	}

	void DrawPhysicsLayerSelectionDropdown(uint64_t& physicsLayerID);

	uint64_t& GetEntityPickingID();
	bool DrawEntitySelectionInput(const char* str_id, uint64_t& uuid);
	bool DrawEntitySelectionInputLabeled(const char* label, uint64_t& uuid);

	AssetHandle ContentBrowserAssetDragDropTarget(AssetType type);
	bool ContentBrowserAssetDragDropTarget(AssetType type, AssetHandle& handle);

	bool DrawVec3Control(const char* label, glm::vec3& values, glm::vec3 resetValue = glm::vec3(0.0f), float columnWidth = 100.0f);
	void DrawTransformControl(const char* label, Transform& value);

	void DrawWindowInnerShadows(const ImVec4& color, const float thickness);

	bool IsHoveredTooltipTimer();
	bool IsItemHoveredTooltip();

	bool CollapsingHeader(const char* label, const bool defaultOpen = true);
	bool SelectableTreeNode(ImGuiID id, const char* text, ImGuiTreeNodeFlags treeFlags, bool selected, ImGuiSelectableFlags selectableFlags);

	// Viewport
	void DrawViewportRenderSettingsButton(SceneRendererContext::RendererVisualizationMode& rendererVisualizationMode);
	void DrawCameraSpeedButton(EditorCamera& editorCamera, float& cameraBaseSpeed, int& cameraSpeedScale);

	// Dockspace
	ImGuiID BeginDockspaceWindow(const char* name, bool* open, ImGuiWindowFlags windowFlags, ImGuiWindowClass* windowClass);
	ImGuiWindowClass CreateDockingRestrictionClass(const char* className);

	// Window
	void CenterAppearingWindow(const ImVec2& size);
	void CenterAppearingWindow(const float screenRatio);
	glm::vec2 GetScreenSizeRatio(const float screenRatio);

	// Internal Systems
	void SetEditorContext(EditorLayer* context);
	void PostUIUpdate();

	// Generic Inputs
	void Text(const char* text);
	void Image(const Ref<Texture2D> texture, const ImVec2& size);
	bool Checkbox(const char* label, bool* v);
	bool DragFloat(const char* label, float* v, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", ImGuiSliderFlags flags = 0);
	bool DragFloat2(const char* label, float v[2], float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", ImGuiSliderFlags flags = 0);
	bool DragFloat3(const char* label, float v[3], float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", ImGuiSliderFlags flags = 0);
	bool DragFloat4(const char* label, float v[4], float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", ImGuiSliderFlags flags = 0);
	bool SliderFloat(const char* label, float* v, float v_min, float v_max, const char* format = "%.3f", ImGuiSliderFlags flags = 0);
	bool DragU32(const char* label, uint32_t* v, float v_speed = 1.0f, int v_max = 0, const char* format = nullptr, ImGuiSliderFlags flags = 0);
	bool Combo(const char* label, int* current_item, const char* const items[], int items_count, int popup_max_height_in_items = -1);
	bool ColorEdit3(const char* label, float col[3], ImGuiColorEditFlags flags = 0);
	bool ColorEdit4(const char* label, float col[4], ImGuiColorEditFlags flags = 0);
}