#include "SceneHierarchyPanel.h"

#include "EditorResources.h"

#include "Dymatic/Scene/Components.h"
#include "Dymatic/Scripting/ScriptEngine.h"
#include "Dymatic/Asset/AssetManager.h"

#include "UI.h"
#include "Fonts.h"
#include "Settings/Preferences.h"
#include "TextSymbols.h"

#include "Dymatic/Utils/PlatformUtils.h"

#include "Dymatic/Math/StringUtils.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

#include <glm/gtc/type_ptr.hpp>

#include <cstring>

/* The Microsoft C++ compiler is non-compliant with the C++ standard and needs
 * the following definition to disable a security warning on std::strncpy().
 */
#ifdef _MSVC_LANG
#define _CRT_SECURE_NO_WARNINGS
#endif

namespace Dymatic {

	static constexpr ImVec4 EntityColor = ImVec4(0.825f, 0.55f, 0.352f, 1.0f);
	static constexpr ImVec4 PrefabColor = ImVec4(0.32f, 0.70f, 0.87f, 1.0f);
	static constexpr ImVec4 InvalidPrefabColor = ImVec4(0.87f, 0.17f, 0.17f, 1.0f);
	static constexpr ImColor BoneColor = ImColor(168, 212, 236);

	SceneHierarchyPanel::SceneHierarchyPanel()
	{
	}
	
	SceneHierarchyPanel::SceneHierarchyPanel(const Ref<Scene>& context)
	{
		SetContext(context);
	}

	void SceneHierarchyPanel::SetContext(const Ref<Scene>& context)
	{
		m_Context = context;
		m_ActiveEntity = {};
	}

	void SceneHierarchyPanel::OnImGuiRender()
	{
		// Check shortcut to open create window
		if (m_ShowCreateMenu)
		{
			ImGui::OpenPopup(ImGui::GetID("##CreateEntityPopup"));
			m_ShowCreateMenu = false;
		}

		// Create Entity Context Popup
		if (ImGui::BeginPopupEx(ImGui::GetID("##CreateEntityPopup"), ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings))
		{
			DisplayCreateEntityPopup();
			ImGui::EndPopup();
		}

		// Scene Settings Panel
		if (auto& sceneSettingsVisible = Preferences::GetEditorWindowVisible(Preferences::EditorWindow::SceneSettings))
		{
			ImGui::Begin(FILE_ICON_SCENE " Scene Settings", &sceneSettingsVisible);

			if (ImGui::CollapsingHeader(FA_ATOM_SIMPLE " Physics", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::Indent();

				UI::DrawVec3Control(FA_PERSON_FALLING " Gravity", m_Context->m_Gravity, glm::vec3(0.0f, -9.81f, 0.0f));
				
				ImGui::Unindent();
			}

			ImGui::End();
		}

		// Scene Hierarchy Panel
		if (auto& sceneHierarchyVisible = Preferences::GetEditorWindowVisible(Preferences::EditorWindow::SceneHierarchy))
		{
			ImGui::Begin(CHARACTER_ICON_SCENE_HIERARCHY " Scene Hierarchy", &sceneHierarchyVisible);
			auto& style = ImGui::GetStyle();

			// Search bar
			{
				ImGui::BeginGroup();

				// Draw BK
				{
					auto window = ImGui::GetCurrentWindow();
					ImVec2 frame_size = ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetFontSize() + style.FramePadding.y * 2.0f);
					const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + frame_size);
					ImGui::RenderFrame(frame_bb.Min, frame_bb.Max, ImGui::GetColorU32(ImGuiCol_FrameBg), true, style.FrameRounding);
				}

				auto size = ImVec2(ImGui::GetFontSize(), ImGui::GetFontSize());
				ImGui::SameLine();
				ImGui::BeginGroup();
				ImGui::Dummy({ 0.0f, style.FramePadding.y * 0.25f });
				ImGui::Image((ImTextureID)EditorResources::SearchbarIcon->GetRendererID(), size, { 0, 1 }, { 1, 0 }, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
				ImGui::EndGroup();
				ImGui::SameLine();

				ImGui::PushStyleColor(ImGuiCol_FrameBg, {});
				ImGui::PushStyleColor(ImGuiCol_FrameBgActive, {});
				ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, {});
				char buffer[256];
				memset(buffer, 0, sizeof(buffer));
				std::strncpy(buffer, m_SearchBuffer.c_str(), sizeof(buffer));
				ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth() - size.x - style.FramePadding.x * 3.0f);
				if (ImGui::InputTextWithHint("##SceneHierarchySearchbar", "Search...", buffer, sizeof(buffer)))
					m_SearchBuffer = std::string(buffer);
				ImGui::PopStyleColor(3);

				if (!m_SearchBuffer.empty())
				{
					ImGui::SameLine();
					ImGui::BeginGroup();
					ImGui::PushStyleColor(ImGuiCol_Button, {});
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, {});
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {});
					ImGui::Dummy({ 0.0f, style.FramePadding.y * 0.25f });
					auto& cpos = ImGui::GetCursorPos();
					if (ImGui::Button("##ClearSearchbarButton", size))
						m_SearchBuffer.clear();
					ImGui::SetCursorPos(cpos);
					ImGui::PopStyleColor(3);

					ImVec4 color = ImGui::GetStyleColorVec4(ImGui::IsItemActive() ? ImGuiCol_HeaderActive : (ImGui::IsItemHovered() ? ImGuiCol_HeaderHovered : ImGuiCol_Text));
					ImGui::PushStyleColor(ImGuiCol_Button, {});
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, {});
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {});
					ImGui::Image((ImTextureID)EditorResources::ClearIcon->GetRendererID(), size, { 0, 1 }, { 1, 0 }, color);
					ImGui::PopStyleColor(3);
					ImGui::EndGroup();
				}

				ImGui::EndGroup();
			}

			// Main Hierarchy List
			ImGui::BeginChild("##SceneHierarchyList");

			// Push Style Flags
			ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, {});
			ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2{});
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 3, 3 });

			// Setup Table
			const ImGuiTableFlags flags = ImGuiTableFlags_PadOuterX | ImGuiTableFlags_Resizable | ImGuiTableFlags_NoBordersInBody | ImGuiTableFlags_NoPadInnerX;
			if (ImGui::BeginTable("##SceneHierarchyTable", 3, flags))
			{
				ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_NoHide);
				ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 64.0f);
				ImGui::TableSetupColumn("Modifiers", ImGuiTableColumnFlags_WidthFixed, 80.0f);
				ImGui::TableHeadersRow();

				m_OnComplete = nullptr;

				// Display Main Items
				if (m_Context)
				{
					m_Context->m_Registry.each([&](auto entityID)
					{
						Entity entity{ entityID , m_Context.get() };

						if (!entity.HasComponent<SceneComponent>() && !entity.HasParent())
							DrawEntityNode(entity);
					});
				}

				if (m_OnComplete)
					m_OnComplete();

				if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
					ClearSelection();

				// Create Entity Context Popup
				if (ImGui::BeginPopupContextWindow(0, ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
				{
					DisplayCreateEntityPopup();
					ImGui::EndPopup();
				}

				// End Table
				ImGui::EndTable();
			}

			// Pop Table Styles
			ImGui::PopStyleVar(2);
			ImGui::PopStyleColor();

			ImGui::EndChild();

			ImGui::End();
		}

		// Properties Panel
		if (auto& propertiesVisible = Preferences::GetEditorWindowVisible(Preferences::EditorWindow::Properties))
		{
			ImGui::Begin(CHARACTER_ICON_PROPERTIES " Properties", &propertiesVisible);
			if (m_ActiveEntity)
			{
				DrawComponents(m_ActiveEntity);
			}

			ImGui::End();
		}
	}

	void SceneHierarchyPanel::SelectedEntity(Entity entity)
	{
		if (!IsEntitySelectable(entity))
			return;

		m_ActiveEntity = entity;

		if (Input::IsKeyPressed(Key::LeftShift) || Input::IsKeyPressed(Key::RightShift))
			m_Context->AddSelectedEntity(entity);
		else
			m_Context->SetSelectedEntity(entity);
	}

	void SceneHierarchyPanel::ClearSelection()
	{
		m_ActiveEntity = {};
		m_Context->ClearSelectedEntities();
	}

	void SceneHierarchyPanel::DuplicateEntities()
	{
		auto& entities = GetSelectedEntities();
		for (auto& entity : entities)
			m_Context->DuplicateEntity({ entity, m_Context.get() });
	}

	void SceneHierarchyPanel::DeleteEntity(Entity entity)
	{
		m_Context->DestroyEntity(entity);
		
		if (m_ActiveEntity == entity)
			m_ActiveEntity = {};
		
		m_Context->RemoveSelectedEntity(entity);
	}

	void SceneHierarchyPanel::DeleteEntities()
	{
		for (auto entity : m_Context->GetSelectedEntities())
			m_Context->DestroyEntity({ entity, m_Context.get() });

		ClearSelection();
	}

	void SceneHierarchyPanel::UpdatePrefab(Entity entity)
	{
		if (!entity.HasComponent<PrefabComponent>())
			return;

		auto& pc = entity.GetComponent<PrefabComponent>();
		const Ref<Prefab> prefab = AssetManager::GetAsset<Prefab>(pc.PrefabID);
		if (!prefab)
			return;
		
		prefab->UpdateRoot(entity);
		AssetManager::SerializeAsset(prefab);
	}

	void SceneHierarchyPanel::RevertPrefab(Entity entity)
	{
		m_Context->RevertPrefab(entity);
		ClearSelection();
	}

	void SceneHierarchyPanel::UnlinkPrefab(Entity entity)
	{
		if (entity.HasComponent<PrefabComponent>())
			entity.RemoveComponent<PrefabComponent>();
	}

	bool SceneHierarchyPanel::IsNodeSearchable(const std::string& nodeName)
	{
		if (m_SearchBuffer.empty())
			return true;

		std::string name = nodeName;
		std::string search = m_SearchBuffer;
		String::TransformLower(name);
		String::TransformLower(search);

		return name.find(search) != std::string::npos;
	}

	void SceneHierarchyPanel::DrawEntityNode(Entity entity)
	{
		if (!IsNodeSearchable(entity.GetName()))
			return;

		ImGui::PushID(entity.GetUUID());

		// Setup Row
		ImGui::TableNextRow();
		ImGui::TableNextColumn();

		const bool isPrefab = entity.HasComponent<PrefabComponent>();
		const bool isValidPrefab = isPrefab && AssetManager::DoesAssetExist(entity.GetComponent<PrefabComponent>().PrefabID);

		const bool isFolder = entity.HasComponent<FolderComponent>();
		if (isFolder)
		{
			auto& color = entity.GetComponent<FolderComponent>().Color;
			ImGui::TextColored({ color.r, color.g, color.b, 1.0f }, FA_FOLDER);
		}
		else if (isPrefab)
			ImGui::TextColored(isValidPrefab ? PrefabColor : InvalidPrefabColor, FILE_ICON_PREFAB);
		else
			ImGui::TextColored(EntityColor, CHARACTER_ICON_EMPTY);

		ImGui::SameLine();
		ImGui::Selectable("##EntitySelectable", m_Context->IsEntitySelected(entity), ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap);

		if (ImGui::IsItemClicked())
			SelectedEntity(entity);

		bool openNode = false;
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCENE_HIERARCHY_ENTITY"))
			{
				openNode = true;

				// Setup Parenting
				Entity droppedEntity = *(Entity*)payload->Data;
				m_Context->ParentEntity(droppedEntity, entity);
			}
			ImGui::EndDragDropTarget();
		}

		if (ImGui::BeginDragDropSource())
		{
			ImGui::Text(entity.GetName().c_str());
			ImGui::SetDragDropPayload("SCENE_HIERARCHY_ENTITY", &entity, sizeof(Entity));
			ImGui::EndDragDropSource();
		}

		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem(CHARACTER_ICON_DELETE " Delete"))
				m_OnComplete = [=]() { DeleteEntity(entity); };
			if (ImGui::MenuItem(CHARACTER_ICON_DUPLICATE " Duplicate"))
				DuplicateEntities();

			if (entity.HasParent() && ImGui::MenuItem(FA_HANDS_HOLDING_CHILD " Unparent"))
				m_Context->UnparentEntity(entity);

			ImGui::Separator();

			if (!isFolder)
			{
				if (isPrefab)
				{
					if (ImGui::MenuItem(FILE_ICON_PREFAB " Update Prefab"))
						m_OnComplete = [=]() { UpdatePrefab(entity); };
					if (ImGui::MenuItem(FA_UNDO " Revert Prefab"))
						m_OnComplete = [=]() { RevertPrefab(entity); };
					if (ImGui::MenuItem(FA_LINK_SLASH " Unlink Prefab"))
						m_OnComplete = [=]() { UnlinkPrefab(entity); };
				}
				else
				{
					if (ImGui::MenuItem(FILE_ICON_PREFAB " Create Prefab"))
						;
				}
			}	

			ImGui::EndPopup();
		}

		// Open TreeNode (if required)
		if (openNode)
			ImGui::SetNextItemOpen(openNode);

		// Draw the node
		ImGui::SameLine();
		ImGui::PushStyleColor(ImGuiCol_Header, {});
		ImGui::PushStyleColor(ImGuiCol_HeaderActive, {});
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, {});

		const bool showBones = (Preferences::GetData().BoneAttachmentEditMode == Preferences::BoneAttachmentEditMode::Hierarchy);
		const Ref<Model> model = (showBones && entity.HasComponent<StaticMeshComponent>()) ? entity.GetComponent<StaticMeshComponent>().GetModel() : nullptr;
		const Ref<Skeleton> skeleton = (showBones && model) ? model->GetSkeleton() : nullptr;

		const bool leaf = !entity.HasChildren() && !skeleton;
		
		const ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_AllowItemOverlap;
		const bool opened = ImGui::TreeNodeBehavior(ImGui::GetID((void*)(uint64_t)(uint32_t)entity), flags | (leaf ? ImGuiTreeNodeFlags_Leaf : 0), "");
		ImGui::PopStyleColor(3);

		// Draw the entity node name
		// Color all entities that are a prefab. This color also indicates if the prefab ID is valid or if it is broken/unlinked
		auto& tag = entity.GetComponent<TagComponent>().Tag;
		ImGui::SameLine();
		if (isPrefab)
			ImGui::TextColored(isValidPrefab ? PrefabColor : InvalidPrefabColor, tag.c_str());
		else
			ImGui::TextUnformatted(tag.c_str());

		// Type Section
		ImGui::TableNextColumn();
		ImGui::TextUnformatted(
			isPrefab ? "Prefab" :
			isFolder ? "Folder" :
			"Entity"
		);

		// Modifiers Section
		ImGui::TableNextColumn();

		ImGui::PushStyleColor(ImGuiCol_Button, {});
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {});
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, {});
		ImGui::ImageButton((ImTextureID)(true ? EditorResources::VisibleIcon : EditorResources::HiddenIcon)->GetRendererID(), { 15.0f, 15.0f }, { 0, 1 }, { 1, 0 }, -1, {}, ImGui::GetStyleColorVec4(ImGuiCol_Text));
		ImGui::SameLine();
		const bool isSelectable = IsEntitySelectable(entity);
		if (ImGui::ImageButton((ImTextureID)(isSelectable ? EditorResources::SelectableIcon : EditorResources::NonSelectableIcon)->GetRendererID(), { 15.0f, 15.0f }, { 0, 1 }, { 1, 0 }, -1, {}, ImGui::GetStyleColorVec4(isSelectable ? ImGuiCol_Text : ImGuiCol_TextDisabled)))
			ToggleEntitySelectable(entity);
		ImGui::SameLine();
		const bool isLocked = IsEntityLocked(entity);
		if (ImGui::ImageButton((ImTextureID)(isLocked ? EditorResources::LockedIcon : EditorResources::UnlockedIcon)->GetRendererID(), { 15.0f, 15.0f }, { 0, 1 }, { 1, 0 }, -1, {}, ImGui::GetStyleColorVec4(isLocked ? ImGuiCol_Text : ImGuiCol_TextDisabled)))
			ToggleEntityLocked(entity);
		ImGui::PopStyleColor(3);

		if (opened)
		{
			auto children = entity.GetChildren();

			// Draw bone nodes
			if (skeleton)
				DrawBoneNode(entity, children, skeleton->GetRootNode());

			// Draw unattached child nodes
			for (auto& child : children)
				if (!skeleton || !child.HasComponent<AttachmentComponent>() || !skeleton->IsValidBoneName(child.GetComponent<AttachmentComponent>().BoneName))
					DrawEntityNode(child);

			ImGui::TreePop();
		}

		ImGui::PopID();
	}

	void SceneHierarchyPanel::DrawBoneNode(Entity entity, const std::vector<Entity>& children, const BoneNodeData& node)
	{
		ImGui::PushID(node.Name.c_str());

		// Setup Row
		ImGui::TableNextRow();
		ImGui::TableNextColumn();

		ImGui::TextColored(BoneColor, FA_BONE);

		ImGui::SameLine();
		ImGui::Selectable("##BoneSelectable", false, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap);

		// Select the entity whose skeleton this belongs to
		if (ImGui::IsItemClicked())
			SelectedEntity(entity);

		bool openNode = false;

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCENE_HIERARCHY_ENTITY"))
			{
				openNode = true;

				// Setup Parenting
				Entity droppedEntity = *(Entity*)payload->Data;
				m_Context->ParentEntity(droppedEntity, entity, node.Name);
			}

			ImGui::EndDragDropTarget();
		}

		// Open TreeNode (if needed)
		if (openNode)
			ImGui::SetNextItemOpen(openNode);

		// Draw the tree node
		ImGui::SameLine();
		ImGui::PushStyleColor(ImGuiCol_Header, {});
		ImGui::PushStyleColor(ImGuiCol_HeaderActive, {});
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, {});

		const Ref<Model> model = entity.HasComponent<StaticMeshComponent>() ? entity.GetComponent<StaticMeshComponent>().GetModel() : nullptr;
		const Ref<Skeleton> skeleton = model ? model->GetSkeleton() : nullptr;
		const bool leaf = node.Children.empty();

		const ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_AllowItemOverlap;
		const bool opened = ImGui::TreeNodeBehavior(ImGui::GetID((void*)(uint64_t)(uint32_t)entity), flags | (leaf ? ImGuiTreeNodeFlags_Leaf : 0), "");
		ImGui::PopStyleColor(3);

		// Draw the title
		ImGui::SameLine();
		ImGui::TextUnformatted(node.Name.c_str());

		// Type Section
		ImGui::TableNextColumn();
		ImGui::TextUnformatted("Bone");

		// Modifiers Section
		ImGui::TableNextColumn();
		ImGui::Dummy(ImVec2(15.0f) + ImGui::GetStyle().FramePadding);
		ImGui::PopID();

		if (opened)
		{
			// Draw bone nodes
			for (const auto& child : node.Children)
				DrawBoneNode(entity, children, child);

			// Draw Entities 'attached' to this bone
			for (const auto& child : children)
				if (child.HasComponent<AttachmentComponent>() && child.GetComponent<AttachmentComponent>().BoneName == node.Name)
					DrawEntityNode(child);

			ImGui::TreePop();
		}
	}

	template<typename T, typename UIFunction, typename UISettings>
	static void DrawComponent(const std::string& name, Entity entity, UIFunction uiFunction, UISettings uiSettings = nullptr, bool removeable = true)
	{
		const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
		if (entity.HasComponent<T>())
		{
			auto& component = entity.GetComponent<T>();
			ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
			float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
			ImGui::Separator();
			bool open = ImGui::TreeNodeEx((void*)typeid(T).hash_code(), treeNodeFlags, name.c_str());
			ImGui::PopStyleVar();

			bool removeComponent = false;

			// Only draw the button if the component has options
			if (!std::is_same_v<UISettings, std::nullptr_t> || removeable)
			{
				const float button_width = 31.0f;
				ImGui::SameLine(contentRegionAvailable.x - ((button_width + ImGui::GetStyle().FramePadding.x * 2.0f) * 0.5f));
				if (ImGui::Button(CHARACTER_ICON_PREFERENCES, ImVec2{ button_width, lineHeight }))
					ImGui::OpenPopup("ComponentSettings");

				if (ImGui::BeginPopup("ComponentSettings"))
				{
					ImGui::TextDisabled(CHARACTER_ICON_GEAR " Component Settings");
					ImGui::Separator();

					// If nullptr is passed to this function, the below constexpr compiles it out so it is not called avoiding
					// rendering an empty UI window.
					if constexpr(!std::is_same_v<UISettings, std::nullptr_t>)
						uiSettings(component);

					if (removeable)
					{
						if (ImGui::MenuItem(CHARACTER_ICON_DELETE " Remove component"))
							removeComponent = true;
					}

					ImGui::EndPopup();
				}
			}

			if (open)
			{
				ImGui::PushID(name.c_str());
				uiFunction(component);
				ImGui::PopID();

				ImGui::TreePop();
			}

			if (removeComponent)
				entity.RemoveComponent<T>();
		}
	}

	namespace Utils {

		static void DrawConstraintTargetInput(EntityHandle& target)
		{
			ImGui::TextUnformatted(FA_BULLSEYE " Target");
			ImGui::SameLine();
			UI::DrawEntitySelectionInput("##Target", target);
		}

		static void DrawConstraintSpaceSelectionInput(ConstraintSpace& space, const bool allowAutomatic = false)
		{
			ImGui::PushID(&space);
			const char* spaceTypes[] = { FA_CUBE " Local Space", FA_GLOBE " World Space", FA_ROTATE " Automatic" };
			UI::Combo(FA_CHART_SCATTER_3D " Space", (int*)&space, spaceTypes, allowAutomatic ? 3 : 2);
			ImGui::PopID();
		}

		static void DrawConstraintSwingTypeSelectionInput(ConstraintSwingType& swingType)
		{
			const char* swingTypes[] = { FA_TRAFFIC_CONE " Cone", FA_CHART_PYRAMID " Pyramid" };
			UI::Combo(FA_GEAR " Swing Type", (int*)&swingType, swingTypes, IM_ARRAYSIZE(swingTypes));
		}

		static void DrawFractionInput(const char* label, Fraction& fraction)
		{
			ImGui::PushID(label);
			UI::Text(label);
			ImGui::SameLine();
			ImGui::PushMultiItemsWidths(2, ImGui::GetContentRegionAvailWidth() - 10.0f);
			ImGui::DragInt("##Numerator", &fraction.Numerator, 0.25f);
			ImGui::PopItemWidth();
			ImGui::SameLine();
			UI::PushFont(FontType::Bold);
			ImGui::TextUnformatted(":");
			UI::PopFont();
			ImGui::SameLine();
			ImGui::DragInt("##Denominator", &fraction.Denominator, 0.25f);
			ImGui::PopItemWidth();
			ImGui::PopID();
		}

		static void DrawFlaggedFloatInput(const char* label, float* value, float disabledValue, float enabledValue = 0.0f, float v_min = 0.0f, float v_max = 0.0f)
		{
			ImGui::PushID(label);

			bool flag = (*value != disabledValue);
			if (UI::Checkbox(label, &flag))
				*value = flag ? enabledValue : disabledValue;

			if (flag)
			{
				ImGui::SameLine();
				ImGui::DragFloat("##FlaggedFloat", value, 1.0f, v_min, v_max);
			}

			ImGui::PopID();
		}

		static glm::vec3 GetAxisValue(const Axis axis)
		{
			switch (axis)
			{
			case Axis::X:	return c_AxisX;
			case Axis::Y:	return c_AxisY;
			case Axis::Z:	return c_AxisZ;
			}

			return c_AxisX;
		}

		static void DrawAxisSelection(const char* label, glm::vec3& axis, Axis defaultAxis)
		{
			ImGui::PushID(label);
			UI::Text(label);

			// If an axis is indicated by a marker as being in use then we have a main axis, otherwise a custom axis must be in use
			if (glm::any(glm::greaterThanEqual(axis, glm::vec3(c_AxisMarker))))
			{
				const char* axisIcons[] = { FA_CIRCLE_X, FA_CIRCLE_Y, FA_CIRCLE_Z };
				for (uint32_t axisIndex = 0; axisIndex < c_AxisCount; axisIndex++)
				{
					ImGui::SameLine();

					bool selected = (axis[axisIndex] >= c_AxisMarker);
					if (UI::DrawTextIconButton(axisIcons[axisIndex], &selected))
						axis = GetAxisValue((Axis)((uint8_t)Axis::X + axisIndex));
				}

				ImGui::SameLine();
				ImGui::Dummy(ImVec2(7.5f, 0.0f));
				ImGui::SameLine();

				if (UI::DrawTextIconButton(FA_PEN_CIRCLE))
					axis = glm::normalize(axis);
			}
			else
			{
				// Custom axis input control
				ImGui::SameLine();
				const glm::vec3 defaultValue = GetAxisValue(defaultAxis);
				UI::DrawVec3Control(label, axis, glm::normalize(defaultValue), -1.0f);

				ImGui::SameLine();

				// Switch back to main 3 axes button
				if (UI::DrawTextIconButton(CHARACTER_ICON_EMPTY))
					axis = defaultValue;

				ImGui::SameLine();

				// Normalize vector button
				if (UI::DrawTextIconButton(FA_CIRCLE_ARROW_UP_RIGHT))
					axis = glm::normalize(axis);
			}

			ImGui::PopID();
		}

	}

	void SceneHierarchyPanel::DrawComponents(Entity entity)
	{
		bool entityDeleted = false;

		auto& style = ImGui::GetStyle();
		const ImVec2 buttonSize = ImVec2(31.0f, 24.0f);

		if (entity.HasComponent<TagComponent>())
		{
			auto& tag = entity.GetComponent<TagComponent>().Tag;

			ImGui::Text(FA_TAG);
			ImGui::SameLine();

			char buffer[256];
			memset(buffer, 0, sizeof(buffer));
			strncpy_s(buffer, sizeof(buffer), tag.c_str(), sizeof(buffer));
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth() - (buttonSize.x + style.FramePadding.x * 2.0f) * 3.0f);
			ImGui::InputText("##Tag", buffer, sizeof(buffer));
			if (ImGui::IsItemDeactivatedAfterEdit())
				tag = std::string(buffer);
		}

		ImGui::SameLine();
		ImGui::PushItemWidth(-1);

		if (!entity.HasComponent<FolderComponent>())
			if (ImGui::Button(CHARACTER_ICON_ADD, buttonSize))
				ImGui::OpenPopup("##AddComponent");
		ImGui::SameLine();
		if (ImGui::Button(CHARACTER_ICON_DUPLICATE, buttonSize))
			m_Context->DuplicateEntity(m_ActiveEntity);
		ImGui::SameLine();
		if (ImGui::Button(CHARACTER_ICON_DELETE, buttonSize))
			entityDeleted = true;

		if (ImGui::BeginPopup("##AddComponent"))
		{
			ImGui::TextDisabled(CHARACTER_ICON_ADD " Add Component");
			ImGui::Separator();

			DisplayAddComponentEntry<StaticMeshComponent>(FILE_ICON_MESH " Mesh");

			// Allow Bone attachments to be added if in the correct edit mode and if the parent has a skeletal mesh
			if (Preferences::GetData().BoneAttachmentEditMode == Preferences::BoneAttachmentEditMode::Component && entity.HasParent())
			{
				Entity parent = entity.GetParent();
				const Ref<Model> model = parent.HasComponent<StaticMeshComponent>() ? parent.GetComponent<StaticMeshComponent>().GetModel() : nullptr;

				if (model && model->GetSkeleton())
					DisplayAddComponentEntry<AttachmentComponent>(FA_BONE " Bone Attachment");
			}

			if (ImGui::BeginMenu(CHARACTER_ICON_POINT_LIGHT " Light"))
			{
				DisplayAddComponentEntry<DirectionalLightComponent>(CHARACTER_ICON_SUN " Directional Light");
				DisplayAddComponentEntry<PointLightComponent>(CHARACTER_ICON_POINT_LIGHT " Point Light");
				DisplayAddComponentEntry<SpotLightComponent>(CHARACTER_ICON_SPOT_LIGHT " Spot Light");
				DisplayAddComponentEntry<SkyLightComponent>(CHARACTER_ICON_CLOUDS " Sky Light");
				ImGui::Separator();
				DisplayAddComponentEntry<VolumeComponent>(CHARACTER_ICON_SMOKE " Volume");
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu(FA_HURRICANE "Effects"))
			{
				DisplayAddComponentEntry<DecalComponent>(FA_STAMP " Decal");
				DisplayAddComponentEntry<PostProcessVolumeComponent>(FA_LAYER_GROUP " Post Process Volume");
				DisplayAddComponentEntry<CaptureComponent>(FA_PROJECTOR " Capture");
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu(CHARACTER_ICON_IMAGE " 2D"))
			{
				DisplayAddComponentEntry<SpriteRendererComponent>(CHARACTER_ICON_IMAGE " Sprite Renderer");
				DisplayAddComponentEntry<CircleRendererComponent>(CHARACTER_ICON_SHADING_UNLIT " Circle Renderer");
				DisplayAddComponentEntry<TextComponent>(FILE_ICON_FONT " Text");
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu(CHARACTER_ICON_PHYSICS " Physics"))
			{
				DisplayAddComponentEntry<RigidBodyComponent>(CHARACTER_ICON_RIGIDBODY " Rigid Body");
				DisplayAddComponentEntry<SoftBodyComponent>(FA_FLAG_SWALLOWTAIL " Soft Body");
				DisplayAddComponentEntry<FieldComponent>(FA_MAGNET " Field");
				if (ImGui::BeginMenu(CHARACTER_ICON_BOX_COLLIDER " Collider"))
				{
					DisplayAddComponentEntry<BoxColliderComponent>(CHARACTER_ICON_BOX_COLLIDER " Box Collider");
					DisplayAddComponentEntry<SphereColliderComponent>(CHARACTER_ICON_SPHERE_COLLIDER " Sphere Collider");
					DisplayAddComponentEntry<CapsuleColliderComponent>(CHARACTER_ICON_CAPSULE_COLLIDER " Capsule Collider");
					DisplayAddComponentEntry<MeshColliderComponent>(CHARACTER_ICON_MESH_COLLIDER " Mesh Collider");
					ImGui::EndMenu();
				}

				if (ImGui::BeginMenu(FA_LINK " Constraint"))
				{
					DisplayAddComponentEntry<PointConstraintComponent>(FA_THUMBTACK " Point Constraint");
					DisplayAddComponentEntry<ConeConstraintComponent>(FA_RULER " Cone Constraint");
					DisplayAddComponentEntry<DistanceConstraintComponent>(FA_RULER " Distance Constraint");
					DisplayAddComponentEntry<SpringConstraintComponent>(CHARACTER_ICON_SPRING " Spring Constraint");
					DisplayAddComponentEntry<HingeConstraintComponent>(FA_ANGLE " Hinge Constraint");
					DisplayAddComponentEntry<FixedConstraintComponent>(FA_OBJECT_UNION " Fixed Constraint");
					DisplayAddComponentEntry<GearConstraintComponent>(FA_GEARS " Gear Constraint");
					DisplayAddComponentEntry<PulleyConstraintComponent>(FA_CIRCLE_NOTCH " Pulley Constraint");
					DisplayAddComponentEntry<RackAndPinionConstraintComponent>(FA_GEAR_COMPLEX " Rack and Pinion Constraint");
					DisplayAddComponentEntry<SwingTwistConstraintComponent>(FA_SHUFFLE " Swing Twist Constraint");
					DisplayAddComponentEntry<SliderConstraintComponent>(FA_GRIP_LINES " Slider Constraint");
					DisplayAddComponentEntry<SixDOFConstraintComponent>(FA_360_DEGREES " Six DOF Constraint");
					DisplayAddComponentEntry<FollowConstraintComponent>(FA_ROUTE " Follow Constraint");
					ImGui::EndMenu();
				}

				if (ImGui::BeginMenu(CHARACTER_ICON_CONTROLLER " Controller"))
				{
					DisplayAddComponentEntry<RagdollComponent>(FA_PERSON_FALLING " Ragdoll");
					DisplayAddComponentEntry<CharacterMovementComponent>(CHARACTER_ICON_RUNNING " Character Movement");
					DisplayAddComponentEntry<VehicleMovementComponent>(CHARACTER_ICON_VEHICLE " Vehicle Movement");
					DisplayAddComponentEntry<SpringArmComponent>(CHARACTER_ICON_SPRING " Spring Arm");
					ImGui::EndMenu();
				}

				ImGui::Separator();

				if (ImGui::BeginMenu(CHARACTER_ICON_SQUARE " 2D"))
				{
					DisplayAddComponentEntry<RigidBody2DComponent>(CHARACTER_ICON_RIGIDBODY " Rigid Body 2D");
					DisplayAddComponentEntry<BoxCollider2DComponent>(CHARACTER_ICON_SQUARE " Box Collider 2D");
					DisplayAddComponentEntry<CircleCollider2DComponent>(CHARACTER_ICON_CIRCLE " Circle Collider 2D");
					ImGui::EndMenu();
				}

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu(CHARACTER_ICON_CANVAS " UI"))
			{
				DisplayAddComponentEntry<UICanvasComponent>(CHARACTER_ICON_CANVAS " Canvas");
				DisplayAddComponentEntry<UIImageComponent>(CHARACTER_ICON_IMAGE " Image");
				DisplayAddComponentEntry<UIButtonComponent>(CHARACTER_ICON_BUTTON " Button");
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu(FA_COMPASS " Navigation"))
			{
				DisplayAddComponentEntry<NavigationMeshComponent>(FA_MAP_LOCATION_DOT " Navigation Mesh");
				DisplayAddComponentEntry<NavigationModifierComponent>(FA_LOCATION_PLUS " Navigation Modifier");
				DisplayAddComponentEntry<NavigationLinkComponent>(FA_LINK " Navigation Link");
				ImGui::EndMenu();
			}

			ImGui::Separator();

			DisplayAddComponentEntry<CameraComponent>(CHARACTER_ICON_CAMERA " Camera");
			DisplayAddComponentEntry<ParticleSystemComponent>(CHARACTER_ICON_PARTICLES " Particle System");
			DisplayAddComponentEntry<AudioComponent>(CHARACTER_ICON_AUDIO " Audio");
			DisplayAddComponentEntry<SplineComponent>(FA_BEZIER_CURVE " Spline");
			DisplayAddComponentEntry<LandscapeComponent>(FA_MOUNTAIN_SUN " Landscape");

			ImGui::Separator();

			DisplayAddComponentEntry<ScriptComponent>(FILE_ICON_SCRIPT " Script");

			ImGui::EndPopup();
		}

		ImGui::PopItemWidth();

		if (Preferences::GetData().AdvancedEditMode)
		{
			DrawComponent<IDComponent>(FA_KEY " ID", entity, [](auto& component)
			{
				uint64_t id = component.ID;
				ImGui::InputScalar("##IDComponentInputScalar", ImGuiDataType_U64, &id);
				if (ImGui::IsItemDeactivatedAfterEdit())
					component.ID = id;
			}, nullptr, false);

			DrawComponent<PrefabComponent>(FILE_ICON_PREFAB " PREFAB", entity, [](auto& component)
			{
				uint64_t id = component.PrefabID;
				ImGui::InputScalar("##PrefabComponentInputScalar", ImGuiDataType_U64, &id);
				if (ImGui::IsItemDeactivatedAfterEdit())
					component.PrefabID = id;
			}, nullptr, false);
		}

		DrawComponent<TransformComponent>(CHARACTER_ICON_TRANSFORM " TRANSFORM", entity, [&entity, this](auto& component)
		{
			Transform& transform = component.Transform;

			if (UI::DrawVec3Control(FA_UP_DOWN_LEFT_RIGHT " Translation", transform.Translation))
				m_Context->UpdateEntityTranslation(entity);

			glm::vec3 rotation = transform.GetRotationDegrees();
			if (UI::DrawVec3Control(FA_ROTATE " Rotation", rotation))
				m_Context->SetEntityRotation(entity, glm::radians(rotation));

			if (UI::DrawVec3Control(FA_EXPAND " Scale", transform.Scale, glm::vec3(1.0f)))
				m_Context->UpdateEntityScale(entity);
		},
		[](auto& component) 
		{
			Transform& transform = component.Transform;

			if (ImGui::MenuItem(CHARACTER_ICON_COPY " Copy"))
			{
				std::stringstream ss;
				glm::vec3 rotation = transform.GetRotationDegrees();

				ss << transform.Translation.x << " " << transform.Translation.y << " " << transform.Translation.z
					<< " " << rotation.x  << " " << rotation.y  << " " << rotation.z
					<< " " << transform.Scale.x  << " " << transform.Scale.y << " " << transform.Scale.z;
				ImGui::SetClipboardText(ss.str().c_str());
			}

			if (ImGui::MenuItem(CHARACTER_ICON_PASTE " Paste"))
			{
				std::stringstream ss;
				ss << ImGui::GetClipboardText();

				glm::vec3 rotation;

				ss >> transform.Translation.x >> transform.Translation.y >> transform.Translation.z
					>> rotation.x >> rotation.y >> rotation.z
					>> transform.Scale.x >> transform.Scale.y >> transform.Scale.z;

				transform.SetRotationDegrees(rotation);
			}

			if (ImGui::MenuItem(CHARACTER_ICON_RESTART " Reset"))
			{
				transform = Transform();
			}

		}, false);

		if (Preferences::GetData().BoneAttachmentEditMode == Preferences::BoneAttachmentEditMode::Component)
		{
			DrawComponent<AttachmentComponent>(FA_BONE " BONE ATTACHMENT", entity, [&entity](auto& component)
			{
				// Check that the skeleton is valid
				Entity parent = entity.GetParent();
				const Ref<Model> model = (parent && parent.HasComponent<StaticMeshComponent>()) ? parent.GetComponent<StaticMeshComponent>().GetModel() : nullptr;
				const Ref<Skeleton> skeleton = model ? model->GetSkeleton() : nullptr;

				if (!skeleton)
				{
					ImGui::TextDisabledUnformatted(FA_TRIANGLE_EXCLAMATION " Entity does not have a parent with a valid skeletal mesh.");
					ImGui::TextDisabledUnformatted("This component will be ignored!");
					return;
				}

				const bool isValidBoneName = skeleton->IsValidBoneName(component.BoneName);

				if (!isValidBoneName)
					ImGui::PushStyleColor(ImGuiCol_FrameBg, { 1.0f, 0.4f, 0.4f, 0.5f });

				ImGui::InputTextWithHint(FA_BONE " Bone Name", FA_MAGNIFYING_GLASS " Select Bone...", &component.BoneName);

				if (!isValidBoneName)
					ImGui::PopStyleColor();

				const ImVec2 inputPosition = ImVec2(ImGui::GetItemRectMin().x, ImGui::GetItemRectMax().y);

				if (ImGui::IsItemActive() || ImGui::IsPopupOpen("##BoneSearchPopup"))
				{
					ImGui::OpenPopup("##BoneSearchPopup");
					if (ImGui::BeginPopup("##BoneSearchPopup", ImGuiWindowFlags_NoFocusOnAppearing))
					{
						ImGui::SetWindowPos(inputPosition);

						bool found = false;
						const auto& boneInfoMap = skeleton->GetBoneInfoMap();
						for (auto& [name, boneInfo] : boneInfoMap)
						{
							if (String::ToLower(name).find(String::ToLower(component.BoneName)) != std::string::npos)
							{
								if (ImGui::MenuItem(fmt::format(FA_BONE " {}", name).c_str()))
									component.BoneName = name;

								found = true;
							}
						}

						if (!found)
							ImGui::TextDisabled(FA_CIRCLE_XMARK " No Bones Found");

						ImGui::EndPopup();

						if (ImGui::IsWindowFocused())
							ImGui::CloseCurrentPopup();
					}
				}

			}, nullptr);
		}

		DrawComponent<CameraComponent>(CHARACTER_ICON_CAMERA " CAMERA", entity, [](auto& component)
		{
			auto& camera = component.Camera;

			UI::Checkbox(FA_CIRCLE_PLAY " Primary", &component.Primary);

			const char* projectionTypes[] = { FA_EYE " Perspective",  FA_SQUARE " Orthographic" };
			SceneCamera::ProjectionType projectionType = camera.GetProjectionType();
			if (UI::Combo("Projection", (int*)&projectionType, projectionTypes, IM_ARRAYSIZE(projectionTypes)))
				camera.SetProjectionType(projectionType);

			if (projectionType == SceneCamera::ProjectionType::Perspective)
			{
				float perspectiveVerticalFov = glm::degrees(camera.GetPerspectiveVerticalFOV());
				if (UI::DragFloat(FA_BINOCULARS " Vertical FOV", &perspectiveVerticalFov))
					camera.SetPerspectiveVerticalFOV(glm::radians(perspectiveVerticalFov));

				float perspectiveNear = camera.GetPerspectiveNearClip();
				if (UI::DragFloat(FA_MAGNIFYING_GLASS " Near", &perspectiveNear))
					camera.SetPerspectiveNearClip(perspectiveNear);

				float perspectiveFar = camera.GetPerspectiveFarClip();
				if (UI::DragFloat(FA_TELESCOPE " Far", &perspectiveFar))
					camera.SetPerspectiveFarClip(perspectiveFar);
			}

			if (projectionType == SceneCamera::ProjectionType::Orthographic)
			{
				float orthoSize = camera.GetOrthographicSize();
				if (UI::DragFloat(FA_EXPAND " Size", &orthoSize))
					camera.SetOrthographicSize(orthoSize);

				float orthoNear = camera.GetOrthographicNearClip();
				if (UI::DragFloat(FA_MAGNIFYING_GLASS " Near", &orthoNear))
					camera.SetOrthographicNearClip(orthoNear);

				float orthoFar = camera.GetOrthographicFarClip();
				if (UI::DragFloat(FA_TELESCOPE " Far", &orthoFar))
					camera.SetOrthographicFarClip(orthoFar);

				UI::Checkbox(FA_LOCK " Fixed Aspect Ratio", &component.FixedAspectRatio);
			}

			if (UI::CollapsingHeader(FA_BARS " Settings"))
			{
				auto& settings = camera.GetCameraSettings();

				// Bloom
				ImGui::TextDisabled(FA_SUN_HAZE " Bloom");
				ImGui::Indent();
				UI::DragFloat(FA_FILTER " Threshold", &settings.BloomThreshold, 0.1f);

				ImGui::Text(FA_SPRAY_CAN_SPARKLES " Dirt Texture");
				ImGui::SameLine();
				UI::DrawAssetSelectionDropdown(AssetType::Texture, settings.LUT);
				ImGui::Unindent();

				ImGui::Separator();

				// DOF
				ImGui::TextDisabled(FA_APERTURE " Depth of Field");

				ImGui::Indent();
				UI::DragFloat(FA_DUMBBELL " Strength##DOF", &settings.DOFStrength, 0.1f);
				UI::DragFloat(FA_CROSSHAIRS " Target Distance##DOF", &settings.DOFTarget, 0.1f);
				UI::DragFloat(FA_RULER " Focus Range##DOF", &settings.DOFFocusRange, 0.1f);
				UI::DragFloat(FA_ELLIPSIS " Focus Falloff##DOF", &settings.DOFFocusFalloff, 0.1f);
				ImGui::Unindent();

				ImGui::Separator();

				// LUT
				ImGui::TextDisabled(FA_TABLE " LUT");
				ImGui::Indent();
				UI::DrawAssetSelectionDropdown(AssetType::Texture, settings.LUT);
				ImGui::Unindent();

				ImGui::TreePop();
			}

		}, nullptr);

		DrawComponent<CaptureComponent>(FA_PROJECTOR " CAPTURE", entity, [](auto& component)
		{
			UI::Checkbox(FA_RECORD_VINYL " Capture", &component.Capture);
			UI::Checkbox(FA_LAYER_PLUS " Cumulative", &component.Cumulative);

			const SceneRendererContext::RendererVisualizationMode modeOptions[] = {
				SceneRendererContext::RendererVisualizationMode::Rendered,
				SceneRendererContext::RendererVisualizationMode::LightingOnly,
				SceneRendererContext::RendererVisualizationMode::PrePostProcessing,
				SceneRendererContext::RendererVisualizationMode::Albedo,
				SceneRendererContext::RendererVisualizationMode::Depth,
				SceneRendererContext::RendererVisualizationMode::LinearDepth,
				SceneRendererContext::RendererVisualizationMode::Position,
				SceneRendererContext::RendererVisualizationMode::Normal,
				SceneRendererContext::RendererVisualizationMode::Emissive,
				SceneRendererContext::RendererVisualizationMode::Roughness,
				SceneRendererContext::RendererVisualizationMode::Metallic,
				SceneRendererContext::RendererVisualizationMode::Specular,
				SceneRendererContext::RendererVisualizationMode::AmbientOcclusion,
				SceneRendererContext::RendererVisualizationMode::Velocity,
				SceneRendererContext::RendererVisualizationMode::EntityID,
				SceneRendererContext::RendererVisualizationMode::SubmeshIndex
			};

			ImGui::TextUnformatted(FA_GEAR " Type");
			ImGui::SameLine();
			if (ImGui::BeginCombo("##CaptureTypeDropdown", SceneRendererContext::RenderVisualizationModeToString(component.Type)))
			{
				for (uint32_t modeIndex = 0; modeIndex < IM_ARRAYSIZE(modeOptions); modeIndex++)
					if (ImGui::MenuItem(SceneRendererContext::RenderVisualizationModeToString(modeOptions[modeIndex])))
						component.Type = modeOptions[modeIndex];

				ImGui::EndCombo();
			}

			ImGui::TextUnformatted(FILE_ICON_VIRTUAL_TEXTURE " Target Virtual Texture");
			ImGui::SameLine();
			UI::DrawAssetSelectionDropdown(AssetType::VirtualTexture, component.Target);

			const char* maskTypeOptions[] = { FA_CIRCLE_XMARK " None", FA_CIRCLE_MINUS " Exclusive", FA_CIRCLE_PLUS " Inclusive" };
			UI::Combo(FA_CIRCLE_HALF_STROKE " Mask Type", (int*)&component.MaskType, maskTypeOptions, IM_ARRAYSIZE(maskTypeOptions));

		}, nullptr);

		DrawComponent<ScriptComponent>(FILE_ICON_SCRIPT " SCRIPT", entity, [this, entity, scene = m_Context](auto& component) mutable
		{
			bool scriptClassExists = ScriptEngine::EntityClassExists(component.ClassName);
			bool scriptEmpty = component.ClassName.empty();

			static char buffer[64];
			strcpy_s(buffer, sizeof(buffer), component.ClassName.c_str());

			if (!scriptClassExists && !scriptEmpty)
			{
				ImGui::PushStyleColor(ImGuiCol_FrameBg, { 1.0f, 0.4f, 0.4f, 0.5f });
			}

			ImGui::InputTextWithHint(FA_BRACKETS_CURLY " Class", FA_MAGNIFYING_GLASS " Select Script...", buffer, sizeof(buffer));

			if (ImGui::IsItemDeactivatedAfterEdit())
			{
				component.ClassName = buffer;
				scene->UpdateEntityScriptName(entity);
			}

			if (!scriptClassExists && !scriptEmpty)
				ImGui::PopStyleColor();

			ImVec2 inputPosition = ImVec2(ImGui::GetItemRectMin().x, ImGui::GetItemRectMax().y);

			if (ImGui::IsItemActive() || ImGui::IsPopupOpen("##ScriptSearchPopup"))
			{
				ImGui::OpenPopup("##ScriptSearchPopup");
				if (ImGui::BeginPopup("##ScriptSearchPopup", ImGuiWindowFlags_NoFocusOnAppearing))
				{
					ImGui::SetWindowPos(inputPosition);

					bool found = false;
					for (auto& [name, scriptClass] : ScriptEngine::GetEntityClasses())
					{
						if (String::ToLower(name).find(String::ToLower(component.ClassName)) != std::string::npos)
						{
							found = true;
							if (ImGui::MenuItem(fmt::format(FA_FILE_CODE " {}", name).c_str()))
							{
								component.ClassName = name;
								scene->UpdateEntityScriptName(entity);
							}
						}
					}

					if (!found)
						ImGui::TextDisabled(FA_CIRCLE_XMARK " No Scripts Found");

					ImGui::EndPopup();

					if (ImGui::IsWindowFocused())
						ImGui::CloseCurrentPopup();
				}
			}

			// Fields
			scriptClassExists = ScriptEngine::EntityClassExists(component.ClassName);
			if (scriptClassExists)
			{
				if (ImGui::CollapsingHeader(FA_LIST_TIMELINE " Fields"))
				{
					ImGui::Indent();

					bool sceneRunning = scene->IsRunning();
					if (sceneRunning)
					{
						Ref<ScriptInstance> scriptInstance = ScriptEngine::GetEntityScriptInstance(entity.GetUUID());
						if (scriptInstance)
						{
							const auto& fields = scriptInstance->GetScriptClass()->GetFields();
							for (const auto& [name, field] : fields)
							{
								#define DRAW_SCRIPT_FIELD(dymatic, internal)						\
								case ScriptFieldType::dymatic:										\
								{																	\
									internal data = scriptInstance->GetFieldValue<internal>(name);	\
									if (DrawScriptField(entity, name, field, &data))				\
										scriptInstance->SetFieldValue(name, data);					\
									break;															\
								}

								if (field.Type == ScriptFieldType::None)
									ImGui::TextDisabled(fmt::format(FA_CIRCLE_DOT " {}", name).c_str());

								switch (field.Type)
								{
									DRAW_SCRIPT_FIELD(Float, float);
									DRAW_SCRIPT_FIELD(Double, double);
									DRAW_SCRIPT_FIELD(Bool, bool);
									DRAW_SCRIPT_FIELD(Char, uint16_t);
									DRAW_SCRIPT_FIELD(Byte, uint8_t);
									DRAW_SCRIPT_FIELD(Short, int16_t);
									DRAW_SCRIPT_FIELD(Int, int32_t);
									DRAW_SCRIPT_FIELD(Long, int64_t);
									DRAW_SCRIPT_FIELD(UShort, uint16_t);
									DRAW_SCRIPT_FIELD(UInt, uint32_t);
									DRAW_SCRIPT_FIELD(ULong, uint64_t);
									DRAW_SCRIPT_FIELD(Vector2, glm::vec2);
									DRAW_SCRIPT_FIELD(Vector3, glm::vec3);
									DRAW_SCRIPT_FIELD(Vector4, glm::vec4);
									DRAW_SCRIPT_FIELD(Entity, uint64_t);
									DRAW_SCRIPT_FIELD(Asset, uint64_t);
									DRAW_SCRIPT_FIELD(Scene, uint64_t);
									DRAW_SCRIPT_FIELD(Texture, uint64_t);
									DRAW_SCRIPT_FIELD(VirtualTexture, uint64_t);
									DRAW_SCRIPT_FIELD(Mesh, uint64_t);
									DRAW_SCRIPT_FIELD(Animation, uint64_t);
									DRAW_SCRIPT_FIELD(Material, uint64_t);
									DRAW_SCRIPT_FIELD(Audio, uint64_t);
									DRAW_SCRIPT_FIELD(VideoPlayer, uint64_t);
								}
							}
						}
					}
					else
					{
						if (scriptClassExists)
						{
							Ref<ScriptClass> entityClass = ScriptEngine::GetEntityClass(component.ClassName);
							const auto& fields = entityClass->GetFields();

							auto& entityFields = ScriptEngine::GetScriptFieldMap(entity);
							for (const auto& [name, field] : fields)
							{
								// Field has been set in editor
								if (entityFields.find(name) != entityFields.end())
								{
									ScriptFieldInstance& scriptField = entityFields.at(name);

									#define DRAW_SCRIPT_FIELD(dymatic, internal)			\
									case ScriptFieldType::dymatic:							\
									{														\
										internal data = scriptField.GetValue<internal>();	\
										if (DrawScriptField(entity, name, field, &data))	\
											scriptField.SetValue(data);						\
										break;												\
									}

									if (field.Type == ScriptFieldType::None)
										ImGui::TextDisabled(fmt::format(FA_CIRCLE_DOT " {}", name).c_str());

									switch (field.Type)
									{
										DRAW_SCRIPT_FIELD(Float, float);
										DRAW_SCRIPT_FIELD(Double, double);
										DRAW_SCRIPT_FIELD(Bool, bool);
										DRAW_SCRIPT_FIELD(Char, uint16_t);
										DRAW_SCRIPT_FIELD(Byte, uint8_t);
										DRAW_SCRIPT_FIELD(Short, int16_t);
										DRAW_SCRIPT_FIELD(Int, int32_t);
										DRAW_SCRIPT_FIELD(Long, int64_t);
										DRAW_SCRIPT_FIELD(UShort, uint16_t);
										DRAW_SCRIPT_FIELD(UInt, uint32_t);
										DRAW_SCRIPT_FIELD(ULong, uint64_t);
										DRAW_SCRIPT_FIELD(Vector2, glm::vec2);
										DRAW_SCRIPT_FIELD(Vector3, glm::vec3);
										DRAW_SCRIPT_FIELD(Vector4, glm::vec4);
										DRAW_SCRIPT_FIELD(Entity, uint64_t);
										DRAW_SCRIPT_FIELD(Asset, uint64_t);
										DRAW_SCRIPT_FIELD(Scene, uint64_t);
										DRAW_SCRIPT_FIELD(Texture, uint64_t);
										DRAW_SCRIPT_FIELD(VirtualTexture, uint64_t);
										DRAW_SCRIPT_FIELD(Mesh, uint64_t);
										DRAW_SCRIPT_FIELD(Animation, uint64_t);
										DRAW_SCRIPT_FIELD(Material, uint64_t);
										DRAW_SCRIPT_FIELD(Audio, uint64_t);
										DRAW_SCRIPT_FIELD(VideoPlayer, uint64_t);
									}
								}
								else
								{
									// Controls to set parameters

									#define DRAW_SCRIPT_FIELD(dymatic, internal)						\
									case ScriptFieldType::dymatic:										\
									{																	\
										internal data = {};												\
										if (DrawScriptField(entity, name, field, &data))				\
										{																\
											ScriptFieldInstance& fieldInstance = entityFields[name];	\
											fieldInstance.Field = field;								\
											fieldInstance.SetValue(data);								\
										}																\
										break;															\
									}

									if (field.Type == ScriptFieldType::None)
										ImGui::TextDisabled(fmt::format(FA_CIRCLE_DOT " {}", name).c_str());

									switch (field.Type)
									{
										DRAW_SCRIPT_FIELD(Float, float);
										DRAW_SCRIPT_FIELD(Double, double);
										DRAW_SCRIPT_FIELD(Bool, bool);
										DRAW_SCRIPT_FIELD(Char, uint16_t);
										DRAW_SCRIPT_FIELD(Byte, uint8_t);
										DRAW_SCRIPT_FIELD(Short, int16_t);
										DRAW_SCRIPT_FIELD(Int, int32_t);
										DRAW_SCRIPT_FIELD(Long, int64_t);
										DRAW_SCRIPT_FIELD(UShort, uint16_t);
										DRAW_SCRIPT_FIELD(UInt, uint32_t);
										DRAW_SCRIPT_FIELD(ULong, uint64_t);
										DRAW_SCRIPT_FIELD(Vector2, glm::vec2);
										DRAW_SCRIPT_FIELD(Vector3, glm::vec3);
										DRAW_SCRIPT_FIELD(Vector4, glm::vec4);
										DRAW_SCRIPT_FIELD(Entity, uint64_t);
										DRAW_SCRIPT_FIELD(Asset, uint64_t);
										DRAW_SCRIPT_FIELD(Scene, uint64_t);
										DRAW_SCRIPT_FIELD(Texture, uint64_t);
										DRAW_SCRIPT_FIELD(VirtualTexture, uint64_t);
										DRAW_SCRIPT_FIELD(Mesh, uint64_t);
										DRAW_SCRIPT_FIELD(Animation, uint64_t);
										DRAW_SCRIPT_FIELD(Material, uint64_t);
										DRAW_SCRIPT_FIELD(Audio, uint64_t);
										DRAW_SCRIPT_FIELD(VideoPlayer, uint64_t);
									}
								}
							}
						}
					}
					ImGui::Unindent();
				}
			}
		}, nullptr);

		DrawComponent<SpriteRendererComponent>(CHARACTER_ICON_IMAGE " SPRITE RENDERER", entity, [this](auto& component)
		{
			ImGui::ColorEdit4(FA_PALETTE " Color", glm::value_ptr(component.Color));
			
			ImGui::ImageButton(component.Texture ? ((ImTextureID)component.Texture->GetRendererID()) : ((ImTextureID)EditorResources::CheckerboardTexture->GetRendererID()), ImVec2(100.0f, 100.0f), ImVec2( 0, 1 ), ImVec2( 1, 0));
			UI::DrawAssetSelectionDropdown(AssetType::Texture, component.Texture);

			UI::DragFloat(FA_GRID_5 " Tiling Factor", &component.TilingFactor, 0.1f, 0.0f, 100.0f);
		}, nullptr);

		DrawComponent<CircleRendererComponent>(CHARACTER_ICON_SHADING_UNLIT " CIRCLE RENDERER", entity, [](auto& component)
		{
			UI::ColorEdit4(FA_PALETTE " Color", glm::value_ptr(component.Color));
			UI::DragFloat(FA_CIRCLE_NOTCH " Thickness", &component.Thickness, 0.025f, 0.0f, 1.0f);
			UI::DragFloat(FA_KEYBOARD_BRIGHTNESS " Fade", &component.Fade, 0.00025f, 0.0f, 1.0f);
		}, nullptr);

		DrawComponent<TextComponent>(FILE_ICON_FONT " TEXT", entity, [this](auto& component)
		{
			ImGui::Text(FA_TEXT " Text String");
			ImGui::InputTextMultiline("##TextComponentInput", &component.TextString, ImVec2(-1.0f, 200.0f));

			ImGui::Separator();
			
			UI::ColorEdit4(FA_PALETTE " Color", glm::value_ptr(component.Color));

			ImGui::Text(FA_ALIGN_LEFT " Alignment");
			ImGui::SameLine();
			const char* alignmentNames[] = { FA_ALIGN_LEFT, FA_ALIGN_CENTER, FA_ALIGN_RIGHT, FA_ALIGN_JUSTIFY };
			ImGui::SwitchButtonEx("##TextAlignmentInput", alignmentNames, 4, (int*)&component.Alignment, ImVec2(ImGui::GetContentRegionAvailWidth(), 0.0f));
			
			UI::DrawAssetSelectionDropdown(AssetType::Font, component.Font, true);

			ImGui::Separator();
			
			UI::DragFloat(FA_KERNING " Kerning", &component.Kerning);
			UI::DragFloat(FA_LINE_HEIGHT " Line Spacing", &component.LineSpacing);

			ImGui::Separator();

			UI::DragFloat(FA_TEXT_WIDTH " Max Width", &component.MaxWidth);
		}, nullptr);

		DrawComponent<ParticleSystemComponent>(CHARACTER_ICON_PARTICLES " PARTICLE SYSTEM", entity, [](auto& component)
		{		
			if (component.Player)
			{
				ImGui::TextUnformatted(FA_DROPLET " Particle Count:");
				ImGui::SameLine();
				ImGui::TextDisabled("%d", component.Player->GetParticleCount());
			}

			ImGui::TextUnformatted(FILE_ICON_PARTICLE_SYSTEM " Particle System");
			ImGui::SameLine();
			UI::DrawAssetSelectionDropdown(AssetType::ParticleSystem, component.GetParticleSystem(), [&](Ref<ParticleSystem> particleSystem)
			{
				component.SetParticleSystem(particleSystem);
			});

			ImGui::TextUnformatted(FILE_ICON_MATERIAL " Material");
			ImGui::SameLine();
			UI::DrawAssetSelectionDropdown("##ParticleMaterialDropdown", AssetType::Material, component.Material);

		}, nullptr);

		DrawComponent<RigidBody2DComponent>(CHARACTER_ICON_RIGIDBODY " RIGID BODY 2D", entity, [](auto& component)
		{
			const char* bodyTypes[] = { FA_LOCK " Static", FA_PERSON_RUNNING " Dynamic", FA_JOYSTICK " Kinematic" };
			UI::Combo(FA_CUBES_STACKED " Type", (int*)&component.Type, bodyTypes, IM_ARRAYSIZE(bodyTypes));

			UI::Checkbox(FA_ANCHOR " Fixed Rotation", &component.FixedRotation);
		}, nullptr);

		DrawComponent<BoxCollider2DComponent>(CHARACTER_ICON_SQUARE " BOX COLLIDER 2D", entity, [](auto& component)
		{
			UI::DragFloat2(FA_UP_DOWN_LEFT_RIGHT " Offset", glm::value_ptr(component.Offset));
			UI::DragFloat2(FA_EXPAND " Size", glm::value_ptr(component.Size));
			UI::DragFloat(FA_WEIGHT_HANGING "  Density", &component.Density, 0.01f, 0.0f, 1.0f);
			UI::DragFloat(FA_SHOE_PRINTS " Friction", &component.Friction, 0.01f, 0.0f, 1.0f);
			UI::DragFloat(FA_WAVES_SINE " Restitution", &component.Restitution, 0.01f, 0.0f, 1.0f);
			UI::DragFloat(FA_CIRCLE_STOP " Restitution Threshold", &component.RestitutionThreshold, 0.01f, 0.0f);
		}, nullptr);

		DrawComponent<CircleCollider2DComponent>(CHARACTER_ICON_CIRCLE " CIRCLE COLLIDER 2D", entity, [](auto& component)
		{
			UI::DragFloat2(FA_UP_DOWN_LEFT_RIGHT " Offset", glm::value_ptr(component.Offset));
			UI::DragFloat(FA_BULLSEYE " Radius", &component.Radius);
			UI::DragFloat(FA_WEIGHT_HANGING " Density", &component.Density, 0.01f, 0.0f, 1.0f);
			UI::DragFloat(FA_SHOE_PRINTS " Friction", &component.Friction, 0.01f, 0.0f, 1.0f);
			UI::DragFloat(FA_WAVES_SINE " Restitution", &component.Restitution, 0.01f, 0.0f, 1.0f);
			UI::DragFloat(FA_CIRCLE_STOP " RestitutionThreshold", &component.RestitutionThreshold, 0.01f, 0.0f);
		}, nullptr);

		DrawComponent<StaticMeshComponent>(FILE_ICON_MESH " MESH", entity, [this](auto& component)
		{
			UI::DrawAssetSelectionDropdown(AssetType::Mesh, component.m_Model, [&](Ref<Model> model)
			{
				component.SetModel(model);
			});
				
			if (component.m_Model)
			{
				// Materials Section
				if (UI::CollapsingHeader(FILE_ICON_MATERIAL " Materials"))
				{
					for (uint32_t i = 0; i < component.m_Materials.size(); i++)
					{
						auto& material = component.m_Materials[i];

						ImGui::PushID(i);

						ImGui::Text(FA_CIRCLE_DOT " [Material %d]", i);
						ImGui::SameLine();
						UI::DrawAssetSelectionDropdown(AssetType::Material, material);

						ImGui::PopID();
					}
					ImGui::TreePop();
				}
				
				// Animation Section (only if the mesh has a skeleton)
				if (component.m_Model->GetSkeleton() && UI::CollapsingHeader(FILE_ICON_ANIMATION " Animation"))
				{
					Ref<AnimationGraphPlayer> animationPlayer = component.GetAnimationPlayer();

					UI::DrawAssetSelectionDropdown(AssetType::AnimationGraph, animationPlayer ? animationPlayer->GetAnimationGraph() : nullptr, [&](Ref<AnimationGraph> animationGraph)
					{
						component.SetAnimationGraph(animationGraph);
					});

					if (animationPlayer)
					{
						bool isPaused = animationPlayer->GetIsPaused();
						if (UI::Checkbox(FA_PAUSE " Paused", &isPaused))
							animationPlayer->SetIsPaused(isPaused);

						float playbackSpeed = 1.0f;
						UI::SliderFloat(FA_CLOCK " Playback Speed", &playbackSpeed, 0.0f, 5.0f);

						// Parameters
						auto& parameters = animationPlayer->GetParameters().Parameters;
						if (!parameters.empty())
						{
							if (UI::CollapsingHeader("Parameters"))
							{
								for (auto& [name, data] : parameters)
								{
									ImGui::PushID(name.c_str());
									ImGui::Text(name.c_str());
									ImGui::SameLine();

									switch (data.Type)
									{
									case AnimationGraphDataType::Bool:		ImGui::Checkbox("##AnimationParameterBoolInput", &data.Bool);								break;
									case AnimationGraphDataType::Int:		ImGui::DragInt("##AnimationParameterIntInput", &data.Int);									break;
									case AnimationGraphDataType::Float:		ImGui::DragFloat("##AnimationParameterFloatInput", &data.Float, 0.05f);						break;
									case AnimationGraphDataType::Vector2:	ImGui::DragFloat2("##AnimationParameterVector2Input", glm::value_ptr(data.Vector2), 0.05f);	break;
									case AnimationGraphDataType::Vector3:	ImGui::DragFloat3("##AnimationParameterVector3Input", glm::value_ptr(data.Vector3), 0.05f);	break;
									case AnimationGraphDataType::Vector4:	ImGui::DragFloat4("##AnimationParameterVector4Input", glm::value_ptr(data.Vector4), 0.05f);	break;
									case AnimationGraphDataType::Transform:	UI::DrawTransformControl("##AnimationParameterTransformInput", data.Transform);				break;
									}

									ImGui::PopID();
								}

								ImGui::TreePop();
							}
						}
					}

					ImGui::TreePop();
				}

				// Blend Shape Section
				if (component.m_AnimationGraphPlayer)
				{
					Ref<BlendShapeWeightList> blendShapeWeights = component.m_AnimationGraphPlayer->GetBlendShapeWeights();
					const auto& blendShapes = component.m_Model->GetBlendShapes();
					if (!blendShapeWeights->empty())
					{
						if (UI::CollapsingHeader(FA_FACE_SMILE_WINK " Blend Shapes"))
						{
							for (size_t blendShapeIndex = 0; blendShapeIndex < blendShapes.size() && blendShapeIndex < blendShapeWeights->size(); blendShapeIndex++)
								UI::DragFloat(fmt::format(FA_CIRCLE_DOT " {}", blendShapes[blendShapeIndex]).c_str(), (float*)(&blendShapeWeights->at(blendShapeIndex)), 0.01f, 0.0f, 1.0f);

							ImGui::TreePop();
						}
					}
				}
			}
			
		}, nullptr);

		DrawComponent<DirectionalLightComponent>(CHARACTER_ICON_SUN " DIRECTIONAL LIGHT", entity, [](auto& component)
		{
			UI::ColorEdit3(FA_PALETTE " Color", glm::value_ptr(component.Color));
			UI::DragFloat(FA_BRIGHTNESS " Intensity", &component.Intensity);
		}, nullptr);

		DrawComponent<PointLightComponent>(CHARACTER_ICON_POINT_LIGHT " POINT LIGHT", entity, [](auto& component)
		{
			UI::ColorEdit3(FA_PALETTE " Color", glm::value_ptr(component.Color));
			UI::DragFloat(FA_BRIGHTNESS " Intensity", &component.Intensity);
			UI::DragFloat(FA_BULLSEYE " Radius", &component.Radius);
			UI::Checkbox(FA_ECLIPSE " Casts Shadows", &component.CastsShadows);
		}, nullptr);

		DrawComponent<SpotLightComponent>(CHARACTER_ICON_SPOT_LIGHT " SPOT LIGHT", entity, [](auto& component)
		{
			UI::ColorEdit3(FA_PALETTE " Color", glm::value_ptr(component.Color));
			UI::DragFloat(FA_TRIANGLE " Cut Off", &component.CutOff);
			UI::DragFloat(FA_TRIANGLE " Outer Cut Off", &component.OuterCutOff);
			UI::DragFloat(FA_BRIGHTNESS " Constant", &component.Constant);
			UI::DragFloat(FA_BRIGHTNESS " Linear", &component.Linear);
			UI::DragFloat(FA_BRIGHTNESS " Quadratic", &component.Quadratic);
		}, [](auto& component) {});

		DrawComponent<SkyLightComponent>(CHARACTER_ICON_CLOUDS " SKY LIGHT", entity, [](auto& component)
		{
			const char* skyTypes[] = { FILE_ICON_ENVIRONMENT_MAP " Environment Map", FA_CLOUDS_SUN " Dynamic Sky" };
			UI::Combo(FILE_ICON_ENVIRONMENT_MAP " Type", (int*)&component.Type, skyTypes, IM_ARRAYSIZE(skyTypes));

			if (component.Type == SkyLightComponent::SkyType::EnvironmentMap)
			{
				ImGui::Separator();

				ImGui::Text(FILE_ICON_ENVIRONMENT_MAP " Environment Map");
				UI::DrawAssetSelectionDropdown("##EnvironmentMapAssetSelection", AssetType::EnvironmentMap, component.EnvironmentMap);

				ImGui::Text(FA_ARROW_PROGRESS " Flow Map");
				UI::DrawAssetSelectionDropdown("##FlowMapAssetSelection", AssetType::Texture, component.FlowMap);
			}

			UI::DragFloat(FA_BRIGHTNESS " Intensity", &component.Intensity);

		}, nullptr);

		DrawComponent<DecalComponent>(FA_STAMP " DECAL", entity, [](auto& component)
		{
			bool enabled = true;
			UI::Checkbox(FA_CIRCLE_CHECK " Enabled", &enabled);
			
			ImGui::Text(FILE_ICON_TEXTURE " Texture");
			ImGui::SameLine();
			UI::DrawAssetSelectionDropdown(AssetType::Texture, component.Texture);

			UI::Checkbox(FA_COMPASS_SLASH " Constrain Angle", &component.ConstrainAngle);			
		}, nullptr);

		DrawComponent<VolumeComponent>(CHARACTER_ICON_SMOKE " VOLUME", entity, [](auto& component)
		{
			const char* blendTypes[] = { FA_EQUALS " Set", FA_PLUS " Add" };
			UI::Combo(FA_DROPLET " Blend Type", (int*)&component.Blend, blendTypes, IM_ARRAYSIZE(blendTypes));

			UI::ColorEdit3(FA_PALETTE " Color", glm::value_ptr(component.Color));
			UI::DragFloat(FA_CLOUDS " Scattering Distribution", &component.ScatteringDistribution, 0.01f, 0.0f, 1.0f);
			UI::DragFloat(FA_SUN_DUST " Scattering Intensity", &component.ScatteringIntensity, 0.01f, 0.0f, 1.0f);
			UI::DragFloat(FA_SUNSET " Extinction Scale", &component.ExtinctionScale, 0.01f, 0.0f, 1.0f);
		}, nullptr);

		DrawComponent<PostProcessVolumeComponent>(FA_LAYER_GROUP " POST PROCESS VOLUME", entity, [](auto& component)
		{
			UI::Checkbox(FA_CIRCLE_CHECK " Enabled", &component.Enabled);
			UI::Checkbox(FA_VECTOR_SQUARE " Bounded", &component.Bounded);
			
			ImGui::Text(FILE_ICON_MATERIAL " Material");
			ImGui::SameLine();
			UI::DrawAssetSelectionDropdown(AssetType::Material, component.Material);
		}, nullptr);

		DrawComponent<AudioComponent>(CHARACTER_ICON_AUDIO " AUDIO", entity, [](auto& component)
		{
			UI::DrawAssetSelectionDropdown(AssetType::Audio, component.AudioSound, [&](Ref<Audio> audio){ component.AudioSound = audio; });

			auto& sound = component.AudioSound;
			if (!sound)
				return;

			if (UI::CollapsingHeader(FA_VOLUME " Sound Properties", false))
			{
				bool is3D = sound->Is3D();
				if (UI::Checkbox(FA_CHART_SCATTER_3D " 3D", &is3D))
					sound->SetIs3D(is3D);

				bool isLooping = sound->IsLooping();
				if (UI::Checkbox(FA_REPEAT " Is Looping", &isLooping))
					sound->SetLooping(isLooping);

				bool startOnAwake = component.StartOnAwake;
				if (UI::Checkbox(FA_CIRCLE_PLAY " Start On Awake", &startOnAwake))
					component.StartOnAwake = startOnAwake;

				float radius = sound->GetRadius();
				if (UI::DragFloat(FA_BULLSEYE " Radius", &radius))
					sound->SetRadius(radius);

				{
					ImGui::Text(FA_CLOCK " Play Position");
					ImGui::SameLine();

					int position = sound->IsActive() ? sound->GetPlayPosition() : component.StartPosition;
					ImGui::PushStyleColor(ImGuiCol_Text, {});
					if (ImGui::SliderInt("##PlayPositionSlider", &position, 0, sound->GetPlayLength()))
					{
						if (sound->IsActive())
							sound->SetPlayPosition(position);
						else
							component.StartPosition = position;
					}

					ImGui::PopStyleColor();
					const ImVec2& min = ImGui::GetItemRectMin();
					const ImVec2& max = ImGui::GetItemRectMax();

					int milliseconds = (position / 10) % 1000;
					int seconds = (position / 1000) % 60;
					int minutes = ((position / (1000 * 60)) % 60);

					std::string time = (minutes < 10 ? "0" : "") + std::to_string(minutes) + (seconds < 10 ? " : 0" : " : ") + std::to_string(seconds) + (milliseconds < 10 ? " : 00" : (milliseconds < 100 ? " : 0" : " : ")) + std::to_string(milliseconds);
					ImGui::GetWindowDrawList()->AddText(min + ((max - min - ImGui::CalcTextSize(time.c_str())) * 0.5f), ImGui::GetColorU32(ImGuiCol_Text), time.c_str());
				}

				float volume = sound->GetVolume();
				if (UI::SliderFloat(FA_VOLUME " Volume", &volume, 0.0f, 1.0f))
					sound->SetVolume(volume);

				float pan = sound->GetPan();
				if (UI::SliderFloat(FA_SCALE_UNBALANCED " Pan", &pan, -1.0f, 1.0f))
					sound->SetPan(pan);

				float speed = sound->GetSpeed();
				if (UI::SliderFloat(FA_FORWARD " Speed", &speed, 0.0f, 4.0f))
					sound->SetSpeed(speed);

				bool echo = sound->GetEcho();
				if (UI::Checkbox(FA_MOUNTAIN " Echo", &echo))
					sound->SetEcho(echo);

				ImGui::TreePop();
			}
		}, nullptr);

		DrawComponent<SplineComponent>(FA_BEZIER_CURVE " SPLINE", entity, [](auto& component)
		{
			uint32_t pointIndex = 0;
			for (auto& point : component.Points)
			{
				ImGui::PushID(pointIndex);

				ImGui::TextDisabled(FA_CIRCLE_DOT " [Point %d]", pointIndex);
				ImGui::SameLine();

				if (UI::DrawTextIconButton(FA_TRASH))
					component.RemovePoint(pointIndex);

				ImGui::SameLine();

				if (UI::DrawTextIconButton(FA_COPY))
					component.DuplicatePoint(pointIndex);

				ImGui::Indent();

				UI::DrawVec3Control(FA_LOCATION_DOT " Position", point.Position);
				UI::DrawVec3Control(FA_DASH " Tangent", point.Tangent);

				const char* types[] = { FA_WAVE_SINE " Curve", FA_WAVE_TRIANGLE " Linear", FA_WAVE_SQUARE " Constant" };
				UI::Combo(FA_BARS " Type", (int*)&point.Type, types, IM_ARRAYSIZE(types));

				ImGui::Unindent();
				ImGui::PopID();

				pointIndex++;
			}

			if (ImGui::Button(FA_PLUS " Add Point", ImVec2(ImGui::GetContentRegionAvailWidth(), 35.0f)))
				component.AddPoint();

		}, nullptr);

		DrawComponent<RigidBodyComponent>(CHARACTER_ICON_RIGIDBODY " RIGID BODY", entity, [](auto& component)
		{
			const char* types[] = { FA_LOCK " Static", FA_CUBES_STACKED " Dynamic", FA_PERSON_RUNNING " Kinematic"};
			UI::Combo(FA_GEAR " Type", (int*)&component.Type, types, IM_ARRAYSIZE(types));

			UI::DrawPhysicsLayerSelectionDropdown(component.Layer);

			UI::Checkbox(FA_SENSOR_ON " Sensor", &component.Sensor);

			if (component.Type != RigidBodyComponent::BodyType::Static)
			{
				const char* modes[] = { FA_WEIGHT_HANGING " Density", FA_WEIGHT_SCALE " Mass" };
				UI::Combo(FA_BARS " Mode", (int*)&component.Mode, modes, IM_ARRAYSIZE(modes));
				UI::DragFloat(component.Mode == RigidBodyComponent::MassMode::Density ? FA_WEIGHT_HANGING " Density" : FA_WEIGHT_SCALE " Mass", &component.Density, 0.1f, 0.0f, 0.0f, "%.2f");
			}

			if (UI::CollapsingHeader(FILE_ICON_MATERIAL " Material Properties", false))
			{
				UI::DragFloat(FA_SHOE_PRINTS " Friction", &component.Friction, 0.5f, 0.0f, 1.0f, "%.2f");
				UI::DragFloat(FA_WAVE_SINE " Restitution", &component.Restitution, 0.5f, 0.0f, 1.0f, "%.2f");
				ImGui::TreePop();
			}

		}, nullptr);

		DrawComponent<SoftBodyComponent>(FA_FLAG_SWALLOWTAIL " SOFT BODY", entity, [](auto& component)
		{
			UI::DrawPhysicsLayerSelectionDropdown(component.Layer);

			UI::DragFloat(FA_SHOE_PRINTS " Friction", &component.Friction, 0.5f, 0.0f, 1.0f, "%.2f");
			UI::DragFloat(FA_WAVE_SINE " Restitution", &component.Restitution, 0.5f, 0.0f, 1.0f, "%.2f");
			UI::DragFloat(FA_BALLOON " Pressure", &component.Pressure, 0.5f, 0.0f, 0.0f, "%.2f");

			ImGui::Separator();

			UI::DragFloat(FA_WEIGHT_HANGING " Vertex Mass", &component.VertexMass, 0.5f, 0.0f, 0.0f, "%.2f");
			UI::DragFloat(FA_CIRCLE_DOT " Vertex Radius", &component.VertexRadius, 0.5f, 0.0f, 0.0f, "%.2f");
			UI::Checkbox(FA_PAINTBRUSH " Use Vertex Color As Weight", &component.UseVertexColorAsWeight);
		}, nullptr);

		DrawComponent<RagdollComponent>(FA_PERSON_FALLING " RAGDOLL", entity, [](auto& component)
		{
			UI::DrawPhysicsLayerSelectionDropdown(component.Layer);
		}, nullptr);

		DrawComponent<CharacterMovementComponent>(CHARACTER_ICON_RUNNING " CHARACTER MOVEMENT", entity, [](auto& component)
		{
			// Object Properties
			if (UI::CollapsingHeader(FA_GEAR " Object Properties", false))
			{
				UI::DrawPhysicsLayerSelectionDropdown(component.Layer);

				UI::DragFloat(FA_WEIGHT_HANGING " Mass", &component.Mass, 0.1f, 0.0f, 0.0f, "%.2f");
				UI::DragFloat(CHARACTER_ICON_CAPSULE_COLLIDER " Capsule Radius", &component.CapsuleRadius, 0.1f, 0.0f, 0.0f, "%.2f");
				UI::DragFloat(CHARACTER_ICON_CAPSULE_COLLIDER " Capsule Height", &component.CapsuleHeight, 0.1f, 0.0f, 0.0f, "%.2f");
				UI::DragFloat(FA_PERCENT " Inner Shape Fraction", &component.InnerShapeFraction, 0.1f, 0.0f, 0.0f, "%.2f");

				ImGui::TreePop();
			}
			
			// Character Movement
			if (UI::CollapsingHeader(FA_PERSON_WALKING " Character Movement", false))
			{
				UI::DragFloat(FA_PERSON_RUNNING_FAST " Max Walk Speed", &component.MaxWalkSpeed, 0.1f, 0.0f, 0.0f, "%.2f");
				UI::DragFloat(FA_PERSON_FALLING " Jump Speed", &component.JumpSpeed, 0.1f, 0.0f, 0.0f, "%.2f");
				UI::DragFloat(FA_EARTH_AMERICAS " Gravity Scale", &component.GravityScale, 0.1f, 0.0f, 0.0f, "%.2f");
				UI::DragFloat(FA_ANGLE " Max Slope Angle", &component.MaxSlopeAngle, 0.1f, 0.0f, 0.0f, "%.2f");
				UI::DragFloat(FA_WIND " Air Control", &component.AirControl, 0.1f, 0.0f, 0.0f, "%.2f");
				UI::DragFloat(FA_WAVE_SINE " Velocity Inertia Blend Weight", &component.VelocityBlendWeight, 0.1f, 0.0f, 0.0f, "%.2f");
				UI::Checkbox(FA_COMPASS " Rotate To Motion", &component.RotateToMotion);

				if (component.RotateToMotion)
					UI::DragFloat(FA_ROTATE " Rotation Rate", &component.RotationRate, 0.1f, 0.0f, 0.0f, "%.2f");

				UI::DragFloat(FA_DUMBBELL " Max Strength", &component.MaxStrength, 0.1f, 0.0f, 0.0f, "%.2f");
				UI::DragFloat(FA_SHOE_PRINTS " Ground Friction", &component.Friction, 0.1f, 0.0f, 0.0f, "%.2f");
				UI::DragFloat(FA_STAIRS " Max Step Height", &component.MaxStepHeight, 0.1f, 0.0f, 0.0f, "%.2f");
				UI::DragFloat(FA_STAIRS " Min Step Forward", &component.MinStepForward, 0.1f, 0.0f, 0.0f, "%.2f");
				UI::Checkbox(FA_BOOT " Stick To Floor", &component.StickToFloor);

				ImGui::TreePop();
			}

		}, nullptr);

		DrawComponent<SpringArmComponent>(CHARACTER_ICON_SPRING " SPRING ARM", entity, [](auto& component)
		{
			UI::DragFloat(FA_CROSSHAIRS " Target Length", &component.TargetLength, 0.1f, 0.0f, 0.0f, "%.2f");
			UI::DrawVec3Control(FA_ARROWS_UP_DOWN_LEFT_RIGHT " Target Offset", component.TargetOffset);
			UI::DrawVec3Control(FA_ARROWS_UP_DOWN_LEFT_RIGHT " Socket Offset", component.SocketOffset);
			UI::DragFloat(FA_COMPACT_DISC " Probe Radius", &component.ProbeRadius, 0.1f, 0.0f, 0.0f, "%.2f");
		}, nullptr);

		DrawComponent<FieldComponent>(FA_MAGNET " FIELD", entity, [](auto& component)
		{
			const char* types[] = { CHARACTER_ICON_DIRECTIONAL_FORCE " Directional", CHARACTER_ICON_RADIAL_FORCE " Radial", FA_BUOY_MOORING " Buoyancy" };
			if (UI::Combo(FA_GEAR " Type", (int*)&component.Type, types, IM_ARRAYSIZE(types)))
				component.SetType(component.Type);

			UI::DrawPhysicsLayerSelectionDropdown(component.Layer);

			if (component.Type == FieldComponent::FieldType::Directional)
			{
				UI::DrawVec3Control(FA_WEIGHT_SCALE " Force", component.Force);
			}
			else if (component.Type == FieldComponent::FieldType::Radial)
			{
				UI::DragFloat(FA_WEIGHT_SCALE " Magnitude", &component.Magnitude, 0.1f, 0.0f, 0.0f, "%.2f");
				UI::DragFloat(FA_BULLSEYE " Radius", &component.Radius, 0.1f, 0.0f, 0.0f, "%.2f");
				UI::DragFloat(FA_ARROW_TREND_DOWN " Falloff", &component.Falloff, 0.1f, 0.0f, 0.0f, "%.2f");
			}
			else if (component.Type == FieldComponent::FieldType::Buoyancy)
			{
				UI::DragFloat(FA_WATER " Fluid Buoyancy", &component.Buoyancy, 0.1f, 0.0f, 0.0f, "%.2f");
				UI::DragFloat(FA_GAUGE_SIMPLE_LOW " Linear Drag", &component.LinearDrag, 0.1f, 0.0f, 0.0f, "%.2f");
				UI::DragFloat(FA_GROUP_ARROWS_ROTATE " Angular Drag", &component.AngularDrag, 0.1f, 0.0f, 0.0f, "%.2f");
				UI::DrawVec3Control(FA_HOUSE_FLOOD_WATER_CIRCLE_ARROW_RIGHT " Fluid Velocity", component.FluidVelocity);
			}
		}, nullptr);

		DrawComponent<BoxColliderComponent>(CHARACTER_ICON_BOX_COLLIDER " BOX COLLIDER", entity, [](auto& component)
		{
			UI::DrawVec3Control(FA_EXPAND " Size", component.Size);
		}, nullptr);

		DrawComponent<SphereColliderComponent>(CHARACTER_ICON_SPHERE_COLLIDER " SPHERE COLLIDER", entity, [](auto& component)
		{
			UI::DragFloat(FA_BULLSEYE " Radius", &component.Radius, 0.1f, 0.0f, 0.0f, "%.2f");
		}, nullptr);

		DrawComponent<CapsuleColliderComponent>(CHARACTER_ICON_CAPSULE_COLLIDER " CAPSULE COLLIDER", entity, [](auto& component)
		{
			UI::DragFloat(FA_BULLSEYE " Radius", &component.Radius, 0.1f, 0.0f, 0.0f, "%.2f");
			UI::DragFloat(FA_RULER_VERTICAL " Half Height", &component.HalfHeight, 0.1f, 0.0f, 0.0f, "%.2f");
		}, nullptr);

		DrawComponent<MeshColliderComponent>(CHARACTER_ICON_MESH_COLLIDER " MESH COLLIDER", entity, [](auto& component)
		{
			const char* meshTypes[] = { FA_TRIANGLE " Triangle", FA_VECTOR_POLYGON " Convex" };
			UI::Combo(FA_GEAR " Type", (int*)&component.Type, meshTypes, IM_ARRAYSIZE(meshTypes));
		}, nullptr);

		DrawComponent<PointConstraintComponent>(FA_THUMBTACK " POINT CONSTRAINT", entity, [](auto& component)
		{
			Utils::DrawConstraintTargetInput(component.Target);
			Utils::DrawConstraintSpaceSelectionInput(component.Space);

			UI::DrawVec3Control(FA_ARROWS_UP_DOWN_LEFT_RIGHT " Local Point", component.LocalPoint);
			UI::DrawVec3Control(FA_ARROWS_UP_DOWN_LEFT_RIGHT " Target Point", component.TargetPoint);

		}, nullptr);

		DrawComponent<ConeConstraintComponent>(FA_TRIANGLE " CONE CONSTRAINT", entity, [](auto& component)
		{
			Utils::DrawConstraintTargetInput(component.Target);
			Utils::DrawConstraintSpaceSelectionInput(component.Space);

			UI::DragFloat(FA_ANGLE " Half Cone Angle", &component.HalfConeAngle, 1.0f, 0.0f);

			ImGui::PushID("##ConeLocal");
			ImGui::TextDisabledUnformatted("Local Reference Frame");
			UI::DrawVec3Control(FA_ARROWS_UP_DOWN_LEFT_RIGHT " Offset", component.LocalReferenceFrame.Offset);
			Utils::DrawAxisSelection(FA_ARROW_UP_FROM_DOTTED_LINE " Twist Axis", component.LocalReferenceFrame.TwistAxis, Axis::X);
			ImGui::PopID();

			ImGui::PushID("##ConeTarget");
			ImGui::TextDisabledUnformatted("Target Reference Frame");
			UI::DrawVec3Control(FA_ARROWS_UP_DOWN_LEFT_RIGHT " Offset", component.TargetReferenceFrame.Offset);
			Utils::DrawAxisSelection(FA_ARROW_UP_FROM_DOTTED_LINE " Twist Axis", component.TargetReferenceFrame.TwistAxis, Axis::X);
			ImGui::PopID();

		}, nullptr);

		DrawComponent<DistanceConstraintComponent>(FA_RULER " DISTANCE CONSTRAINT", entity, [](auto& component)
		{
			Utils::DrawConstraintTargetInput(component.Target);

			const char* constraintTypes[] = { FA_ROTATE " Default", FA_DASH " Fixed", FA_ARROWS_LEFT_RIGHT_TO_LINE " Range" };
			if (UI::Combo(FA_GEAR " Type", (int*)&component.Type, constraintTypes, IM_ARRAYSIZE(constraintTypes)))
				component.UpdateType();

			if (component.Type == DistanceConstraintComponent::DistanceType::Fixed)
				UI::DragFloat(FA_RULER_COMBINED " Distance", &component.Distance);
			else if (component.Type == DistanceConstraintComponent::DistanceType::Range)
			{
				UI::DragFloat(FA_CHEVRON_DOWN " Min Distance", &component.MinDistance);
				UI::DragFloat(FA_CHEVRON_UP " Max Distance", &component.MaxDistance);
			}

		}, nullptr);

		DrawComponent<SpringConstraintComponent>(CHARACTER_ICON_SPRING " SPRING CONSTRAINT", entity, [](auto& component)
		{
			const char* types[] = { FA_WAVE_SINE " Frequency And Damping", FA_ANCHOR " Stiffness And Damping" };
			UI::Combo(FA_GEAR " Type", (int*)&component.Type, types, IM_ARRAYSIZE(types));
			UI::DragFloat(FA_WIND " Damping", &component.Damping);

			// Note: Stored in a union so can access either element
			ImGui::DragFloat(component.Type == SpringConstraintComponent::SpringType::FrequencyAndDamping ? FA_WAVE_SINE " Frequency" : FA_ANCHOR " Stiffness", &component.Frequency);

		}, nullptr);

		DrawComponent<HingeConstraintComponent>(FA_ANGLE " HINGE CONSTRAINT", entity, [](auto& component)
		{
			Utils::DrawConstraintTargetInput(component.Target);
			Utils::DrawConstraintSpaceSelectionInput(component.Space);

			ImGui::PushID("##HingeLocal");
			ImGui::TextDisabledUnformatted("Local Reference Frame");
			UI::DrawVec3Control(FA_ARROWS_UP_DOWN_LEFT_RIGHT " Point", component.LocalReferenceFrame.Point);
			Utils::DrawAxisSelection(FA_ROTATE " Hinge Axis", component.LocalReferenceFrame.HingeAxis, Axis::Y);
			Utils::DrawAxisSelection(FA_ARROW_UP_FROM_LINE " Normal Axis", component.LocalReferenceFrame.NormalAxis, Axis::X);
			ImGui::PopID();

			ImGui::PushID("##HingeTarget");
			ImGui::TextDisabledUnformatted("Target Reference Frame");
			UI::DrawVec3Control(FA_ARROWS_UP_DOWN_LEFT_RIGHT " Point", component.TargetReferenceFrame.Point);
			Utils::DrawAxisSelection(FA_ROTATE " Hinge Axis", component.TargetReferenceFrame.HingeAxis, Axis::Y);
			Utils::DrawAxisSelection(FA_ARROW_UP_FROM_LINE " Normal Axis", component.TargetReferenceFrame.NormalAxis, Axis::X);
			ImGui::PopID();

			UI::DragFloat(FA_CHEVRON_DOWN " Min Rotation", &component.MinRotation, 1.0f, -180.0f, 0.0f);
			UI::DragFloat(FA_CHEVRON_UP " Max Rotation", &component.MaxRotation, 1.0f, 0.0f, 180.0f);

			UI::DragFloat(FA_GEAR " Maximum Friction Torque", &component.MaxFrictionTorque);

		}, nullptr);

		DrawComponent<FixedConstraintComponent>(FA_OBJECT_UNION " FIXED CONSTRAINT", entity, [](auto& component)
		{
			Utils::DrawConstraintTargetInput(component.Target);
			Utils::DrawConstraintSpaceSelectionInput(component.Type, true);
		}, nullptr);

		DrawComponent<GearConstraintComponent>(FA_GEARS " GEAR CONSTRAINT", entity, [](auto& component)
		{
			Utils::DrawConstraintTargetInput(component.Target);
			Utils::DrawConstraintSpaceSelectionInput(component.Space);
			
			Utils::DrawAxisSelection(FA_ARROWS_ROTATE " Local Hinge Axis", component.LocalHingeAxis, Axis::X);
			Utils::DrawAxisSelection(FA_ARROWS_ROTATE " Target Hinge Axis",component.TargetHingeAxis, Axis::X);

			Utils::DrawFractionInput(FA_PERCENT " Ratio", component.Ratio);

		}, nullptr);

		DrawComponent<PulleyConstraintComponent>(FA_CIRCLE_NOTCH " PULLY CONSTRAINT", entity, [](auto& component)
		{
			Utils::DrawConstraintTargetInput(component.Target);
			Utils::DrawConstraintSpaceSelectionInput(component.Space);

			ImGui::Separator();

			ImGui::PushID("##Local");
			ImGui::TextDisabledUnformatted("Local Reference Frame");
			UI::DrawVec3Control(FA_LOCATION_DOT " Body Point", component.LocalReferenceFrame.BodyPoint);
			UI::DrawVec3Control(FA_WRENCH " Fixed Point", component.LocalReferenceFrame.FixedPoint);
			ImGui::PopID();

			ImGui::PushID("##Target");
			ImGui::TextDisabledUnformatted("Local Reference Frame");
			UI::DrawVec3Control(FA_LOCATION_DOT " Body Point", component.TargetReferenceFrame.BodyPoint);
			UI::DrawVec3Control(FA_WRENCH " Fixed Point", component.TargetReferenceFrame.FixedPoint);
			ImGui::PopID();

			ImGui::Separator();

			Utils::DrawFractionInput(FA_PERCENT " Ratio", component.Ratio);
			Utils::DrawFlaggedFloatInput(FA_CHEVRON_DOWN " Minimum Length", &component.MinLength, PulleyConstraintComponent::AutomaticLengthCalculationFlag, 0.0f);
			Utils::DrawFlaggedFloatInput(FA_CHEVRON_UP " Maximum Length", &component.MaxLength, PulleyConstraintComponent::AutomaticLengthCalculationFlag, 0.0f);

		}, nullptr);

		DrawComponent<RackAndPinionConstraintComponent>(FA_GEAR_COMPLEX " RACK AND PINION CONSTRAINT", entity, [](auto& component)
		{
			Utils::DrawConstraintTargetInput(component.Target);
			Utils::DrawConstraintSpaceSelectionInput(component.Space);

			ImGui::Separator();

			Utils::DrawAxisSelection(FA_ROTATE " Hinge Axis", component.HingeAxis, Axis::X);
			Utils::DrawAxisSelection(FA_LEFT_RIGHT " Slider Axis", component.SliderAxis, Axis::X);

			ImGui::Separator();

			const char* modes[] = { FA_SLIDERS " Properties", FA_PERCENT " Ratio" };
			if (UI::Combo(FA_GEAR " Ratio Mode", (int*)&component.Mode, modes, IM_ARRAYSIZE(modes)))
				component.UpdateMode();

			if (component.Mode == RackAndPinionConstraintComponent::RatioMode::Properties)
			{
				UI::DragU32(FA_GEARS " Rack Teeth Count", &component.RackTeethCount);
				UI::DragU32(FA_GEARS " Pinion Teeth Count", &component.PinionTeethCount);
				UI::DragFloat(FA_RULER " Rack Length", &component.RackLength);
			}
			else
			{
				UI::DragFloat(FA_PERCENT " Ratio", &component.Ratio);
			}

		}, nullptr);

		DrawComponent<SwingTwistConstraintComponent>(FA_SHUFFLE " SWING TWIST CONSTRAINT", entity, [](auto& component)
		{
			Utils::DrawConstraintTargetInput(component.Target);
			Utils::DrawConstraintSpaceSelectionInput(component.Space);
			Utils::DrawConstraintSwingTypeSelectionInput(component.SwingType);

			ImGui::Separator();

			ImGui::PushID("##Local");
			ImGui::TextDisabledUnformatted("Local Reference Frame");
			UI::DrawVec3Control(FA_LOCATION_DOT " Position", component.LocalReferenceFrame.Position);
			Utils::DrawAxisSelection(FA_SHUFFLE " Twist Axis", component.LocalReferenceFrame.TwistAxis, Axis::X);
			Utils::DrawAxisSelection(FA_ARROW_UP_FROM_DOTTED_LINE " Plane Axis", component.LocalReferenceFrame.PlaneAxis, Axis::Y);
			ImGui::PopID();

			ImGui::PushID("##Target");
			ImGui::TextDisabledUnformatted("Target Reference Frame");
			UI::DrawVec3Control(FA_LOCATION_DOT " Position", component.TargetReferenceFrame.Position);
			Utils::DrawAxisSelection(FA_SHUFFLE " Twist Axis", component.TargetReferenceFrame.TwistAxis, Axis::X);
			Utils::DrawAxisSelection(FA_ARROW_UP_FROM_DOTTED_LINE " Plane Axis", component.TargetReferenceFrame.PlaneAxis, Axis::Y);
			ImGui::PopID();

			ImGui::Separator();

			UI::DragFloat(FA_ANGLE " Normal Half Cone Angle", &component.NormalHalfConeAngle);
			UI::DragFloat(FA_ANGLE " Plane Half Cone Angle", &component.PlaneHalfConeAngle);
			UI::DragFloat(FA_CHEVRON_DOWN " Minimum Twist Angle", &component.TwistMinAngle);
			UI::DragFloat(FA_CHEVRON_UP " Maximum Twist Angle", &component.TwistMaxAngle);
			UI::DragFloat(FA_GEAR " Maximum Friction Torque", &component.MaxFrictionTorque);

		}, nullptr);

		DrawComponent<SliderConstraintComponent>(FA_GRIP_LINES " SLIDER CONSTRAINT", entity, [](auto& component)
		{
			Utils::DrawConstraintTargetInput(component.Target);
			Utils::DrawConstraintSpaceSelectionInput(component.Space, true);

			ImGui::Separator();
			
			ImGui::PushID("##SliderLocal");
			ImGui::TextDisabledUnformatted("Local Reference Frame");

			if (component.Space != ConstraintSpace::Automatic)
				UI::DrawVec3Control(FA_ARROWS_UP_DOWN_LEFT_RIGHT " Point", component.LocalReferenceFrame.Point);

			Utils::DrawAxisSelection(FA_LEFT_RIGHT " Slider Axis", component.LocalReferenceFrame.SliderAxis, Axis::X);
			Utils::DrawAxisSelection(FA_ARROW_UP_FROM_LINE " Normal Axis", component.LocalReferenceFrame.NormalAxis, Axis::Y);
			ImGui::PopID();

			ImGui::Separator();

			ImGui::PushID("##SliderTarget");
			ImGui::TextDisabledUnformatted("Target Reference Frame");

			if (component.Space != ConstraintSpace::Automatic)
				UI::DrawVec3Control(FA_ARROWS_UP_DOWN_LEFT_RIGHT " Point", component.TargetReferenceFrame.Point);

			Utils::DrawAxisSelection(FA_LEFT_RIGHT " Slider Axis", component.TargetReferenceFrame.SliderAxis, Axis::X);
			Utils::DrawAxisSelection(FA_ARROW_UP_FROM_LINE " Normal Axis", component.TargetReferenceFrame.NormalAxis, Axis::Y);
			ImGui::PopID();

			ImGui::Separator();

			// Constraint min/max limits
			Utils::DrawFlaggedFloatInput(FA_CHEVRON_DOWN " Slider Min", &component.SliderMin, -FLT_MAX, 0.0f, -FLT_MAX, 0.0f);
			Utils::DrawFlaggedFloatInput(FA_CHEVRON_UP " Slider Max", &component.SliderMax, FLT_MAX, 0.0f, 0.0f, FLT_MAX);

			UI::DragFloat(FA_RIGHT_LEFT " Max Friction Force", &component.MaxFrictionForce);

		}, nullptr);

		DrawComponent<SixDOFConstraintComponent>(FA_360_DEGREES " SIX DOF CONSTRAINT", entity, [](auto& component)
		{
			Utils::DrawConstraintTargetInput(component.Target);
			Utils::DrawConstraintSpaceSelectionInput(component.Space);
			Utils::DrawConstraintSwingTypeSelectionInput(component.SwingType);

			ImGui::Separator();

			ImGui::PushID("##Local");
			ImGui::TextDisabled("Local Reference Frame");
			UI::DrawVec3Control(FA_LOCATION_DOT " Position", component.LocalReferenceFrame.Position);
			Utils::DrawAxisSelection(FA_CIRCLE_X " Axis X", component.LocalReferenceFrame.AxisX, Axis::X);
			Utils::DrawAxisSelection(FA_CIRCLE_Y " Axis Y", component.LocalReferenceFrame.AxisY, Axis::Y);
			ImGui::PopID();

			ImGui::PushID("##Target");
			ImGui::TextDisabled("Target Reference Frame");
			UI::DrawVec3Control(FA_LOCATION_DOT " Position", component.TargetReferenceFrame.Position);
			Utils::DrawAxisSelection(FA_CIRCLE_X " Axis X", component.TargetReferenceFrame.AxisX, Axis::X);
			Utils::DrawAxisSelection(FA_CIRCLE_Y " Axis Y", component.TargetReferenceFrame.AxisY, Axis::Y);
			ImGui::PopID();

			ImGui::Separator();

			const char* axisLabels[] = {
				FA_CIRCLE_X " Translation",
				FA_CIRCLE_Y " Translation",
				FA_CIRCLE_Z " Translation",
				FA_CIRCLE_X " Rotation",
				FA_CIRCLE_Y " Rotation",
				FA_CIRCLE_Z " Rotation"
			};

			for (uint32_t axisIndex = 0; axisIndex < SixDOFConstraintComponent::AxisCount; axisIndex++)
			{
				if (UI::CollapsingHeader(axisLabels[axisIndex]))
				{
					const SixDOFConstraintComponent::Axis axis = (SixDOFConstraintComponent::Axis)axisIndex;
					const SixDOFConstraintComponent::AxisStatus axisStatus = component.GetAxisStatus(axis);
					const char* axisStatuses[] = { FA_UNLOCK " Free", FA_LOCK " Locked", FA_GEAR " Custom" };
					if (UI::Combo(FA_LINK " Axis Constraint", (int*)&axisStatus, axisStatuses, IM_ARRAYSIZE(axisStatuses)))
						component.SetAxisStatus(axis, axisStatus);

					const bool translation = (axisIndex <= SixDOFConstraintComponent::Axis::TranslationZ);
					const float min = translation ? 0.0f : -180.0f;
					const float max = translation ? 0.0f : 180.0f;

					UI::DragFloat(FA_GRIP_LINES_VERTICAL " Max Friction", &component.MaxFriction[axis]);

					if (axisStatus == SixDOFConstraintComponent::AxisStatus::Custom)
					{
						UI::DragFloat(translation ? FA_CHEVRON_DOWN " Min Translation" : FA_ANGLE " Min Rotation", &component.LimitMin[axis], 1.0f, min, max);
						UI::DragFloat(translation ? FA_CHEVRON_UP " Max Translation" : FA_360_DEGREES " Max Rotation", &component.LimitMax[axis], 1.0f, min, max);
					}

					ImGui::TreePop();
				}
		 	}

		}, nullptr);

		DrawComponent<FollowConstraintComponent>(FA_ROUTE " FOLLOW CONSTRAINT", entity, [](auto& component)
		{
			Utils::DrawConstraintTargetInput(component.Target);
			UI::Checkbox(FA_REPEAT " Looping", &component.Looping);

			Utils::DrawAxisSelection(FA_ARROW_UP " Normal", component.Normal, Axis::Y);
			UI::DragFloat(FA_PERCENT " Start Fraction", &component.StartFraction);
			UI::DragFloat(FA_RIGHT_LEFT " Max Friction Force##Follow", &component.MaxFrictionForce);

			const char* types[] =
			{
				FA_ARROWS_UP_DOWN_LEFT_RIGHT " Free",
				FA_ARROW_RIGHT " Around Tangent",
				FA_ARROWS_TO_DOTTED_LINE " Around Normal",
				FA_ARROWS_TO_DOT " Around Binormal",
				FA_BEZIER_CURVE " To Path",
				FA_LOCK " Constrained",
			};

			UI::Combo(FA_ROTATE " Rotation Constraint", (int*)&component.RotationConstraint, types, IM_ARRAYSIZE(types));

			ImGui::TextUnformatted(FA_BULLSEYE " Base Entity");
			ImGui::SameLine();
			ImGui::TextDisabledUnformatted("(Optional)");
			ImGui::SameLine();
			UI::DrawEntitySelectionInput("##FollowConstraintBase", component.BaseTarget);

			if (UI::CollapsingHeader(FA_ENGINE " Motor"))
			{
				const char* states[] = { FA_BAN " Off", FA_GAUGE " Velocity", FA_LOCATION_DOT " Position" };
				UI::Combo(FA_GEAR " State", (int*)&component.Motor.MotorState, states, IM_ARRAYSIZE(states));

				UI::DragFloat(FA_GAUGE " Target Velocity", &component.TargetVelocity, -10.0f, 10.0f, 0.1f);
				UI::DragFloat(FA_PERCENT " Target Path Fraction", &component.TargetPathFraction, 0.0f, 1.0f, 0.01f);
				UI::DragFloat(FA_ROCKET_LAUNCH " Max Acceleration", &component.Motor.MaxMotorAcceleration, 0.0f, 100.0f, 1.0f);
				UI::DragFloat(FA_WAVE_SINE " Frequency", &component.Motor.Frequency, 0.0f, 20.0f, 0.1f);
				UI::DragFloat(FA_HAND " Damping", &component.Motor.Damping, 0.0f, 2.0f, 0.01f);
				UI::DragFloat(FA_GRIP_LINES_VERTICAL " Max Friction Acceleration", &component.MaxFrictionAcceleration, 0.0f, 10.0f, 0.1f);

				ImGui::TreePop();
			}

		}, nullptr);

		DrawComponent<LandscapeComponent>(FA_MOUNTAIN_SUN " LANDSCAPE", entity, [](auto& component)
		{
			ImGui::Button(FA_PEN " Edit", ImVec2(ImGui::GetContentRegionAvailWidth(), 35.0f));

			ImGui::TextDisabledUnformatted(FA_EXPAND " Resolution");

			if (UI::DragU32("X##Landscape", &component.Resolution.x))
				component.Allocate();

			if (UI::DragU32("Y##Landscape", &component.Resolution.y))
				component.Allocate();

			ImGui::TextDisabledUnformatted(FILE_ICON_MATERIAL " Material");
			UI::DrawAssetSelectionDropdown(AssetType::Material, component.Material);

			UI::Checkbox(FA_CUBES_STACKED " Heightfield Physics", &component.Physics);

			if (component.Physics)
				UI::DrawPhysicsLayerSelectionDropdown(component.Layer);

		}, nullptr);

		DrawComponent<NavigationMeshComponent>(FA_MAP_LOCATION_DOT " NAVIGATION MESH", entity, [](auto& component)
		{
		}, nullptr);

		DrawComponent<NavigationModifierComponent>(FA_LOCATION_PLUS " NAVIGATION MODIFIER", entity, [](auto& component)
		{
		}, nullptr);

		DrawComponent<NavigationLinkComponent>(FA_LINK " NAVIGATION LINK", entity, [](auto& component)
		{
		}, nullptr);

		DrawComponent<UICanvasComponent>(CHARACTER_ICON_CANVAS " UI CANVAS", entity, [](auto& component)
		{
			UI::Checkbox(FA_CIRCLE_CHECK " Enabled", &component.Enabled);
			UI::DragFloat2(FA_SQUARE_DOWN_LEFT " Minimum", glm::value_ptr(component.Min), 0.05f, 0.0f, 0.0f, "%.2f");
			UI::DragFloat2(FA_SQUARE_UP_RIGHT " Maximum", glm::value_ptr(component.Max), 0.05f, 0.0f, 0.0f, "%.2f");
		}, nullptr);

		DrawComponent<UIImageComponent>(CHARACTER_ICON_IMAGE " UI IMAGE", entity, [this](auto& component)
		{
			UI::DragFloat2(FA_ANCHOR " Anchor", glm::value_ptr(component.Anchor), 0.05f, 0.0f, 0.0f, "%.2f");
			UI::DragFloat2(FA_UP_DOWN_LEFT_RIGHT " Position", glm::value_ptr(component.Position), 0.25f, 0.0f, 0.0f, "%.2f");
			UI::DragFloat2(FA_EXPAND " Size", glm::value_ptr(component.Size), 0.25f, 0.0f, 0.0f, "%.2f");
			UI::Image(component.Image ? component.Image : EditorResources::CheckerboardTexture, { 100.0f, 100.0f });
			
			UI::DrawAssetSelectionDropdown(AssetType::Texture, component.Image);

		}, nullptr);

		DrawComponent<UIButtonComponent>(CHARACTER_ICON_BUTTON " UI BUTTON", entity, [](auto& component)
		{
		}, nullptr);

		DrawComponent<FolderComponent>(CHARACTER_ICON_FOLDER " FOLDER SETTINGS", entity, [](auto& component)
		{
			// Folder color picker
			ImGui::TextDisabledUnformatted(FA_PALETTE " Folder Color");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(-1);
			ImGui::ColorEdit3("##FolderColorPicker", glm::value_ptr(component.Color));

			constexpr ImVec4 presets[] = {
				{ 1.0f, 1.0f, 1.0f, 1.0f }, // White
				{ 0.7f, 0.7f, 0.7f, 1.0f }, // Gray
				{ 0.8f, 0.1f, 0.2f, 1.0f }, // Red
				{ 0.8f, 0.4f, 0.2f, 1.0f }, // Orange
				{ 1.0f, 0.9f, 0.3f, 1.0f }, // Yellow
				{ 0.4f, 0.8f, 0.4f, 1.0f }, // Green
				{ 0.2f, 0.7f, 1.0f, 1.0f }, // Blue
				{ 0.7f, 0.5f, 1.0f, 1.0f }, // Purple
				{ 1.0f, 0.6f, 1.0f, 1.0f }, // Pink
				{ 0.6f, 0.4f, 0.2f, 1.0f }, // Brown
			};

			// Draw presets
			ImGui::TextDisabledUnformatted(FA_GEAR " Presets");
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { (ImGui::GetContentRegionAvailWidth() / 10.0f - ImGui::GetFrameHeight() - ImGui::GetStyle().FramePadding.x), 0.0f });

			for (uint32_t presetIndex = 0; presetIndex < IM_ARRAYSIZE(presets); presetIndex++)
			{
				ImGui::PushID(presetIndex);
				ImGui::SameLine();

				if (ImGui::ColorButton("##FolderPreset", presets[presetIndex]))
					component.Color = presets[presetIndex];

				ImGui::PopID();
			}

			ImGui::PopStyleVar();

		}, nullptr, false);

		if (entityDeleted)
			DeleteEntity(entity);
	}

	static AssetType GetScriptFieldTypeAssetType(ScriptFieldType type)
	{
		switch (type)
		{
		case ScriptFieldType::Asset: return AssetType::None;
		case ScriptFieldType::Scene: return AssetType::Scene;
		case ScriptFieldType::Texture: return AssetType::Texture;
		case ScriptFieldType::VirtualTexture: return AssetType::VirtualTexture;
		case ScriptFieldType::Mesh: return AssetType::Mesh;
		case ScriptFieldType::Animation: return AssetType::Animation;
		case ScriptFieldType::Material: return AssetType::Material;
		case ScriptFieldType::Audio: return AssetType::Audio;
		case ScriptFieldType::VideoPlayer: return AssetType::VideoPlayer;
		}

		return AssetType::None;
	}

	bool SceneHierarchyPanel::DrawScriptField(const Entity& entity, const std::string& name, const ScriptField& field, void* data)
	{
		bool returnValue = false;

		if (field.Type == ScriptFieldType::None)
		{
			ImGui::TextDisabled(name.c_str());
		}
		else if (field.Type == ScriptFieldType::Float)
		{
			returnValue = ImGui::DragFloat(name.c_str(), (float*)data);
		}
		else if (field.Type == ScriptFieldType::Double)
		{
			returnValue = ImGui::InputDouble(name.c_str(), (double*)data);
		}
		else if (field.Type == ScriptFieldType::Bool)
		{
			returnValue = ImGui::Checkbox(name.c_str(), (bool*)data);
		}
		else if (field.Type == ScriptFieldType::Char)
		{

			uint32_t val = *((uint16_t*)data);

			char buf[64];
			ImFormatString(buf, IM_ARRAYSIZE(buf), "#%08X", val);

			if (ImGui::InputText(name.c_str(), buf, IM_ARRAYSIZE(buf), ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase))
			{
				char* p = buf;
				while (*p == '#' || ImCharIsBlankA(*p))
					p++;

				sscanf(p, "%08X", &val);

				*((uint16_t*)data) = val;
				returnValue = true;
			}
		}
		else if (field.Type == ScriptFieldType::Byte)
		{
			static const uint8_t min = 0;
			static const uint8_t max = 255;
			returnValue = ImGui::DragScalar(name.c_str(), ImGuiDataType_U8, data, 1.0f, &min, &max, "%d", ImGuiSliderFlags_AlwaysClamp);
		}
		else if (field.Type == ScriptFieldType::Short)
		{
			returnValue = ImGui::DragScalar(name.c_str(), ImGuiDataType_S16, data, 1.0f, nullptr, nullptr, "%d", ImGuiSliderFlags_AlwaysClamp);
		}
		else if (field.Type == ScriptFieldType::Int)
		{
			returnValue = ImGui::DragScalar(name.c_str(), ImGuiDataType_S32, data, 1.0f, nullptr, nullptr, "%d", ImGuiSliderFlags_AlwaysClamp);
		}
		else if (field.Type == ScriptFieldType::Long)
		{
			returnValue = ImGui::DragScalar(name.c_str(), ImGuiDataType_S64, data, 1.0f, nullptr, nullptr, "%lld", ImGuiSliderFlags_AlwaysClamp);
		}
		else if (field.Type == ScriptFieldType::UShort)
		{
			returnValue = ImGui::DragScalar(name.c_str(), ImGuiDataType_U16, data, 1.0f, nullptr, nullptr, "%hu", ImGuiSliderFlags_AlwaysClamp);
		}
		else if (field.Type == ScriptFieldType::UInt)
		{
			returnValue = ImGui::DragScalar(name.c_str(), ImGuiDataType_U32, data, 1.0f, nullptr, nullptr, "%u", ImGuiSliderFlags_AlwaysClamp);
		}
		else if (field.Type == ScriptFieldType::ULong)
		{
			returnValue = ImGui::DragScalar(name.c_str(), ImGuiDataType_U64, data, 1.0f, nullptr, nullptr, "%llu", ImGuiSliderFlags_AlwaysClamp);
		}
		else if (field.Type == ScriptFieldType::Vector2)
		{
			returnValue = ImGui::DragFloat2(name.c_str(), glm::value_ptr(*(glm::vec2*)(data)));
		}
		else if (field.Type == ScriptFieldType::Vector3)
		{
			returnValue = ImGui::DragFloat3(name.c_str(), glm::value_ptr(*(glm::vec3*)(data)));
		}
		else if (field.Type == ScriptFieldType::Vector4)
		{
			returnValue = ImGui::DragFloat4(name.c_str(), glm::value_ptr(*(glm::vec4*)(data)));
		}
		else if (field.Type == ScriptFieldType::Entity)
		{
			uint64_t uuid = *(uint64_t*)(data);
			if (UI::DrawEntitySelectionInput(name.c_str(), uuid) && entity == m_ActiveEntity)
			{
				*(uint64_t*)(data) = uuid;
				returnValue = true;
			}
			ImGui::SameLine();
			ImGui::Text(name.c_str());
		}
		else if (field.Type == ScriptFieldType::Asset || GetScriptFieldTypeAssetType(field.Type) != AssetType::None)
		{
			ImGui::Text(name.c_str());
			ImGui::SameLine();
			ImGui::PushID(name.c_str());
			UI::DrawAssetSelectionDropdown(GetScriptFieldTypeAssetType(field.Type), *(uint64_t*)data, [&](UUID handle) { *(uint64_t*)data = handle; returnValue = true; });
			ImGui::PopID();
		}

		return returnValue;
	}

	template<typename T>
	void SceneHierarchyPanel::DisplayAddComponentEntry(const std::string& entryName)
	{
		if (!m_ActiveEntity.HasComponent<T>())
		{
			if (ImGui::MenuItem(entryName.c_str()))
			{
				m_ActiveEntity.AddComponent<T>();
				ImGui::CloseCurrentPopup();
			}
		}
	}

	template<typename T>
	void SceneHierarchyPanel::DisplayCreateEntityEntry(const std::string& label, const std::string& entityName)
	{
		if (ImGui::MenuItem(label.c_str()))
		{
			Entity entity = m_Context->CreateEntity(entityName.c_str());
			entity.AddComponent<T>();
			SelectedEntity(entity);
		}
	}

	void SceneHierarchyPanel::DisplayCreateEntityPopup()
	{
		ImGui::TextDisabled(CHARACTER_ICON_ADD " Create");
		ImGui::Separator();

		if (ImGui::MenuItem(CHARACTER_ICON_EMPTY " Empty Entity"))
			SelectedEntity(m_Context->CreateEntity("Empty Entity"));

		DisplayCreateEntityEntry<FolderComponent>(CHARACTER_ICON_FOLDER " Folder", "Folder");

		ImGui::Separator();

		DisplayCreateEntityEntry<StaticMeshComponent>(FILE_ICON_MESH " Mesh", "Mesh");

		if (ImGui::BeginMenu(CHARACTER_ICON_POINT_LIGHT " Light"))
		{
			DisplayCreateEntityEntry<DirectionalLightComponent>(CHARACTER_ICON_SUN " Directional Light", "Directional Light");
			DisplayCreateEntityEntry<PointLightComponent>(CHARACTER_ICON_POINT_LIGHT " Point Light", "Point Light");
			DisplayCreateEntityEntry<SpotLightComponent>(CHARACTER_ICON_SPOT_LIGHT " Spot Light", "Spot Light");
			DisplayCreateEntityEntry<SkyLightComponent>(CHARACTER_ICON_CLOUDS " Sky Light", "Sky Light");

			ImGui::Separator();

			DisplayCreateEntityEntry<VolumeComponent>(CHARACTER_ICON_SMOKE " Volume", "Volume");

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu(FA_HURRICANE " Effects"))
		{
			DisplayCreateEntityEntry<DecalComponent>(FA_STAMP " Decal", "Decal");
			DisplayCreateEntityEntry<PostProcessVolumeComponent>(FA_LAYER_GROUP " Post Process Volume", "Post Process Volume");
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu(FA_COMPASS " Navigation"))
		{
			DisplayCreateEntityEntry<NavigationMeshComponent>(FA_MAP_LOCATION_DOT " Navigation Mesh", "Navigation Mesh");
			DisplayCreateEntityEntry<NavigationModifierComponent>(FA_LOCATION_PLUS " Navigation Modifier", " Navigation Modifier");
			DisplayCreateEntityEntry<NavigationLinkComponent>(FA_LINK " Navigation Link", "Navigation Link");
			ImGui::EndMenu();
		}

		ImGui::Separator();

		if (ImGui::BeginMenu(CHARACTER_ICON_IMAGE " 2D"))
		{
			DisplayCreateEntityEntry<SpriteRendererComponent>(CHARACTER_ICON_IMAGE " Sprite", "Sprite");
			DisplayCreateEntityEntry<CircleRendererComponent>(CHARACTER_ICON_SHADING_UNLIT " Circle", "Circle");
			DisplayCreateEntityEntry<TextComponent>(FILE_ICON_FONT " Text", "Text");

			ImGui::EndMenu();
		}

		ImGui::Separator();

		DisplayCreateEntityEntry<FieldComponent>(FA_MAGNET " Field", "Field");
		DisplayCreateEntityEntry<ParticleSystemComponent>(CHARACTER_ICON_PARTICLES " Particle System", "Particle System");
		DisplayCreateEntityEntry<AudioComponent>(CHARACTER_ICON_AUDIO " Audio", "Audio");
		DisplayCreateEntityEntry<SplineComponent>(FA_BEZIER_CURVE " Spline", "Spline");
		DisplayCreateEntityEntry<LandscapeComponent>(FA_MOUNTAIN_SUN " Landscape", "Landscape");
		DisplayCreateEntityEntry<CameraComponent>(CHARACTER_ICON_CAMERA " Camera", "Camera");

		ImGui::Separator();

		DisplayCreateEntityEntry<ScriptComponent>(FILE_ICON_SCRIPT " Script", "Script");
	}

}