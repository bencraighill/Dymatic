#include "Panels/Assets/VideoPlayerPanel.h"

#include "Dymatic/Asset/AssetManager.h"

#include "TextSymbols.h"
#include "Panels/Assets/Utils/AssetViewerPanelUtils.h"

namespace Dymatic {

	VideoPlayerPanel::VideoPlayerPanel(Ref<VideoReader> videoPlayer)
		: m_VideoPlayer(videoPlayer)
	{
	}

	void VideoPlayerPanel::OnUpdate(Timestep ts)
	{
	}

	void VideoPlayerPanel::OnImGuiRender(bool& open)
	{
		const std::filesystem::path assetPath = AssetManager::GetMetadata(m_VideoPlayer->Handle).FilePath;

		ImGui::PushID(m_VideoPlayer->Handle);

		if (m_Focused)
		{
			ImGui::SetNextWindowFocus();
			m_Focused = false;
		}

		UI::CenterAppearingWindow(ImVec2(512.0f, 300.0f));
		ImGui::Begin(Utils::GetViewerWindowName(FILE_ICON_VIDEO_PLAYER, m_VideoPlayer->Handle, assetPath, "Video Player Viewer").c_str(), &open);

		ImGui::TextDisabledUnformatted(FA_SLIDERS " Video Player Properties");

		ImGui::TextUnformatted("Video Source");
		ImGui::SameLine();
		if (UI::DrawAssetSelectionDropdown("##SourceDropdown", AssetType::Video, m_VideoPlayer->GetSource()))
			OnModify();

		ImGui::TextUnformatted("Subtitles");
		ImGui::SameLine();
		if (UI::DrawAssetSelectionDropdown("##SubtitlesDropdown", AssetType::Subtitle, m_VideoPlayer->GetSubtitles()))
			OnModify();

		ImGui::TextUnformatted("Target Virtual Texture");
		ImGui::SameLine();
		if (UI::DrawAssetSelectionDropdown("##TargetDropdown", AssetType::VirtualTexture, m_VideoPlayer->GetTarget()))
			OnModify();

		ImGui::End();
		ImGui::PopID();
	}

	void VideoPlayerPanel::OnModify()
	{
		m_VideoPlayer->Invalidate();
		AssetManager::SerializeAsset(m_VideoPlayer);
	}

}
