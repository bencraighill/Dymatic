#pragma once

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include "../Preferences.h"

#include "Dymatic/Core/Base.h"

namespace Dymatic {

	struct PopupData
	{
		std::string title, message;
		std::vector<std::string> buttons;
		int buttonClicked = -1;
		int id = -1;
		bool loading = false;
	};

	struct NotificationData
	{
		std::string nameID, title, message;
		std::string buttons[10];
		std::string buttonReturnEvents[10];
		int numberOfButtons;
		bool loading;
		int identifier;
		float time;
		float displayTime;
		std::string executedTime;
		INT64 toastID = -1;
		int buttonClicked = -1;
	};

	class PopupsAndNotifications
	{
	public:
		PopupsAndNotifications(Preferences* preferencesRef);


		PopupData Popup(PopupData popupData);
		PopupData Popup(std::string title, std::string message, std::vector<std::string> buttons, bool loading = false);

		void RemoveTopmostPopup();

		PopupData PopupUpdate();


		void Notification(std::string identifier, int notificationIndex, std::string title, std::string message, std::vector<std::string> buttons, bool loading, float displayTime = 0.0f);
		NotificationData NotificationUpdate(float programTime);
		void ClearNotificationList();
		bool GetPopupOpen() { return m_PopupOpen; }

		bool IsForegroundProcess(DWORD pid);
	private:
		std::string m_CurrentTitle = "";
		ImGuiIO& io = ImGui::GetIO();
		ImVec2 dockspaceWindowPosition;
		std::vector<NotificationData> m_NotificationList;
		float m_ProgramTime;

		std::vector<PopupData> m_PopupList;

		bool m_PopupOpen = false;
		bool m_PreviousPopupOpen = false;

		Preferences* m_PreferencesReference;
	};

}
