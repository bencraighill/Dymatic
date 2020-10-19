#pragma once

#include "Core.h"

#include "Window.h"
#include "Dymatic/LayerStack.h"
#include "Dymatic/Events/Event.h"
#include "Dymatic/Events/ApplicationEvent.h"

#include "Dymatic/ImGui/ImGuiLayer.h"

#include "Dymatic/Renderer/Shader.h"
#include "Dymatic/Renderer/Buffer.h"
#include "Dymatic/Renderer/VertexArray.h"

#include "Dymatic/Renderer/OrthographicCamera.h"

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
		ImGuiLayer* m_ImGuiLayer;
		bool m_Running = true;
		LayerStack m_LayerStack;
 
		std::shared_ptr<Shader> m_Shader;
		std::shared_ptr<VertexArray> m_VertexArray;

		std::shared_ptr<Shader> m_BlueShader;
		std::shared_ptr<VertexArray> m_SquareVA;

		OrthographicCamera m_Camera;
	private:
		static Application* s_Instance;
	};

	//Define In Client
	Application* CreateApplication();

}
