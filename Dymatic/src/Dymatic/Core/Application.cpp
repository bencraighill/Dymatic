#include "dypch.h"
#include "Dymatic/Core/Application.h"

#include "Dymatic/Core/Log.h"

#include "Dymatic/Renderer/Renderer.h"

#include "Dymatic/Core/Input.h"

#include <GLFW/glfw3.h>

#include "../vendor/CSplash/Splash.h"

namespace Dymatic {

	Application* Application::s_Instance = nullptr;

	CSplash splash1(TEXT("splash.bmp"), RGB(255, 0, 0));

	Application::Application(const std::string& name, ApplicationCommandLineArgs args)
		: m_CommandLineArgs(args)
	{
		DY_PROFILE_FUNCTION();
		splash1.ShowSplash();

		DY_CORE_ASSERT(!s_Instance, "Application already exists!");
		s_Instance = this;
		m_Window = Window::Create(WindowProps(name));
		m_Window->SetEventCallback(DY_BIND_EVENT_FN(Application::OnEvent));

		Renderer::Init();

		m_ImGuiLayer = new ImGuiLayer();
		PushOverlay(m_ImGuiLayer);
	}

	Application::~Application()
	{
		DY_PROFILE_FUNCTION();

		Renderer::Shutdown();
	}

	void Application::PushLayer(Layer* layer)
	{
		DY_PROFILE_FUNCTION();

		splash1.CloseSplash();

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
		dispatcher.Dispatch<WindowCloseEvent>(DY_BIND_EVENT_FN(Application::OnWindowClose));
		dispatcher.Dispatch<WindowResizeEvent>(DY_BIND_EVENT_FN(Application::OnWindowResize));

		for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend(); ++it)
		{
			if (e.Handled)
				break;
			(*it)->OnEvent(e);
		}
	}

	void Application::Run()
	{
		DY_PROFILE_FUNCTION();

		while (m_Running)
		{
			DY_PROFILE_SCOPE("RunLoop");

			float time = (float)glfwGetTime();
			m_Timestep = time - m_LastFrameTime;
			m_LastFrameTime = time;

			if (!m_Minimized)
			{
				{
					DY_PROFILE_SCOPE("LayerStack OnUpdate");

					for (Layer* layer : m_LayerStack)
						layer->OnUpdate(m_Timestep);
				}

			}
			//OnImGuiRender used to be reliant on being not minimized, but this meant unlocked windows wouldn't work when GLFW was minimized.
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
