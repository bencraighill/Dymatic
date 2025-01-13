#include "Panels/ProjectSettingsPanel.h"

#include "Dymatic/Project/Project.h"
#include "Settings/ProjectSettings.h"

#include "UI.h"
#include "TextSymbols.h"

#include <imgui_stdlib.h>

namespace Dymatic {

	namespace Utils {

		static void DrawProjectFilepathInput(const char* name, std::filesystem::path& filepath)
		{
			ImGui::PushID(name);
			ImGui::TextDisabled(name);
			ImGui::SameLine();

			std::string filepathString = filepath.string();
			ImGui::SetNextItemWidth(-1);
			ImGui::InputText("##ProjectSettingsInput", &filepathString);

			if (ImGui::IsItemDeactivatedAfterEdit())
			{
				filepath = filepathString;
				Project::Save();
			}

			ImGui::PopID();
		}

		static void SaveProjectOnEdited()
		{
			if (ImGui::IsItemDeactivatedAfterEdit())
				Project::Save();
		}

	}

	void ProjectSettingsPanel::OnImGuiRender()
	{
		if (!m_Visible)
			return;

		UI::CenterAppearingWindow(0.5f);
		ImGui::Begin(FA_FOLDER_GEAR " Project Settings", &m_Visible);

		auto& config = Project::GetActive()->GetConfig();

		ImGui::TextDisabled(FA_SLIDER " PROJECT CONFIGURATION");

		ImGui::Indent();
		ImGui::TextDisabled(FA_FLAG_PENNANT " Start Scene");
		ImGui::SameLine();
		if (UI::DrawAssetSelectionDropdown(AssetType::Scene, config.StartScene))
			Project::Save();

		Utils::DrawProjectFilepathInput(FA_FOLDER " Asset Directory", config.AssetDirectory);
		Utils::DrawProjectFilepathInput(FA_FOLDER " Core Module Path", config.CoreModulePath);
		Utils::DrawProjectFilepathInput(FA_FOLDER " Script Module Path", config.ScriptModulePath);
		Utils::DrawProjectFilepathInput(FA_FOLDER " Cache Directory", config.CacheDirectory);
		ImGui::Unindent();

		ImGui::TextDisabled(FA_ROCKET_LAUNCH " PHYSICS");
		ImGui::Indent();
		auto& physics = config.PhysicsSettings;

		ImGui::TextDisabled(FA_FILE_LINES " Generate Debug Log");
		ImGui::SameLine();
		bool enableDebugLog = ProjectSettings::GetEnablePhysicsLogging();
		if (ImGui::Checkbox("##DebugLoggingCheckbox", &enableDebugLog))
		{
			ProjectSettings::SetEnablePhysicsLogging(enableDebugLog);
			ProjectSettings::Serialize();
		}

		ImGui::TextDisabled(FA_SNOOZE " Allow Sleeping");
		ImGui::SameLine();
		ImGui::Checkbox("##AllowSleepingCheckbox", &physics.AllowSleeping);
		Utils::SaveProjectOnEdited();

		ImGui::TextDisabled(FA_ALARM_SNOOZE " Sleep Timer");
		ImGui::SameLine();
		ImGui::DragFloat("##SleepTimerCheckbox", &physics.SleepTimer, 0.1f);
		Utils::SaveProjectOnEdited();

		ImGui::TextDisabled(FA_ELLIPSIS " Position Steps");
		ImGui::SameLine();
		ImGui::DragScalar("##PositionStepsInput", ImGuiDataType_U32, &physics.PositionSteps);
		Utils::SaveProjectOnEdited();

		ImGui::TextDisabled(FA_ELLIPSIS " Velocity Steps");
		ImGui::SameLine();
		ImGui::DragScalar("##VelocityStepsInput", ImGuiDataType_U32, &physics.VelocitySteps);
		Utils::SaveProjectOnEdited();

		ImGui::TextDisabled(FA_ROTATE " Deterministic");
		ImGui::SameLine();
		ImGui::Checkbox("##DeterministicCheckbox", &physics.Deterministic);
		Utils::SaveProjectOnEdited();

		if (UI::CollapsingHeader(FA_LAYER_GROUP " Layers"))
		{
			ImGui::BeginChild("##LayerList", ImVec2(ImGui::GetContentRegionAvailWidth() * 0.5f, 200.0f));
			
			if (ImGui::Button(FA_PLUS " New Layer"))
			{
				physics.Layers[UUID()] = PhysicsLayer{ "New Layer" };
				Project::Save();
			}

			ImGui::Separator();
			
			for (auto& [id, layer] : physics.Layers)
			{
				ImGui::PushID(id);
				if (ImGui::SelectableInput("##LayerInput", ImGui::GetContentRegionAvailWidth(), m_SelectedPhysicsLayer == id, 0, &layer.Name))
					m_SelectedPhysicsLayer = id;
				ImGui::PopID();
			}

			ImGui::EndChild();

			if (physics.Layers.find(m_SelectedPhysicsLayer) != physics.Layers.end())
			{
				auto& selectedLayer = physics.Layers[m_SelectedPhysicsLayer];

				ImGui::SameLine();
				ImGui::BeginChild("##LayerProperties", ImVec2(ImGui::GetContentRegionAvailWidth(), 200.0f));

				ImGui::TextDisabled(FA_SQUARE " Layer Properties");

				ImGui::TextDisabled(FA_TAG " Name");
				ImGui::SameLine();
				ImGui::Text(selectedLayer.Name.c_str());

				ImGui::TextDisabled(FA_KEY " ID");
				ImGui::SameLine();
				ImGui::Text("%llu", m_SelectedPhysicsLayer);

				const bool remove = ((m_SelectedPhysicsLayer != 0) && ImGui::Button(FA_TRASH " Delete Layer"));

				ImGui::Separator();

				ImGui::TextDisabled(FA_CAR_BURST " Collides With");
				ImGui::Indent();
				for (const auto& [id, layer] : physics.Layers)
				{
					ImGui::PushID(id);
					ImGui::Text(FA_SQUARE " %s", layer.Name.c_str());
					ImGui::SameLine();
					bool collides = selectedLayer.ExclusionMask.find(id) == selectedLayer.ExclusionMask.end();
					if (ImGui::Checkbox("##LayerCollideCheckbox", &collides))
					{
						if (collides)
							selectedLayer.ExclusionMask.erase(id);
						else
							selectedLayer.ExclusionMask.insert(id);

						Project::Save();
					}
					ImGui::PopID();
				}
				ImGui::Unindent();

				ImGui::EndChild();

				if (remove)
				{
					physics.Layers.erase(m_SelectedPhysicsLayer);
					m_SelectedPhysicsLayer = -1;
					Project::Save();
				}
			}

			ImGui::TreePop();
		}

		ImGui::Unindent();

		ImGui::End();
	}

}