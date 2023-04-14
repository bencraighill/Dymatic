#include "SceneHierarchyPanel.h"
#include "Dymatic/Scene/Components.h"

#include "Dymatic/Scripting/ScriptEngine.h"
#include "Dymatic/Asset/AssetManager.h"

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

	static bool s_Init = false;
	static Ref<Texture2D> s_SearchbarIcon;
	static Ref<Texture2D> s_ClearIcon;
	static Ref<Texture2D> s_EntityIcon;
	static Ref<Texture2D> s_FolderIcon;

	SceneHierarchyPanel::SceneHierarchyPanel()
	{
		if (!s_Init)
		{
			s_Init = true;

			s_SearchbarIcon = Texture2D::Create("Resources/Icons/SceneHierarchy/SearchbarIcon.png");
			s_ClearIcon = Texture2D::Create("Resources/Icons/SceneHierarchy/ClearIcon.png");
			s_EntityIcon = Texture2D::Create("Resources/Icons/Properties/ComponentIcons/SceneIconEmptyEntity.png");
			s_FolderIcon = Texture2D::Create("Resources/Icons/SceneHierarchy/FolderIcon.png");
		}
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

		// Display Scene Hierarchy Panel
		if (auto& sceneHierarchyVisible = Preferences::GetEditorWindowVisible(Preferences::EditorWindow::SceneHierarchy))
		{
			ImGui::Begin(CHARACTER_ICON_WORLD " World Settings");
			ImGui::End();

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
				ImGui::Image((ImTextureID)s_SearchbarIcon->GetRendererID(), size, { 0, 1 }, { 1, 0 }, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
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
					ImGui::Image((ImTextureID)s_ClearIcon->GetRendererID(), size, { 0, 1 }, { 1, 0 }, color);
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

				// Display Main Items
				if (m_Context)
				{
					m_Context->m_Registry.each([&](auto entityID)
						{
							Entity entity{ entityID , m_Context.get() };
							if (!m_Context->IsEntityParented(entity))
								DrawEntityNode(entity);
						});

				}

				if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
				{
					m_ActiveEntity = {};
					m_Context->ClearSelectedEntities();
				}

				// Create Entity Context Popup
				if (ImGui::BeginPopupContextWindow(0, 1, false))
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

		// Draw properties panel
		if (auto& propertiesVisible = Preferences::GetEditorWindowVisible(Preferences::EditorWindow::Properties))
		{
			ImGui::Begin(CHARACTER_ICON_PROPERTIES " Properties", &propertiesVisible);
			if (m_ActiveEntity)
			{
				DrawComponents(m_ActiveEntity);
			}

			ImGui::End();
		}

		// Reset Picker after use
		if ((Input::IsKeyPressed(Key::Escape) || Input::IsMouseButtonPressed(Mouse::ButtonLeft)) && m_PickingID != 0)
		{
			m_PickingID = 0;
			m_PickingField.clear();
		}
	}

	void SceneHierarchyPanel::SelectedEntity(Entity entity)
	{
		m_ActiveEntity = entity;

		if (Input::IsKeyPressed(Key::LeftShift) || Input::IsKeyPressed(Key::RightShift))
			m_Context->AddSelectedEntity(entity);
		else
			m_Context->SetSelectedEntity(entity);
	}

	void SceneHierarchyPanel::DuplicateEntities()
	{
		auto& entities = GetSelectedEntities();
		std::unordered_set<entt::entity> newEntities;
		newEntities.reserve(entities.size());

		for (auto& entity : entities)
			newEntities.insert(m_Context->DuplicateEntity({ entity, m_Context.get() }));
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

		m_ActiveEntity = {};
		m_Context->ClearSelectedEntities();
	}

	void SceneHierarchyPanel::DrawEntityNode(Entity entity)
	{
		if (entity.HasComponent<SceneComponent>())
			return;

		if (!m_SearchBuffer.empty())
		{
			std::string name = entity.GetName();
			std::string search = m_SearchBuffer;
			transform(name.begin(), name.end(), name.begin(), ::tolower);
			transform(search.begin(), search.end(), search.begin(), ::tolower);
			if (name.find(search) == std::string::npos)
				return;
		}

		ImGui::PushID(entity.GetUUID());

		// Setup Row
		ImGui::TableNextRow();
		ImGui::TableNextColumn();

		auto& tag = entity.GetComponent<TagComponent>().Tag;

		if (entity.HasComponent<FolderComponent>())
		{
			auto& color = entity.GetComponent<FolderComponent>().Color;
			ImGui::Image((ImTextureID)s_FolderIcon->GetRendererID(), ImVec2{ 16, 16 }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 }, { color.r, color.g, color.b, 1.0f });
		}
		else
			ImGui::Image((ImTextureID)s_EntityIcon->GetRendererID(), ImVec2{ 16, 16 }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 }, ImVec4{ 0.75f, 0.5f, 0.32f, 1.0f });
		ImGui::SameLine();
		ImGui::Selectable("##EntitySelectable", m_Context->IsEntitySelected(entity), ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap);

		if (ImGui::IsItemClicked())
			SelectedEntity(entity);

		bool open_node = false;
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCENE_HIERARCHY_ENTITY"))
			{
				open_node = true;

				// Setup Parenting
				Entity dropped_entity = *(Entity*)payload->Data;
				m_Context->SetEntityParent(dropped_entity, entity);
			}
			ImGui::EndDragDropTarget();
		}
		if (ImGui::BeginDragDropSource())
		{
			ImGui::Text(entity.GetName().c_str());
			ImGui::SetDragDropPayload("SCENE_HIERARCHY_ENTITY", &entity, sizeof(Entity));
			ImGui::EndDragDropSource();
		}

		bool entityDeleted = false;
		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem((std::string(CHARACTER_ICON_DELETE) + " Delete").c_str()))
				entityDeleted = true;
			if (ImGui::MenuItem((std::string(CHARACTER_ICON_DUPLICATE) + " Duplicate").c_str()))
				m_Context->DuplicateEntity(entity);

			ImGui::EndPopup();
		}

		ImGui::SameLine();
		ImGui::PushStyleColor(ImGuiCol_Header, {});
		ImGui::PushStyleColor(ImGuiCol_HeaderActive, {});
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, {});
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_AllowItemOverlap;
		bool opened = false;
		if (m_Context->DoesEntityHaveChildren(entity))
			opened = ImGui::TreeNodeBehavior(ImGui::GetID((void*)(uint64_t)(uint32_t)entity), flags, "");
		else
			if (ImGui::TreeNodeBehavior(ImGui::GetID((void*)(uint64_t)(uint32_t)entity), flags | ImGuiTreeNodeFlags_Leaf, "")) ImGui::TreePop();
		ImGui::PopStyleColor(3);

		// Open TreeNode
		if (open_node)
			ImGui::GetStateStorage()->SetInt(ImGui::GetItemID(), 1);

		ImGui::SameLine();
		ImGui::Text(tag.c_str());

		// Type Section
		ImGui::TableNextColumn();
		ImGui::Text("Entity");

		// Modifiers Section
		ImGui::TableNextColumn();

		ImGui::PushStyleColor(ImGuiCol_Button, {});
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {});
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, {});
		ImGui::ImageButton((ImTextureID)(true ? m_VisibleIcon : m_HiddenIcon)->GetRendererID(), { 15.0f, 15.0f }, { 0, 1 }, { 1, 0 }, -1, {}, ImGui::GetStyleColorVec4(ImGuiCol_Text));
		ImGui::SameLine();
		bool isSelectable = IsEntitySelectable(entity);
		if (ImGui::ImageButton((ImTextureID)(isSelectable ? m_SelectableIcon : m_NonSelectableIcon)->GetRendererID(), { 15.0f, 15.0f }, { 0, 1 }, { 1, 0 }, -1, {}, ImGui::GetStyleColorVec4(isSelectable ? ImGuiCol_Text : ImGuiCol_TextDisabled))) ToggleEntitySelectable(entity);
		ImGui::SameLine();
		bool isLocked = IsEntityLocked(entity);
		if (ImGui::ImageButton((ImTextureID)(isLocked ? m_LockedIcon : m_UnlockedIcon)->GetRendererID(), { 15.0f, 15.0f }, { 0, 1 }, { 1, 0 }, -1, {}, ImGui::GetStyleColorVec4(isLocked ? ImGuiCol_Text : ImGuiCol_TextDisabled))) ToggleEntityLocked(entity);
		ImGui::PopStyleColor(3);

		ImGui::PopID();

		if (opened)
		{
			for (auto& child : m_Context->GetEntityChildren(entity))
				DrawEntityNode(child);

			ImGui::TreePop();
		}

		if (entityDeleted)
			DeleteEntity(entity);

	}

	static void DrawVec3Control(const std::string& label, glm::vec3& values, glm::vec3 resetValue = glm::vec3(0.0f), float columnWidth = 100.0f)
	{
		ImGuiIO& io = ImGui::GetIO();
		auto boldFont = io.Fonts->Fonts[0];

		ImGui::PushID(label.c_str());

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, columnWidth);
		ImGui::Text(label.c_str());
		ImGui::NextColumn();

		ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

		float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
		ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::ButtonCornersEx("X", buttonSize, 0, ImDrawFlags_RoundCornersLeft))
			values.x = resetValue.x;
		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();
		ImGui::Dummy(ImVec2{ GImGui->Style.FramePadding.x, 0.0f });
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::ButtonCornersEx("Y", buttonSize, 0, ImDrawFlags_RoundCornersLeft))
			values.y = resetValue.y;
		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();
		ImGui::Dummy(ImVec2{ GImGui->Style.FramePadding.x, 0.0f });
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::ButtonCornersEx("Z", buttonSize, 0, ImDrawFlags_RoundCornersLeft))
			values.z = resetValue.z;
		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();

		ImGui::PopStyleVar();

		ImGui::Columns(1);

		ImGui::PopID();
	}

	template<typename T, typename UIFunction, typename UISettings>
	static void DrawComponent(const std::string& name, Entity entity, UIFunction uiFunction, UISettings uiSettings, bool removeable = true)
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
			ImGui::PopStyleVar(
			);

			const float button_width = 31.0f;
			ImGui::SameLine(contentRegionAvailable.x - ((button_width + ImGui::GetStyle().FramePadding.x * 2.0f) * 0.5f));
			if (ImGui::Button(CHARACTER_ICON_PREFERENCES, ImVec2{ button_width, lineHeight }))
				ImGui::OpenPopup("ComponentSettings");

			bool removeComponent = false;
			if (ImGui::BeginPopup("ComponentSettings"))
			{
				ImGui::TextDisabled(CHARACTER_ICON_GEAR " Component Settings");
				ImGui::Separator();

				uiSettings(component);

				if (removeable)
				{
					if (ImGui::MenuItem(CHARACTER_ICON_DELETE " Remove component"))
						removeComponent = true;
				}

				ImGui::EndPopup();
			}

			if (open)
			{
				uiFunction(component);
				ImGui::TreePop();
			}

			if (removeComponent)
				entity.RemoveComponent<T>();
		}
	}

	template<typename T, typename R>
	static void DrawAssetSelectionDropdown(AssetType type, const Ref<T>& asset, R onSelect)
	{
		if (ImGui::BeginCombo("##Asset", asset ? AssetManager::GetMetadata(asset->Handle).FilePath.filename().stem().string().c_str() : "Select Asset"))
		{
			auto& metadataRegistry = AssetManager::GetMetadataRegistry();

			bool found = false;
			for (auto& [handle, metadata] : metadataRegistry)
			{
				if (metadata.Type == type)
				{
					found = true;
					if (ImGui::Selectable(metadata.FilePath.filename().stem().string().c_str(), asset ? asset->Handle == handle : false))
						onSelect(AssetManager::GetAsset<T>(handle));
				}
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
				Ref<T> asset = AssetManager::GetAsset<T>(assetPath);
				if (asset)
					onSelect(asset);
			}
			ImGui::EndDragDropTarget();
		}

		if (asset)
		{
			// Clear selected asset context menu
			if (ImGui::BeginPopupContextItem())
			{
				if (ImGui::MenuItem("Clear"))
					onSelect(nullptr);
				ImGui::EndPopup();
			}

			// Clear selected asset button
			{
				ImGui::SameLine();

				float size = ImGui::GetTextLineHeight();

				ImGui::BeginGroup();
				ImGui::PushStyleColor(ImGuiCol_Button, {});
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, {});
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {});
				ImGui::Dummy({ 0.0f, ImGui::GetStyle().FramePadding.y * 0.25f });
				ImVec2 cpos = ImGui::GetCursorPos();
				if (ImGui::Button("##ClearSelectedAssetButton", { size, size }))
					onSelect(nullptr);
				ImGui::SetCursorPos(cpos);
				ImGui::PopStyleColor(3);

				ImVec4 color = ImGui::GetStyleColorVec4(ImGui::IsItemActive() ? ImGuiCol_HeaderActive : (ImGui::IsItemHovered() ? ImGuiCol_HeaderHovered : ImGuiCol_Text));
				ImGui::PushStyleColor(ImGuiCol_Button, {});
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, {});
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {});
				ImGui::Image((ImTextureID)s_ClearIcon->GetRendererID(), { size, size }, { 0, 1 }, { 1, 0 }, color);
				ImGui::PopStyleColor(3);
				ImGui::EndGroup();
			}
		}
	}

	void SceneHierarchyPanel::DrawComponents(Entity entity)
	{
		bool entityDeleted = false;

		if (entity.HasComponent<TagComponent>())
		{
			auto& tag = entity.GetComponent<TagComponent>().Tag;

			char buffer[256];
			memset(buffer, 0, sizeof(buffer));
			strncpy_s(buffer, sizeof(buffer), tag.c_str(), sizeof(buffer));
			if (ImGui::InputText("##Tag", buffer, sizeof(buffer)))
			{
				tag = std::string(buffer);
			}
		}

		ImGui::SameLine();
		ImGui::PushItemWidth(-1);

		if (!entity.HasComponent<FolderComponent>())
			if (ImGui::Button(CHARACTER_ICON_ADD, ImVec2{ 31.0f, 24.0f }))
				ImGui::OpenPopup("##AddComponent");
		ImGui::SameLine();
		if (ImGui::Button(CHARACTER_ICON_DUPLICATE, ImVec2{ 31.0f, 24.0f }))
			m_Context->DuplicateEntity(m_ActiveEntity);
		ImGui::SameLine();
		if (ImGui::Button(CHARACTER_ICON_DELETE, ImVec2{ 31.0f, 24.0f }))
			entityDeleted = true;

		if (ImGui::BeginPopup("##AddComponent"))
		{
			ImGui::TextDisabled(CHARACTER_ICON_ADD " Add Component");
			ImGui::Separator();

			if (ImGui::BeginMenu(CHARACTER_ICON_CUBE " Mesh"))
			{
				DisplayAddComponentEntry<StaticMeshComponent>(CHARACTER_ICON_CUBE " Static Mesh");
				ImGui::EndMenu();
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

			if (ImGui::BeginMenu(CHARACTER_ICON_IMAGE " 2D"))
			{
				DisplayAddComponentEntry<SpriteRendererComponent>(CHARACTER_ICON_IMAGE " Sprite Renderer");
				DisplayAddComponentEntry<CircleRendererComponent>(CHARACTER_ICON_SHADING_UNLIT " Circle Renderer");
				DisplayAddComponentEntry<TextComponent>(CHARACTER_ICON_FONT " Text");
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu(CHARACTER_ICON_PHYSICS " Physics"))
			{
				DisplayAddComponentEntry<RigidbodyComponent>(CHARACTER_ICON_RIGIDBODY " Rigidbody");
				if (ImGui::BeginMenu(CHARACTER_ICON_BOX_COLLIDER " Collider"))
				{
					DisplayAddComponentEntry<BoxColliderComponent>(CHARACTER_ICON_BOX_COLLIDER " Box Collider");
					DisplayAddComponentEntry<SphereColliderComponent>(CHARACTER_ICON_SPHERE_COLLIDER " Sphere Collider");
					DisplayAddComponentEntry<CapsuleColliderComponent>(CHARACTER_ICON_CAPSULE_COLLIDER " Capsule Collider");
					DisplayAddComponentEntry<MeshColliderComponent>(CHARACTER_ICON_MESH_COLLIDER " Mesh Collider");
					ImGui::EndMenu();
				}
				if (ImGui::BeginMenu(CHARACTER_ICON_CONTROLLER " Controller"))
				{
					DisplayAddComponentEntry<CharacterMovementComponent>(CHARACTER_ICON_RUNNING " Character Movement");
					DisplayAddComponentEntry<VehicleMovementComponent>(CHARACTER_ICON_VEHICLE " Vehicle Movement");
					DisplayAddComponentEntry<SpringArmComponent>(CHARACTER_ICON_SPRING " Spring Arm");
					ImGui::EndMenu();
				}
				if (ImGui::BeginMenu(CHARACTER_ICON_DIRECTIONAL_FORCE " Field"))
				{
					DisplayAddComponentEntry<DirectionalFieldComponent>(CHARACTER_ICON_DIRECTIONAL_FORCE " Directional Field");
					DisplayAddComponentEntry<RadialFieldComponent>(CHARACTER_ICON_RADIAL_FORCE " Radial Field");
					DisplayAddComponentEntry<BouyancyFieldComponent>(CHARACTER_ICON_BOUYANCY_FORCE " Bouyancy Field");
					ImGui::EndMenu();
				}

				ImGui::Separator();

				if (ImGui::BeginMenu(CHARACTER_ICON_SQUARE " 2D"))
				{
					DisplayAddComponentEntry<Rigidbody2DComponent>(CHARACTER_ICON_RIGIDBODY " Rigidbody 2D");
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

			ImGui::Separator();

			DisplayAddComponentEntry<CameraComponent>(CHARACTER_ICON_CAMERA " Camera");
			DisplayAddComponentEntry<ParticleSystemComponent>(CHARACTER_ICON_PARTICLES " Particle System");
			DisplayAddComponentEntry<AudioComponent>(CHARACTER_ICON_AUDIO " Audio");

			ImGui::Separator();

			DisplayAddComponentEntry<ScriptComponent>(CHARACTER_ICON_SCRIPT " Script");

			ImGui::EndPopup();
		}

		ImGui::PopItemWidth();

		DrawComponent<TransformComponent>(CHARACTER_ICON_TRANSFORM " TRANSFORM", entity, [](auto& component)
		{
			DrawVec3Control("Translation", component.Translation);
			glm::vec3 rotation = glm::degrees(component.Rotation);
			DrawVec3Control("Rotation", rotation);
			component.Rotation = glm::radians(rotation);
			DrawVec3Control("Scale", component.Scale, glm::vec3(1.0f));
		},
		[](auto& component) 
		{
			if (ImGui::MenuItem(CHARACTER_ICON_COPY " Copy"))
			{
				std::stringstream ss;
				ss << component.Translation.x << " " << component.Translation.y << " " << component.Translation.z
					<< " " << component.Rotation.x  << " " << component.Rotation.y  << " " << component.Rotation.z
					<< " " << component.Scale.x  << " " << component.Scale.y << " " << component.Scale.z;
				ImGui::SetClipboardText(ss.str().c_str());
			}
			if (ImGui::MenuItem(CHARACTER_ICON_PASTE " Paste"))
			{
				std::stringstream ss;
				ss << ImGui::GetClipboardText();
				ss >> component.Translation.x >> component.Translation.y >> component.Translation.z
					>> component.Rotation.x >> component.Rotation.y >> component.Rotation.z
					>> component.Scale.x >> component.Scale.y >> component.Scale.z;
			}
			if (ImGui::MenuItem(CHARACTER_ICON_RESTART " Reset"))
			{
				component.Translation = glm::vec3(0.0f);
				component.Rotation = glm::vec3(0.0f);
				component.Scale = glm::vec3(1.0f);
			}
		}, false);

		DrawComponent<CameraComponent>(CHARACTER_ICON_CAMERA " CAMERA", entity, [](auto& component)
		{
			auto& camera = component.Camera;

			ImGui::Checkbox("Primary", &component.Primary);

			const char* projectionTypeStrings[] = { "Perspective", "Orthographic" };
			const char* currentProjectionTypeString = projectionTypeStrings[(int)camera.GetProjectionType()];
			if (ImGui::BeginCombo("Projection", currentProjectionTypeString))
			{
				for (int i = 0; i < 2; i++)
				{
					bool isSelected = currentProjectionTypeString == projectionTypeStrings[i];
					if (ImGui::Selectable(projectionTypeStrings[i], isSelected))
					{
						currentProjectionTypeString = projectionTypeStrings[i];
						camera.SetProjectionType((SceneCamera::ProjectionType)i);
					}

					if (isSelected)
						ImGui::SetItemDefaultFocus();
				}

				ImGui::EndCombo();
			}

			if (camera.GetProjectionType() == SceneCamera::ProjectionType::Perspective)
			{
				float perspectiveVerticalFov = glm::degrees(camera.GetPerspectiveVerticalFOV());
				if (ImGui::DragFloat("Vertical FOV", &perspectiveVerticalFov))
					camera.SetPerspectiveVerticalFOV(glm::radians(perspectiveVerticalFov));

				float perspectiveNear = camera.GetPerspectiveNearClip();
				if (ImGui::DragFloat("Near", &perspectiveNear))
					camera.SetPerspectiveNearClip(perspectiveNear);

				float perspectiveFar = camera.GetPerspectiveFarClip();
				if (ImGui::DragFloat("Far", &perspectiveFar))
					camera.SetPerspectiveFarClip(perspectiveFar);
			}

			if (camera.GetProjectionType() == SceneCamera::ProjectionType::Orthographic)
			{
				float orthoSize = camera.GetOrthographicSize();
				if (ImGui::DragFloat("Size", &orthoSize))
					camera.SetOrthographicSize(orthoSize);

				float orthoNear = camera.GetOrthographicNearClip();
				if (ImGui::DragFloat("Near", &orthoNear))
					camera.SetOrthographicNearClip(orthoNear);

				float orthoFar = camera.GetOrthographicFarClip();
				if (ImGui::DragFloat("Far", &orthoFar))
					camera.SetOrthographicFarClip(orthoFar);

				ImGui::Checkbox("Fixed Aspect Ratio", &component.FixedAspectRatio);
			}
		}, [](auto& component) {});

		DrawComponent<ScriptComponent>(CHARACTER_ICON_SCRIPT " SCRIPT", entity, [this, entity, scene = m_Context](auto& component) mutable
		{
			bool scriptClassExists = ScriptEngine::EntityClassExists(component.ClassName);
			bool scriptEmpty = component.ClassName.empty();

			static char buffer[64];
			strcpy_s(buffer, sizeof(buffer), component.ClassName.c_str());

			if (!scriptClassExists && !scriptEmpty)
			{
				ImGui::PushStyleColor(ImGuiCol_FrameBg, { 1.0f, 0.4f, 0.4f, 0.5f });
			}

			if (ImGui::InputTextWithHint("Class", "Select Script...", buffer, sizeof(buffer)))
				component.ClassName = buffer;

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
					for (auto& [name, entity] : ScriptEngine::GetEntityClasses())
					{
						if (String::ToLower(name).find(String::ToLower(component.ClassName)) != std::string::npos)
						{
							found = true;
							if (ImGui::MenuItem(name.c_str()))
								component.ClassName = name;
						}
					}

					if (!found)
						ImGui::TextDisabled("No Scripts Found");

					ImGui::EndPopup();

					if (ImGui::IsWindowFocused())
						ImGui::CloseCurrentPopup();
				}
			}

			// Fields
			if (scriptClassExists)
			{
				if (ImGui::CollapsingHeader("Fields"))
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
									ImGui::TextDisabled(name.c_str());

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
										ImGui::TextDisabled(name.c_str());

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
										ImGui::TextDisabled(name.c_str());

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
									}
								}
							}
						}
					}
					ImGui::Unindent();
				}
			}
		}, [](auto& component) {});

		DrawComponent<SpriteRendererComponent>(CHARACTER_ICON_IMAGE " SPRITE RENDERER", entity, [this](auto& component)
		{
			ImGui::ColorEdit4("Color", glm::value_ptr(component.Color));
			
			ImGui::ImageButton(component.Texture ? ((ImTextureID)component.Texture->GetRendererID()) : ((ImTextureID)m_CheckerboardTexture->GetRendererID()), ImVec2(100.0f, 100.0f), ImVec2( 0, 1 ), ImVec2( 1, 0));
			DrawAssetSelectionDropdown(AssetType::Texture, component.Texture, [&](Ref<Texture2D> texture) 
			{
				if (!texture)
					component.Texture = nullptr;
				else if (texture->IsLoaded())
					component.Texture = texture;
			});

			ImGui::DragFloat("Tiling Factor", &component.TilingFactor, 0.1f, 0.0f, 100.0f);
		}, [](auto& component) {});

		DrawComponent<CircleRendererComponent>(CHARACTER_ICON_SHADING_UNLIT " CIRCLE RENDERER", entity, [](auto& component)
		{
			ImGui::ColorEdit4("Color", glm::value_ptr(component.Color));
			ImGui::DragFloat("Thickness", &component.Thickness, 0.025f, 0.0f, 1.0f);
			ImGui::DragFloat("Fade", &component.Fade, 0.00025f, 0.0f, 1.0f);
		}, [](auto& component) {});

		DrawComponent<TextComponent>(CHARACTER_ICON_FONT " TEXT", entity, [this](auto& component)
		{
			ImGui::Text("Text String");
			ImGui::InputTextMultiline("##TextComponentInput", &component.TextString, ImVec2(-1.0f, 200.0f));

			ImGui::Separator();
			
			ImGui::Text("Color");
			ImGui::SameLine();
			ImGui::ColorEdit4("##TextColorInput", glm::value_ptr(component.Color));

			if (component.Font)
				ImGui::Text("Font Handle: %llu", component.Font->Handle);
			
			DrawAssetSelectionDropdown(AssetType::Font, component.Font, [&](Ref<Font> font) { component.Font = font; });

			ImGui::Separator();
			
			ImGui::Text("Kerning");
			ImGui::SameLine();
			ImGui::DragFloat("##KerningInput", &component.Kerning);
			
			ImGui::Text("Line Spacing");
			ImGui::SameLine();
			ImGui::DragFloat("##LineSpacingInput", &component.LineSpacing);

			ImGui::Separator();

			ImGui::Text("Max Width");
			ImGui::SameLine();
			ImGui::DragFloat("##MaxWidthInput", &component.MaxWidth);
			
		}, [](auto& component) {});

		DrawComponent<ParticleSystemComponent>(CHARACTER_ICON_PARTICLES " PARTICLE SYSTEM", entity, [](auto& component)
		{
			DrawVec3Control("Offset", component.Offset);

			DrawVec3Control("Velocity", component.Velocity);
			DrawVec3Control("Velocity Variation", component.VelocityVariation);
			DrawVec3Control("Gravity (m/s)", component.Gravity, glm::vec3(0.0f, -9.8f, 0.0f));

			const char* colorMethodStrings[] = { "Linear", "Constant", "Points" };
			const char* currentColorMethodString = colorMethodStrings[(int)component.ColorMethod];
			if (ImGui::BeginCombo("Color Method", currentColorMethodString))
			{
				for (int i = 0; i < 3; i++)
				{
					bool isSelected = currentColorMethodString == colorMethodStrings[i];
					if (ImGui::Selectable(colorMethodStrings[i], isSelected))
					{
						currentColorMethodString = colorMethodStrings[i];
						component.ColorMethod = i;
					}

					if (isSelected)
						ImGui::SetItemDefaultFocus();
				}

				ImGui::EndCombo();
			}

			if (component.ColorMethod == 0)
			{
				ImGui::ColorEdit4("Color Begin", glm::value_ptr(component.ColorBegin));
				ImGui::ColorEdit4("Color End", glm::value_ptr(component.ColorEnd));
			}
			else if (component.ColorMethod == 1)
			{
				ImGui::ColorEdit4("Color", glm::value_ptr(component.ColorConstant));
			}
			else if (component.ColorMethod == 2)
			{
				ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
				bool open = (ImGui::TreeNodeEx("Color Points", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding));

				float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
				ImGui::SameLine(contentRegionAvailable.x + lineHeight * 0.35f);
				if (ImGui::Button("+##AddColorPoint", ImVec2(lineHeight, lineHeight)))
					component.ColorPoints.push_back({ component.GetNextColorPointId() });
				ImGui::PopStyleVar();
				
				if (open)
				{
					for (int i = 0; i < component.ColorPoints.size(); i++)
					{
						ImGui::PushID(component.ColorPoints[i].id);

						bool removeIndex = false;

						ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x / 6);
						if (ImGui::DragFloat("ParticlePointOffset", &component.ColorPoints[i].point, 0.001f, 0.0f, 1.0f))
							component.RecalculateColorPointOrder();
						ImGui::PopItemWidth();

						//Remove Context
						if (ImGui::BeginPopupContextItem())
						{
							if (ImGui::MenuItem("Remove Color Point"))
								removeIndex = true;
							if (ImGui::MenuItem("Duplicate Color Point"))
								component.DuplicateColorPoint(i);

							ImGui::EndPopup();
						}

						ImGui::SameLine();
						ImGui::ColorEdit4("##ParticlePointColor", glm::value_ptr(component.ColorPoints[i].color));

						if (removeIndex)
							component.ColorPoints.erase(component.ColorPoints.begin() + i);

						ImGui::PopID();
					}
					ImGui::TreePop();
				}
			}

			ImGui::DragFloat("Size Begin", &component.SizeBegin);
			ImGui::DragFloat("Size End", &component.SizeEnd);
			ImGui::DragFloat("Size Variation", &component.SizeVariation);

			ImGui::DragFloat("Life Time", &component.LifeTime);
			ImGui::DragInt("Emission Number", &component.EmissionNumber);

			ImGui::Checkbox("Active", &component.Active);
			ImGui::Checkbox("Face Camera", &component.FaceCamera);

			if (ImGui::Button("Emit", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
			{
				component.Emit();
			}

			if (ImGui::Button("Reset", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
			{
				component.ClearParticlePool();
			}
		}, [](auto& component) {});

		DrawComponent<Rigidbody2DComponent>(CHARACTER_ICON_RIGIDBODY " RIGIDBODY 2D", entity, [](auto& component)
		{
			const char* bodyTypeStrings[] = { "Static", "Dynamic", "Kinematic" };
			const char* currentBodyTypeString = bodyTypeStrings[(int)component.Type];
			if (ImGui::BeginCombo("Type", currentBodyTypeString))
			{
				for (int i = 0; i < 3; i++)
				{
					bool isSelected = currentBodyTypeString == bodyTypeStrings[i];
					if (ImGui::Selectable(bodyTypeStrings[i], isSelected))
					{
						currentBodyTypeString = bodyTypeStrings[i];
						component.Type = (Rigidbody2DComponent::BodyType)i;
					}

					if (isSelected)
						ImGui::SetItemDefaultFocus();
				}

				ImGui::EndCombo();
			}

			ImGui::Checkbox("Fixed Rotation", &component.FixedRotation);
		}, [](auto& component) {});

		DrawComponent<BoxCollider2DComponent>(CHARACTER_ICON_SQUARE " BOX COLLIDER 2D", entity, [](auto& component)
		{
			ImGui::DragFloat2("Offset", glm::value_ptr(component.Offset));
			ImGui::DragFloat2("Size", glm::value_ptr(component.Size));
			ImGui::DragFloat("Density", &component.Density, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat("Friction", &component.Friction, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat("Restitution", &component.Restitution, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat("RestitutionThreshold", &component.RestitutionThreshold, 0.01f, 0.0f);
		}, [](auto& component) {});

		DrawComponent<CircleCollider2DComponent>(CHARACTER_ICON_CIRCLE " CIRCLE COLLIDER 2D", entity, [](auto& component)
		{
			ImGui::DragFloat2("Offset", glm::value_ptr(component.Offset));
			ImGui::DragFloat("Radius", &component.Radius);
			ImGui::DragFloat("Density", &component.Density, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat("Friction", &component.Friction, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat("Restitution", &component.Restitution, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat("RestitutionThreshold", &component.RestitutionThreshold, 0.01f, 0.0f);
		}, [](auto& component) {});

		DrawComponent<StaticMeshComponent>(CHARACTER_ICON_CUBE " MESH", entity, [this](auto& component)
		{
			DrawAssetSelectionDropdown(AssetType::Mesh, component.m_Model, [&](Ref<Model> model) { component.SetModel(model); });
				
			if (component.m_Model)
			{
				ImGui::Text("Model Handle: %llu", component.m_Model->Handle);

				// Materials Section
				{
					ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
					bool open = ImGui::TreeNodeEx("Materials", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding);
					ImGui::PopStyleVar();
					if (open)
					{
						for (uint32_t i = 0; i < component.m_Materials.size(); i++)
						{
							auto& material = component.m_Materials[i];

							ImGui::PushID(i);

							ImGui::Text("[Material %d]", i);
							ImGui::SameLine();

							DrawAssetSelectionDropdown(AssetType::Material, material, [&](Ref<Material> newMaterial) { material = newMaterial; });
							
							ImGui::PopID();
						}
						ImGui::TreePop();
					}
				}

				// Blend Shape Section
				auto& blendShapeWeights = component.m_Model->GetBlendShapeWeights();
				if (!blendShapeWeights.empty())
				{
					ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
					bool open = ImGui::TreeNodeEx("Blend Shapes", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding);
					ImGui::PopStyleVar();
					if (open)
					{
						for (auto& [name, weight] : blendShapeWeights)
						{
							ImGui::PushID(name.c_str());

							ImGui::Text("%s", name.c_str());
							ImGui::SameLine();
							
							if (ImGui::DragFloat("##Weight", (float*)(&weight), 0.01f, 0.0f, 1.0f))
								component.m_Model->UpdateBlendShapes();

							ImGui::PopID();
						}
						ImGui::TreePop();
					}
				}
				
				// Animation Section
				{
					ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
					bool open = ImGui::TreeNodeEx("Animations", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding);
					ImGui::PopStyleVar();
					if (open)
					{
						if (ImGui::Button("Open Animation", ImVec2(-1.0f, 0.0f)))
						{
							std::string filepath = FileDialogs::OpenFile("3D Animation");
							if (!filepath.empty())
								component.LoadAnimation(filepath);
						}

						if (component.GetAnimator()->HasAnimation())
						{
							ImGui::Checkbox("##AnimationPaused", &component.GetAnimator()->GetIsPaused());
							ImGui::SameLine();
							ImGui::Text("Paused");

							ImGui::SliderFloat("##MouseDoubleClickSpeedSlider", &component.GetAnimator()->GetAnimationTime(), 0.0f, component.GetAnimator()->GetAnimationDuration());
							ImGui::SameLine();
							ImGui::Text("Current Time");
						}

						ImGui::TreePop();
					}
				}
			}
			
		}, [](auto& component) {});

		DrawComponent<DirectionalLightComponent>(CHARACTER_ICON_SUN " DIRECTIONAL LIGHT", entity, [](auto& component)
		{
			ImGui::ColorEdit3("Color", glm::value_ptr(component.Color));
			ImGui::DragFloat("Intensity", &component.Intensity);
		}, [](auto& component) {});

		DrawComponent<PointLightComponent>(CHARACTER_ICON_POINT_LIGHT " POINT LIGHT", entity, [](auto& component)
		{
			ImGui::ColorEdit3("Color", glm::value_ptr(component.Color));
			ImGui::DragFloat("Intensity", &component.Intensity);
			ImGui::DragFloat("Radius", &component.Radius);
			ImGui::Checkbox("Casts Shadows", &component.CastsShadows);
		}, [](auto& component) {});

		DrawComponent<SpotLightComponent>(CHARACTER_ICON_SPOT_LIGHT " SPOT LIGHT", entity, [](auto& component)
		{
			ImGui::ColorEdit3("Color", glm::value_ptr(component.Color));
			ImGui::DragFloat("Cut Off", &component.CutOff);
			ImGui::DragFloat("Outer Cut Off", &component.OuterCutOff);
			ImGui::DragFloat("Constant", &component.Constant);
			ImGui::DragFloat("Linear", &component.Linear);
			ImGui::DragFloat("Quadratic", &component.Quadratic);
		}, [](auto& component) {});

		DrawComponent<SkyLightComponent>(CHARACTER_ICON_CLOUDS " SKY LIGHT", entity, [](auto& component)
		{
			if (ImGui::BeginCombo("Type", component.Type == 0 ? "HDRI" : "Dynamic"))
			{
				if (ImGui::MenuItem("HDRI"))
					component.Type = 0;

				if (ImGui::MenuItem("Dynamic"))
					component.Type = 1;

				ImGui::EndCombo();
			}

			if (component.Type == 0)
			{
				if (ImGui::Button("Load HDRI", ImVec2(-1.0f, 0.0f)))
				{
					std::string filepath = FileDialogs::OpenFile("HDRI");
					if (!filepath.empty())
					{
						Ref<Texture2D> texture = Texture2D::Create(filepath);
						if (texture->IsLoaded())
						{
							component.SkyboxHDRI = texture;
							component.Filepath = filepath;
						}
					}
				}

				if (ImGui::Button("Load FlowMap", ImVec2(-1.0f, 0.0f)))
				{
					std::string filepath = FileDialogs::OpenFile("HDRI Flow Map");
					if (!filepath.empty())
					{
						Ref<Texture2D> texture = Texture2D::Create(filepath);
						if (texture->IsLoaded())
						{
							component.SkyboxFlowMap = texture;
						}
					}
				}
			}

			ImGui::DragFloat("Intensity", &component.Intensity);
		}, [](auto& component) {});

		DrawComponent<VolumeComponent>(CHARACTER_ICON_SMOKE " VOLUME", entity, [](auto& component)
		{
			ImGui::Text("Blend Type");
			ImGui::SameLine();
			if (ImGui::BeginCombo("##VolumeBlendType", component.Blend == VolumeComponent::BlendType::Set ? "Set" : "Add"))
			{
				if (ImGui::MenuItem("Set"))
					component.Blend = VolumeComponent::BlendType::Set;

				if (ImGui::MenuItem("Add"))
					component.Blend = VolumeComponent::BlendType::Add;

				ImGui::EndCombo();
			}

			ImGui::DragFloat("##VolumeScatteringDistributionInput", &component.ScatteringDistribution, 0.01f, 0.0f, 1.0f);
			ImGui::SameLine();
			ImGui::Text("Scattering Distribution");
			
			ImGui::DragFloat("##VolumeScatteringIntensityInput", &component.ScatteringIntensity, 0.01f, 0.0f, 1.0f);
			ImGui::SameLine();
			ImGui::Text("Scattering Intensity");
			
			ImGui::DragFloat("##VolumeExtinctionScaleInput", &component.ExtinctionScale, 0.01f, 0.0f, 1.0f);
			ImGui::SameLine();
			ImGui::Text("Extinction Scale");
		}, [](auto& component) {});

		DrawComponent<AudioComponent>(CHARACTER_ICON_AUDIO " AUDIO", entity, [](auto& component)
		{
			DrawAssetSelectionDropdown(AssetType::Audio, component.AudioSound, [&](Ref<Audio> audio){ component.AudioSound = audio; });

			if (auto& sound = component.AudioSound)
			{
				ImGui::Text("Audio Handle: %llu", component.AudioSound->Handle);

				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
				bool open = ImGui::TreeNodeEx("Sound Properties", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding);
				ImGui::PopStyleVar();
				if (open)
				{
					bool is3D = sound->Is3D();
					if (ImGui::Checkbox("3D", &is3D))
						sound->SetIs3D(is3D);

					bool isLooping = sound->IsLooping();
					if (ImGui::Checkbox("Is Looping", &isLooping))
						sound->SetLooping(isLooping);

					bool startOnAwake = component.StartOnAwake;
					if (ImGui::Checkbox("Start On Awake", &startOnAwake))
						component.StartOnAwake = startOnAwake;

					float radius = sound->GetRadius();
					if (ImGui::DragFloat("Radius", &radius))
						sound->SetRadius(radius);

					{
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
						auto& min = ImGui::GetItemRectMin();
						auto& max = ImGui::GetItemRectMax();

						int milliseconds = (position / 10) % 1000;
						int seconds = (position / 1000) % 60;
						int minutes = ((position / (1000 * 60)) % 60);

						std::string time = (minutes < 10 ? "0" : "") + std::to_string(minutes) + (seconds < 10 ? " : 0" : " : ") + std::to_string(seconds) + (milliseconds < 10 ? " : 00" : (milliseconds < 100 ? " : 0" : " : ")) + std::to_string(milliseconds);
						ImGui::GetWindowDrawList()->AddText(min + ((max - min - ImGui::CalcTextSize(time.c_str())) * 0.5f), ImGui::GetColorU32(ImGuiCol_Text), time.c_str());
					}

					float volume = sound->GetVolume();
					if (ImGui::SliderFloat("Volume", &volume, 0.0f, 1.0f))
						sound->SetVolume(volume);

					float pan = sound->GetPan();
					if (ImGui::SliderFloat("Pan", &pan, -1.0f, 1.0f))
						sound->SetPan(pan);

					float speed = sound->GetSpeed();
					if (ImGui::SliderFloat("Speed", &speed, 0.0f, 4.0f))
						sound->SetSpeed(speed);

					bool echo = sound->GetEcho();
					if (ImGui::Checkbox("Echo", &echo))
						sound->SetEcho(echo);

					ImGui::TreePop();
				}
			}
		}, [](auto& component) {});

		DrawComponent<RigidbodyComponent>(CHARACTER_ICON_RIGIDBODY " RIGIDBODY", entity, [](auto& component)
		{
			const char* bodyTypeStrings[] = { "Static", "Dynamic" };
			const char* currentBodyTypeString = bodyTypeStrings[(int)component.Type];
			if (ImGui::BeginCombo("Type", currentBodyTypeString))
			{
				for (int i = 0; i < 2; i++)
				{
					bool isSelected = currentBodyTypeString == bodyTypeStrings[i];
					if (ImGui::Selectable(bodyTypeStrings[i], isSelected))
					{
						currentBodyTypeString = bodyTypeStrings[i];
						component.Type = (RigidbodyComponent::BodyType)i;
					}

					if (isSelected)
						ImGui::SetItemDefaultFocus();
				}

				ImGui::EndCombo();
			}

			if (component.Type == RigidbodyComponent::BodyType::Dynamic)
			{
				ImGui::Text("Density");
				ImGui::SameLine();
				ImGui::DragFloat("##DensityInput", &component.Density, 0.1f, 0.0f, 0.0f, "%.2f");
			}

			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
			bool open = ImGui::TreeNodeEx("Material Properties", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding);
			ImGui::PopStyleVar();
			if (open)
			{
				ImGui::Text("Static Friction");
				ImGui::SameLine();
				ImGui::DragFloat("##StaticFrictionInput", &component.StaticFriction, 0.5f, 0.0f, 1.0f, "%.2f");

				ImGui::Text("Dynamic Friction");
				ImGui::SameLine();
				ImGui::DragFloat("##DynamicFrictionInput", &component.DynamicFriction, 0.5f, 0.0f, 1.0f, "%.2f");

				ImGui::Text("Restitution");
				ImGui::SameLine();
				ImGui::DragFloat("##RestitutionInput", &component.Restitution, 0.5f, 0.0f, 1.0f, "%.2f");

				ImGui::TreePop();
			}

		}, [](auto& component) {});

		DrawComponent<CharacterMovementComponent>(CHARACTER_ICON_RUNNING " CHARACTER MOVEMENT", entity, [](auto& component)
		{
			// Object Properties
			{
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
				bool open = ImGui::TreeNodeEx("Object Properties", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding);
				ImGui::PopStyleVar();
				if (open)
				{
					ImGui::Text("Density");
					ImGui::SameLine();
					ImGui::DragFloat("##DensityInput", &component.Density, 0.1f, 0.0f, 0.0f, "%.2f");

					ImGui::Text("Capsule Radius");
					ImGui::SameLine();
					ImGui::DragFloat("##CapsuleRadiusInput", &component.CapsuleRadius, 0.1f, 0.0f, 0.0f, "%.2f");
					
					ImGui::Text("Capsule Height");
					ImGui::SameLine();
					ImGui::DragFloat("##CapsuleHeightInput", &component.CapsuleHeight, 0.1f, 0.0f, 0.0f, "%.2f");
					
					ImGui::TreePop();
				}
			}

			// Character Movement
			{
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
				bool open = ImGui::TreeNodeEx("Character Movement", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding);
				ImGui::PopStyleVar();
				if (open)
				{
					ImGui::Text("Gravity Scale");
					ImGui::SameLine();
					ImGui::DragFloat("##GravityScaleInput", &component.GravityScale, 0.1f, 0.0f, 0.0f, "%.2f");

					ImGui::Text("Step Offset");
					ImGui::SameLine();
					ImGui::DragFloat("##StepOffsetInput", &component.StepOffset, 0.1f, 0.0f, 0.0f, "%.2f");
					
					ImGui::Text("Max Walkable Slope");
					ImGui::SameLine();
					ImGui::DragFloat("##MaxWalkableSlopeInput", &component.MaxWalkableSlope, 0.1f, 0.0f, 0.0f, "%.2f");
					
					ImGui::Text("Max Walk Speed");
					ImGui::SameLine();
					ImGui::DragFloat("##MaxWalkSpeedInput", &component.MaxWalkSpeed, 0.1f, 0.0f, 0.0f, "%.2f");
					
					ImGui::Text("Max Acceleration");
					ImGui::SameLine();
					ImGui::DragFloat("##MaxAccelerationInput", &component.MaxAcceleration, 0.1f, 0.0f, 0.0f, "%.2f");

					ImGui::Text("Braking Deceleration");
					ImGui::SameLine();
					ImGui::DragFloat("##BrakingDecelerationInput", &component.BrakingDeceleration, 0.1f, 0.0f, 0.0f, "%.2f");

					ImGui::Text("Ground Friction");
					ImGui::SameLine();
					ImGui::DragFloat("##GroundFrictionInput", &component.GroundFriction, 0.1f, 0.0f, 0.0f, "%.2f");
					
					ImGui::Text("Air Control");
					ImGui::SameLine();
					ImGui::DragFloat("##AirControlInput", &component.AirControl, 0.1f, 0.0f, 0.0f, "%.2f");
					
					ImGui::TreePop();
				}
			}

		}, [](auto& component) {});

		DrawComponent<SpringArmComponent>(CHARACTER_ICON_SPRING " SPRING ARM", entity, [](auto& component)
		{
		}, [](auto& component) {});

		DrawComponent<DirectionalFieldComponent>(CHARACTER_ICON_DIRECTIONAL_FORCE " DIRECTIONAL FIELD", entity, [](auto& component)
		{
			ImGui::Text("Force");
			ImGui::SameLine();
			ImGui::DragFloat3("##DirectionalFieldForceInput", glm::value_ptr(component.Force), 0.1f, 0.0f, 0.0f, "%.2f");
		}, [](auto& component) {});

		DrawComponent<RadialFieldComponent>(CHARACTER_ICON_RADIAL_FORCE " RADIAL FIELD", entity, [](auto& component)
		{
			ImGui::Text("Magnitude");
			ImGui::SameLine();
			ImGui::DragFloat("##RadialFieldMagnitudeInput", &component.Magnitude, 0.1f, 0.0f, 0.0f, "%.2f");

			ImGui::Text("Radius");
			ImGui::SameLine();
			ImGui::DragFloat("##RadialFieldRadiusInput", &component.Radius, 0.1f, 0.0f, 0.0f, "%.2f");
			
			ImGui::Text("Falloff");
			ImGui::SameLine();
			ImGui::DragFloat("##RadialFieldFalloffInput", &component.Falloff, 0.1f, 0.0f, 0.0f, "%.2f");
		}, [](auto& component) {});

		DrawComponent<BouyancyFieldComponent>(CHARACTER_ICON_BOUYANCY_FORCE " BOUYANCY FIELD", entity, [](auto& component)
		{
			ImGui::Text("Fluid Density");
			ImGui::SameLine();
			ImGui::DragFloat("##FluidDensityInput", &component.FluidDensity, 0.1f, 0.0f, 0.0f, "%.2f");

			ImGui::Text("Linear Damping");
			ImGui::SameLine();
			ImGui::DragFloat("##LinearDampingInput", &component.LinearDamping, 0.1f, 0.0f, 0.0f, "%.2f");

			ImGui::Text("Angular Damping");
			ImGui::SameLine();
			ImGui::DragFloat("##AngularDampingInput", &component.AngularDamping, 0.1f, 0.0f, 0.0f, "%.2f");
		}, [](auto& component) {});

		DrawComponent<BoxColliderComponent>(CHARACTER_ICON_BOX_COLLIDER " BOX COLLIDER", entity, [](auto& component)
		{
			ImGui::Text("Size");
			ImGui::SameLine();
			ImGui::DragFloat3("##BoxColliderScaleInput", glm::value_ptr(component.Size), 0.1f, 0.0f, 0.0f, "%.2f");
		}, [](auto& component) {});

		DrawComponent<SphereColliderComponent>(CHARACTER_ICON_SPHERE_COLLIDER " SPHERE COLLIDER", entity, [](auto& component)
		{
			ImGui::Text("Radius");
			ImGui::SameLine();
			ImGui::DragFloat("##SphereColliderRadiusInput", &component.Radius, 0.1f, 0.0f, 0.0f, "%.2f");
		}, [](auto& component) {});

		DrawComponent<CapsuleColliderComponent>(CHARACTER_ICON_CAPSULE_COLLIDER " CAPSULE COLLIDER", entity, [](auto& component)
		{
			ImGui::Text("Radius");
			ImGui::SameLine();
			ImGui::DragFloat("##CapsuleColliderRadiusInput", &component.Radius, 0.1f, 0.0f, 0.0f, "%.2f");
			ImGui::Text("Half Height");
			ImGui::SameLine();
			ImGui::DragFloat("##CapsuleColliderHalfHeightInput", &component.HalfHeight, 0.1f, 0.0f, 0.0f, "%.2f");
		}, [](auto& component) {});

		DrawComponent<MeshColliderComponent>(CHARACTER_ICON_MESH_COLLIDER " MESH COLLIDER", entity, [](auto& component)
		{
			const char* meshTypeStrings[] = { "Triangle", "Convex" };
			const char* currentMeshTypeString = meshTypeStrings[(int)component.Type];
			if (ImGui::BeginCombo("Type", currentMeshTypeString))
			{
				for (int i = 0; i < 2; i++)
				{
					bool isSelected = currentMeshTypeString == meshTypeStrings[i];
					if (ImGui::Selectable(meshTypeStrings[i], isSelected))
					{
						currentMeshTypeString = meshTypeStrings[i];
						component.Type = (MeshColliderComponent::MeshType)i;
					}

					if (isSelected)
						ImGui::SetItemDefaultFocus();
				}

				ImGui::EndCombo();
			}
		}, [](auto& component) {});

		DrawComponent<UICanvasComponent>(CHARACTER_ICON_CANVAS " UI CANVAS", entity, [](auto& component)
		{
			ImGui::Text("Enabled");
			ImGui::SameLine();
			ImGui::Checkbox("##CanvasEnabledCheckbox", &component.Enabled);

			ImGui::Text("Min");
			ImGui::SameLine();
			ImGui::DragFloat2("##CanvasMinInput", glm::value_ptr(component.Min), 0.05f, 0.0f, 0.0f, "%.2f");

			ImGui::Text("Max");
			ImGui::SameLine();
			ImGui::DragFloat2("##CanvasMaxInput", glm::value_ptr(component.Max), 0.05f, 0.0f, 0.0f, "%.2f");
		}, [](auto& component) {});

		DrawComponent<UIImageComponent>(CHARACTER_ICON_IMAGE " UI IMAGE", entity, [this](auto& component)
		{
			ImGui::Text("Anchor");
			ImGui::SameLine();
			ImGui::DragFloat2("##ImageAnchorInput", glm::value_ptr(component.Anchor), 0.05f, 0.0f, 0.0f, "%.2f");
			ImGui::Text("Position");
			ImGui::SameLine();
			ImGui::DragFloat2("##ImagePositionInput", glm::value_ptr(component.Position), 0.25f, 0.0f, 0.0f, "%.2f");
			ImGui::Text("Size");
			ImGui::SameLine();
			ImGui::DragFloat2("##ImageSizeInput", glm::value_ptr(component.Size), 0.25f, 0.0f, 0.0f, "%.2f");
			ImGui::Image((ImTextureID)(component.Image ? component.Image : m_CheckerboardTexture)->GetRendererID(), { 100.0f, 100.0f }, { 0, 1 }, { 1, 0 });
			
			DrawAssetSelectionDropdown(AssetType::Texture, component.Image, [&](Ref<Texture2D> texture) { component.Image = texture; });

		}, [](auto& component) {});

		DrawComponent<UIButtonComponent>(CHARACTER_ICON_BUTTON " UI BUTTON", entity, [](auto& component)
		{
			ImGui::Text("Button Placeholder");
		}, [](auto& component) {});

		DrawComponent<FolderComponent>(CHARACTER_ICON_FOLDER " FOLDER SETTINGS", entity, [](auto& component)
		{
			ImGui::Text("Folder Color");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(-1);
			ImGui::ColorEdit3("##FolderColorPicker", glm::value_ptr(component.Color));

			ImGui::Text("Presets:");
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { (ImGui::GetContentRegionAvailWidth() / 10.0f - ImGui::GetFrameHeight() - ImGui::GetStyle().FramePadding.x), 0.0f });
			ImGui::SameLine();
			if (ImGui::ColorButton("##FolderPreset0", { 1.0f, 1.0f, 1.0f, 1.0f })) component.Color = { 1.0f, 1.0f, 1.0f, 1.0f }; // White
			ImGui::SameLine();
			if (ImGui::ColorButton("##FolderPreset1", { 0.7f, 0.7f, 0.7f, 1.0f })) component.Color = { 0.7f, 0.7f, 0.7f, 1.0f }; // Gray
			ImGui::SameLine();
			if (ImGui::ColorButton("##FolderPreset2", { 0.8f, 0.1f, 0.2f, 1.0f })) component.Color = { 0.8f, 0.1f, 0.2f, 1.0f }; // Red
			ImGui::SameLine();
			if (ImGui::ColorButton("##FolderPreset3", { 0.8f, 0.4f, 0.2f, 1.0f })) component.Color = { 0.8f, 0.4f, 0.2f, 1.0f }; // Orange
			ImGui::SameLine();
			if (ImGui::ColorButton("##FolderPreset4", { 1.0f, 0.9f, 0.3f, 1.0f })) component.Color = { 1.0f, 0.9f, 0.3f, 1.0f }; // Yellow
			ImGui::SameLine();
			if (ImGui::ColorButton("##FolderPreset5", { 0.4f, 0.8f, 0.4f, 1.0f })) component.Color = { 0.4f, 0.8f, 0.4f, 1.0f }; // Green
			ImGui::SameLine();
			if (ImGui::ColorButton("##FolderPreset6", { 0.2f, 0.7f, 1.0f, 1.0f })) component.Color = { 0.2f, 0.7f, 1.0f, 1.0f }; // Blue
			ImGui::SameLine();
			if (ImGui::ColorButton("##FolderPreset7", { 0.7f, 0.5f, 1.0f, 1.0f })) component.Color = { 0.7f, 0.5f, 1.0f, 1.0f }; // Purple
			ImGui::SameLine();
			if (ImGui::ColorButton("##FolderPreset8", { 1.0f, 0.6f, 1.0f, 1.0f })) component.Color = { 1.0f, 0.6f, 1.0f, 1.0f }; // Pink
			ImGui::SameLine();
			if (ImGui::ColorButton("##FolderPreset9", { 0.6f, 0.4f, 0.2f, 1.0f })) component.Color = { 0.6f, 0.4f, 0.2f, 1.0f }; // Brown
			ImGui::PopStyleVar();
		}, [](auto& component) {}, false);

		if (entityDeleted)
			DeleteEntity(entity);

	}

	bool SceneHierarchyPanel::DrawScriptField(const Entity& entity, const std::string& name, const ScriptField& field, void* data)
	{
		if (field.Type == ScriptFieldType::None)
		{
			ImGui::TextDisabled(name.c_str());
			return false;
		}
		else if (field.Type == ScriptFieldType::Float)
		{
			return ImGui::DragFloat(name.c_str(), (float*)data);
		}
		else if (field.Type == ScriptFieldType::Double)
		{
			return ImGui::InputDouble(name.c_str(), (double*)data);
		}
		else if (field.Type == ScriptFieldType::Bool)
		{
			return ImGui::Checkbox(name.c_str(), (bool*)data);
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
				return true;
			}
			return false;
		}
		else if (field.Type == ScriptFieldType::Byte)
		{
			static const uint8_t min = 0;
			static const uint8_t max = 255;
			return ImGui::DragScalar(name.c_str(), ImGuiDataType_U8, data, 1.0f, &min, &max, "%d", ImGuiSliderFlags_ClampOnInput);
		}
		else if (field.Type == ScriptFieldType::Short)
		{
			return ImGui::DragScalar(name.c_str(), ImGuiDataType_S16, data, 1.0f, nullptr, nullptr, "%d", ImGuiSliderFlags_ClampOnInput);
		}
		else if (field.Type == ScriptFieldType::Int)
		{
			return ImGui::DragScalar(name.c_str(), ImGuiDataType_S32, data, 1.0f, nullptr, nullptr, "%d", ImGuiSliderFlags_ClampOnInput);
		}
		else if (field.Type == ScriptFieldType::Long)
		{
			return ImGui::DragScalar(name.c_str(), ImGuiDataType_S64, data, 1.0f, nullptr, nullptr, "%lld", ImGuiSliderFlags_ClampOnInput);
		}
		else if (field.Type == ScriptFieldType::UShort)
		{
			return ImGui::DragScalar(name.c_str(), ImGuiDataType_U16, data, 1.0f, nullptr, nullptr, "%hu", ImGuiSliderFlags_ClampOnInput);
		}
		else if (field.Type == ScriptFieldType::UInt)
		{
			return ImGui::DragScalar(name.c_str(), ImGuiDataType_U32, data, 1.0f, nullptr, nullptr, "%u", ImGuiSliderFlags_ClampOnInput);
		}
		else if (field.Type == ScriptFieldType::ULong)
		{
			return ImGui::DragScalar(name.c_str(), ImGuiDataType_U64, data, 1.0f, nullptr, nullptr, "%llu", ImGuiSliderFlags_ClampOnInput);
		}
		else if (field.Type == ScriptFieldType::Vector2)
		{
			return ImGui::DragFloat2(name.c_str(), glm::value_ptr(*(glm::vec2*)(data)));
		}
		else if (field.Type == ScriptFieldType::Vector3)
		{
			return ImGui::DragFloat3(name.c_str(), glm::value_ptr(*(glm::vec3*)(data)));
		}
		else if (field.Type == ScriptFieldType::Vector4)
		{
			return ImGui::DragFloat4(name.c_str(), glm::value_ptr(*(glm::vec4*)(data)));
		}
		else if (field.Type == ScriptFieldType::Entity)
		{
			uint64_t uuid = *(uint64_t*)data;
			Entity selectedEntity = m_Context->GetEntityByUUID(uuid);

			bool returnValue = false;

			{
				bool active = ImGui::GetActiveID() == ImGui::GetCurrentWindow()->GetID("##EntityFieldNameInput") || ImGui::IsPopupOpen("##EntityFieldSearchPopup");

				char buffer[256];
				memset(buffer, 0, sizeof(buffer));
				std::strncpy(buffer, active ? (m_EntitySearchBuffer.c_str()) : (selectedEntity ? selectedEntity.GetName().c_str() : "None"), sizeof(buffer));
				ImGui::SetNextItemWidth(-90.0f);
				if (ImGui::InputText("##EntityFieldNameInput", buffer, sizeof(buffer), ImGuiInputTextFlags_AutoSelectAll))
					m_EntitySearchBuffer = buffer;

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

						auto view = m_Context->GetAllEntitiesWith<IDComponent>();
						for (auto& e : view)
						{
							Entity searchedEntity{ e , m_Context.get() };
							if (String::ToLower(searchedEntity.GetName()).find(String::ToLower(buffer)) != std::string::npos)
							{
								found = true;
								if (ImGui::MenuItem(searchedEntity.GetName().c_str()))
								{
									*(uint64_t*)data = searchedEntity.GetUUID();
									returnValue = true;
								}
							}
						}
						
						if (!found)
							ImGui::TextDisabled("No Entities Found");

						ImGui::EndPopup();

						if (ImGui::IsWindowFocused())
							ImGui::CloseCurrentPopup();
					}
				}

				if (GImGui->ActiveId != ImGui::GetCurrentWindow()->GetID("##EntityFieldNameInput") && GImGui->ActiveIdPreviousFrame == ImGui::GetCurrentWindow()->GetID("##EntityFieldNameInput") && !popupFocused)
				{
					if (Entity newEntity = m_Context->FindEntityByName(buffer))
					{
						*(uint64_t*)data = newEntity.GetUUID();
						returnValue = true;
					}

					ImGui::IsWindowHovered();
				}

				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCENE_HIERARCHY_ENTITY"))
					{
						Entity droppedEntity = *(Entity*)payload->Data;
						*(uint64_t*)data = droppedEntity.GetUUID();

						returnValue = true;
					}
					ImGui::EndDragDropTarget();
				}

				ImGui::SameLine();

				{
					const float lineHeight = ImGui::GetTextLineHeight();
					if (ImGui::ImageButton((ImTextureID)(uint64_t)(m_IconPicker->GetRendererID()), ImVec2(lineHeight, lineHeight), { 0, 1 }, { 1, 0 }))
					{
						m_PickingID = 1; // We are now picking.
						m_PickingField = name;
					}

					if (entity == m_ActiveEntity && m_PickingField == name && m_PickingID > 1)
					{
						// We've picked an object
						*(uint64_t*)data = m_PickingID;
						returnValue = true;
					}

					// Draw picking cursor
					if (m_PickingID == 1)
					{
						ImGui::SetMouseCursor(ImGuiMouseCursor_None);
						ImGui::GetForegroundDrawList()->AddImage((ImTextureID)(uint64_t)(m_IconPicker->GetRendererID()), ImGui::GetMousePos() + ImVec2(0.0f, -16.0f), ImGui::GetMousePos() + ImVec2(16.0f, 0.0f), {0, 1}, {1, 0});
					}
				}

				ImGui::SameLine();
				ImGui::Text(name.c_str());
			}

			return returnValue;
		}

		return false;
	}

	template<typename T>
	void SceneHierarchyPanel::DisplayAddComponentEntry(const std::string& entryName) {
		if (!m_ActiveEntity.HasComponent<T>())
		{
			if (ImGui::MenuItem(entryName.c_str()))
			{
				m_ActiveEntity.AddComponent<T>();
				ImGui::CloseCurrentPopup();
			}
		}
	}

	void SceneHierarchyPanel::DisplayCreateEntityPopup()
	{
		if (ImGui::BeginMenu(CHARACTER_ICON_ADD " Create"))
		{

			if (ImGui::MenuItem(CHARACTER_ICON_EMPTY " Empty Entity"))
				SelectedEntity(m_Context->CreateEntity("Empty Entity"));

			if (ImGui::MenuItem(CHARACTER_ICON_FOLDER " Folder")) { auto& entity = m_Context->CreateEntity("Folder"); entity.RemoveComponent<TransformComponent>(); entity.AddComponent<FolderComponent>(); SelectedEntity(entity); }

			ImGui::Separator();

			if (ImGui::BeginMenu(CHARACTER_ICON_CUBE " Mesh"))
			{
				if (ImGui::MenuItem(CHARACTER_ICON_CUBE " Static Mesh")) { auto& entity = m_Context->CreateEntity("Static Mesh"); entity.AddComponent<StaticMeshComponent>(); SelectedEntity(entity); }
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu(CHARACTER_ICON_POINT_LIGHT " Light"))
			{
				if (ImGui::MenuItem(CHARACTER_ICON_SUN " Directional Light")) { auto& entity = m_Context->CreateEntity("Directional Light"); entity.AddComponent<DirectionalLightComponent>(); SelectedEntity(entity); }
				if (ImGui::MenuItem(CHARACTER_ICON_POINT_LIGHT " Point Light")) { auto& entity = m_Context->CreateEntity("Point Light"); entity.AddComponent<PointLightComponent>(); SelectedEntity(entity); }
				if (ImGui::MenuItem(CHARACTER_ICON_SPOT_LIGHT " Spot Light")) { auto& entity = m_Context->CreateEntity("Spot Light"); entity.AddComponent<SpotLightComponent>(); SelectedEntity(entity); }
				if (ImGui::MenuItem(CHARACTER_ICON_CLOUDS " Sky Light")) { auto& entity = m_Context->CreateEntity("Sky Light"); entity.AddComponent<SkyLightComponent>(); SelectedEntity(entity); }
				ImGui::Separator();
				if (ImGui::MenuItem(CHARACTER_ICON_SMOKE " Volume")) { auto& entity = m_Context->CreateEntity("Volume"); entity.AddComponent<VolumeComponent>(); SelectedEntity(entity); }
				ImGui::EndMenu();
			}

			ImGui::Separator();

			if (ImGui::BeginMenu(CHARACTER_ICON_IMAGE " 2D"))
			{
				if (ImGui::MenuItem(CHARACTER_ICON_IMAGE " Sprite")) { auto& entity = m_Context->CreateEntity("Sprite"); entity.AddComponent<SpriteRendererComponent>(); SelectedEntity(entity); }
				if (ImGui::MenuItem(CHARACTER_ICON_SHADING_UNLIT " Circle")) { auto& entity = m_Context->CreateEntity("Circle"); entity.AddComponent<CircleRendererComponent>(); SelectedEntity(entity); }
				if (ImGui::MenuItem(CHARACTER_ICON_FONT " Text")) { auto& entity = m_Context->CreateEntity("Text"); entity.AddComponent<TextComponent>(); SelectedEntity(entity); }
				ImGui::EndMenu();
			}

			ImGui::Separator();

			if (ImGui::MenuItem(CHARACTER_ICON_CAMERA " Camera")) { auto& entity = m_Context->CreateEntity("Camera"); entity.AddComponent<CameraComponent>(); SelectedEntity(entity); }
			if (ImGui::MenuItem(CHARACTER_ICON_PARTICLES " Particle System")) { auto& entity = m_Context->CreateEntity("Particle System"); entity.AddComponent<ParticleSystemComponent>(); SelectedEntity(entity); }
			if (ImGui::MenuItem(CHARACTER_ICON_AUDIO " Audio")) { auto& entity = m_Context->CreateEntity("Audio"); entity.AddComponent<AudioComponent>(); SelectedEntity(entity); }

			ImGui::Separator();

			if (ImGui::MenuItem(CHARACTER_ICON_SCRIPT " Script")) { auto& entity = m_Context->CreateEntity("Script"); entity.AddComponent<ScriptComponent>(); SelectedEntity(entity); }

			ImGui::EndMenu();
		}
	}

}