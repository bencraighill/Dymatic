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

	struct ApplicationCommandLineArgs
	{
		int Count = 0;
		char** Args = nullptr;

		const char* operator[](int index) const
		{
			DY_CORE_ASSERT(index < Count);
			return Args[index];
		}
	};

	struct ApplicationSpecification
	{
		std::string Name = "Dymatic Application";
		std::string WorkingDirectory;
		uint32_t WindowWidth = 1600;
		uint32_t WindowHeight = 900;
		bool Runtime = false;
		bool ConsoleVisible = true;
		bool WindowDecorated = true;
		bool WindowStartHidden = false;
		bool FullScreen = false;
		bool EnableImGui = true;
		bool EnableEngineSystems = true;
		std::filesystem::path ApplicationIcon = "";
		std::filesystem::path SplashImage = "";
		std::string SplashName = "";
		ApplicationCommandLineArgs CommandLineArgs;
	};

	class Application
	{
	public:
		Application(const ApplicationSpecification& specification);
		virtual ~Application();

		void OnEvent(Event& e);

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* layer);

		Window& GetWindow() { return *m_Window; }
		static Application& Get() { return *s_Instance; }

		void Close();


		ImGuiLayer* GetImGuiLayer() { return m_ImGuiLayer; }

		void SubmitToMainThread(const std::function<void()>& function);

		inline const ApplicationSpecification& GetSpecification() const { return m_Specification; }

		// TODO: Code enabled/disabled by this should probably be excluded directly by the preprocessor instead of a runtime flag
		inline static bool IsRuntime() { return s_Instance->m_Specification.Runtime; }
	private:
		void Run();
		void Draw();
		bool OnWindowClose(WindowCloseEvent& e);
		bool OnWindowResize(WindowResizeEvent& e);

		void ExecuteMainThreadQueue();
	private:
		ApplicationSpecification m_Specification;
		Scope<Window> m_Window;
		ImGuiLayer* m_ImGuiLayer;
		bool m_Running = true;
		bool m_Minimized = false;
		LayerStack m_LayerStack;
		float m_LastFrameTime = 0.0f;
		Timestep m_Timestep = 0.0f;

		std::vector<std::function<void()>> m_MainThreadQueue;
		std::mutex m_MainThreadQueueMutex;
	private:
		static Application* s_Instance;
		friend int ::main(int argc, char** argv);
	};

	// To be defined in CLIENT
	Application* CreateApplication(ApplicationCommandLineArgs args);

}