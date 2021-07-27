#include "SceneHierarchyPanel.h"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <glm/gtc/type_ptr.hpp>

#include "Dymatic/Scene/Components.h"
#include <cstring>

#include "../TextSymbols.h"

/* The Microsoft C++ compiler is non-compliant with the C++ standard and needs
 * the following definition to disable a security warning on std::strncpy().
 */
#ifdef _MSVC_LANG
#define _CRT_SECURE_NO_WARNINGS
#endif

namespace Dymatic {

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

			m_Context->m_Registry.each([&](auto entityID)
			{
				Entity entity{ entityID , m_Context.get() };
				DrawEntityNode(entity);
			});

			if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
				m_SelectionContext = {};

			// Right-click on blank space
			if (ImGui::BeginPopupContextWindow(0, 1, false))
			{
				if (ImGui::MenuItem("Create Empty Entity"))
					m_Context->CreateEntity("Empty Entity");

				ImGui::EndPopup();
			}

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

	void SceneHierarchyPanel::DrawEntityNode(Entity entity)
	{
		auto& tag = entity.GetComponent<TagComponent>().Tag;

		ImGuiTreeNodeFlags flags = ((m_SelectionContext == entity) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
		flags |= ImGuiTreeNodeFlags_SpanAvailWidth;
		ImGui::Image(reinterpret_cast<void*>(m_IconEmptyEntity->GetRendererID()), ImVec2{ 16, 16 }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 }, ImVec4{ 0.75f, 0.5f, 0.32f, 1.0f });
		ImGui::SameLine();
		bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, flags, tag.c_str());
		if (ImGui::IsItemClicked())
		{
			m_SelectionContext = entity;
		}

		bool entityDeleted = false;
		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem((std::string(CHARACTER_PROPERTIES_ICON_DELETE) + " Delete").c_str()))
				entityDeleted = true;
			if (ImGui::MenuItem((std::string(CHARACTER_PROPERTIES_ICON_DUPLICATE) + " Duplicate").c_str()))
			{
				m_Context->DuplicateEntity(entity);
			}

			ImGui::EndPopup();
		}

		if (opened)
		{
			//ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
			//bool opened = ImGui::TreeNodeEx((void*)9817239, flags, tag.c_str());
			//if (opened)
			//	ImGui::TreePop();

			ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_Leaf;
			std::vector<std::string> ComponentName;
			//if (entity.HasComponent<TransformComponent>()) { ComponentName.push_back("Transform"); }
			if (entity.HasComponent<SpriteRendererComponent>()) { ComponentName.push_back("Sprite Renderer"); }
			if (entity.HasComponent<ParticleSystem>()) { ComponentName.push_back("Particle System"); }
			if (entity.HasComponent<CameraComponent>()) { ComponentName.push_back("Camera"); }

			for (int i = 0; i < ComponentName.size(); i++)
			{
				ImVec2 valPos = ImGui::GetCursorScreenPos();
				bool opened = ImGui::TreeNodeEx((void*)9817239, flags, ComponentName[i].c_str());
				ImGui::GetWindowDrawList()->AddImage(reinterpret_cast<void*>((ComponentName[i] == "Transform" ? m_IconTransformComponent : ComponentName[i] == "Sprite Renderer" ? m_IconSpriteRendererComponent : ComponentName[i] == "Particle System" ? m_IconParticleSystemComponent : m_IconCameraComponent)->GetRendererID()), valPos, ImVec2{ valPos.x + 16, valPos.y + 16 }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 }, ImGui::ColorConvertFloat4ToU32(ImVec4{0.34f, 0.69f, 0.55f, 1.0f}));
				if (opened)
				{
					ImGui::TreePop();
				}

				if (ImGui::IsItemClicked())
				{
					m_SelectionContext = entity;
				}

				bool entityDeleted = false;
				if (ImGui::BeginPopupContextItem(("##ComponentHeirachyPopup" + ComponentName[i]).c_str()))
				{
					if (ImGui::MenuItem((std::string(CHARACTER_PROPERTIES_ICON_DELETE) + " Remove Component").c_str()))
					{
						if (ComponentName[i] == "Transform") { entity.RemoveComponent<TransformComponent>(); }
						if (ComponentName[i] == "Sprite Renderer") { entity.RemoveComponent<SpriteRendererComponent>(); }
						if (ComponentName[i] == "Particle System") { entity.RemoveComponent<ParticleSystem>(); }
						if (ComponentName[i] == "Camera") { entity.RemoveComponent<Camera>(); }
					}

					ImGui::EndPopup();
				}
			}

			ImGui::TreePop();
		}

		if (entityDeleted)
		{
			m_Context->DestroyEntity(entity);
			if (m_SelectionContext == entity)
				m_SelectionContext = {};
		}
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
		if (ImGui::Button("X", buttonSize))
			values.x = resetValue.x;
		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("Y", buttonSize))
			values.y = resetValue.y;
		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("Z", buttonSize))
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

	template<typename T, typename UIFunction>
	static void DrawComponent(const std::string& name, Entity entity, UIFunction uiFunction)
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
			ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);
			if (ImGui::Button("+", ImVec2{ lineHeight, lineHeight }))
			{
				ImGui::OpenPopup("ComponentSettings");
			}

			bool removeComponent = false;
			if (ImGui::BeginPopup("ComponentSettings"))
			{
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

		//if (ImGui::Button("Add Component"))
		if (ImGui::Button(std::string(CHARACTER_PROPERTIES_ICON_ADD).c_str(), ImVec2{ 31.0f, 24.0f }))
			ImGui::OpenPopup("AddComponent");
		ImGui::SameLine();
		if (ImGui::Button(std::string(CHARACTER_PROPERTIES_ICON_DUPLICATE).c_str(), ImVec2{ 31.0f, 24.0f }))
			m_SelectionContext = m_Context->DuplicateEntity(m_SelectionContext);
		ImGui::SameLine();
		if (ImGui::Button(std::string(CHARACTER_PROPERTIES_ICON_DELETE).c_str(), ImVec2{ 31.0f, 24.0f }))
			entityDeleted = true;

		// Original Image Button Version
		//if (ImGui::ImageButton(reinterpret_cast<void*>(m_IconAddComponent->GetRendererID()), ImVec2{ 18, 18}, ImVec2{ 0, 1 }, ImVec2{ 1, 0 }))
		//	ImGui::OpenPopup("AddComponent");
		//
		//ImGui::SameLine();
		//if (ImGui::ImageButton(reinterpret_cast<void*>(m_IconDuplicate->GetRendererID()), ImVec2{ 18, 18 }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 }))
		//	m_SelectionContext = m_Context->DuplicateEntity(m_SelectionContext);
		//
		//ImGui::SameLine();
		//if (ImGui::ImageButton(reinterpret_cast<void*>(m_IconDelete->GetRendererID()), ImVec2{ 18, 18 }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 }))
		//	entityDeleted = true;

		if (ImGui::BeginPopup("AddComponent"))
		{
			if (ImGui::MenuItem("Camera"))
			{
				if (!m_SelectionContext.HasComponent<CameraComponent>())
					m_SelectionContext.AddComponent<CameraComponent>();
				else
					DY_CORE_WARN("This entity already has the Camera Component!");
				ImGui::CloseCurrentPopup();
			}

			if (ImGui::MenuItem("Sprite Renderer"))
			{
				if (!m_SelectionContext.HasComponent<SpriteRendererComponent>())
					m_SelectionContext.AddComponent<SpriteRendererComponent>();
				else
					DY_CORE_WARN("This entity already has the Sprite Renderer Component!");
				ImGui::CloseCurrentPopup();
			}

			if (ImGui::MenuItem("Particle System"))
			{
				if (!m_SelectionContext.HasComponent<ParticleSystem>())
					m_SelectionContext.AddComponent<ParticleSystem>();
				else
					DY_CORE_WARN("This entity already has the Particle System Component!");
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		ImGui::PopItemWidth();

		DrawComponent<TransformComponent>("Transform", entity, [](auto& component)
		{
			DrawVec3Control("Translation", component.Translation);
			glm::vec3 rotation = glm::degrees(component.Rotation);
			DrawVec3Control("Rotation", rotation);
			component.Rotation = glm::radians(rotation);
			DrawVec3Control("Scale", component.Scale, glm::vec3(1.0f));
		});

		DrawComponent<CameraComponent>("Camera", entity, [](auto& component)
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
		});		

		DrawComponent<SpriteRendererComponent>("Sprite Renderer", entity, [](auto& component)
		{
			ImGui::ColorEdit4("Color", glm::value_ptr(component.Color));
		});

		DrawComponent<ParticleSystem>("Particle System", entity, [](auto& component)
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
		});

		if (entityDeleted)
		{
			m_Context->DestroyEntity(m_SelectionContext);
			m_SelectionContext = {};
		}

	}

}