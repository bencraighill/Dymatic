#include "dypch.h"
#include "Dymatic/Core/Application.h"

#include "Dymatic/Core/Log.h"

#include "Dymatic/Core/Version.h"

#include "Dymatic/Renderer/Renderer.h"
#include "Dymatic/Scripting/ScriptEngine.h"
#include "Dymatic/Audio/AudioEngine.h"
#include "Dymatic/Physics/PhysicsEngine.h"
#include "Dymatic/Asset/AssetManager.h"

#include "Dymatic/Core/Input.h"

#include "Dymatic/Utils/PlatformUtils.h"

#include "GLFW/glfw3.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <cmath>
#include <iostream>
#include <csignal>

namespace Dymatic {

	Application* Application::s_Instance = nullptr;

	extern "C" void ApplicationSignalHandler(int signum)
	{
		const char* signal;
		switch (signum)
		{
		case SIGINT:	signal = "interupt";					break;
		case SIGILL:	signal = "illegal instruction";			break;
		case SIGFPE:	signal = "floating point exception";	break;
		case SIGSEGV:	signal = "segment violation";			break;
		case SIGTERM:	signal = "kill";						break;
		case SIGBREAK:	signal = "break";						break;
		case SIGABRT:	signal = "abort";						break;
		}

#ifdef DY_PLATFORM_WINDOWS
		MessageBoxA(nullptr, fmt::format("Process in engine application layer recieved a {} signal! The engine will now terminate.", signal).c_str(), "Dymatic Critical Engine Failure", MB_OK | MB_ICONERROR);
#endif

		DY_CORE_CRITICAL("Critical engine failure! Process in engine application layer recieved a {} signal", signal);
		exit(1);
	}

	Application::Application(const ApplicationSpecification& specification)
		: m_Specification(specification)
	{
		DY_PROFILE_FUNCTION();

		std::signal(SIGINT,		ApplicationSignalHandler);
		std::signal(SIGILL,		ApplicationSignalHandler);
		std::signal(SIGFPE,		ApplicationSignalHandler);
		std::signal(SIGSEGV,	ApplicationSignalHandler);
		std::signal(SIGTERM,	ApplicationSignalHandler);
		std::signal(SIGBREAK,	ApplicationSignalHandler);
		std::signal(SIGABRT,	ApplicationSignalHandler);
		
		std::cout << "Dymatic Engine " DY_VERSION << std::endl;
		std::cout << DY_VERSION_COPYRIGHT_SAFE << std::endl;
		std::cout << std::endl;
		
		if (m_Specification.ConsoleVisible)
			Log::ShowConsole();
		else
			Log::HideConsole();

		if (!m_Specification.SplashImage.empty())
			Splash::Init(m_Specification.SplashName.empty() ? m_Specification.Name : m_Specification.SplashName, m_Specification.SplashImage, 30);

		Splash::Update("Initializing Dymatic Core...", 14);
		DY_CORE_ASSERT(!s_Instance, "Application already exists!");
		s_Instance = this;

		// Set working directory here
		if (!m_Specification.WorkingDirectory.empty())
			std::filesystem::current_path(m_Specification.WorkingDirectory);
		
		m_Window = Window::Create(WindowProps(m_Specification.Name, m_Specification.WindowWidth, m_Specification.WindowHeight, m_Specification.WindowDecorated, m_Specification.WindowStartHidden, m_Specification.ApplicationIcon));

		Splash::Update("Binding Callbacks...", 37);
		m_Window->SetEventCallback(DY_BIND_EVENT_FN(Application::OnEvent));

		PlatformUtils::Init();

		Splash::Update("Initializing Input...", 42);
		Input::Init();

		if (m_Specification.EnableEngineSystems)
		{
			Splash::Update("Initializing Asset Manager...", 47);
			AssetManager::Init();

			Splash::Update("Initializing Renderer...", 68);
			Renderer::Init();

			Splash::Update("Initializing Script Engine...", 71);
			ScriptEngine::Init();

			Splash::Update("Initializing Audio Engine...", 79);
			AudioEngine::Init();

			Splash::Update("Initializing Physics Engine...", 87);
			PhysicsEngine::Init();
		}

		Splash::Update("Initializing Interface...", 92);

		if (m_Specification.EnableImGui)
		{
			m_ImGuiLayer = new ImGuiLayer();
			PushOverlay(m_ImGuiLayer);
		}
	}

	Application::~Application()
	{
		DY_PROFILE_FUNCTION();

		m_LayerStack.DetachAll();

		if (m_Specification.EnableEngineSystems)
		{
			PhysicsEngine::Shutdown();
			AudioEngine::Shutdown();
			ScriptEngine::Shutdown();
			Renderer::Shutdown();
			AssetManager::Shutdown();
		}
		
		Input::Shutdown();
		PlatformUtils::Shutdown();
	}

	void Application::PushLayer(Layer* layer)
	{
		DY_PROFILE_FUNCTION();

		Splash::Update("Launching Application...", 94);

		m_LayerStack.PushLayer(layer);
		layer->OnAttach();

		Splash::Update("Finalizing...", 100);
		Splash::Shutdown();
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

	void Application::SubmitToMainThread(const std::function<void()>& function)
	{
		std::scoped_lock<std::mutex> lock(m_MainThreadQueueMutex);

		m_MainThreadQueue.emplace_back(function);
	}

	void Application::OnEvent(Event& e)
	{
		DY_PROFILE_FUNCTION();

		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowResizeEvent>(DY_BIND_EVENT_FN(Application::OnWindowResize));

		for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend(); ++it)
		{
			if (e.Handled)
				break;
			(*it)->OnEvent(e);
		}

		if (!e.Handled)
			dispatcher.Dispatch<WindowCloseEvent>(DY_BIND_EVENT_FN(Application::OnWindowClose));
	}

	void Application::Run()
	{
		DY_PROFILE_FUNCTION();

		while (m_Running)
		{
			DY_PROFILE_SCOPE("RunLoop");
			Draw();
		}
	}

	static bool s_DrawGuard;
	void Application::Draw()
	{
		// TODO: Remove this? This prevents drawing during resizing!
		if (s_DrawGuard)
			return;

		s_DrawGuard = true;

		DY_PROFILE_FUNCTION();

		float time = Time::GetTime();
		m_Timestep = time - m_LastFrameTime;
		m_LastFrameTime = time;

		Input::PreUpdate();
		
		ExecuteMainThreadQueue();

		if (m_Specification.EnableEngineSystems)
			AssetManager::OnUpdate();

		if (!m_Minimized)
		{
			{
				DY_PROFILE_SCOPE("LayerStack OnUpdate");

				for (Layer* layer : m_LayerStack)
					layer->OnUpdate(m_Timestep);
			}
		}
		
		// Undocked ImGui windows may not be minimized so still render
		if (m_Specification.EnableImGui)
		{
			m_ImGuiLayer->Begin();
			{
				DY_PROFILE_SCOPE("LayerStack OnImGuiRender");

				for (Layer* layer : m_LayerStack)
					layer->OnImGuiRender();
			}
			m_ImGuiLayer->End();
		}

		Input::PostUpdate();

		m_Window->OnUpdate();

		s_DrawGuard = false;
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

		Draw();

		return false;
	}

	void Application::ExecuteMainThreadQueue()
	{
		std::scoped_lock<std::mutex> lock(m_MainThreadQueueMutex);

		for (auto& func : m_MainThreadQueue)
			func();

		m_MainThreadQueue.clear();
	}

}
