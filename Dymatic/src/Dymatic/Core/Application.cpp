#include "dypch.h"
#include "Dymatic/Core/Application.h"

#include "Dymatic/Core/Log.h"

#include "Dymatic/Renderer/Renderer.h"

#include "Dymatic/Core/Input.h"

#include "Dymatic/Utils/PlatformUtils.h"

#include "GLFW/glfw3.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include "../vendor/CSplash/Splash.h"

// OLE DROP
#include <oleidl.h>

#include <cmath>

#include "Dymatic/Scene/ScriptEngine.h"
#include "Dymatic/Audio/AudioEngine.h"
#include "Dymatic/Physics/PhysicsEngine.h"

namespace Dymatic {

	Application* Application::s_Instance = nullptr;

	class DropManager : public IDropTarget
	{
	public:
		ULONG AddRef() { return 1; }
		ULONG Release() { return 0; }

		HRESULT QueryInterface(REFIID riid, void** ppvObject)
		{
			if (riid == IID_IDropTarget)
			{
				*ppvObject = this;
				return S_OK;
			}
			*ppvObject = NULL;
			return E_NOINTERFACE;
		};

		HRESULT DragEnter(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
		{
			WindowDragEnterEvent e;
			Application::Get().OnEvent(e);

			*pdwEffect &= DROPEFFECT_COPY;
			return S_OK;
		}

		HRESULT DragLeave() 
		{
			WindowDragLeaveEvent e;
			Application::Get().OnEvent(e);

			return S_OK; 
		}

		HRESULT DragOver(DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
		{
			WindowDragOverEvent e;
			Application::Get().OnEvent(e);

			// trigger MouseMove within ImGui, position is within pt.x and pt.y
			// grfKeyState contains flags for control, alt, shift etc

			*pdwEffect &= DROPEFFECT_COPY;
			return S_OK;
		}

		HRESULT Drop(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
		{
			// grfKeyState contains flags for control, alt, shift etc

			FORMATETC fmte = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
			STGMEDIUM stgm;

			if (SUCCEEDED(pDataObj->GetData(&fmte, &stgm)))
			{
				std::vector<std::string> paths;

				HDROP hdrop = (HDROP)stgm.hGlobal;
				UINT file_count = DragQueryFile(hdrop, 0xFFFFFFFF, NULL, 0);

				for (UINT i = 0; i < file_count; i++)
				{
					TCHAR szFile[MAX_PATH];
					UINT cch = DragQueryFile(hdrop, i, szFile, MAX_PATH);
					if (cch > 0 && cch < MAX_PATH)
					{
						std::wstring wstr = szFile;
						std::string str(wstr.begin(), wstr.end());

						paths.push_back(str);
					}
				}

				ReleaseStgMedium(&stgm);

				WindowDropEvent e { paths };
				Application::Get().OnEvent(e);
			}

			*pdwEffect &= DROPEFFECT_COPY;
			return S_OK;
		}
	};

	DropManager dropManager;

	CSplash ApplicationSplash(TEXT("splash.bmp"), RGB(255, 0, 0), 30);

	static void UpdateSplashMessage(const std::string& message, uint8_t percentage)
	{
		ApplicationSplash.ReloadBitmap();
		ApplicationSplash.DrawLoadText(message + " (" + std::to_string(percentage) + "%)", {(LONG)25.0, (LONG)(315.0), (LONG)(500.0), (LONG)331.0}, 14.0f, TEXT("Lato"));
		ApplicationSplash.DrawLoadText("© Dymatic Technologies 2022", {(LONG)475.0, (LONG)(315.0), (LONG)(638.0), (LONG)331.0}, 14.0f, TEXT("Lato"));
		ApplicationSplash.DrawLoadText("V1.2.4", {(LONG)575.0, (LONG)(15.0f), (LONG)(638.0 * 1.0f), (LONG)(30.0f)}, 14.0f, TEXT("Lato"));
		const int min = 25;
		const int max = 620;
		ApplicationSplash.DrawRect(min, 333, max, 340, RGB(50, 50, 50));
		ApplicationSplash.DrawRect(min, 333, min + ((max - min) * (percentage / 100.0f)), 340, RGB(150, 150, 150));
	}

	Application::Application(const ApplicationSpecification& specification)
		: m_Specification(specification)
	{
		DY_PROFILE_FUNCTION();
		//Log::HideConsole(); //!!!

		// OLE
		OleInitialize(NULL);

		ApplicationSplash.ShowSplash();
		UpdateSplashMessage("Initializing Dymatic Core...", 5);
		DY_CORE_ASSERT(!s_Instance, "Application already exists!");
		s_Instance = this;

		// Set working directory here
		if (!m_Specification.WorkingDirectory.empty())
			std::filesystem::current_path(m_Specification.WorkingDirectory);

		UpdateSplashMessage("Creating Window...", 14);
		m_Window = Window::Create(WindowProps(m_Specification.Name));

		UpdateSplashMessage("Binding Callbacks...", 37);
		m_Window->SetEventCallback(DY_BIND_EVENT_FN(Application::OnEvent));

		m_Window->MaximizeWindow();

		// OLE
		RegisterDragDrop(glfwGetWin32Window((GLFWwindow*)m_Window->GetNativeWindow()), &dropManager);

		UpdateSplashMessage("Initializing Renderer...", 42);
		Renderer::Init();

		UpdateSplashMessage("Initializing Script Engine...", 54);
		ScriptEngine::Init();

		UpdateSplashMessage("Initializing Audio Engine...", 71);
		AudioEngine::Init();

		UpdateSplashMessage("Initializing Physics Engine...", 79);
		PhysicsEngine::Init();

		UpdateSplashMessage("Initializing GUI...", 86);
		m_ImGuiLayer = new ImGuiLayer();
		PushOverlay(m_ImGuiLayer);
	}

	Application::~Application()
	{
		DY_PROFILE_FUNCTION();

		Renderer::Shutdown();
		AudioEngine::Shutdown();
		PhysicsEngine::Shutdown();

		// OLE
		RevokeDragDrop(glfwGetWin32Window((GLFWwindow*)m_Window->GetNativeWindow()));

		// OLE
		OleUninitialize();
	}

	void Application::PushLayer(Layer* layer)
	{
		DY_PROFILE_FUNCTION();

		UpdateSplashMessage("Launching Editor...", 97);
		ApplicationSplash.CloseSplash();

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

			float time = Time::GetTime();
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
			// OnImGuiRender used to be reliant on being not minimized, but this meant unlocked windows wouldn't work when GLFW was minimized.
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
