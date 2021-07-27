#pragma once

#include <Dymatic.h>

namespace Dymatic {

	//typedef int(CALLBACK* LPFNDLLFUNC1)();

	enum LauncherPage
	{
		Home,
		Engine,
		Store,
		Library,
		Settings,
		LAUNCHER_PAGE_SIZE
	};

	class LauncherLayer : public Layer
	{
	public:
		LauncherLayer();
		virtual ~LauncherLayer() = default;

		virtual void OnAttach() override;
		virtual void OnDetach() override;

		void OnUpdate(Timestep ts) override;
		virtual void OnImGuiRender() override;
		void OnEvent(Event& e) override;
	private:
		bool OnKeyPressed(KeyPressedEvent& e);
		bool OnMouseButtonPressed(MouseButtonPressedEvent& e);

		void CloseProgramWindow();

		void CheckUpdate();
		void UpdateLauncherInformation();

		int SystemCommand(std::string command);
		std::string ReadFile(std::string path);
		void WriteFile(std::string path, std::string string);

		std::string ReadInEncryptedFile(std::string path);

		std::string EncryptString(std::string string);
		std::string DecryptString(std::string string);
		int Hash(int state);
	private:

		float m_DeltaTime;

		bool m_Online = false;

		Ref<Texture2D> m_DymaticLogo = Texture2D::Create("assets/icons/DymaticLogo.png");

		LauncherPage m_CurrentPage = Engine;

		//System Information
		int m_CurrentVersion = 2;


		//Launcher Info Package
		int m_LatestVersion = 0;
	};

}