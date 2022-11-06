#pragma once

#include "Dymatic/Core/Window.h"
#include "Dymatic/Renderer/GraphicsContext.h"

#include <GLFW/glfw3.h>

namespace Dymatic {

	class WindowsWindow : public Window
	{
	public:
		WindowsWindow(const WindowProps& props);
		virtual ~WindowsWindow();

		void OnUpdate() override;

		uint32_t GetWidth() const override { return m_Data.Width; }
		uint32_t GetHeight() const override { return m_Data.Height; }
		void SetSize(int width, int height) const override;

		uint32_t GetPositionX() const override;
		uint32_t GetPositionY() const override;
		void SetPosition(int x, int y) const override;

		void MinimizeWindow() const override;
		void MaximizeWindow() const override;
		void ReturnWindow() const override;

		bool IsWindowMaximized() const override;

		void SetCursor(int shape) const override;

		void SetTitlebarHoveredQueryCallback(void (*)(int*)) override;
		void SetMinimizeHoveredQueryCallback(void (*)(int*)) override;
		void SetMaximizeHoveredQueryCallback(void (*)(int*)) override;
		void SetCloseHoveredQueryCallback(void (*)(int*)) override;

		// Window attributes
		void SetEventCallback(const EventCallbackFn& callback) override { m_Data.EventCallback = callback; }
		void SetVSync(bool enabled) override;
		bool IsVSync() const override;

		virtual void* GetNativeWindow() const { return m_Window; }
	private:
		virtual void Init(const WindowProps& props);
		virtual void Shutdown();
	private:
		GLFWwindow* m_Window;
		Scope<GraphicsContext> m_Context;

		struct WindowData
		{
			std::string Title;
			unsigned int Width = 1600, Height = 900;
			bool VSync;

			EventCallbackFn EventCallback;
		};

		WindowData m_Data;
	};

}