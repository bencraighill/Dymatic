#include "PopupsAndNotifications.h"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <glm/gtc/type_ptr.hpp>
#include "Dymatic/Math/Math.h"

#include "../WinToast/WinToastHandler.h"
#include "../Preferences.h"

#include <string>
#include <sstream>
#include <ctime>

namespace Dymatic {

	WinToastLib::Toast m_ToastHandle;

	PopupsAndNotifications::PopupsAndNotifications(Preferences* preferencesRef)
	{
		m_PreferencesReference = preferencesRef;
	}

	void PopupsAndNotifications::Popup(PopupData popupData)
	{
		m_PopupList.push_back(popupData);
	}

	void PopupsAndNotifications::Popup(std::string title, std::string message, std::vector<std::string> buttons)
	{
		PopupData data;
		data.title = title;
		data.message = message;
		if (data.message.substr(data.message.size() - 1, 1) != "\n")
		{
			data.message = data.message + "\n";
		}
		data.buttons = buttons;

		m_PopupList.push_back(data);
	}

	Dymatic::PopupData PopupsAndNotifications::PopupUpdate()
	{
		//Popup Update Code

		if (!m_PopupList.empty())
		{
			auto data = m_PopupList[0];
			m_PopupOpen = true;
			m_PreviousPopupOpen = true;
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 5.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
			ImGui::OpenPopup(data.title.c_str());
			if (ImGui::BeginPopupModal(data.title.c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize))
			{
				float totalWidth = 0;
				float additionalPadding = 20.0f;
				for (int i = 0; i < data.buttons.size(); i++)
				{
					totalWidth += ImGui::CalcTextSize(data.buttons[i].c_str()).x + additionalPadding;
				}
				float totalTextWidth = ImGui::CalcTextSize(data.message.c_str()).x;

				float popupWidth = 400;
				float popupHeight = 134.5;

				if (totalTextWidth > totalWidth && totalTextWidth > popupWidth)
					popupWidth = totalTextWidth;

				if (totalWidth > totalTextWidth && totalWidth > popupWidth)
					popupWidth = totalWidth;

				if (m_CurrentTitle != data.title)
				{
					m_CurrentTitle = data.title;
					ImGui::SetWindowPos(ImVec2((((io.DisplaySize.x * 0.5f) - (popupWidth / 2)) + dockspaceWindowPosition.x), (((io.DisplaySize.y * 0.5f) - (popupHeight / 2)) + dockspaceWindowPosition.y)));
				}

				ImGui::Dummy(ImVec2{ popupWidth, 10 });

				size_t numberOfLines = std::count(data.message.begin(), data.message.end(), '\n');

				std::stringstream test(data.message);
				std::string segment;
				std::vector<std::string> seglist;

				while (std::getline(test, segment, '\n'))
				{
					seglist.push_back(segment);
				}

				for (int i = 0; i < numberOfLines; i++)
				{
					ImGui::Spacing();
					const char* messageText = seglist[i].c_str();
					ImGui::SameLine((popupWidth / 2) - ImGui::CalcTextSize(messageText).x / 2);
					ImGui::Text(messageText);
				}


				ImGui::Dummy(ImVec2{ 0, 20 });

				ImGui::Spacing();
				ImVec2 size{ 70, 23 };
				ImGui::SameLine((popupWidth / 2) - ((totalWidth) / 2));

				for (int i = 0; i < data.buttons.size(); i++)
				{
					if (ImGui::Button(data.buttons[i].c_str(), ImVec2(ImGui::CalcTextSize(data.buttons[i].c_str()).x + additionalPadding, size.y)))
					{
						m_PopupList.erase(m_PopupList.begin() + 0);
						data.buttonClicked = i;
						m_CurrentTitle = "";
						
					}
					ImGui::SameLine();
				}
				ImGui::EndPopup();
				ImGui::PopStyleVar(2);
				if (data.buttonClicked != -1)
				{
					return data;
				}
			}
		}
		return {};
	}

	void PopupsAndNotifications::Notification(std::string identifier, int notificationIndex, std::string title, std::string message, std::vector<std::string> buttons, bool loading, float displayTime)
	{
		if (m_PreferencesReference->m_PreferenceData.NotificationEnabled[notificationIndex])
		{

			NotificationData data;
			data.title = title;
			data.message = message;
			for (int i = 0; i < 10; i++)
			{
				if (i < buttons.size())
					data.buttons[i] = buttons[i];
				else
					data.buttons[i] = "";
			}

			int numButtons = buttons.size();
			if (numButtons > 10)
				numButtons = 10;
			data.numberOfButtons = numButtons;
			data.loading = loading;

			data.identifier = Math::GetRandomInRange(1, 9999999);
			data.time = m_ProgramTime;
			data.displayTime = displayTime;

			time_t tt;
			time(&tt);
			tm TM = *localtime(&tt);

			int Hours = TM.tm_hour + 0;
			int Minutes = TM.tm_min + 0;
			int Seconds = TM.tm_sec + 0;

			std::string Hours_S = std::to_string(Hours);
			Hours_S = Hours_S.length() == 1 ? "0" + Hours_S : Hours_S;
			std::string Minutes_S = std::to_string(Minutes);
			Minutes_S = Minutes_S.length() == 1 ? "0" + Minutes_S : Minutes_S;
			std::string Seconds_S = std::to_string(Seconds);
			Seconds_S = Seconds_S.length() == 1 ? "0" + Seconds_S : Seconds_S;

			data.executedTime = ("{ " + Hours_S + " : " + Minutes_S + " : " + Seconds_S + " }");

			data.nameID = identifier;

			std::string linelessMessage = message;
			//linelessMessage.erase(std::remove(linelessMessage.begin(), linelessMessage.end(), 'a'), linelessMessage.end());

			if (m_PreferencesReference->m_PreferenceData.NotificationPreset == 3 ? (m_PreferencesReference->m_PreferenceData.NotificationToastEnabled[notificationIndex] == 2 || (m_PreferencesReference->m_PreferenceData.NotificationToastEnabled[notificationIndex] == 1 && !IsForegroundProcess(GetCurrentProcessId()))) : m_PreferencesReference->m_PreferenceData.NotificationPreset)
				data.toastID = m_ToastHandle.ShowToast(identifier, title, linelessMessage, buttons, (displayTime == 0 ? 1000 : displayTime));

			m_NotificationList.push_back(data);
		}
	}

	NotificationData PopupsAndNotifications::NotificationUpdate(float programTime)
	{
		//Update Popup Stuff
		if (m_PreviousPopupOpen == false)
			m_PopupOpen = false;
		m_PreviousPopupOpen = false;

		io = ImGui::GetIO();
		dockspaceWindowPosition = ImGui::GetWindowPos();
		m_ProgramTime = programTime;
		int m_ListSize = m_NotificationList.size() - 0;
		if (m_ListSize < 0)
			m_ListSize = 0;
		std::string Number = "Number: " + std::to_string(m_ListSize);

		for (int i = 0; i < m_NotificationList.size(); i++)
		{
			if (((m_ProgramTime - m_NotificationList[i].time) > m_NotificationList[i].displayTime) && (m_NotificationList[i].displayTime != 0))
			{
				if (m_NotificationList[i].toastID != -1)
				{
					m_ToastHandle.HideToast(m_NotificationList[i].toastID);
				}
				m_NotificationList.erase(m_NotificationList.begin() + i);
			}
		}

		ImGui::Begin("Notifications");
		ImGui::Text(Number.c_str());


		ImGui::SameLine(ImGui::GetContentRegionAvailWidth() - ImGui::CalcTextSize("Clear").x - 5);
		if (ImGui::Button("Clear"))
		{
			ClearNotificationList();
		}

		for (int i = m_NotificationList.size()-1; i >= 0 ; i = i-1)
		{

			const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
			ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
			float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
			ImGui::Spacing();
			bool open = ImGui::TreeNodeEx((void*)(m_NotificationList[i].identifier), treeNodeFlags, (m_NotificationList[i].executedTime + "  -  " + m_NotificationList[i].title).c_str());
			ImGui::PopStyleVar();

			bool removeNotification = false;
			int buttonPressedOverride = -1;

			if (m_ToastHandle.GetToastClicked(m_NotificationList[i].toastID))
			{

			}

			if (m_ToastHandle.GetToastButtonPressed(m_NotificationList[i].toastID) != -1)
			{
				buttonPressedOverride = m_ToastHandle.GetToastButtonPressed(m_NotificationList[i].toastID);
			}

			ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);
			std::string line = "X##" + std::to_string(m_NotificationList[i].identifier);
			if (ImGui::Button(line.c_str(), ImVec2{ lineHeight, lineHeight })/* || removeNotification*/)
			{
				m_ToastHandle.HideToast(m_NotificationList[i].toastID);
				m_NotificationList.erase(m_NotificationList.begin() + i);
				if (open)
					ImGui::TreePop();
				ImGui::End();
				return {};
			}
			int buttonPressed = -1;
			if (open)
			{
				ImGui::Dummy(ImVec2{ 0, 5 });
				ImGui::Text(m_NotificationList[i].message.c_str());
				ImGui::Dummy(ImVec2{ 0, 2 });
				for (int b = 0; b < m_NotificationList[i].numberOfButtons; b++)
				{
					if (m_NotificationList[i].buttons[b] != "")
					{
						std::string buttonLine = (m_NotificationList[i].buttons[b]) + "##" + std::to_string(m_NotificationList[i].identifier);
						ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
						if (ImGui::Button(buttonLine.c_str()))
						{
							//ImGui::PopStyleVar();
							//ImGui::TreePop();
							buttonPressed = b;
						}
						ImGui::SameLine();
						ImGui::PopStyleVar();
					}
				}
				ImGui::Dummy(ImVec2{ 0, 5 });
				ImGui::TreePop();
			}
			if (buttonPressed != -1 || buttonPressedOverride != -1)
			{
				m_NotificationList[i].buttonClicked = buttonPressedOverride != -1 ? buttonPressedOverride : buttonPressed;
				NotificationData finalData = m_NotificationList[i];

				//Toast Handling
				m_ToastHandle.HideToast(m_NotificationList[i].toastID);

				m_NotificationList.erase(m_NotificationList.begin() + i);

				ImGui::End();

				return finalData;
			}
		}
		ImGui::End();
		return {};
	}

	void PopupsAndNotifications::ClearNotificationList()
	{
		for (NotificationData data : m_NotificationList)
		{
			if (data.toastID != -1)
				m_ToastHandle.HideToast(data.toastID);
		}
		m_NotificationList.clear();
	}

	bool PopupsAndNotifications::IsForegroundProcess(DWORD pid)
	{
		HWND hwnd = GetForegroundWindow();
		if (hwnd == NULL) return false;

		DWORD foregroundPid;
		if (GetWindowThreadProcessId(hwnd, &foregroundPid) == 0) return false;

		return (foregroundPid == pid);
	}

}