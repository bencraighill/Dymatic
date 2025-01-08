#include "Panels/Assets/ParticleSystemPanel.h"

#include "Dymatic/Asset/AssetManager.h"

#include "TextSymbols.h"
#include "Panels/Assets/Utils/AssetViewerPanelUtils.h"

#include <glm/gtc/type_ptr.hpp>

namespace Dymatic {

	ParticleSystemPanel::ParticleSystemPanel(const Ref<ParticleSystem> particleSystem)
		: m_ParticleSystem(particleSystem)
	{}

	void ParticleSystemPanel::OnUpdate(Timestep ts)
	{
	}

	void ParticleSystemPanel::OnImGuiRender(bool& open)
	{
		const std::filesystem::path assetPath = AssetManager::GetMetadata(m_ParticleSystem->Handle).FilePath;

		ImGui::PushID(m_ParticleSystem->Handle);

		if (m_Focused)
		{
			ImGui::SetNextWindowFocus();
			m_Focused = false;
		}

		UI::CenterAppearingWindow(ImVec2(750.0f, 500.0f));
		ImGui::Begin(Utils::GetViewerWindowName(FILE_ICON_PARTICLE_SYSTEM, m_ParticleSystem->Handle, assetPath, "Particle System Viewer").c_str(), &open);

		ImGui::TextDisabledUnformatted(FA_SLIDERS " Particle System Properties");

		auto& data = m_ParticleSystem->GetData();

		const uint32_t u32Minimum = 0;

		ImGui::TextUnformatted(FA_TRIANGLE_EXCLAMATION " Max Particle Count");
		ImGui::SameLine();
		ImGui::DragScalar("##MaxParticlesInput", ImGuiDataType_U32, &data.MaxParticles, 1.0f, &u32Minimum, nullptr);
		CheckModified();

		ImGui::TextUnformatted(FA_FIRE " Emission Count");
		ImGui::SameLine();
		ImGui::DragScalar("##EmissionCountInput", ImGuiDataType_U32, &data.EmissionCount, 1.0f, &u32Minimum, nullptr);
		CheckModified();

		ImGui::TextUnformatted(FA_HOURGLASS_HALF " Life Time");
		ImGui::SameLine();
		ImGui::DragFloat("##LifeTimeInput", &data.Lifetime, 0.1f, 0.0f);
		CheckModified();

		ImGui::TextUnformatted(FA_BULLSEYE " Collision Radius");
		ImGui::SameLine();
		ImGui::DragFloat("##CollisionRadiusInput", &data.CollisionRadius, 0.1f, 0.0f);
		CheckModified();

		ImGui::Separator();

		ImGui::TextUnformatted(FILE_ICON_MATERIAL " Default Material");
		ImGui::SameLine();
		if (UI::DrawAssetSelectionDropdown(AssetType::Material, m_ParticleSystem->GetMaterialHandle()))
			AssetManager::SerializeAsset(m_ParticleSystem);

		ImGui::Separator();

		ImGui::TextUnformatted(FA_MAP_PIN " Minimum Position");
		ImGui::SameLine();
		ImGui::DragFloat3("##MinimumPositionInput", glm::value_ptr(data.MinimumPosition), 0.1f, 0.0f);
		CheckModified();

		ImGui::TextUnformatted(FA_MAP_PIN " Maximum Position");
		ImGui::SameLine();
		ImGui::DragFloat3("##MaximumPositionInput", glm::value_ptr(data.MaximumPosition), 0.1f, 0.0f);
		CheckModified();

		ImGui::TextUnformatted(FA_GAUGE_MIN " Minimum Velocity");
		ImGui::SameLine();
		ImGui::DragFloat3("##MinimumVelocityInput", glm::value_ptr(data.MinimumVelocity), 0.1f, 0.0f);
		CheckModified();

		ImGui::TextUnformatted(FA_GAUGE_MAX " Maximum Velocity");
		ImGui::SameLine();
		ImGui::DragFloat3("##MaximumVelocityInput", glm::value_ptr(data.MaximumVelocity), 0.1f, 0.0f);
		CheckModified();

		ImGui::TextUnformatted(FA_ROCKET_LAUNCH " Acceleration");
		ImGui::SameLine();
		ImGui::DragFloat3("##AccelerationInput", glm::value_ptr(data.Acceleration), 0.1f, 0.0f);
		CheckModified();

		ImGui::End();
		ImGui::PopID();
	}

	void ParticleSystemPanel::CheckModified()
	{
		if (ImGui::IsItemDeactivatedAfterEdit())
			OnModify();
	}

	void ParticleSystemPanel::OnModify()
	{
		m_ParticleSystem->Modify();
		AssetManager::SerializeAsset(m_ParticleSystem);
	}

}
