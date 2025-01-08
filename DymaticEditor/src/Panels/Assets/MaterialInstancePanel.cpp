#include "MaterialInstancePanel.h"

#include "EditorResources.h"

#include "Dymatic/Asset/AssetManager.h"

#include "Panels/Assets/Utils/AssetViewerPanelUtils.h"

#include "Settings/Preferences.h"
#include "TextSymbols.h"

#include <imgui/imgui.h>

#include <glm/gtc/type_ptr.hpp>

namespace Dymatic {

	MaterialInstancePanel::MaterialInstancePanel(Ref<MaterialInstance> materialInstance)
		: m_MaterialInstance(materialInstance)
	{
	}

	void MaterialInstancePanel::OnUpdate(Timestep ts)
	{
		m_Viewport.BeginViewportRender(ts, m_MaterialInstance);
	}

	void MaterialInstancePanel::OnImGuiRender(bool& open)
	{
		m_Viewport.OnPreImGuiRender();

		const std::filesystem::path assetPath = AssetManager::GetMetadata(m_MaterialInstance->Handle).FilePath;

		ImGui::PushID(m_MaterialInstance->Handle);

		if (m_Focused)
		{
			ImGui::SetNextWindowFocus();
			m_Focused = false;
		}

		const ImGuiWindowFlags windowFlags = m_Viewport.IsHovered() ? ImGuiWindowFlags_NoMove : 0;
		UI::CenterAppearingWindow(ImVec2(850.0f, 512.0f));
		ImGui::Begin(Utils::GetViewerWindowName(FILE_ICON_MATERIAL_INSTANCE, m_MaterialInstance->Handle, assetPath, "Material Instance Viewer").c_str(), &open, windowFlags);

		ImGui::TextDisabled(FA_SLIDERS " Asset Details");

		ImGui::Separator();

		ImGui::TextDisabled(FA_FOLDER " Path ");
		ImGui::SameLine();
		ImGui::Text(("Assets" / assetPath).string().c_str());

		ImGui::TextDisabled(FA_SITEMAP " Parent ");
		ImGui::SameLine();
		const AssetHandle parentHandle = m_MaterialInstance->GetParent()->Handle;
		ImGui::Text(fmt::format("{} ({})", AssetManager::GetMetadata(parentHandle).FilePath.string(), parentHandle).c_str());

		ImGui::Separator();

		ImGui::BeginChild("##MaterialInstancePanelViewport", ImVec2(), 0, ImGuiWindowFlags_HorizontalScrollbar);

		ImGui::BeginTable("##MaterialInstanceTable", 2, ImGuiTableFlags_Resizable, ImGui::GetContentRegionAvail());

		ImGui::TableNextColumn();
		ImGui::BeginChild("##MaterialInstanceViewport");
		m_Viewport.OnImGuiRender();
		ImGui::EndChild();
		ImGui::TableNextColumn();

		const float iconSize = 64.0f;
		const float buttonPadding = (iconSize - ImGui::GetTextLineHeight()) * 0.5f - ImGui::GetStyle().FramePadding.y;
		
		const auto& textures = m_MaterialInstance->GetTextures();
		const auto& textureOverrides = m_MaterialInstance->GetTextureOverrides();

		if (UI::CollapsingHeader(FILE_ICON_TEXTURE " Textures"))
		{
			for (const auto& [id, texture] : textures)
			{
				ImGui::PushID(id);

				bool overridden = textureOverrides.find(id) != textureOverrides.end();

				ImGui::BeginGroup();
				ImGui::Dummy(ImVec2(0.0f, buttonPadding));

				if (ImGui::Checkbox("##ParmeterOverrideCheckbox", &overridden))
				{
					if (overridden)
						m_MaterialInstance->SetTextureOverride(id, texture);
					else
						m_MaterialInstance->RemoveTextureOverride(id);

					AssetManager::SerializeAsset(m_MaterialInstance);
				}

				ImGui::EndGroup();
				ImGui::SameLine();

				ImGui::BeginDisabled(!overridden);
				ImGui::ImageButton((ImTextureID)(overridden ? (textureOverrides.at(id)) : (texture ? texture : EditorResources::CheckerboardTexture))->GetRendererID(), ImVec2(iconSize), { 0, 1 }, { 1, 0 });
				ImGui::EndDisabled();


				// Can only perform drag/drop action if overridden
				if (overridden)
				{
					AssetHandle textureHandle = texture->Handle;
					bool updated = UI::ContentBrowserAssetDragDropTarget(AssetType::Texture, textureHandle);

					ImGui::SameLine();

					ImGui::BeginGroup();
					ImGui::Dummy(ImVec2(0.0f, buttonPadding));
					updated |= UI::DrawAssetSelectionDropdown(AssetType::Texture, textureHandle);
					ImGui::EndGroup();

					if (updated)
					{
						const Ref<Texture2D> droppedTexture = AssetManager::GetAsset<Texture2D>(textureHandle);
						m_MaterialInstance->SetTextureOverride(id, droppedTexture ? droppedTexture : texture);
						AssetManager::SerializeAsset(m_MaterialInstance);
					}
				}

				ImGui::PopID();
			}

			ImGui::TreePop();
		}

		const auto& parameters = m_MaterialInstance->GetParameters();
		const auto& parameterOverrides = m_MaterialInstance->GetParameterOverrides();

		if (UI::CollapsingHeader(FA_SLIDERS " Parameters"))
		{
			for (auto& [name, parameter] : parameters)
			{
				ImGui::PushID(name.c_str());

				bool overridden = parameterOverrides.find(name) != parameterOverrides.end();
				if (ImGui::Checkbox("##ParmeterOverrideCheckbox", &overridden))
				{
					if (overridden)
						m_MaterialInstance->SetParameterOverride(name, parameter.Value);
					else
						m_MaterialInstance->RemoveParameterOverride(name);

					AssetManager::SerializeAsset(m_MaterialInstance);
				}

				ImGui::SameLine();
				ImGui::Text(parameter.Name.c_str());
				ImGui::SameLine();

				ImGui::BeginDisabled(!overridden);
				MaterialAsset::MaterialParameterData::Data parameterData = overridden ? parameterOverrides.at(name) : parameter.Value;
				switch (parameter.Type)
				{
				case MaterialAsset::MaterialParameterType::Bool:
				{
					bool value = parameterData.Bool;
					if (ImGui::Checkbox("##ParameterOverrideBoolInput", &value))
					{
						parameterData.Bool = value;
						m_MaterialInstance->SetParameterOverride(name, parameterData);
						AssetManager::SerializeAsset(m_MaterialInstance);
					}
					
					break;
				}
				case MaterialAsset::MaterialParameterType::Float:
				{
					if (ImGui::DragFloat("##ParameterOverrideFloatInput", &parameterData.Float, 0.1f))
					{
						m_MaterialInstance->SetParameterOverride(name, parameterData);
						AssetManager::SerializeAsset(m_MaterialInstance);
					}

					break;
				}
				case MaterialAsset::MaterialParameterType::Vector2:
				{
					if (ImGui::DragFloat2("##ParameterOverrideVector2Input", glm::value_ptr(parameterData.Vector2), 0.1f))
					{
						m_MaterialInstance->SetParameterOverride(name, parameterData);
						AssetManager::SerializeAsset(m_MaterialInstance);
					}

					break;
				}
				case MaterialAsset::MaterialParameterType::Vector3:
				{
					if (ImGui::DragFloat3("##ParameterOverrideVector3Input", glm::value_ptr(parameterData.Vector3), 0.1f))
					{
						m_MaterialInstance->SetParameterOverride(name, parameterData);
						AssetManager::SerializeAsset(m_MaterialInstance);
					}
					
					break;
				}
				case MaterialAsset::MaterialParameterType::Vector4:
				{
					if (ImGui::DragFloat4("##ParameterOverrideVector4Input", glm::value_ptr(parameterData.Vector4), 0.1f))
					{
						m_MaterialInstance->SetParameterOverride(name, parameterData);
						AssetManager::SerializeAsset(m_MaterialInstance);
					}
					
					break;
				}
				}
				ImGui::EndDisabled();

				ImGui::PopID();
			}

			ImGui::TreePop();
		}

		ImGui::EndTable();

		ImGui::EndChild();

		ImGui::End();

		ImGui::PopID();
	}

	void MaterialInstancePanel::Focus()
	{
		m_Focused = true;
	}

}