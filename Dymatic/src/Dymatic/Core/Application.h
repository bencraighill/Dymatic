#pragma once

#include "Dymatic/Core/Base.h"

#include "Dymatic/Core/Window.h"
#include "Dymatic/Core/LayerStack.h"
#include "Dymatic/Events/Event.h"
#include "Dymatic/Events/ApplicationEvent.h"

#include "Dymatic/Core/Timestep.h"

#include "Dymatic/ImGui/ImGuiLayer.h"

int main(int argc, char** argv);

namespace Dymatic {

	class Application
	{
	public:
		Application(const std::string& name = "Dymatic Engine");
		virtual ~Application();

		void OnEvent(Event& e);

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* layer);

		Timestep GetTimestep() { return m_Timestep; }

		bool GetCloseWindowButtonPressed() { return m_CloseWindowCallback == 2; }
		void SetCloseWindowCallback(bool enabled) { m_CloseCallbackEnabled = enabled; }

		Window& GetWindow() { return *m_Window; }

		void Close();

		ImGuiLayer* GetImGuiLayer() { return m_ImGuiLayer; }

		static Application& Get() { return *s_Instance; }
	private:
		void Run();
		bool OnWindowClose(WindowCloseEvent& e);
		bool OnWindowResize(WindowResizeEvent& e);
	private:
		Scope<Window> m_Window;
		ImGuiLayer* m_ImGuiLayer;
		bool m_Running = true;
		bool m_Minimized = false;
		LayerStack m_LayerStack;
		float m_LastFrameTime = 0.0f;
		Timestep m_Timestep = 0.0f;
		// Editor layer uses events to detect when trying to close window
		bool m_CloseCallbackEnabled = false;
		int m_CloseWindowCallback = 0;
	private:
		static Application* s_Instance;
		friend int ::main(int argc, char** argv);
	};

	// To be defined in CLIENT
	Application* CreateApplication();

}