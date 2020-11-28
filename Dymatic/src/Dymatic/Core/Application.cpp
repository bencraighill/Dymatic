#include "dypch.h"
#include "Application.h"


#include "Dymatic/Core/Log.h"

#include "Dymatic/Renderer/Renderer.h"

#include "Input.h"

#include <glfw/glfw3.h>

#include "../vendor/CSplash_src/Splash.h"



namespace Dymatic {

#define BIND_EVENT_FN(x) std::bind(&Application::x, this, std::placeholders::_1)

	Application* Application::s_Instance = nullptr;

	//CSplash Splash(TEXT("splash.bmp"), RGB(128, 128, 128));


	Application::Application()
	{
		DY_PROFILE_FUNCTION();

		DY_CORE_ASSERT(!s_Instance, "Application already exists!")
		s_Instance = this;

		//  Display Engine Splash
		{
			//Splash.ShowSplash();
		}

		m_Window = std::unique_ptr<Window>(Window::Create());
		m_Window->SetEventCallback(BIND_EVENT_FN(OnEvent));

		Renderer::Init();

		m_ImGuiLayer = new ImGuiLayer();
		PushOverlay(m_ImGuiLayer);

		
	}


	Application::~Application()
	{
		DY_PROFILE_FUNCTION();

	}

	void Application::PushLayer(Layer* layer)
	{
		DY_PROFILE_FUNCTION();

		//Close Engine Splash
		//Splash.CloseSplash();

		m_LayerStack.PushLayer(layer);
		layer->OnAttach();
	}

	void Application::PushOverlay(Layer* layer)
	{
		DY_PROFILE_FUNCTION();

		m_LayerStack.PushOverlay(layer);
		layer->OnAttach();
	}

	void Application::Close()
	{
		m_Running = false;
	}

	void Application::OnEvent(Event& e)
	{
		DY_PROFILE_FUNCTION();

		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(OnWindowClose));
		dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT_FN(OnWindowResize));

		

		for (auto it = m_LayerStack.end(); it != m_LayerStack.begin(); )
		{
			(*--it)->OnEvent(e);
				if (e.m_Handled)
					break;
		}
	}

	

	void Application::Run()
	{
		DY_PROFILE_FUNCTION();

		while (m_Running)
		{ 
			DY_PROFILE_FUNCTION("Run Loop");

			float time = (float)glfwGetTime();
			Timestep timestep = time - m_LastFrameTime;
			m_LastFrameTime = time;

			if (!m_Minimized)
			{
				{
					DY_PROFILE_SCOPE("LayerStack OnUpdate");

					for (Layer* layer : m_LayerStack)
						layer->OnUpdate(timestep);
				}
			}

			m_ImGuiLayer->Begin();
			{
				DY_PROFILE_SCOPE("LayerStack OnImGuiRender");

				for (Layer* layer : m_LayerStack)
					layer->OnImGuiRender();
			}
			m_ImGuiLayer->End();

			m_Window->OnUpdate();
		}
	}

	bool Application::OnWindowClose(WindowCloseEvent& e)
	{
		m_Running = false;
		return true;
	}

	bool Application::OnWindowResize(WindowResizeEvent& e)
	{
		DY_PROFILE_FUNCTION();

		if (e.GetWidth() == 0 || e.GetHeight() == 0)
		{
			m_Minimized = true;
			return false;
		}

		m_Minimized = false;
		Renderer::OnWindowResize(e.GetWidth(), e.GetHeight());

		return false;
	}
}
