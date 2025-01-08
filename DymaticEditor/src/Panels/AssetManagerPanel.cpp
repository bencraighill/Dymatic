#include "AssetManagerPanel.h"
#include "Dymatic/Asset/AssetManager.h"

#include "Settings/Preferences.h"

#include "TextSymbols.h"

#include "EditorResources.h"
#include "Dymatic/Math/StringUtils.h"
#include "Panels/PopupsAndNotifications.h"

#include <imgui/imgui.h>
#include <imgui/imgui_stdlib.h>

namespace Dymatic {

	void AssetManagerPanel::OnImGuiRender()
	{
		auto& assetManagerVisible = Preferences::GetEditorWindowVisible(Preferences::EditorWindow::AssetManager);
		if (!assetManagerVisible)
			return;

		const ImGuiStyle& style = ImGui::GetStyle();

		ImGui::Begin(FA_RECTANGLE_LIST " Asset Manager", &assetManagerVisible);

		char buffer[256];
		memset(buffer, 0, sizeof(buffer));
		strncpy(buffer, m_SearchBuffer.c_str(), sizeof(buffer));
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());
		if (ImGui::InputTextWithHint("##AssetRegistrySearch", FA_MAGNIFYING_GLASS " Search Registry...", buffer, sizeof(buffer)))
			m_SearchBuffer = buffer;

		ImGui::BeginChild("##ManagerChildWindow");
		auto& registry = AssetManager::s_MetadataRegistry;
		auto search = String::ToLower(m_SearchBuffer);
		bool advancedEditMode = Preferences::GetData().AdvancedEditMode;
		uint32_t index = 0;
		for (auto& [key, entry] : registry)
		{
			auto handle = std::to_string(entry.Handle);
			if (
				((entry.MemoryOnly || entry.FilePath.empty()) ? std::string("[memory only]") : String::ToLower(entry.FilePath.string())).find(search) != std::string::npos ||
				String::ToLower(AssetManager::AssetTypeToString(entry.Type)).find(search) != std::string::npos ||
				handle.find(search) != std::string::npos
				)
			{
				ImGui::Separator();

				if (advancedEditMode)
				{
					ImGui::PushID(index);

					ImGui::Text(FileManager::GetFileTypeCharacterIcon(FileManager::GetFileType(entry.Type)));
					ImGui::SameLine();
					ImGui::Text(entry.FilePath.stem().string().c_str());

					ImGui::SameLine(ImGui::GetContentRegionAvailWidth() - ImGui::CalcTextSize(FA_TRASH).x - style.FramePadding.x * 2.0f);

					if (ImGui::Button(FA_TRASH))
					{
						AssetHandle handle = entry.Handle;
						
						Popup::Create(FA_TRASH " Purge Asset", fmt::format("Are you sure you want to purge this asset?\n\n\t" FA_KEY "Handle: {}\n\t" FA_FILE_LINES "Type: {}\n\t" FA_FILE_SIGNATURE "Path: {}", entry.Handle, AssetManager::AssetTypeToString(entry.Type), entry.FilePath),
						{
							ButtonData(FA_CIRCLE_XMARK " Cancel"),
							ButtonData(FA_TRASH " Delete", [handle]()
							{
								AssetManager::RemoveAsset(handle);
							})
						}, EditorResources::ErrorIcon);
					}

					ImGui::Text(FA_KEY " Handle: ");
					ImGui::SameLine();
					ImGui::SetNextItemWidth(-1);
					uint64_t handle = entry.Handle;
					ImGui::InputScalar("##AssetHandleInputScalar", ImGuiDataType_U64, &handle);
					if (ImGui::IsItemDeactivatedAfterEdit())
						entry.Handle = handle;

					ImGui::Text(FA_FILE_LINES " Type");
					ImGui::SameLine();
					ImGui::SetNextItemWidth(-1);
					if (ImGui::BeginCombo("##AssetTypeInputText", AssetManager::AssetTypeToString(entry.Type)))
					{
						for (uint8_t type = 0; type < (uint8_t)AssetType::ASSET_TYPE_SIZE; type++)
						{
							AssetType assetType = (AssetType)type;
							if (ImGui::Selectable(AssetManager::AssetTypeToString(assetType), assetType == entry.Type))
								entry.Type = assetType;
						}

						ImGui::EndCombo();
					}

					ImGui::Text(FA_FILE_SIGNATURE " File Path");
					ImGui::SameLine();
					std::string buffer = entry.FilePath.string();
					ImGui::SetNextItemWidth(-1);
					ImGui::InputText("##AssetFilePathInputText", &buffer);
					if (ImGui::IsItemDeactivatedAfterEdit())
						entry.FilePath = std::filesystem::path(buffer).lexically_normal();

					ImGui::PopID();
					index++;
				}
				else
				{
					ImGui::Text(FA_KEY " Handle: %s", handle.c_str());
					ImGui::Text(FA_FILE_LINES " Type: %s", AssetManager::AssetTypeToString(entry.Type));
					ImGui::Text(FA_FILE_SIGNATURE " File Path: %s", entry.FilePath.empty() ? "[Memory Only]" : entry.FilePath.string().c_str());
				}
			}
		}
		ImGui::EndChild();

		ImGui::End();
	}

}