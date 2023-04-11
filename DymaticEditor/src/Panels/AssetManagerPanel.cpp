#include "AssetManagerPanel.h"
#include "Dymatic/Asset/AssetManager.h"

#include "Settings/Preferences.h"

#include "../TextSymbols.h"

#include "Dymatic/Math/StringUtils.h"

#include <imgui/imgui.h>


namespace Dymatic {

	void AssetManagerPanel::OnImGuiRender()
	{
		auto& assetManagerVisible = Preferences::GetEditorWindowVisible(Preferences::EditorWindow::AssetManager);
		if (!assetManagerVisible)
			return;

		ImGui::Begin(CHARACTER_ICON_STATISTICS " Asset Manager", &assetManagerVisible);

		char buffer[256];
		memset(buffer, 0, sizeof(buffer));
		strncpy(buffer, m_SearchBuffer.c_str(), sizeof(buffer));
		if (ImGui::InputTextWithHint("##AssetRegistrySearch", "Search Registry...", buffer, sizeof(buffer)))
			m_SearchBuffer = buffer;

		ImGui::BeginChild("##ManagerChildWindow");
		auto& registry = AssetManager::s_MetadataRegistry;
		auto search = String::ToLower(m_SearchBuffer);
		for (auto& [key, entry] : registry)
		{
			auto handle = std::to_string(entry.Handle);
			if (
				String::ToLower(entry.FilePath.string()).find(search) != std::string::npos ||
				String::ToLower(AssetManager::AssetTypeToString(entry.Type)).find(search) != std::string::npos ||
				handle.find(search) != std::string::npos
				)
			{
				ImGui::Separator();
				ImGui::Text("Handle: %s", handle.c_str());
				ImGui::Text("Type: %s", AssetManager::AssetTypeToString(entry.Type));
				ImGui::Text("FilePath: %s", entry.FilePath.string().c_str());
			}
		}
		ImGui::EndChild();

		ImGui::End();
	}

}