#pragma once

#include "Settings/Preferences.h"

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
		Popup(const std::string& title, const std::string& message, const std::vector<ButtonData>& buttons, Ref<Texture2D> icon, bool loading, std::function<void()> onRender = nullptr, glm::vec2 onRenderSize = {});

		static void Create(const std::string& title, const std::string& message, std::vector<ButtonData> buttons, Ref<Texture2D> icon = nullptr, bool loading = false, std::function<void()> onRender = nullptr, glm::vec2 onRenderSize = {});
		static void RemoveTopmostPopup();

		static void OnImGuiRender(Timestep ts);

	private:
		// Popup Data
		UUID ID;
		Ref<Texture2D> Icon = nullptr;
		std::string Title, Message;
		std::vector<ButtonData> Buttons;
		bool Loading = false;
		std::function<void()> OnRender = nullptr;
		glm::vec2 OnRenderSize;
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
		UUID ID;
		std::string Title, Message;
		std::vector<ButtonData> Buttons;
		float Time;
		float DisplayTime;
		bool Loading;
		std::string Timestamp;

	private:
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
	};

}
