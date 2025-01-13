#include "UI.h"

#include "EditorResources.h"
#include "Settings/Preferences.h"

#include "Dymatic/Project/Project.h"
#include "Dymatic/Renderer/Texture.h"
#include "Dymatic/Math/Math.h"
#include "Thumbnails/ThumbnailManager.h"

#include "Fonts.h"
#include "TextSymbols.h"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/imgui_stdlib.h>

#include "Dymatic/Math/StringUtils.h"

namespace Dymatic::UI {

	static EditorLayer* s_EditorContext = nullptr;
	static std::function<void(AssetHandle)> s_ShowAssetSelectionCallback = nullptr;

	void SetShowAssetSelectionCallback(const std::function<void(AssetHandle)>& callback)
	{
		s_ShowAssetSelectionCallback = callback;
	}

	static void DrawAssetSelectionButton(const char* text, const std::function<void(AssetHandle)>& callback, AssetHandle handle)
	{
		if (DrawTextIconButton(text) && callback)
			callback(handle);
	}

	bool DrawTextIconButton(const char* text, const ImVec2 buttonSize)
	{
		bool result = false;

		ImGui::PushID(text);

		const ImGuiStyle& style = ImGui::GetStyle();
		const ImVec2 buttonPosition = ImGui::GetCursorScreenPos() + ImVec2(-style.FramePadding.x, style.FramePadding.y);
		if (ImGui::InvisibleButton("##AssetSelectionButton", buttonSize))
			result = true;

		const bool disabled = (GImGui->CurrentItemFlags & ImGuiItemFlags_Disabled) != 0;
		const ImVec4 color = ImGui::GetStyleColorVec4(ImGui::IsItemActive() ? ImGuiCol_HeaderActive : (ImGui::IsItemHovered() ? ImGuiCol_HeaderHovered : disabled ? ImGuiCol_TextDisabled : ImGuiCol_Text));
		ImGui::GetWindowDrawList()->AddText(buttonPosition, ImGui::GetColorU32(color), text);

		ImGui::PopID();

		return result;
	}

	bool DrawTextIconButton(const char* text)
	{
		const float lineHeight = ImGui::GetTextLineHeight();
		return DrawTextIconButton(text, ImVec2(lineHeight, lineHeight));
	}

	bool DrawTextIconButton(const char* text, bool* value)
	{
		const bool disabled = !(*value);

		if (disabled) ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
		const bool result = DrawTextIconButton(text);
		if (disabled) ImGui::PopStyleColor();

		if (result)
			*value = disabled;

		return result;
	}

	std::string GetAssetHandleLabel(AssetHandle handle)
	{
		const auto& metadata = AssetManager::GetMetadata(handle);
		const char* fileTypeIcon = FileManager::GetFileTypeCharacterIcon(FileManager::GetFileType(metadata.Type));
		return handle == 0 ? "Select Asset" : AssetManager::DoesAssetExist(handle) ? fmt::format("{} {}", fileTypeIcon, (metadata.MemoryOnly || metadata.FilePath.empty()) ? "[Memory Only]" : AssetManager::GetMetadata(handle).FilePath.filename().stem().string()) : FA_QUESTION " Unknown Asset";
	}

	void DrawAssetSelectionDropdown(AssetType type, UUID selectedHandle, std::function<void(UUID)> onSelect)
	{
		const ImGuiStyle& style = ImGui::GetStyle();
		const float buttonSize = ImGui::GetTextLineHeight();
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth() - (selectedHandle == 0 ? 0.0f : (buttonSize + style.FramePadding.x * 2.0f) * 2.0f));

		const std::string& currentSelection = GetAssetHandleLabel(selectedHandle);
		if (ImGui::BeginCombo("##Asset", currentSelection.c_str()))
		{
			const auto& metadataRegistry = AssetManager::GetMetadataRegistry();

			bool found = false;
			for (auto& [handle, metadata] : metadataRegistry)
			{
				if ((metadata.MemoryOnly || metadata.FilePath.empty()) || (type != AssetType::None && !AssetManager::IsAssetTypeCompatible(type, metadata.Type)))
					continue;

				found = true;
				ImGui::PushID(handle);

				if (Preferences::GetData().ShowThumbnails)
				{
					Ref<Texture2D> thumbnail = ThumbnailManager::GetOrCreateThumbnail(handle);

					if (thumbnail)
					{
						ImGui::Image((ImTextureID)thumbnail->GetRendererID(), ImVec2(ImGui::GetTextLineHeight(), ImGui::GetTextLineHeight()), { 0, 1 }, { 1, 0 });
						ImGui::SameLine();
					}
				}

				const char* fileTypeIcon = FileManager::GetFileTypeCharacterIcon(FileManager::GetFileType(metadata.Type));
				if (ImGui::Selectable(fmt::format("{} {}", fileTypeIcon, metadata.FilePath.filename().stem().string()).c_str(), handle == selectedHandle))
					onSelect(handle);
				ImGui::PopID();
			}

			if (!found)
				ImGui::TextDisabled("No Assets Available");

			ImGui::EndCombo();
		}

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
			{
				const wchar_t* path = (const wchar_t*)payload->Data;
				std::filesystem::path assetPath(path);

				if (AssetManager::DoesAssetExist(assetPath))
				{
					const AssetMetadata& metadata = AssetManager::GetMetadata(assetPath);
					if (AssetManager::IsAssetTypeCompatible(type, metadata.Type))
						onSelect(metadata.Handle);
				}
			}
			ImGui::EndDragDropTarget();
		}

		const bool hasSelection = selectedHandle != 0;

		// Clear selected asset context menu
		if (ImGui::BeginPopupContextItem())
		{
			if (hasSelection && ImGui::MenuItem(FA_COPY " Copy"))
				ImGui::SetClipboardText(std::to_string(selectedHandle).c_str());

			if (ImGui::MenuItem(FA_PASTE " Paste"))
			{
				String::TryGetHandleFromString(ImGui::GetClipboardText(), [&](uint64_t handle)
				{
					if (AssetManager::DoesAssetExist(handle) && AssetManager::IsAssetTypeCompatible(type, AssetManager::GetMetadata(handle).Type))
						onSelect(handle);
				});
			}

			if (hasSelection && ImGui::MenuItem(FA_CIRCLE_XMARK " Clear"))
				onSelect(0);

			ImGui::EndPopup();
		}

		if (hasSelection)
		{
			ImGui::SameLine();
			DrawAssetSelectionButton(FA_MAGNIFYING_GLASS, s_ShowAssetSelectionCallback, selectedHandle);
			ImGui::SameLine();
			DrawAssetSelectionButton(FA_CIRCLE_XMARK, onSelect, 0);
		}
	}

	bool DrawAssetSelectionDropdown(AssetType type, AssetHandle& handle)
	{
		bool result = false;

		DrawAssetSelectionDropdown(type, handle, [&](AssetHandle newHandle)
		{
			handle = newHandle;
			result = true;
		});

		return result;
	}

	AssetHandle ContentBrowserAssetDragDropTarget(AssetType type)
	{
		AssetHandle handle = 0;

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
			{
				const wchar_t* path = (const wchar_t*)payload->Data;
				std::filesystem::path assetPath(path);

				if (AssetManager::DoesAssetExist(assetPath))
				{
					const AssetMetadata& metadata = AssetManager::GetMetadata(assetPath);
					if (type == AssetType::None || metadata.Type == type)
						handle = metadata.Handle;
				}
			}
			ImGui::EndDragDropTarget();
		}

		return handle;
	}

	bool ContentBrowserAssetDragDropTarget(AssetType type, AssetHandle& handle)
	{
		bool result = false;

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
			{
				const wchar_t* path = (const wchar_t*)payload->Data;
				std::filesystem::path assetPath(path);

				if (AssetManager::DoesAssetExist(assetPath))
				{
					const AssetMetadata& metadata = AssetManager::GetMetadata(assetPath);
					if (type == AssetType::None || metadata.Type == type)
					{
						handle = metadata.Handle;
						result = true;
					}
				}
			}

			ImGui::EndDragDropTarget();
		}

		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem("Clear"))
			{
				handle = 0;
				result = true;
			}

			ImGui::EndPopup();
		}

		return result;
	}

	void DrawPhysicsLayerSelectionDropdown(uint64_t& physicsLayerID)
	{
		ImGui::Text(FA_SQUARE " Layer");
		ImGui::SameLine();

		const auto& layers = Project::GetActiveConfig().PhysicsSettings.Layers;
		if (ImGui::BeginCombo("##PhysicsLayerCombo", layers.find(physicsLayerID) != layers.end() ? layers.at(physicsLayerID).Name.c_str() : "Unknown Physics Layer"))
		{
			for (const auto& [id, layer] : layers)
			{
				ImGui::PushID(id);

				if (ImGui::MenuItem(layer.Name.c_str()))
					physicsLayerID = id;

				ImGui::PopID();
			}

			ImGui::EndCombo();
		}
	}

	static std::string s_EntitySearchBuffer;
	static ImGuiID s_PickingInputID = -1;
	static uint64_t s_PickingID = 0;

	uint64_t& GetEntityPickingID()
	{
		return s_PickingID;
	}

	bool DrawEntitySelectionInput(const char* str_id, uint64_t& uuid)
	{
		const ImGuiID currentPickingInputID = ImGui::GetID(str_id);
		ImGui::PushID(str_id);

		const Ref<Scene> context = s_EditorContext->GetActiveScene();

		bool returnValue = false;

		const bool typing = ImGui::GetActiveID() == ImGui::GetCurrentWindow()->GetID("##EntityFieldNameInput");
		const bool active = typing || ImGui::IsPopupOpen("##EntityFieldSearchPopup");

		if (!active && context->DoesEntityExist(uuid))
			s_EntitySearchBuffer = context->GetEntityByUUID(uuid).GetName();
		else if (!s_EntitySearchBuffer.empty() && (uuid == 0 || !context->DoesEntityExist(uuid)))
			s_EntitySearchBuffer.clear();

		ImGui::SetNextItemWidth(-90.0f);
		ImGui::InputTextWithHint("##EntityFieldNameInput", "None", &s_EntitySearchBuffer, ImGuiInputTextFlags_AutoSelectAll);

		bool popupFocused = false;

		if (active)
		{
			const ImVec2 pos = { ImGui::GetItemRectMin().x, ImGui::GetItemRectMax().y };

			ImGui::OpenPopup("##EntityFieldSearchPopup");
			if (ImGui::BeginPopup("##EntityFieldSearchPopup", ImGuiWindowFlags_NoFocusOnAppearing))
			{
				ImGui::SetWindowPos(pos);
				popupFocused = ImGui::IsWindowHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Left);

				bool found = false;

				auto view = context->GetRegistry().view<IDComponent, TagComponent>();
				for (auto& e : view)
				{
					auto& [id, tag] = view.get<IDComponent, TagComponent>(e);

					if (String::ToLower(tag.Tag).find(String::ToLower(s_EntitySearchBuffer)) != std::string::npos)
					{
						found = true;

						if (ImGui::MenuItem(tag.Tag.c_str()))
						{
							uuid = id.ID;
							returnValue = true;
						}
					}
				}

				if (!found)
					ImGui::TextDisabled("No Entities Found");

				if (!typing && !ImGui::IsWindowFocused(ImGuiFocusedFlags_None) && !ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
					ImGui::CloseCurrentPopup();

				ImGui::EndPopup();
			}
		}

		if (GImGui->ActiveId != ImGui::GetCurrentWindow()->GetID("##EntityFieldNameInput") && GImGui->ActiveIdPreviousFrame == ImGui::GetCurrentWindow()->GetID("##EntityFieldNameInput") && !popupFocused)
		{
			if (Entity newEntity = context->FindEntityByName(s_EntitySearchBuffer))
			{
				uuid = newEntity.GetUUID();
				returnValue = true;
			}

			ImGui::IsWindowHovered();
		}

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCENE_HIERARCHY_ENTITY"))
			{
				Entity droppedEntity = *(Entity*)payload->Data;
				uuid = droppedEntity.GetUUID();

				returnValue = true;
			}
			ImGui::EndDragDropTarget();
		}

		{
			ImGui::SameLine();

			const float lineHeight = ImGui::GetTextLineHeight();
			if (UI::DrawTextIconButton(FA_EYE_DROPPER))
			{
				s_PickingID = 1; // We are now picking.
				s_PickingInputID = currentPickingInputID;
			}

			ImGui::SameLine();

			if (UI::DrawTextIconButton(FA_CIRCLE_XMARK))
			{
				// Reset the object
				uuid = 0;
				returnValue = true;
				s_PickingID = 0;
			}
			
			if (s_PickingInputID == currentPickingInputID)
			{
				if (s_PickingID > 1)
				{
					// We've picked an object
					uuid = s_PickingID;
					returnValue = true;
				}
				else if (s_PickingID == 1)
				{
					// Draw picking cursor
					ImGui::SetMouseCursor(ImGuiMouseCursor_None);
					ImGui::GetForegroundDrawList()->AddImage((ImTextureID)EditorResources::IconPicker->GetRendererID(), ImGui::GetMousePos() + ImVec2(0.0f, -16.0f), ImGui::GetMousePos() + ImVec2(16.0f, 0.0f), { 0, 1 }, { 1, 0 });
				}
			}	
		}

		ImGui::PopID();

		return returnValue;
	}

	bool DrawEntitySelectionInputLabeled(const char* label, uint64_t& uuid)
	{
		UI::Text(label);
		ImGui::SameLine();
		return DrawEntitySelectionInput(label, uuid);
	}

	void DrawWindowInnerShadows(const ImVec4& color, const float thickness)
	{
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		const ImU32& minColor = ImGui::ColorConvertFloat4ToU32(ImVec4(color.x, color.y, color.z, 0.0f));
		const ImU32& maxColor = ImGui::ColorConvertFloat4ToU32(ImVec4(color.x, color.y, color.z, 0.25f));
		drawList->AddRectFilledMultiColor(ImGui::GetWindowPos(), ImGui::GetWindowPos() + ImVec2(thickness, ImGui::GetWindowSize().y), maxColor, minColor, minColor, maxColor);
		drawList->AddRectFilledMultiColor(ImGui::GetWindowPos(), ImGui::GetWindowPos() + ImVec2(ImGui::GetWindowSize().x, thickness), maxColor, maxColor, minColor, minColor);
		drawList->AddRectFilledMultiColor(ImGui::GetWindowPos() + ImGui::GetWindowSize(), ImGui::GetWindowPos() + ImGui::GetWindowSize() - ImVec2(thickness, ImGui::GetWindowSize().y), maxColor, minColor, minColor, maxColor);
		drawList->AddRectFilledMultiColor(ImGui::GetWindowPos() + ImGui::GetWindowSize(), ImGui::GetWindowPos() + ImGui::GetWindowSize() - ImVec2(ImGui::GetWindowSize().x, thickness), maxColor, maxColor, minColor, minColor);
	}

	bool IsHoveredTooltipTimer()
	{
		return ImGui::GetCurrentContext()->HoveredIdNotActiveTimer >= Preferences::GetData().TooltipHoverDelay * 0.001f;
	}

	bool IsItemHoveredTooltip()
	{
		return ImGui::IsItemHovered() && IsHoveredTooltipTimer();
	}

	void DrawViewportRenderSettingsButton(SceneRendererContext::RendererVisualizationMode& rendererVisualizationMode)
	{
		const char* visualizationModeNames[] =
		{
			CHARACTER_ICON_SHADING_RENDERED "Rendered",
			CHARACTER_ICON_SHADING_WIREFRAME "Wireframe",
			CHARACTER_ICON_SHADING_SOLID "Lighting Only",
			CHARACTER_ICON_SHADING_UNLIT "Pre Post Processing",
			FA_LIGHTS_HOLIDAY "Path Traced",
			CHARACTER_ICON_MEMORY "Albedo",
			CHARACTER_ICON_MEMORY "Depth",
			CHARACTER_ICON_MEMORY "Depth",
			CHARACTER_ICON_MEMORY "Position",
			CHARACTER_ICON_MEMORY "Normal",
			CHARACTER_ICON_MEMORY "Emissive",
			CHARACTER_ICON_MEMORY "Roughness",
			CHARACTER_ICON_MEMORY "Metallic",
			CHARACTER_ICON_MEMORY "Specular",
			CHARACTER_ICON_MEMORY "Ambient Occlusion",
			CHARACTER_ICON_MEMORY "Velocity",
			CHARACTER_ICON_MEMORY "Object ID",
			CHARACTER_ICON_MEMORY "Submesh Index"
		};

		ImGui::SetNextItemWidth(150.0f + ImGui::GetStyle().FramePadding.x * 2.0f + 10.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.0f, 5.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.5f, 7.5f));
		bool open = ImGui::BeginCombo("##ViewportRenderSettings", visualizationModeNames[(int)rendererVisualizationMode], ImGuiComboFlags_HeightLargest);
		ImGui::PopStyleVar();
		if (open)
		{
			ImGui::TextDisabled("View Modes");
			if (ImGui::MenuItem(CHARACTER_ICON_SHADING_RENDERED " Rendered"))			rendererVisualizationMode = SceneRendererContext::RendererVisualizationMode::Rendered;
			if (ImGui::MenuItem(CHARACTER_ICON_SHADING_WIREFRAME " Wireframe"))			rendererVisualizationMode = SceneRendererContext::RendererVisualizationMode::Wireframe;
			if (ImGui::MenuItem(CHARACTER_ICON_SHADING_SOLID " Lighting Only"))			rendererVisualizationMode = SceneRendererContext::RendererVisualizationMode::LightingOnly;
			if (ImGui::MenuItem(CHARACTER_ICON_SHADING_UNLIT " Pre Post Processing"))	rendererVisualizationMode = SceneRendererContext::RendererVisualizationMode::PrePostProcessing;
			if (ImGui::MenuItem(FA_LIGHTS_HOLIDAY " Path Traced"))						rendererVisualizationMode = SceneRendererContext::RendererVisualizationMode::PathTraced;

			ImGui::Separator();

			ImGui::TextDisabled("Buffer Visualization");
			if (ImGui::MenuItem(CHARACTER_ICON_MEMORY " Albedo"))				rendererVisualizationMode = SceneRendererContext::RendererVisualizationMode::Albedo;
			if (ImGui::MenuItem(CHARACTER_ICON_MEMORY " Depth"))				rendererVisualizationMode = SceneRendererContext::RendererVisualizationMode::LinearDepth;
			if (ImGui::MenuItem(CHARACTER_ICON_MEMORY " Normal"))				rendererVisualizationMode = SceneRendererContext::RendererVisualizationMode::Normal;
			if (ImGui::MenuItem(CHARACTER_ICON_MEMORY " Emissive"))				rendererVisualizationMode = SceneRendererContext::RendererVisualizationMode::Emissive;
			if (ImGui::MenuItem(CHARACTER_ICON_MEMORY " Roughness"))			rendererVisualizationMode = SceneRendererContext::RendererVisualizationMode::Roughness;
			if (ImGui::MenuItem(CHARACTER_ICON_MEMORY " Metallic"))				rendererVisualizationMode = SceneRendererContext::RendererVisualizationMode::Metallic;
			if (ImGui::MenuItem(CHARACTER_ICON_MEMORY " Specular"))				rendererVisualizationMode = SceneRendererContext::RendererVisualizationMode::Specular;
			if (ImGui::MenuItem(CHARACTER_ICON_MEMORY " Ambient Occlusion"))	rendererVisualizationMode = SceneRendererContext::RendererVisualizationMode::AmbientOcclusion;
			if (ImGui::MenuItem(CHARACTER_ICON_MEMORY " Velocity"))				rendererVisualizationMode = SceneRendererContext::RendererVisualizationMode::Velocity;
			if (ImGui::MenuItem(CHARACTER_ICON_MEMORY " Object ID"))			rendererVisualizationMode = SceneRendererContext::RendererVisualizationMode::EntityID;
			if (ImGui::MenuItem(CHARACTER_ICON_MEMORY " Submesh Index"))		rendererVisualizationMode = SceneRendererContext::RendererVisualizationMode::SubmeshIndex;

			ImGui::EndCombo();
		}
		ImGui::PopStyleVar();
	}

	void DrawCameraSpeedButton(EditorCamera& editorCamera, float& cameraBaseSpeed, int& cameraSpeedScale)
	{
		if (ImGui::Button((CHARACTER_ICON_CAMERA + std::to_string(cameraSpeedScale)).c_str(), ImVec2(50, 30)))
			ImGui::OpenPopup("##CameraSpeedPopup");

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.0f, 5.0f));
		if (ImGui::BeginPopup("##CameraSpeedPopup", ImGuiWindowFlags_NoMove))
		{
			ImGui::Text(FA_STOPWATCH " Speed Scale");
			if (ImGui::SliderInt("##CameraSpeedScaleInput", &cameraSpeedScale, 1, 8))
				editorCamera.SetMoveSpeed(cameraBaseSpeed * cameraSpeedScale);

			ImGui::Separator();

			ImGui::Text(FA_GAUGE_HIGH " Base Speed (m/s)");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());
			if (ImGui::DragFloat("##CameraBaseSpeedInput", &cameraBaseSpeed, 0.1f, 0.001f))
			{
				if (cameraBaseSpeed < 0.001f)
					cameraBaseSpeed = 0.001f;
				editorCamera.SetMoveSpeed(cameraBaseSpeed * cameraSpeedScale);
			}

			ImGui::Text(FA_HOURGLASS_HALF " Smoothing Time");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());
			float smoothingTime = editorCamera.GetSmoothingTime();
			if (ImGui::DragFloat("##CameraSmoothingTimeInput", &smoothingTime, 0.025f, 0.0f, 1.0f))
			{
				if (smoothingTime < 0.0f)
					smoothingTime = 0.0f;
				editorCamera.SetSmoothingTime(smoothingTime);
			}

			if (ImGui::BeginMenu(FA_VIDEO " Camera Type"))
			{
				if (ImGui::MenuItem(FA_EYE " First Person"))
				{
					editorCamera.SetOrbitalEnabled(false);
					editorCamera.SetFirstPersonEnabled(true);
				}
				if (ImGui::MenuItem(FA_BULLSEYE " Orbital"))
				{
					editorCamera.SetOrbitalEnabled(true);
					editorCamera.SetFirstPersonEnabled(false);
				}
				if (ImGui::MenuItem(FA_CIRCLE_HALF_STROKE " Hybrid"))
				{
					editorCamera.SetOrbitalEnabled(true);
					editorCamera.SetFirstPersonEnabled(true);
				}

				ImGui::EndMenu();
			}

			ImGui::Separator();

			if (ImGui::MenuItem(FA_UNDO " Reset to Origin"))
				editorCamera.SetTransform(EditorCamera::EditorCameraTransform());

			ImGui::EndPopup();
		}
		ImGui::PopStyleVar();
	}

	bool DrawVec3Control(const char* label, glm::vec3& values, glm::vec3 resetValue, float columnWidth)
	{
		bool result = false;

		const bool drawLabel = columnWidth != -1.0f;
		ImGui::PushID(label);

		if (drawLabel)
		{
			ImGui::Columns(2, nullptr, false);
			ImGui::SetColumnWidth(0, columnWidth);
			ImGui::Text(label);
			ImGui::NextColumn();
		}

		ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

		const auto& style = ImGui::GetStyle();

		float lineHeight = GImGui->Font->FontSize + style.FramePadding.y * 2.0f;
		ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };
		float inputWidth = (ImGui::GetContentRegionAvailWidth() / 3.0f) - buttonSize.x - style.FramePadding.x * 2.0f;

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
		UI::PushFont(FontType::Bold);
		ImGui::PushItemFlag(ImGuiItemFlags_NoNav, true);

		if (ImGui::ButtonCornersEx("X", buttonSize, ImGuiButtonFlags_NoNavFocus, ImDrawFlags_RoundCornersLeft))
		{
			values.x = resetValue.x;
			result = true;
		}

		ImGui::PopItemFlag();
		UI::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::SetNextItemWidth(inputWidth);

		if (ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f"))
			result = true;

		ImGui::PopItemWidth();
		ImGui::SameLine();
		ImGui::Dummy(ImVec2{ style.FramePadding.x, 0.0f });
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		UI::PushFont(FontType::Bold);
		ImGui::PushItemFlag(ImGuiItemFlags_NoNav, true);

		if (ImGui::ButtonCornersEx("Y", buttonSize, ImGuiButtonFlags_NoNavFocus, ImDrawFlags_RoundCornersLeft))
		{
			values.y = resetValue.y;
			result = true;
		}

		ImGui::PopItemFlag();
		UI::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::SetNextItemWidth(inputWidth);

		if (ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f"))
			result = true;

		ImGui::PopItemWidth();
		ImGui::SameLine();
		ImGui::Dummy(ImVec2{ style.FramePadding.x, 0.0f });
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
		UI::PushFont(FontType::Bold);
		ImGui::PushItemFlag(ImGuiItemFlags_NoNav, true);

		if (ImGui::ButtonCornersEx("Z", buttonSize, ImGuiButtonFlags_NoNavFocus, ImDrawFlags_RoundCornersLeft))
		{
			values.z = resetValue.z;
			result = true;
		}

		ImGui::PopItemFlag();
		UI::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::SetNextItemWidth(inputWidth);

		if (ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f"))
			result = true;

		ImGui::PopItemWidth();

		ImGui::PopStyleVar();

		if (drawLabel)
			ImGui::Columns(1);

		ImGui::PopID();

		return result;
	}

	static bool IsMat4Nan(glm::mat4 matrix)
	{
		for (int i = 0; i < 4; ++i)
			for (int j = 0; j < 4; ++j)
				if (std::isnan(matrix[i][j]))
					return true;

		return false;
	}

	void DrawTransformControl(const char* str_id, Transform& value)
	{
		// Draw UI elements (with appropriate reset values)
		ImGui::PushID(str_id);
		ImGui::BeginGroup();
		UI::DrawVec3Control("Translation", value.Translation, glm::vec3(0.0f), -1.0f);

		glm::vec3 rotation = value.GetRotationDegrees();
		if (UI::DrawVec3Control("Rotation", rotation, glm::vec3(0.0f), -1.0f))
			value.SetRotationDegrees(rotation);

		UI::DrawVec3Control("Scale", value.Scale, glm::vec3(1.0f), -1.0f);
		ImGui::EndGroup();
		ImGui::PopID();
	}

	bool CollapsingHeader(const char* label, const bool defaultOpen)
	{
		const ImGuiTreeNodeFlags treeNodeFlags = (defaultOpen ? ImGuiTreeNodeFlags_DefaultOpen : 0) | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
		
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));
		bool open = ImGui::TreeNodeEx(label, treeNodeFlags);
		ImGui::PopStyleVar();

		return open;
	}

	ImGuiID BeginDockspaceWindow(const char* name, bool* open, ImGuiWindowFlags windowFlags, ImGuiWindowClass* windowClass)
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2());
		ImGui::Begin(name, open, windowFlags);
		ImGui::PopStyleVar();

		ImGuiStyle& style = ImGui::GetStyle();
		float minWinSizeX = style.WindowMinSize.x;
		style.WindowMinSize.x = 150.0f;
		ImGuiID dockspaceID = ImGui::GetID("##NodeDockSpace");
		ImGui::DockSpace(dockspaceID, ImVec2(0.0f, 0.0f), 0, windowClass);
		style.WindowMinSize.x = minWinSizeX;

		return dockspaceID;
	}

	ImGuiWindowClass CreateDockingRestrictionClass(const char* className)
	{
		ImGuiWindowClass windowClass;
		windowClass.ClassId = ImGui::GetID(className);
		windowClass.DockingAllowUnclassed = false;
		return windowClass;
	}

	void CenterAppearingWindow(const ImVec2& size)
	{
		ImGui::SetNextWindowSize(size, ImGuiCond_Appearing);
		ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	}

	void CenterAppearingWindow(const float screenRatio)
	{
		CenterAppearingWindow(GetScreenSizeRatio(screenRatio));
	}

	glm::vec2 GetScreenSizeRatio(const float screenRatio)
	{
		return screenRatio * glm::vec2(Application::Get().GetWindow().GetWidth(), Application::Get().GetWindow().GetHeight());
	}

	void PostUIUpdate()
	{
		// Reset Entity Picker after use
		if ((Input::IsKeyPressed(Key::Escape) || Input::IsMouseButtonPressed(Mouse::ButtonLeft)) && s_PickingID != 0)
		{
			s_PickingID = 0;
			s_PickingInputID = -1;
		}
	}

	void SetEditorContext(EditorLayer* context)
	{
		s_EditorContext = context;
	}

	bool SelectableTreeNode(ImGuiID id, const char* text, ImGuiTreeNodeFlags treeFlags, bool selected, ImGuiSelectableFlags selectableFlags)
	{
		ImGui::PushStyleColor(ImGuiCol_Header, {});
		ImGui::PushStyleColor(ImGuiCol_HeaderActive, {});
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, {});
		const bool returnValue = ImGui::TreeNodeBehavior(id, treeFlags, "");
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::Selectable(text, selected, selectableFlags, ImVec2(ImGui::GetContentRegionAvailWidth(), 0.0f));

		return returnValue;
	}

	void Text(const char* text)
	{
		ImGui::TextEx(text, ImGui::FindRenderedTextEnd(text), ImGuiTextFlags_NoWidthForLargeClippedText);
	}

	void Image(const Ref<Texture2D> texture, const ImVec2& size)
	{
		ImGui::Image((ImTextureID)texture->GetRendererID(), size, { 0, 1 }, { 1, 0 });
	}

#define EXECUTE_WITH_LABEL(widgetCall)	\
	ImGui::PushID(label);				\
	UI::Text(label);					\
	ImGui::SameLine();					\
	const bool result = (widgetCall);	\
	ImGui::PopID();						\
	return result;

	bool Checkbox(const char* label, bool* v)
	{
		EXECUTE_WITH_LABEL(ImGui::Checkbox("##Checkbox", v));
	}

	static bool DragScalar(const char* label, ImGuiDataType data_type, void* v, float v_speed, const void* v_min, const void* v_max, const char* format, ImGuiSliderFlags flags)
	{
		EXECUTE_WITH_LABEL(ImGui::DragScalar("##DragScalar", data_type, v, v_speed, v_min, v_max, format, flags | ImGuiSliderFlags_AlwaysClamp));
	}

	bool DragFloat(const char* label, float* v, float v_speed, float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
	{
		return DragScalar(label, ImGuiDataType_Float, v, v_speed, &v_min, &v_max, format, flags);
	}

	static bool DragFloatN(const char* label, float* v, int components, float v_speed, float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
	{
		EXECUTE_WITH_LABEL(ImGui::DragScalarN("##DragFloatN", ImGuiDataType_Float, v, components, v_speed, &v_min, &v_max, format, flags));
	}

	bool Combo(const char* label, int* current_item, const char* const items[], int items_count, int popup_max_height_in_items)
	{
		EXECUTE_WITH_LABEL(ImGui::Combo("##Combo", current_item, items, items_count, popup_max_height_in_items));
	}

	bool ColorEdit3(const char* label, float col[3], ImGuiColorEditFlags flags)
	{
		EXECUTE_WITH_LABEL(ImGui::ColorEdit3("##ColorEdit3", col, flags));
	}

	bool ColorEdit4(const char* label, float col[4], ImGuiColorEditFlags flags)
	{
		EXECUTE_WITH_LABEL(ImGui::ColorEdit4("##ColorEdit4", col, flags));
	}

	bool DragFloat2(const char* label, float v[2], float v_speed, float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
	{
		return DragFloatN(label, v, 2, v_speed, v_min, v_max, format, flags);
	}

	bool DragFloat3(const char* label, float v[3], float v_speed, float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
	{
		return DragFloatN(label, v, 3, v_speed, v_min, v_max, format, flags);
	}

	bool DragFloat4(const char* label, float v[4], float v_speed, float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
	{
		return DragFloatN(label, v, 4, v_speed, v_min, v_max, format, flags);
	}

	bool SliderFloat(const char* label, float* v, float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
	{
		EXECUTE_WITH_LABEL(ImGui::SliderFloat("##SliderFloat", v, v_min, v_max, format, flags));
	}

	bool DragU32(const char* label, uint32_t* v, float v_speed, int v_max, const char* format, ImGuiSliderFlags flags)
	{
		return DragScalar(label, ImGuiDataType_U32, v, v_speed, nullptr, nullptr, format, flags);
	}


}