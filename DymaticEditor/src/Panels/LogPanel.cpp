#include "LogPanel.h"

#include "Settings/Preferences.h"
#include "Panels/PopupsAndNotifications.h"

#include "Dymatic/Math/StringUtils.h"
#include "Dymatic/Math/Math.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "Dymatic/UI/UI.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include <imgui/imgui_stdlib.h>

#include "../TextSymbols.h"

namespace Dymatic {

	LogPanel::LogPanel()
	{
	}

	void LogPanel::OnEvent(Event& e)
	{
	}

	static ImU32 GetLogColor(int level)
	{
		return ImGui::GetColorU32(
			level == 0 ? ImGuiCol_LogTrace :
			level == 2 ? ImGuiCol_LogInfo :
			level == 3 ? ImGuiCol_LogWarn :
			level == 4 ? ImGuiCol_LogError :
			level == 5 ? ImGuiCol_LogCritical :
			ImGuiCol_TextDisabled);
	} 

	void LogPanel::OnImGuiRender(Timestep ts)
	{
		auto& logVisible = Preferences::GetEditorWindowVisible(Preferences::EditorWindow::Log);
		if (!logVisible)
			return;

		auto& style = ImGui::GetStyle();
		const float lineHeight = ImGui::GetTextLineHeight();
		
		if (ImGui::Begin(CHARACTER_ICON_CONSOLE " Log", &logVisible))
		{
			if (ImGui::Button("Clear"))
				ClearLog();
			ImGui::SameLine();
			ImGui::Checkbox("Clear On Play", &Preferences::GetData().LogClearOnPlay);
			ImGui::SameLine();
			ImGui::Checkbox("Scroll To Bottom", &Preferences::GetData().LogScrollToBottom);

			const int numLogSymbols = 6;
			const char* logSymbols[numLogSymbols] = { FA_MAGNIFYING_GLASS, "", FA_CIRCLE_INFO, FA_TRIANGLE_EXCLAMATION, FA_OCTAGON_XMARK, FA_FIRE_FLAME_CURVED };
			ImDrawList* drawList = ImGui::GetWindowDrawList();

			ImGui::SameLine();
			ImGui::Dummy({ ImGui::GetContentRegionAvailWidth() - (30.0f + style.FramePadding.x * 2.0f) * 5, 0.0f });

			for (uint32_t i = 0; i < numLogSymbols; i++)
			{
				if (strcmp(logSymbols[i], "") == 0)
					continue;

				ImGui::PushID(i);

				auto& logFilter = Preferences::GetData().LogFilters[i];

				ImGui::SameLine();
				if (ImGui::InvisibleButton("##LogFilter", ImVec2(30.0f, lineHeight)))
				{
					logFilter = !logFilter;
					UpdateDisplayList();
				}

				const bool hovered = ImGui::IsItemHovered();
				style.Alpha = logFilter ? (hovered ? 0.75f : 1.0f) : (hovered ? 0.3f : 0.6f);
				drawList->AddText(ImGui::GetItemRectMin() + style.FramePadding, GetLogColor(i), logSymbols[i]);
				style.Alpha = 1.0f;

				ImGui::PopID();
			}
		

			ImGui::Separator();

			const ImGuiTableFlags flags = ImGuiTableFlags_PadOuterX | ImGuiTableFlags_NoBordersInBody | ImGuiTableFlags_NoPadInnerX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_ScrollX;
			if (ImGui::BeginTable("##LogPanelTable", 3, flags, ImGui::GetContentRegionAvail()))
			{
				ImGui::TableSetupColumn(FA_GAUGE_SIMPLE_HIGH " Severity", ImGuiTableColumnFlags_WidthFixed, 85.0f);
				ImGui::TableSetupColumn(FA_CLOCK " Time", ImGuiTableColumnFlags_WidthFixed, 100.0f);
				ImGui::TableSetupColumn(FA_COMMENT " Message", ImGuiTableColumnFlags_WidthStretch, 0.0f);
				ImGui::TableSetupScrollFreeze(3, 1);
				ImGui::TableHeadersRow();

				ImDrawList* drawList = ImGui::GetWindowDrawList();

				ImGuiListClipper clipper;
				clipper.Begin(m_LogDisplayList.size());
				while (clipper.Step())
				{
					for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
					{
						auto& message = m_LogDisplayList[i];
						
						ImGui::PushID(i);

						ImGui::TableNextRow();
						ImGui::TableNextColumn();

						// Draw selectable
						bool even = i % 2 == 0;
						UI::ScopedStyleColor headerStyleColor(ImGuiCol_HeaderActive, ImGui::GetStyleColorVec4(ImGuiCol_HeaderActive) * ImVec4(0.95f, 0.95f, 0.95f, 1.0f), even);
						ImGui::Selectable("##LogMessage", even, ImGuiSelectableFlags_SpanAllColumns, ImVec2(0.0f, ImGui::CalcTextSize(message.Text.c_str()).y));

						if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && ImGui::IsItemHovered())
						{
							Popup::Create("Log Message", "", { ButtonData("Dismiss") }, nullptr, false, [&message]()
							{
								ImGui::InputTextMultiline("##LogMessageInputText", &message.FormattedText, ImGui::GetContentRegionAvail() - ImVec2(0.0f, 50.0f), ImGuiInputTextFlags_ReadOnly);
							}, ImVec2(750.0f, 400.0f));
						}

						if (ImGui::BeginPopupContextItem("##LogMessagePopup"))
						{
							if (ImGui::MenuItem(CHARACTER_ICON_COPY " Copy"))
								ImGui::SetClipboardText(message.FormattedText.c_str());

							ImGui::EndPopup();
						}

						// Draw Icon
						const float itemHeight = ImGui::GetItemRectSize().y - style.FramePadding.y * 2.0f;
						const ImVec2 offset = style.FramePadding + ImVec2((ImGui::GetContentRegionAvailWidth() - ImGui::CalcTextSize(logSymbols[message.Level]).x), itemHeight - lineHeight) * 0.5f;
						const ImVec2 position = ImGui::GetItemRectMin() + offset;
						const ImU32 color = GetLogColor(message.Level);
						drawList->AddText(position, color, logSymbols[message.Level]);

						ImGui::TableNextColumn();

						ImGui::Text(message.Time.c_str());

						ImGui::TableNextColumn();

						ImGui::Text(message.Text.c_str());

						ImGui::PopID();
					}
				}

				if (m_ScrollToBottom)
				{
					m_ScrollToBottom = false;
					ImGui::SetScrollHereY(1.0f);
				}

				ImGui::EndTable();
			}
		}
		ImGui::End();
	}

	void LogPanel::OnLog(const Log::Message& message)
	{
		if (!message.IsCore)
		{
			m_LogMessages.push_back(message);

			auto& message = m_LogMessages.back();
			auto& text = message.Text;
			text.erase(std::remove_if(text.begin(), text.end(), [](char c) { return c == '\n'; }), text.end());

			if (Preferences::GetData().LogFilters[message.Level])
				m_LogDisplayList.push_back(message);

			if (Preferences::GetData().LogScrollToBottom)
				m_ScrollToBottom = true;
		}
	}

	void LogPanel::ClearLog()
	{
		m_LogMessages.clear();
		m_LogDisplayList.clear();
	}

	void LogPanel::UpdateDisplayList()
	{
		m_LogDisplayList.clear();
		auto& filters = Preferences::GetData().LogFilters;
		
		std::copy_if(m_LogMessages.begin(), m_LogMessages.end(), std::back_inserter(m_LogDisplayList), [&filters](const Log::Message& message) 
		{
			return filters[message.Level];
		});
	}

}