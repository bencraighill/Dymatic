#include "Panels/Assets/TextureViewerPanel.h"

#include "Dymatic/Asset/AssetManager.h"

#include "Dymatic/Core/Application.h"

#include "Panels/Assets/Utils/AssetViewerPanelUtils.h"

namespace Dymatic {

	TextureViewerPanel::TextureViewerPanel(const AssetHandle handle)
	{
		m_Texture = AssetManager::RequestAsset<Texture2D>(handle);
	}

	void TextureViewerPanel::OnUpdate(Timestep ts)
	{
	}

	void TextureViewerPanel::OnImGuiRender(bool& open)
	{
		const std::filesystem::path assetPath = AssetManager::GetMetadata(m_Texture->Handle).FilePath;

		ImGui::PushID(m_Texture->Handle);

		if (m_Focused)
		{
			ImGui::SetNextWindowFocus();
			m_Focused = false;
		}

		UI::CenterAppearingWindow(ImVec2(428.0f, 512.0f));
		ImGui::Begin(Utils::GetViewerWindowName(FILE_ICON_TEXTURE, m_Texture->Handle, assetPath, "Texture Viewer").c_str(), &open);

		ImGui::TextDisabled(FA_SLIDERS " Asset Details");

		ImGui::Separator();

		ImGui::TextDisabled(FA_FOLDER " Path:");
		ImGui::SameLine();
		ImGui::Text(("Assets" / assetPath).string().c_str());
		ImGui::TextDisabled(FA_IMAGE_POLAROID " Format:");
		ImGui::SameLine();
		ImGui::Text(Utils::TextureFormatToString(m_Texture->GetSpecification().Format));
		
		// Channel view options
		ImGui::TextDisabled(FA_DROPLET " Channel View Flags");
		ImGui::SameLine();
		ImGui::Checkbox("R", &m_RedChannel);
		ImGui::SameLine();
		ImGui::Checkbox("G", &m_GreenChannel);
		ImGui::SameLine();
		ImGui::Checkbox("B", &m_BlueChannel);
		ImGui::SameLine();
		ImGui::Checkbox("A", &m_AlphaChannel);

		ImGui::Separator();

		ImGui::BeginChild("##TextureViewerPanelViewport", ImVec2(), 0, ImGuiWindowFlags_HorizontalScrollbar);

		const ImGuiIO& io = ImGui::GetIO();
		if (ImGui::IsWindowHovered() && io.KeyCtrl && io.MouseWheel != 0.0f)
			m_Zoom = std::clamp(m_Zoom * (1.0f + io.MouseWheel * 0.1f), 1.0f, 100.0f);

		const ImVec4 tintColor = ImVec4(m_RedChannel ? 1.0f : 0.0f, m_GreenChannel ? 1.0f : 0.0f, m_BlueChannel ? 1.0f : 0.0f, m_AlphaChannel ? 1.0f : 0.0f);
		ImGui::Image((ImTextureID)m_Texture->GetRendererID(), Utils::GetMaximizedSize(m_Texture->GetSize(), ImGui::GetWindowSize()) * m_Zoom, { 0, 1 }, { 1, 0 }, tintColor);
		ImGui::EndChild();

		ImGui::End();
		
		ImGui::PopID();
	}

	void TextureViewerPanel::Focus()
	{
		m_Focused = true;
	}

}