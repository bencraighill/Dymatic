#pragma once

#include "../Preferences.h"

#include "Dymatic/Core/Base.h"
#include <functional>

namespace Dymatic {

	struct ButtonData
	{
		std::string Name;
		std::function<void()> OnPressedFunction;

		ButtonData(std::string name, std::function<void()> onPressed)
			: Name(name), OnPressedFunction(onPressed) {}
	};

	class Popup
	{
	public:
		Popup(const std::string& title, const std::string& message, const std::vector<ButtonData>& buttons, Ref<Texture2D> icon, bool loading);

		static void Create(const std::string& title, const std::string& message, std::vector<ButtonData> buttons, Ref<Texture2D> icon = nullptr, bool loading = false);
		static void RemoveTopmostPopup();

		static void OnImGuiRender(Timestep ts);

	private:
		// Popup Data
		UUID m_ID;
		Ref<Texture2D> m_Icon = nullptr;
		std::string m_Title, m_Message;
		std::vector<ButtonData> m_Buttons;
		bool m_Loading = false;
	};

	class Notification
	{
	public:
		Notification(const std::string& title, const std::string& message, const std::vector<ButtonData>& buttons, float displayTime, bool loading);
		
		static void Init();
		static void Create(const std::string& title, const std::string& message, const std::vector<ButtonData>& buttons = {}, float displayTime = 0.0f, bool loading = false);
		static void Clear();

		static void OnImGuiRender(Timestep ts);

	private:
		// Notification Data
		UUID m_ID;
		std::string m_Title, m_Message;
		std::vector<ButtonData> m_Buttons;
		float m_Time;
		float m_DisplayTime;
		bool m_Loading;
		std::string m_Timestamp;

		// Draw Data
		float _height = 0.0f;
		float _offsetMin = 0.0f;
		float _offsetMax = 0.0f;
		bool _fadeOut = false;
		bool _fadeIn = true;
		float _currentOpacity = 0.0f;

		friend class NotificationsPannel;
	};

	class NotificationsPannel
	{
	public:
		NotificationsPannel() = default;
		void OnImGuiRender(Timestep ts);

		bool& GetNotificationPannelVisible() { return m_NotificationPannelVisible; }
	private:
		bool m_NotificationPannelVisible = false;
	};

}
