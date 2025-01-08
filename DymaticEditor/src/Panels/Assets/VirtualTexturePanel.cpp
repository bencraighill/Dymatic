#include "Panels/Assets/VirtualTexturePanel.h"

#include "Dymatic/Asset/AssetManager.h"

#include "TextSymbols.h"
#include "Panels/Assets/Utils/AssetViewerPanelUtils.h"

namespace Dymatic {

	VirtualTexturePanel::VirtualTexturePanel(Ref<Texture2D> virtualTexture)
		: m_VirtualTexture(virtualTexture), m_VirtualSpecification(virtualTexture->GetSpecification())
	{
	}

	void VirtualTexturePanel::OnUpdate(Timestep ts)
	{
	}

	void VirtualTexturePanel::OnImGuiRender(bool& open)
	{
		const std::filesystem::path assetPath = AssetManager::GetMetadata(m_VirtualTexture->Handle).FilePath;

		ImGui::PushID(m_VirtualTexture->Handle);

		if (m_Focused)
		{
			ImGui::SetNextWindowFocus();
			m_Focused = false;
		}

		UI::CenterAppearingWindow(ImVec2(512.0f, 300.0f));
		ImGui::Begin(Utils::GetViewerWindowName(FILE_ICON_VIRTUAL_TEXTURE, m_VirtualTexture->Handle, assetPath, "Virtual Texture Viewer").c_str(), &open);

		ImGui::TextDisabledUnformatted(FA_SLIDERS " Virtual Texture Properties");

		if (ImGui::BeginCombo("##VirtualFormatDropdown", Utils::TextureFormatToString(m_VirtualSpecification.Format)))
		{
			for (uint32_t formatIndex = (uint32_t)TextureFormat::RGBA8; formatIndex <= (uint32_t)TextureFormat::DEPTH24STENCIL8; formatIndex++)
			{
				if (ImGui::MenuItem(Utils::TextureFormatToString((TextureFormat)formatIndex)))
				{
					m_VirtualSpecification.Format = (TextureFormat)formatIndex;
					OnModified();
				}
			}

			ImGui::EndCombo();
		}

		ImGui::TextUnformatted("Width");
		ImGui::SameLine();
		ImGui::DragScalar("##VirtualWidthInput", ImGuiDataType_U32, &m_VirtualSpecification.Width);
		if (ImGui::IsItemDeactivatedAfterEdit())
			OnModified();

		ImGui::TextUnformatted("Height");
		ImGui::SameLine();
		ImGui::DragScalar("##VirtualHeightInput", ImGuiDataType_U32, &m_VirtualSpecification.Height);
		if (ImGui::IsItemDeactivatedAfterEdit())
			OnModified();

		ImGui::End();
		ImGui::PopID();
	}

	void VirtualTexturePanel::OnModified()
	{
		m_VirtualTexture->SetSpecification(m_VirtualSpecification);
		AssetManager::SerializeAsset(m_VirtualTexture);
	}

}
