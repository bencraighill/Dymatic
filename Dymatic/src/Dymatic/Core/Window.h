 #pragma once

#include <sstream>

#include "Dymatic/Core/Base.h"
#include "Dymatic/Events/Event.h"

namespace Dymatic {

	struct WindowProps
	{
		std::string Title;
		uint32_t Width;
		uint32_t Height;
		bool Decorated;
		bool StartHidden;
		std::string Icon;

		WindowProps(const std::string& title = "Dymatic Engine",
			uint32_t width = 1600,
			uint32_t height = 900,
			bool decorated = true,
			bool startHidden = false,
			std::string icon = "")
			: Title(title), Width(width), Height(height), Decorated(decorated), StartHidden(startHidden), Icon(icon)
		{
		}
	};

	// Interface representing a desktop system based Window
	class Window
	{
	public:
		using EventCallbackFn = std::function<void(Event&)>;

		virtual ~Window() = default;

		virtual void OnUpdate() = 0;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;
		virtual  void SetSize(int width, int height) const = 0;

		virtual uint32_t GetPositionX() const = 0;
		virtual uint32_t GetPositionY() const = 0;
		virtual void SetPosition(int x, int y) const = 0;

		virtual bool IsWindowMinimized() const = 0;
		virtual void MinimizeWindow() const = 0;
		
		virtual bool IsWindowMaximized() const = 0;
		virtual void MaximizeWindow() const = 0;

		virtual void RestoreWindow() const = 0;

		virtual bool IsWindowFocused() const = 0;
		virtual void FocusWindow() const = 0;

		virtual void ShowWindow() const = 0;
		virtual void HideWindow() const = 0;

		virtual void SetCursor(int shape) const = 0;
		virtual void LockCursor(bool locked) const = 0;

		virtual void SetTitlebarHoveredQueryCallback(void (*)(int*)) = 0;
		virtual void SetMinimizeHoveredQueryCallback(void (*)(int*)) = 0;
		virtual void SetMaximizeHoveredQueryCallback(void (*)(int*)) = 0;
		virtual void SetCloseHoveredQueryCallback(void (*)(int*)) = 0;

		// Window attributes
		virtual void SetEventCallback(const EventCallbackFn& callback) = 0;
		virtual void SetVSync(bool enabled) = 0;
		virtual bool IsVSync() const = 0;

		virtual void* GetNativeWindow() const = 0;

		static Scope<Window> Create(const WindowProps& props = WindowProps());
	};

}
