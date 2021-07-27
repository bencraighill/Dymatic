#include "FilePrompt.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

namespace Dymatic {

	FilePrompt::FilePrompt(Preferences* preferencesRef)
	{
		m_PreferencesReference = preferencesRef;
	}

	void FilePrompt::OnImGuiRender(Timestep ts)
	{
		ImGui::Begin("File Prompt", NULL, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking);
		ImGui::BeginChild("##MainChildWindow", ImGui::GetContentRegionAvail() - ImVec2(0, 45));

		static float offsetA = 0.0f;
		static float offsetB = 0.0f;

		float panelA = (ImGui::GetContentRegionAvail().x / 5 * 1) - offsetA;
		float panelB = (ImGui::GetContentRegionAvail().x / 5 * 4) - offsetB;

		//ImGui::Splitter("##NodeEditorSplitInfo", true, 2.0f, &panelA, &panelB, 50.0f, 50.0f);
		ImGui::Splitter("##NodeEditorSplitInfo", true, 2.0f, &offsetA, &offsetB, 50.0f, 50.0f);

		ImGui::BeginChild("##FilePaths", ImVec2(panelA, ImGui::GetContentRegionAvail().y));

		ImGui::EndChild();

		ImGui::SameLine();

		ImGui::BeginChild("##FileExplorer", ImVec2(panelB, ImGui::GetContentRegionAvail().y));

		auto files = m_InternalContentBrowser.GetFilesDisplayed();
		for (int i = 0; i < files.size(); i++)
		{
			ImGui::Selectable((files[i].filename).c_str());

			if (ImGui::BeginPopupContextItem())
			{
				if (ImGui::MenuItem("Delete")) {}
				if (ImGui::MenuItem("Duplicate"))
				{
				}

				ImGui::EndPopup();
			}
		}
		ImGui::EndChild();
		ImGui::EndChild();

		ImGui::Text("File name:");
		ImGui::SameLine();
		ImGui::PushItemWidth(-1);

		char buffer[256];
		memset(buffer, 0, sizeof(buffer));
		std::strncpy(buffer, m_SaveFileNameBar.c_str(), sizeof(buffer));
		if (ImGui::InputText("##SaveFilenameBar", buffer, sizeof(buffer)))
		{
			m_SaveFileNameBar = std::string(buffer);
		}
		ImGui::PopItemWidth();

		ImGui::End();
	}

}