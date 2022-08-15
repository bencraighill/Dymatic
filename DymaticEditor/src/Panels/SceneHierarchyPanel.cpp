#include "SceneHierarchyPanel.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <glm/gtc/type_ptr.hpp>

#include "Dymatic/Scene/Components.h"
#include <cstring>

#include "../TextSymbols.h"

#include "Dymatic/Utils/PlatformUtils.h"

#include "Dymatic/Scene/ScriptEngine.h"

#include "Dymatic/Math/StringUtils.h"

/* The Microsoft C++ compiler is non-compliant with the C++ standard and needs
 * the following definition to disable a security warning on std::strncpy().
 */
#ifdef _MSVC_LANG
#define _CRT_SECURE_NO_WARNINGS
#endif

namespace Dymatic {

	extern const std::filesystem::path g_AssetPath;

	SceneHierarchyPanel::SceneHierarchyPanel(const Ref<Scene>& context)
	{
		SetContext(context);
	}

	void SceneHierarchyPanel::SetContext(const Ref<Scene>& context)
	{
		m_Context = context;
		m_SelectionContext = {};
	}

	void SceneHierarchyPanel::OnImGuiRender()
	{
		if (m_SceneHierarchyVisible)
		{
			ImGui::Begin((std::string(CHARACTER_WINDOW_ICON_SCENE_HIERARCHY) + " Scene Hierarchy").c_str(), &m_SceneHierarchyVisible);
			auto& style = ImGui::GetStyle();

			// Searchbar
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
				ImGui::Image((ImTextureID)m_SearchbarIcon->GetRendererID(), size, { 0, 1 }, { 1, 0 }, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
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

					auto color = ImGui::GetStyleColorVec4(ImGui::IsItemActive() ? ImGuiCol_ButtonActive : (ImGui::IsItemHovered() ? ImGuiCol_ButtonHovered : ImGuiCol_Text));
					ImGui::PushStyleColor(ImGuiCol_Button, {});
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, {});
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {});
					ImGui::Image((ImTextureID)m_ClearIcon->GetRendererID(), size, { 0, 1 }, { 1, 0 }, color);
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
			ImGui::BeginTable("##SceneHierarchyTable", 3, flags);
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

			// Pop Table Styles
			ImGui::PopStyleVar(2);
			ImGui::PopStyleColor();

			if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
				m_SelectionContext = {};

			// Right-click on blank space
			if (m_ShowCreateMenu)
			{
				ImGui::OpenPopup(ImGui::GetID("##CreateEntityPopup"));
				m_ShowCreateMenu = false;
			}

			// Create Entity Context Popup
			if (ImGui::BeginPopupContextWindow(0, 1, false) || ImGui::BeginPopupEx(ImGui::GetID("##CreateEntityPopup"), ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings))
			{
				if (ImGui::BeginMenu("Create"))
				{

					if (ImGui::MenuItem("Empty Entity"))
						SetSelectedEntity(m_Context->CreateEntity("Empty Entity"));

					if (ImGui::MenuItem("Folder")) { auto& entity = m_Context->CreateEntity("Folder"); entity.RemoveComponent<TransformComponent>(); entity.AddComponent<FolderComponent>(); SetSelectedEntity(entity); }

					ImGui::Separator();

					if (ImGui::BeginMenu("Mesh"))
					{
						if (ImGui::MenuItem("Static Mesh")) { auto& entity = m_Context->CreateEntity("Static Mesh"); entity.AddComponent<StaticMeshComponent>(); SetSelectedEntity(entity); }
						ImGui::EndMenu();
					}

					if (ImGui::BeginMenu("Light"))
					{
						if (ImGui::MenuItem("Directional Light")) { auto& entity = m_Context->CreateEntity("Directional Light"); entity.AddComponent<DirectionalLightComponent>(); SetSelectedEntity(entity); }
						if (ImGui::MenuItem("Point Light")) { auto& entity = m_Context->CreateEntity("Point Light"); entity.AddComponent<PointLightComponent>(); SetSelectedEntity(entity); }
						if (ImGui::MenuItem("Spot Light")) { auto& entity = m_Context->CreateEntity("Spot Light"); entity.AddComponent<SpotLightComponent>(); SetSelectedEntity(entity); }
						if (ImGui::MenuItem("Sky Light")) { auto& entity = m_Context->CreateEntity("Sky Light"); entity.AddComponent<SkylightComponent>(); SetSelectedEntity(entity); }
						ImGui::EndMenu();
					}

					ImGui::Separator();

					if (ImGui::MenuItem("Camera")) { auto& entity = m_Context->CreateEntity("Camera"); entity.AddComponent<CameraComponent>(); SetSelectedEntity(entity); }
					if (ImGui::MenuItem("Sprite")) { auto& entity = m_Context->CreateEntity("Sprite"); entity.AddComponent<SpriteRendererComponent>(); SetSelectedEntity(entity); }
					if (ImGui::MenuItem("Particle System")) { auto& entity = m_Context->CreateEntity("Particle System"); entity.AddComponent<ParticleSystemComponent>(); SetSelectedEntity(entity); }
					if (ImGui::MenuItem("Audio")) { auto& entity = m_Context->CreateEntity("Audio"); entity.AddComponent<AudioComponent>(); SetSelectedEntity(entity); }

					ImGui::Separator();

					if (ImGui::MenuItem("Native Script")) { auto& entity = m_Context->CreateEntity("Native Script"); entity.AddComponent<NativeScriptComponent>(); SetSelectedEntity(entity); }

					ImGui::EndMenu();
				}
				ImGui::EndPopup();
			}

			// End Table
			ImGui::EndTable();
			ImGui::EndChild();

			ImGui::End();
		}

		if (m_PropertiesVisible)
		{
			ImGui::Begin((std::string(CHARACTER_WINDOW_ICON_PROPERTIES) + " Properties").c_str(), &m_PropertiesVisible);
			if (m_SelectionContext)
			{
				DrawComponents(m_SelectionContext);
			}

			ImGui::End();
		}
	}

	void SceneHierarchyPanel::SetSelectedEntity(Entity entity)
	{
		m_SelectionContext = entity;
	}

	void SceneHierarchyPanel::DeleteEntity(Entity entity)
	{
		m_Context->DestroyEntity(entity);
		if (m_SelectionContext == entity)
			m_SelectionContext = {};
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
			ImGui::Image((ImTextureID)m_FolderIcon->GetRendererID(), ImVec2{ 16, 16 }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 }, { color.r, color.g, color.b, 1.0f });
		}
		else
			ImGui::Image((ImTextureID)m_EntityIcon->GetRendererID(), ImVec2{ 16, 16 }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 }, ImVec4{ 0.75f, 0.5f, 0.32f, 1.0f });
		ImGui::SameLine();
		ImGui::Selectable("##EntitySelectable", m_SelectionContext == entity, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap);

		if (ImGui::IsItemClicked())
			m_SelectionContext = entity;

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
			if (ImGui::MenuItem((std::string(CHARACTER_PROPERTIES_ICON_DELETE) + " Delete").c_str()))
				entityDeleted = true;
			if (ImGui::MenuItem((std::string(CHARACTER_PROPERTIES_ICON_DUPLICATE) + " Duplicate").c_str()))
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
	static void DrawComponent(const std::string& name, Entity entity, UIFunction uiFunction, UISettings uiSettings)
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
			//ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);
			//if (ImGui::Button("+", ImVec2{ lineHeight, lineHeight }))
			const float button_width = 31.0f;
			ImGui::SameLine(contentRegionAvailable.x - ((button_width + ImGui::GetStyle().FramePadding.x * 2.0f) * 0.5f));
			if (ImGui::Button(CHARACTER_WINDOW_ICON_PREFERENCES, ImVec2{ button_width, lineHeight }))
			{
				ImGui::OpenPopup("ComponentSettings");
			}

			bool removeComponent = false;
			if (ImGui::BeginPopup("ComponentSettings"))
			{
				ImGui::TextDisabled("Component Settings");
				ImGui::Separator();

				uiSettings(component);

				if (ImGui::MenuItem("Remove component"))
					removeComponent = true;

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

	void SceneHierarchyPanel::DrawComponents(Entity entity)
	{
		bool entityDeleted = false;

		if (entity.HasComponent<TagComponent>())
		{
			auto& tag = entity.GetComponent<TagComponent>().Tag;

			char buffer[256];
			memset(buffer, 0, sizeof(buffer));
			std::strncpy(buffer, tag.c_str(), sizeof(buffer));
			if (ImGui::InputText("##Tag", buffer, sizeof(buffer)))
			{
				tag = std::string(buffer);
			}
		}

		ImGui::SameLine();
		ImGui::PushItemWidth(-1);

		if (!entity.HasComponent<FolderComponent>())
			if (ImGui::Button(std::string(CHARACTER_PROPERTIES_ICON_ADD).c_str(), ImVec2{ 31.0f, 24.0f }))
				ImGui::OpenPopup("AddComponent");
		ImGui::SameLine();
		if (ImGui::Button(std::string(CHARACTER_PROPERTIES_ICON_DUPLICATE).c_str(), ImVec2{ 31.0f, 24.0f }))
			m_Context->DuplicateEntity(m_SelectionContext);
		ImGui::SameLine();
		if (ImGui::Button(std::string(CHARACTER_PROPERTIES_ICON_DELETE).c_str(), ImVec2{ 31.0f, 24.0f }))
			entityDeleted = true;

		if (ImGui::BeginPopup("AddComponent"))
		{
			ImGui::TextDisabled("Add Component");
			ImGui::Separator();

			if (ImGui::BeginMenu("Mesh"))
			{
				DisplayAddComponentEntry<StaticMeshComponent>("Static Mesh");
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Light"))
			{
				DisplayAddComponentEntry<DirectionalLightComponent>("Directional Light");
				DisplayAddComponentEntry<PointLightComponent>("Point Light");
				DisplayAddComponentEntry<SpotLightComponent>("Spot Light");
				DisplayAddComponentEntry<SkylightComponent>("Sky Light");
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("2D"))
			{
				DisplayAddComponentEntry<SpriteRendererComponent>("Sprite Renderer");
				DisplayAddComponentEntry<CircleRendererComponent>("Circle Renderer");
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Physics"))
			{
				DisplayAddComponentEntry<RigidbodyComponent>("Rigidbody");
				DisplayAddComponentEntry<BoxColliderComponent>("Box Collider");
				DisplayAddComponentEntry<SphereColliderComponent>("Sphere Collider");
				DisplayAddComponentEntry<CapsuleColliderComponent>("Capsule Collider");
				DisplayAddComponentEntry<MeshColliderComponent>("Mesh Collider");

				ImGui::Separator();

				if (ImGui::BeginMenu("2D"))
				{
					DisplayAddComponentEntry<Rigidbody2DComponent>("Rigidbody 2D");
					DisplayAddComponentEntry<BoxCollider2DComponent>("Box Collider 2D");
					DisplayAddComponentEntry<CircleCollider2DComponent>("Circle Collider 2D");
					ImGui::EndMenu();
				}

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("UI"))
			{
				DisplayAddComponentEntry<UICanvasComponent>("Canvas");
				DisplayAddComponentEntry<UIImageComponent>("Image");
				DisplayAddComponentEntry<UIButtonComponent>("Button");
				ImGui::EndMenu();
			}

			ImGui::Separator();

			if (!m_SelectionContext.HasComponent<CameraComponent>())
			{
				if (ImGui::MenuItem("Camera"))
				{
					m_SelectionContext.AddComponent<CameraComponent>();
					ImGui::CloseCurrentPopup();
				}
			}

			if (!m_SelectionContext.HasComponent<ParticleSystemComponent>())
			{
				if (ImGui::MenuItem("Particle System"))
				{
					m_SelectionContext.AddComponent<ParticleSystemComponent>();
					ImGui::CloseCurrentPopup();
				}
			}

			if (!m_SelectionContext.HasComponent<AudioComponent>())
			{
				if (ImGui::MenuItem("Audio"))
				{
					m_SelectionContext.AddComponent<AudioComponent>();
					ImGui::CloseCurrentPopup();
				}
			}

			ImGui::Separator();

			if (!m_SelectionContext.HasComponent<NativeScriptComponent>())
			{
				if (ImGui::MenuItem("Native Script"))
				{
					m_SelectionContext.AddComponent<NativeScriptComponent>();
					ImGui::CloseCurrentPopup();
				}
			}

			ImGui::EndPopup();
		}

		ImGui::PopItemWidth();

		DrawComponent<TransformComponent>("TRANSFORM", entity, [](auto& component)
			{
				DrawVec3Control("Translation", component.Translation);
				glm::vec3 rotation = glm::degrees(component.Rotation);
				DrawVec3Control("Rotation", rotation);
				component.Rotation = glm::radians(rotation);
				DrawVec3Control("Scale", component.Scale, glm::vec3(1.0f));
			},
			[](auto& component) 
			{
				if (ImGui::MenuItem("Copy"))
				{
					std::stringstream ss;
					ss << component.Translation.x << component.Translation.y << component.Translation.z
						<< component.Rotation.x  << component.Rotation.y  << component.Rotation.z
						<< component.Scale.x  << component.Scale.y << component.Scale.z;
					ImGui::SetClipboardText(ss.str().c_str());
				}
				if (ImGui::MenuItem("Paste"))
				{
					std::stringstream ss;
					ss << ImGui::GetClipboardText();
					ss >> component.Translation.x >> component.Translation.y >> component.Translation.z
						>> component.Rotation.x >> component.Rotation.y >> component.Rotation.z
						>> component.Scale.x >> component.Scale.y >> component.Scale.z;
				}
			}
			);

		DrawComponent<CameraComponent>("CAMERA", entity, [](auto& component)
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

		DrawComponent<SpriteRendererComponent>("SPRITE RENDERER", entity, [this](auto& component)
		{
			ImGui::ColorEdit4("Color", glm::value_ptr(component.Color));
			
			ImGui::ImageButton(component.Texture ? ((ImTextureID)component.Texture->GetRendererID()) : ((ImTextureID)m_CheckerboardTexture->GetRendererID()), ImVec2(100.0f, 100.0f), ImVec2( 0, 1 ), ImVec2( 1, 0));
			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
				{
					const wchar_t* path = (const wchar_t*)payload->Data;
					std::filesystem::path texturePath = std::filesystem::path(g_AssetPath) / path;
					Ref<Texture2D> texture = Texture2D::Create(texturePath.string());
					if (texture->IsLoaded())
						component.Texture = texture;
					else
						DY_WARN("Could not load texture {0}", texturePath.filename().string());
				}
				ImGui::EndDragDropTarget();
			}

			ImGui::DragFloat("Tiling Factor", &component.TilingFactor, 0.1f, 0.0f, 100.0f);
		}, [](auto& component) {});

		DrawComponent<CircleRendererComponent>("CIRCLE RENDERER", entity, [](auto& component)
		{
			ImGui::ColorEdit4("Color", glm::value_ptr(component.Color));
			ImGui::DragFloat("Thickness", &component.Thickness, 0.025f, 0.0f, 1.0f);
			ImGui::DragFloat("Fade", &component.Fade, 0.00025f, 0.0f, 1.0f);
		}, [](auto& component) {});

		DrawComponent<ParticleSystemComponent>("PARTICLE SYSTEM", entity, [](auto& component)
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
				if (ImGui::Button(std::string("+##AddColorPoint").c_str(), ImVec2{ lineHeight, lineHeight }))
				{
					component.ColorPoints.push_back({ component.GetNextColorPointId() });
				}
				ImGui::PopStyleVar();
				if (open)
				{
					for (int i = 0; i < component.ColorPoints.size(); i++)
					{
						bool removeIndex = false;
						DY_CORE_WARN(component.ColorPoints[i].GetId());

						ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x / 6);
						if (ImGui::DragFloat((std::string("##ColorPoint-Point-") + std::to_string(component.ColorPoints[i].GetId())).c_str(), &component.ColorPoints[i].point, 0.001f, 0.0f, 1.0f)) { component.RecalculateColorPointOrder(); }
						ImGui::PopItemWidth();

						//Remove Context
						if (ImGui::BeginPopupContextItem((std::string("##RemoveColorPoint-") + std::to_string(component.ColorPoints[i].GetId())).c_str()))
						{
							if (ImGui::MenuItem("Remove Color Point"))
								removeIndex = true;
							if (ImGui::MenuItem("Duplicate Color Point"))
								component.DuplicateColorPoint(i);

							ImGui::EndPopup();
						}

						ImGui::SameLine();
						ImGui::ColorEdit4((std::string("##ColorPoint-Color-") + std::to_string(component.ColorPoints[i].id)).c_str(), glm::value_ptr(component.ColorPoints[i].color));

						if (removeIndex) { component.ColorPoints.erase(component.ColorPoints.begin() + i); }
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

		DrawComponent<Rigidbody2DComponent>("RIGIDBODY 2D", entity, [](auto& component)
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

		DrawComponent<BoxCollider2DComponent>("BOX COLLIDER 2D", entity, [](auto& component)
		{
			ImGui::DragFloat2("Offset", glm::value_ptr(component.Offset));
			ImGui::DragFloat2("Size", glm::value_ptr(component.Size));
			ImGui::DragFloat("Density", &component.Density, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat("Friction", &component.Friction, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat("Restitution", &component.Restitution, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat("RestitutionThreshold", &component.RestitutionThreshold, 0.01f, 0.0f);
		}, [](auto& component) {});

		DrawComponent<CircleCollider2DComponent>("CIRCLE COLLIDER 2D", entity, [](auto& component)
		{
			ImGui::DragFloat2("Offset", glm::value_ptr(component.Offset));
			ImGui::DragFloat("Radius", &component.Radius);
			ImGui::DragFloat("Density", &component.Density, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat("Friction", &component.Friction, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat("Restitution", &component.Restitution, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat("RestitutionThreshold", &component.RestitutionThreshold, 0.01f, 0.0f);
		}, [](auto& component) {});

		DrawComponent<StaticMeshComponent>("MESH", entity, [this](auto& component)
			{
				if (!component.m_Path.empty())
					ImGui::Text("Model: %s", component.m_Path.c_str());

				if (component.m_Model)
				{
					// Materials Section
					{
						ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
						bool open = ImGui::TreeNodeEx("Materials", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding);
						ImGui::PopStyleVar();
						if (open)
						{
							for (auto& mesh : component.m_Model->GetMeshes())
							{
								if (ImGui::TreeNode(mesh->GetName().c_str()))
								{
									auto material = mesh->GetMaterial();

									if (ImGui::TreeNodeEx("Albedo", ImGuiTreeNodeFlags_DefaultOpen))
									{
										ImGui::Image((ImTextureID)(material->m_AlbedoMap ? material->m_AlbedoMap->GetRendererID() : m_CheckerboardTexture->GetRendererID()), ImVec2(25.0f, 25.0f), { 0, 1 }, { 1, 0 });
										if (ImGui::IsItemClicked())
										{
											std::string filepath = FileDialogs::OpenFile("");
											if (!filepath.empty())
											{
												Ref<Texture2D> texture = Texture2D::Create(filepath);
												if (texture->IsLoaded())
													material->m_AlbedoMap = texture;
											}
										}
										if (ImGui::BeginPopupContextItem("##Context"))
										{
											if (ImGui::MenuItem("Clear"))
												material->m_AlbedoMap = nullptr;
											ImGui::EndPopup();
										}

										ImGui::SameLine();

										ImGui::ColorEdit3("##AlbedoColorPicker", glm::value_ptr(material->m_MaterialData.Albedo));
										ImGui::SameLine();
										ImGui::Text("Color");

										ImGui::TreePop();
									}

									if (ImGui::TreeNodeEx("Normal", ImGuiTreeNodeFlags_DefaultOpen))
									{
										ImGui::Image((ImTextureID)(material->m_NormalMap ? material->m_NormalMap->GetRendererID() : m_CheckerboardTexture->GetRendererID()), ImVec2(25.0f, 25.0f), { 0, 1 }, { 1, 0 });
										if (ImGui::IsItemClicked())
										{
											std::string filepath = FileDialogs::OpenFile("");
											if (!filepath.empty())
											{
												Ref<Texture2D> texture = Texture2D::Create(filepath);
												if (texture->IsLoaded())
													material->m_NormalMap = texture;
											}
										}
										if (ImGui::BeginPopupContextItem("##Context"))
										{
											if (ImGui::MenuItem("Clear"))
												material->m_NormalMap = nullptr;
											ImGui::EndPopup();
										}

										ImGui::SameLine();

										ImGui::DragFloat("##NormalValueInput", &material->m_MaterialData.Normal, 0.01f, 0.0, 1.0f);
										ImGui::SameLine();
										ImGui::Text("Multiplier");

										ImGui::TreePop();
									}

									if (ImGui::TreeNodeEx("Specular", ImGuiTreeNodeFlags_DefaultOpen))
									{
										ImGui::Image((ImTextureID)(material->m_SpecularMap ? material->m_SpecularMap->GetRendererID() : m_CheckerboardTexture->GetRendererID()), ImVec2(25.0f, 25.0f), { 0, 1 }, { 1, 0 });
										if (ImGui::IsItemClicked())
										{
											std::string filepath = FileDialogs::OpenFile("");
											if (!filepath.empty())
											{
												Ref<Texture2D> texture = Texture2D::Create(filepath);
												if (texture->IsLoaded())
													material->m_SpecularMap = texture;
											}
										}
										if (ImGui::BeginPopupContextItem("##Context"))
										{
											if (ImGui::MenuItem("Clear"))
												material->m_SpecularMap = nullptr;
											ImGui::EndPopup();
										}

										ImGui::SameLine();

										ImGui::ColorEdit3("##SpecularColorPicker", glm::value_ptr(material->m_MaterialData.Specular));
										ImGui::SameLine();
										ImGui::Text("Color");

										ImGui::TreePop();
									}

									if (ImGui::TreeNodeEx("Rougness", ImGuiTreeNodeFlags_DefaultOpen))
									{
										ImGui::Image((ImTextureID)(material->m_RougnessMap ? material->m_RougnessMap->GetRendererID() : m_CheckerboardTexture->GetRendererID()), ImVec2(25.0f, 25.0f), { 0, 1 }, { 1, 0 });
										if (ImGui::IsItemClicked())
										{
											std::string filepath = FileDialogs::OpenFile("");
											if (!filepath.empty())
											{
												Ref<Texture2D> texture = Texture2D::Create(filepath);
												if (texture->IsLoaded())
													material->m_RougnessMap = texture;
											}
										}
										if (ImGui::BeginPopupContextItem("##Context"))
										{
											if (ImGui::MenuItem("Clear"))
												material->m_RougnessMap = nullptr;
											ImGui::EndPopup();
										}

										ImGui::SameLine();

										ImGui::DragFloat("##RougnessValueInput", &material->m_MaterialData.Rougness, 0.01f, 0.0, 1.0f);
										ImGui::SameLine();
										ImGui::Text("Rougness");

										ImGui::TreePop();
									}

									if (ImGui::TreeNodeEx("Alpha", ImGuiTreeNodeFlags_DefaultOpen))
									{
										if (ImGui::BeginCombo("##AlphaBlendMode", Material::AlphaBlendModeToString((Dymatic::Material::AlphaBlendMode)material->m_MaterialData.BlendMode)))
										{
											if (ImGui::MenuItem("Opaque")) material->m_MaterialData.BlendMode = Material::AlphaBlendMode::Opaque;
											if (ImGui::MenuItem("Masked")) material->m_MaterialData.BlendMode = Material::AlphaBlendMode::Masked;
											if (ImGui::MenuItem("Translucent")) material->m_MaterialData.BlendMode = Material::AlphaBlendMode::Translucent;
											ImGui::EndCombo();
										}

										ImGui::Image((ImTextureID)(material->m_AlphaMap ? material->m_AlphaMap->GetRendererID() : m_CheckerboardTexture->GetRendererID()), ImVec2(25.0f, 25.0f), { 0, 1 }, { 1, 0 });
										if (ImGui::IsItemClicked())
										{
											std::string filepath = FileDialogs::OpenFile("");
											if (!filepath.empty())
											{
												Ref<Texture2D> texture = Texture2D::Create(filepath);
												if (texture->IsLoaded())
													material->m_AlphaMap = texture;
											}
										}
										if (ImGui::BeginPopupContextItem("##Context"))
										{
											if (ImGui::MenuItem("Clear"))
												material->m_AlphaMap = nullptr;
											ImGui::EndPopup();
										}

										ImGui::SameLine();

										ImGui::DragFloat("##AlphaValueInput", &material->m_MaterialData.Alpha, 0.01f, 0.0, 1.0f);
										ImGui::SameLine();
										ImGui::Text("Alpha");

										ImGui::TreePop();
									}

									ImGui::TreePop();
								}
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

				if (ImGui::Button("Open Model", ImVec2(-1.0f, 0.0f)))
				{
					std::string filepath = FileDialogs::OpenFile("3D Object");
					if (!filepath.empty())
						component.LoadModel(filepath);
				}

				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
					{
						const wchar_t* path = (const wchar_t*)payload->Data;
						std::filesystem::path modelPath = std::filesystem::path(g_AssetPath) / path;
						component.LoadModel(modelPath.string());
					}
					ImGui::EndDragDropTarget();
				}

				if (!component.m_Path.empty())
					if (ImGui::Button("Reload", ImVec2(-1.0f, 0.0f)))
						component.ReloadModel();
			}, [](auto& component) {});

			DrawComponent<DirectionalLightComponent>("DIRECTIONAL LIGHT", entity, [](auto& component)
			{
				ImGui::ColorEdit3("Color", glm::value_ptr(component.Color));
			}, [](auto& component) {});

			DrawComponent<PointLightComponent>("POINT LIGHT", entity, [](auto& component)
			{
				ImGui::ColorEdit3("Color", glm::value_ptr(component.Color));
				ImGui::DragFloat("Intensity", &component.Intensity);
				ImGui::DragFloat("Constant", &component.Constant);
				ImGui::DragFloat("Linear", &component.Linear);
				ImGui::DragFloat("Quadratic", &component.Quadratic);
			}, [](auto& component) {});

			DrawComponent<SpotLightComponent>("SPOT LIGHT", entity, [](auto& component)
			{
				ImGui::ColorEdit3("Color", glm::value_ptr(component.Color));
				ImGui::DragFloat("Cut Off", &component.CutOff);
				ImGui::DragFloat("Outer Cut Off", &component.OuterCutOff);
				ImGui::DragFloat("Constant", &component.Constant);
				ImGui::DragFloat("Linear", &component.Linear);
				ImGui::DragFloat("Quadratic", &component.Quadratic);
			}, [](auto& component) {});

			DrawComponent<SkylightComponent>("SKY LIGHT", entity, [](auto& component)
			{
				if (ImGui::Button("Load Cubemap", ImVec2(-1.0f, 0.0f)))
				{
					std::string paths[6];
					paths[0] = FileDialogs::OpenFile("Right");
					paths[1] = FileDialogs::OpenFile("Left");
					paths[2] = FileDialogs::OpenFile("Top");
					paths[3] = FileDialogs::OpenFile("Bottom");
					paths[4] = FileDialogs::OpenFile("Front");
					paths[5] = FileDialogs::OpenFile("Back");

					Ref<TextureCube> texture = TextureCube::Create(paths);
					if (texture->IsLoaded())
					{
						component.EnvironmentMap = texture;
					}
					else
						DY_WARN("Could not load cubemap {0}", paths[0]);
				}

				ImGui::DragFloat("Intensity", &component.Intensity);
			}, [](auto& component) {});

			DrawComponent<AudioComponent>("AUDIO", entity, [](auto& component)
			{
					if (ImGui::Button("Open Audio", ImVec2(-1.0f, 0.0f)))
					{
						std::string filepath = FileDialogs::OpenFile("Audio File");
						if (!filepath.empty())
							component.LoadSound(filepath);
					}
					if (ImGui::BeginDragDropTarget())
					{
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
						{
							const wchar_t* path = (const wchar_t*)payload->Data;
							std::filesystem::path soundPath = std::filesystem::path(g_AssetPath) / path;
							component.LoadSound(soundPath.string());
						}
						ImGui::EndDragDropTarget();
					}
					auto& sound = component.GetSound();
					if (sound)
					{
						ImGui::Text(component.GetPath().c_str());

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

							bool startOnAwake = sound->GetStartOnAwake();
							if (ImGui::Checkbox("Start On Awake", &startOnAwake))
								sound->SetStartOnAwake(startOnAwake);

							float radius = sound->GetRadius();
							if (ImGui::InputFloat("Radius", &radius))
								sound->SetRadius(radius);

							{
								int position = sound->GetPlayPosition();
								ImGui::PushStyleColor(ImGuiCol_Text, {});
								if (ImGui::SliderInt("##PlayPositionSlider", &position, 0, sound->GetPlayLength()))
									sound->SetPlayPosition(position);
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

			DrawComponent<RigidbodyComponent>("RIGIDBODY", entity, [](auto& component)
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

				ImGui::Text("Density");
				ImGui::SameLine();
				ImGui::DragFloat("##DensityInput", &component.Density, 0.1f, 0.0f, 0.0f, "%.2f");
			}, [](auto& component) {});

			DrawComponent<BoxColliderComponent>("BOX COLLIDER", entity, [](auto& component)
			{
				ImGui::Text("Size");
				ImGui::SameLine();
				ImGui::DragFloat3("##BoxColliderScaleInput", glm::value_ptr(component.Size), 0.1f, 0.0f, 0.0f, "%.2f");
			}, [](auto& component) {});

			DrawComponent<SphereColliderComponent>("SPHERE COLLIDER", entity, [](auto& component)
			{
				ImGui::Text("Radius");
				ImGui::SameLine();
				ImGui::DragFloat("##SphereColliderRadiusInput", &component.Radius, 0.1f, 0.0f, 0.0f, "%.2f");
			}, [](auto& component) {});

			DrawComponent<CapsuleColliderComponent>("CAPSULE COLLIDER", entity, [](auto& component)
			{
				ImGui::Text("Radius");
				ImGui::SameLine();
				ImGui::DragFloat("##CapsuleColliderRadiusInput", &component.Radius, 0.1f, 0.0f, 0.0f, "%.2f");
				ImGui::Text("Half Height");
				ImGui::SameLine();
				ImGui::DragFloat("##CapsuleColliderHalfHeightInput", &component.HalfHeight, 0.1f, 0.0f, 0.0f, "%.2f");
			}, [](auto& component) {});

			DrawComponent<MeshColliderComponent>("MESH COLLIDER", entity, [](auto& component)
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

			DrawComponent<UICanvasComponent>("UI CANVAS", entity, [](auto& component)
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

			DrawComponent<UIImageComponent>("UI IMAGE", entity, [this](auto& component)
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
				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
					{
						const wchar_t* path = (const wchar_t*)payload->Data;
						std::filesystem::path texturePath = std::filesystem::path(g_AssetPath) / path;
						Ref<Texture2D> texture = Texture2D::Create(texturePath.string());
						if (texture->IsLoaded())
							component.Image = texture;
						else
							DY_WARN("Could not load texture {0}", texturePath.filename().string());
					}
					ImGui::EndDragDropTarget();
				}

			}, [](auto& component) {});

			DrawComponent<UIButtonComponent>("UI BUTTON", entity, [](auto& component)
			{
				ImGui::Text("Button Placeholder");
			}, [](auto& component) {});

			DrawComponent<NativeScriptComponent>("NATIVE SCRIPT", entity, [this](auto& component)
			{
				bool exists = ScriptEngine::DoesScriptExist(component.ScriptName);
				if (!exists)
					ImGui::PushStyleColor(ImGuiCol_FrameBg, { 1.0f, 0.4f, 0.4f, 0.5f });

				bool existsNow = exists;

				char buffer[256];
				memset(buffer, 0, sizeof(buffer));
				std::strncpy(buffer, component.ScriptName.c_str(), sizeof(buffer));
				if (ImGui::InputText("##ScriptNameInput", buffer, sizeof(buffer), ImGuiInputTextFlags_AutoSelectAll))
					existsNow = ScriptEngine::UpdateScriptName(component, std::string(buffer));

				if (ImGui::IsItemActive() || ImGui::IsPopupOpen("##ScriptSearchPopup"))
				{
					const ImVec2 pos = { ImGui::GetItemRectMin().x, ImGui::GetItemRectMax().y };

					ImGui::OpenPopup("##ScriptSearchPopup");
					if (ImGui::BeginPopup("##ScriptSearchPopup", ImGuiWindowFlags_NoFocusOnAppearing))
					{
						ImGui::SetWindowPos(pos);

						bool found = false;
						for (auto& script : ScriptEngine::GetScriptList())
						{
							std::string search = component.ScriptName;
							String::ToLower(search);
							std::string name = script;
							String::ToLower(name);

							if (name.find(search) != std::string::npos)
							{
								found = true;
								if (ImGui::MenuItem(script.c_str()))
									ScriptEngine::UpdateScriptName(component, script);
							}
						}
						if (!found)
							ImGui::TextDisabled("No Scripts Found");
						ImGui::EndPopup();
						if (ImGui::IsWindowFocused())
							ImGui::CloseCurrentPopup();
					}
				}

				if (exists)
				{
					std::vector<ScriptEngine::ParamData> params;
					ScriptEngine::GetScriptParamData(component.ScriptName, params);

					std::vector<void*> data;
					if (component.Instance)
						ScriptEngine::GetInstanceParamPointers(component, data);
					for (size_t i = 0; i < params.size() && i < component.Params.size(); i++)
					{
						ImGui::PushID(i);
						auto& param = params[i];
						ImGui::Text(param.name.c_str());
						ImGui::SameLine();
						switch (param.type)
						{
						case ScriptEngine::ParamData::None:
							ImGui::TextDisabled("Unknown Type");
							break;
						case ScriptEngine::ParamData::Boolean:
							ImGui::Checkbox("##ParamBooleanCheckbox", component.Instance ? (bool*)data[i] : &component.Params[i].Boolean);
							break;
						case ScriptEngine::ParamData::Integer:
							ImGui::InputInt("##ParamIntegerInput", component.Instance ? (int*)data[i] : &component.Params[i].Integer);
							break;
						case ScriptEngine::ParamData::Float:
							ImGui::InputFloat("##ParamFloatInput", component.Instance ? (float*)data[i] : &component.Params[i].Float);
							break;
						}
						
						if (!component.Instance)
						{
							bool modified = false;
							switch (param.type)
							{
							case ScriptEngine::ParamData::Boolean:
								modified = component.Params[i].Boolean != param.defaultValue.Boolean;
								break;
							case ScriptEngine::ParamData::Integer:
								modified = component.Params[i].Integer != param.defaultValue.Integer;
								break;
							case ScriptEngine::ParamData::Float:
								modified = component.Params[i].Float != param.defaultValue.Float;
								break;
							}

							if (modified)
							{
								ImGui::SameLine();
								ImGui::PushStyleColor(ImGuiCol_Button, {});
								ImGui::BeginGroup();
								ImGui::Dummy({ 0.0f, ImGui::GetItemRectSize().y * 0.5f - 20.0f });

								if (ImGui::ImageButton((ImTextureID)m_IconRevert->GetRendererID(), { 10.0f, 10.0f }, { 0, 1 }, { 1, 0 }))
								{
									switch (param.type)
									{
									case ScriptEngine::ParamData::Boolean:
										component.Params[i].Boolean = param.defaultValue.Boolean;
										break;
									case ScriptEngine::ParamData::Integer:
										component.Params[i].Integer = param.defaultValue.Integer;
										break;
									case ScriptEngine::ParamData::Float:
										component.Params[i].Float = param.defaultValue.Float;
										break;
									}
								}

								ImGui::EndGroup();
								ImGui::PopStyleColor();
							}
						}

						ImGui::PopID();
					}
				}

				if (!exists)
					ImGui::PopStyleColor();
			}, [](auto& component) {});

			DrawComponent<FolderComponent>("FOLDER SETTINGS", entity, [](auto& component)
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
			}, [](auto& component) {});

		if (entityDeleted)
			DeleteEntity(entity);

	}

	template<typename T>
	void SceneHierarchyPanel::DisplayAddComponentEntry(const std::string& entryName) {
		if (!m_SelectionContext.HasComponent<T>())
		{
			if (ImGui::MenuItem(entryName.c_str()))
			{
				m_SelectionContext.AddComponent<T>();
				ImGui::CloseCurrentPopup();
			}
		}
	}

}