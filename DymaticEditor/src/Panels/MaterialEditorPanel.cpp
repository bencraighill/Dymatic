#include "MaterialEditorPanel.h"

#include "Dymatic/Asset/AssetManager.h"

#include "Settings/Preferences.h"
#include "TextSymbols.h"

#include <imgui/imgui.h>

#include <glm/gtc/type_ptr.hpp>

namespace Dymatic {

	MaterialEditorPanel::MaterialEditorPanel()
	{
		m_CheckerboardTexture = Texture2D::Create("Resources/Textures/Checkerboard.png");
	}

	void MaterialEditorPanel::OnImGuiRender()
	{
		auto& materialEditorVisible = Preferences::GetEditorWindowVisible(Preferences::EditorWindow::MaterialEditor);
		if (!materialEditorVisible)
			return;

		ImGui::Begin(CHARACTER_ICON_MATERIAL " Material Editor", &materialEditorVisible);
		ImGui::BeginTabBar("##MaterialEditorTabBar", ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_AutoSelectNewTabs);

		for (uint32_t i = 0; i < m_ActiveMaterials.size(); i++)
		{
			auto& material = m_ActiveMaterials[i];

			bool open = true;
			if (ImGui::BeginTabItem(material->GetName().c_str(), &open))
			{
				DrawMaterialTexture("Albedo", material, material->GetAlbedoMap());
				ImGui::SameLine();
				if (ImGui::ColorEdit3("##AlbedoInput", glm::value_ptr(material->GetAlbedo())))
					AssetManager::SerializeAsset(material);

				DrawMaterialTexture("Normal", material, material->GetNormalMap());

				DrawMaterialTexture("Emissive", material, material->GetEmissiveMap());
				ImGui::SameLine();
				if (ImGui::ColorEdit3("##EmissiveInput", glm::value_ptr(material->GetEmissive())))
					AssetManager::SerializeAsset(material);
				ImGui::SameLine();
				if (ImGui::DragFloat("##IntensityInput", &material->GetEmissiveIntensity(), 0.1f, 0.0f))
					AssetManager::SerializeAsset(material);

				DrawMaterialTexture("Specular", material, material->GetSpecularMap());
				ImGui::SameLine();
				if (ImGui::ColorEdit3("##SpecularInput", glm::value_ptr(material->GetSpecular())))
					AssetManager::SerializeAsset(material);

				DrawMaterialTexture("Metalness", material, material->GetMetalnessMap());
				ImGui::SameLine();
				if (ImGui::DragFloat("##MetalnessInput", &material->GetMetalness(), 0.01f, 0.0f, 1.0f, "%.2f"))
					AssetManager::SerializeAsset(material);

				DrawMaterialTexture("Roughness", material, material->GetRoughnessMap());
				ImGui::SameLine();
				if (ImGui::DragFloat("##RoughnessInput", &material->GetRoughness(), 0.01f, 0.0f, 1.0f, "%.2f"))
					AssetManager::SerializeAsset(material);

				DrawMaterialTexture("Alpha", material, material->GetAlphaMap());
				ImGui::SameLine();
				if (ImGui::DragFloat("##AlphaInput", &material->GetAlpha(), 0.01f, 0.0f, 1.0f, "%.2f"))
					AssetManager::SerializeAsset(material);

				if (ImGui::BeginCombo("Alpha Blend Mode", Material::AlphaBlendModeToString(material->GetAlphaBlendMode())))
				{
					for (uint32_t i = 0; i < Material::ALPHA_BLEND_MODE_SIZE; i++)
					{
						if (ImGui::MenuItem(Material::AlphaBlendModeToString((Material::AlphaBlendMode)i)))
						{
							material->SetAlphaBlendMode((Material::AlphaBlendMode)i);
							AssetManager::SerializeAsset(material);
						}
					}

					ImGui::EndCombo();
				}
				
				DrawMaterialTexture("Ambient Occlusion", material, material->GetAmbientOcclusionMap());
				ImGui::SameLine();
				if (ImGui::DragFloat("##AmbientOcclusionInput", &material->GetAmbientOcclusion(), 0.01f, 0.0f, 1.0f, "%.2f"))
					AssetManager::SerializeAsset(material);

				ImGui::EndTabItem();
			}

			if (!open)
			{
				m_ActiveMaterials.erase(m_ActiveMaterials.begin() + i);
				i--;
			}
		}

		ImGui::EndTabBar();
		ImGui::End();
	}

	void MaterialEditorPanel::Open(const std::filesystem::path& path)
	{
		if (auto material = AssetManager::GetAsset<Material>(path))
		{
			Preferences::GetEditorWindowVisible(Preferences::EditorWindow::MaterialEditor) = true;

			for (auto& mat : m_ActiveMaterials)
				if (mat->Handle == material->Handle)
					return;

			m_ActiveMaterials.push_back(material);
		}
	}

	void MaterialEditorPanel::DrawMaterialTexture(const char* name, const Ref<Material>& material, Ref<Texture2D>& texture)
	{
		ImGui::PushID(name);

		ImGui::TextDisabled(name);
		
		ImGui::ImageButton((ImTextureID)(texture ? texture : m_CheckerboardTexture)->GetRendererID(), ImVec2(64, 64), { 0, 1 }, { 1, 0 });

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
			{
				const wchar_t* path = (const wchar_t*)payload->Data;
				if (Ref<Texture2D> newTexture = AssetManager::GetAsset<Texture2D>(path))
				{
					texture = newTexture;
					AssetManager::SerializeAsset(material);
				}
			}
			ImGui::EndDragDropTarget();
		}

		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem("Clear"))
			{
				texture = nullptr;
				AssetManager::SerializeAsset(material);
			}
			ImGui::EndPopup();
		}

		ImGui::PopID();
	}

}