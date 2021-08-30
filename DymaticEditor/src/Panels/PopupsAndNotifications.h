#pragma once

#include "../Preferences.h"

#include "Dymatic/Core/Base.h"
#include <functional>

namespace Dymatic {

	struct ButtonData
	{
		unsigned int id;
		std::string name;
		std::function<void()> func;
		ButtonData(unsigned int id, std::string name, std::function<void()> func)
			: id(id), name(name), func(func)
		{
		}
	};

	struct PopupData
	{
		unsigned int id;
		std::string title, message;
		std::vector<ButtonData> buttons;
		bool loading = false;
	};

	struct NotificationData
	{
		unsigned int id;
		std::string title, message;
		std::vector<ButtonData> buttons;
		float time;
		float displayTime;
		bool loading;
		std::string executedTime;
		float height = 0.0f;
		float offsetMin = 0.0f;
		float offsetMax = 0.0f;
		bool fadeOut = false;
		bool fadeIn = true;
		float currentOpacity = 0.0f;
	};

	class PopupsAndNotifications
	{
	public:
		PopupsAndNotifications(Preferences* preferencesRef);

		void Popup(std::string title, std::string message, std::vector<ButtonData> buttons, bool loading = false);

		void RemoveTopmostPopup();

		void PopupUpdate();

		void Notification(int notificationIndex, std::string title, std::string message, std::vector<ButtonData> buttons = {}, bool loading = false, float displayTime = 0.0f);
		void NotificationUpdate(Timestep ts, float programTime);
		void ClearNotificationList();
		bool GetPopupOpen() { return m_PopupOpen; }

		bool& GetNotificationsVisible() { return m_NotificationsVisible; }

		unsigned int GetNextNotificationId() { m_NextNotificationId++; return m_NextNotificationId; }
	private:
	private:
		bool m_NotificationsVisible = false;

		Ref<Texture2D> m_DymaticLogo = Texture2D::Create("assets/icons/DymaticLogoTransparent.png");

		unsigned int m_NextNotificationId = 0;

		std::string m_CurrentTitle = "";
		ImGuiIO& io = ImGui::GetIO();
		ImVec2 dockspaceWindowPosition;
		std::vector<NotificationData> m_NotificationList;
		float m_ProgramTime = 0.0f;

		std::vector<PopupData> m_PopupList;

		bool m_PopupOpen = false;
		bool m_PreviousPopupOpen = false;

		Preferences* m_PreferencesReference;
	};

}
