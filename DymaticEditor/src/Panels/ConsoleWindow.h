#pragma once
#include <Dymatic.h>
#include "Dymatic/Core/Base.h"

namespace Dymatic {

	class ConsoleWindow
	{
	public:
		ConsoleWindow();

		void OnEvent(Event& e);

		bool OnKeyPressed(KeyPressedEvent& e);

		void OnImGuiRender(Timestep ts);
		void UpdateLogList();
		void ExecuteConsoleCommand(std::string input);

		bool& GetConsoleWindowVisible() { return m_ConsoleWindowVisible; }
	private:
		void OutputToConsole(std::string string, int level);
		bool CheckArgumentsNumber(int size, int min, int max);
		void SystemCommand(std::string command);
	private:
		bool m_ConsoleWindowVisible = false;

		std::vector<std::string> m_LogList;
		int m_MaxLinesDisplayed = 5;

		std::string m_LogPath = "Dymatic.log";
		std::string m_SystemLogPath = "SystemConsole.log";

		float m_TimeSinceUpdate = 100;
		int m_UpdatesPerSecond = 18;
		std::string m_CommandBuffer = "";
		bool m_CommandLineActive = false;
		bool m_CommandLineSetFocus = false;
		bool m_CommandLineEnter = false;
		int m_Logger = 0;
	};

}
