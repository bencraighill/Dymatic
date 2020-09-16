#pragma once

#include "Core.h"

#include "Window.h"
#include "Dymatic/LayerStack.h"
#include "Dymatic/Events/Event.h"
#include "Dymatic/Events/ApplicationEvent.h"




namespace Dymatic {

	class DYMATIC_API Application
	{
	public:
		Application();
		virtual ~Application();

		void Run();

		void OnEvent(Event& e);

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* layer);

		inline Window& GetWindow() { return *m_Window; }

		inline static Application& Get() { return *s_Instance; }
	private:
		bool OnWindowClose(WindowCloseEvent& e);

		std::unique_ptr<Window> m_Window;
		bool m_Running = true;
		LayerStack m_LayerStack;
	private:
		static Application* s_Instance;
	};

	//Define In Client
	Application* CreateApplication();

}
