#include "ConsoleWindow.h"

#include "Dymatic/Math/StringUtils.h"
#include "Dymatic/Math/Math.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

#include "../TextSymbols.h"

namespace Dymatic {

	ConsoleWindow::ConsoleWindow()
	{
	}

	void ConsoleWindow::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);

		dispatcher.Dispatch<KeyPressedEvent>(DY_BIND_EVENT_FN(ConsoleWindow::OnKeyPressed));
	}

	bool ConsoleWindow::OnKeyPressed(KeyPressedEvent& e)
	{
		if (e.GetKeyCode() == Key::Enter)
		{
			m_CommandLineEnter = true;
		}
		return false;
	}

	void ConsoleWindow::OnImGuiRender(Timestep ts)
	{
		if (m_ConsoleWindowVisible)
		{
			ImGui::Begin((std::string(CHARACTER_WINDOW_ICON_CONSOLE) + " Console").c_str(), &m_ConsoleWindowVisible);
			if (false)
			{
				ImGui::BeginChild("##MainLogView", ImGui::GetContentRegionAvail() - ImVec2(0, 30));
				for (int i = 0; i < m_LogList.size(); i++)
				{
					static std::string types[6] = { "[none]", "[trace]", "[info]", "[warning]", "[error]", "[critical]" };
					static ImVec4 typecolors[6] = { ImGui::GetStyleColorVec4(ImGuiCol_Text), ImVec4(0.9f, 0.9f, 0.9f, 1.0f), ImVec4(0.2f, 0.8f, 0.2f, 1.0f), ImVec4(1.0f, 1.0f, 0.6f, 1.0f), ImVec4(1.0f, 0.4f, 0.5f, 1.0f), ImVec4(1.0f, 1.0f, 1.0f, 1.0f) };
					int typeIndex = 0;
					int closestIndex = 99999;
					for (int x = 0; x < 6; x++)
					{
						if (m_LogList[i].find(types[x]) != std::string::npos && m_LogList[i].find(types[x]) < closestIndex) { typeIndex = x; closestIndex = m_LogList[i].find(types[x]); }
					}

					ImGui::PushStyleColor(ImGuiCol_Text, typecolors[typeIndex]);
					if (typeIndex == 5)
					{
						auto cursorPos = ImGui::GetCursorPos();
						ImGui::Text(m_LogList[i].c_str());
						ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::ColorConvertFloat4ToU32(ImVec4(0.9f, 0.1f, 0.2f, 1.0f)));
						ImGui::SetCursorPos(cursorPos);
					}
					ImGui::Text(m_LogList[i].c_str());
					ImGui::SetScrollHere(0.5f);
					ImGui::PopStyleColor();
				}

				ImGui::EndChild();

				static int buttonWidth = 60;
				ImGui::PushItemWidth(-buttonWidth);

				char buffer[256];
				memset(buffer, 0, sizeof(buffer));
				std::strncpy(buffer, (m_CommandBuffer).c_str(), sizeof(buffer));
				if (ImGui::InputTextWithHint("##CommandLineInput", ">>> ", buffer, sizeof(buffer)))
				{
					m_CommandBuffer = std::string(buffer);
				}
				bool commandLineActive = ImGui::IsItemActive();
				if (m_CommandLineSetFocus)
				{
					ImGui::SetKeyboardFocusHere();
					m_CommandLineSetFocus = false;
				}

				ImGui::SameLine();
				ImGui::PushItemWidth(buttonWidth / 2);
				bool execute = ImGui::Button("Execute");
				ImGui::PopItemWidth();

				if ((m_CommandLineActive && m_CommandLineEnter) || execute)
				{
					ImGui::ClearActiveID();
					ExecuteConsoleCommand(m_CommandBuffer);
					if (!execute) { m_CommandLineSetFocus = true; }
					m_CommandBuffer.clear();
				}
				m_CommandLineActive = commandLineActive;


				m_CommandLineEnter = false;

				m_TimeSinceUpdate += ts.GetSeconds();
				if (m_TimeSinceUpdate > 1.0f / m_UpdatesPerSecond)
				{
					UpdateLogList();
				}
			}
			ImGui::End();
		}
	}

	void ConsoleWindow::UpdateLogList()
	{
		m_TimeSinceUpdate = 0.0f;

		m_LogList.clear();

		std::string line;
		std::ifstream myfile(m_LogPath);

		if (myfile.is_open())
		{
			while (!myfile.eof())
			{
				std::getline(myfile, line);
				if (line != "" && line != "\n")
				{
					m_LogList.push_back(line);
				}
			}
			myfile.clear();
			myfile.close();
		}

		for (int i = 0; i < m_LogList.size(); i++)
		{
			if (m_LogList[i] == "")
			{
				m_LogList.erase(m_LogList.begin() + i);
			}
			while (m_LogList[i][0] != '[' && m_LogList[i] != "")
			{
				m_LogList[i].erase(0, 1);
			}
		}

		std::ofstream outFile;
		outFile.open(m_LogPath);
		outFile.clear();
		

		int startPos = m_LogList.size() - m_MaxLinesDisplayed - 1;
		if (startPos < 0) { startPos = 0; }
		for (int i = startPos; i < m_LogList.size(); i++)
		{
			outFile << (m_LogList[i]) << std::endl;
		}
		
		outFile.close();

		for (int i = 0; i < m_LogList.size(); i++)
		{
			static float timeSize = 80;
			static float typeSize = 80;
			static float loggerSize = 100;
		
			//Time Spacing
			while (ImGui::CalcTextSize(m_LogList[i].substr(0, String::Find_nth_of(m_LogList[i], "[", 1)).c_str()).x < timeSize)
			{
				m_LogList[i].insert(String::Find_nth_of(m_LogList[i], "[", 1), " ");
			}
		
			//Type Spacing
			int nextNonSpaceIndex = -1;
			for (int x = String::Find_nth_of(m_LogList[i], "]", 2) + 1; x < m_LogList[i].size(); x++)
			{
				if (m_LogList[i][x] != ' ')
				{
					nextNonSpaceIndex = x;
					break;
				}
			}
		
			if (nextNonSpaceIndex != -1)
			{
				while (ImGui::CalcTextSize(m_LogList[i].substr(String::Find_nth_of(m_LogList[i], "[", 1), nextNonSpaceIndex - String::Find_nth_of(m_LogList[i], "[", 1)).c_str()).x < typeSize)
				{
					m_LogList[i].insert(String::Find_nth_of(m_LogList[i], "]", 2) + 1, " ");
					nextNonSpaceIndex++;
				}
			}
		
			//Logger Spacing
		
			int nextNonSpaceIndexLogger = -1;
			for (int x = String::Find_nth_of(m_LogList[i], ":", 3) + 1; x < m_LogList[i].size(); x++)
			{
				if (m_LogList[i][x] != ' ')
				{
					nextNonSpaceIndexLogger = x;
					break;
				}
			}
		
			if (nextNonSpaceIndexLogger != -1)
			{
				while (ImGui::CalcTextSize(m_LogList[i].substr(nextNonSpaceIndex/*Uses First Next Non Index*/, nextNonSpaceIndexLogger - nextNonSpaceIndex).c_str()).x < loggerSize)
				{
					m_LogList[i].insert(String::Find_nth_of(m_LogList[i], ":", 3) + 1, " ");
					nextNonSpaceIndexLogger++;
				}
			}
		}
	}

	void ConsoleWindow::ExecuteConsoleCommand(std::string input)
	{
		if (input != "")
		{
			bool openQuote = false;
			for (int i = 0; i < input.size(); i++)
			{
				if (!openQuote)
				{
					input[i] = std::tolower(input[i]);
				}

				if (input[i] == '\"' || input[i] == '\'')
				{
					openQuote = !openQuote;
				}

				if (input[i] == ' ' && input[i + 1] == ' ' && !openQuote)
				{
					input.erase(i, 1);
					i--;
				}
			}
			
			if (openQuote)
			{
				DY_CORE_ERROR("String never closed!");
			}

			if (input[0] == ' ') { input.erase(0, 1); }
			std::string command = input.substr(0, input.find_first_of(" "));

			std::string argumentList = "";
			if (input.find(" ") != std::string::npos) { argumentList = input.substr(input.find(" ") + 1, input.size() - input.find(" ")); }
			std::string argumentString = argumentList;

			bool quoteOpen = false;
			for (int i = 0; i < argumentList.size(); i++)
			{
				if (argumentList[i] == '\"' || argumentList[i] == '\'')
				{
					openQuote = !openQuote;
				}

				if (argumentList[i] == ' ' && openQuote)
				{
					argumentList.erase(i, 1);
					argumentList.insert(i, "[__SPACE_DYMATIC_VALUE__]");
				}
			}

			std::vector<std::string> arguments;

			while (argumentList.find(" ") != std::string::npos)
			{
				arguments.push_back(argumentList.substr(0, argumentList.find(" ")));
				argumentList.erase(0, argumentList.find(" ") + 1);
			}
			if (argumentList != "" && argumentList != " ")
			{
				arguments.push_back(argumentList);
			}

			for (int i = 0; i < arguments.size(); i++)
			{
				while (arguments[i].find("[__SPACE_DYMATIC_VALUE__]") != std::string::npos)
				{
					arguments[i].replace(arguments[i].find("[__SPACE_DYMATIC_VALUE__]"), 25, " ");
				}
				for (int x = 0; x < arguments[i].length(); x++)
				{
					if (arguments[i][x] == '\"' || arguments[i][x] == '\'')
					{
						arguments[i].erase(x, 1);
					}
				}
			}

					if (command == "echo")		{ if (CheckArgumentsNumber(arguments.size(), 1, -1)) { std::string outPrint = ""; for (int i = 0; i < arguments.size(); i++) { outPrint += arguments[i] + " "; } OutputToConsole(outPrint, 2); } }
			else	if (command == "logger")	{ if (CheckArgumentsNumber(arguments.size(), 1,  1)) { if (arguments[0] == "core") { m_Logger = 0; OutputToConsole("Updated Logger To Core", 2); } else if (arguments[0] == "application") { m_Logger = 1; OutputToConsole("Updated Logger To Application", 2); } else if (arguments[0] == "query") {OutputToConsole("Current Logger Type Is: " + std::string(m_Logger == 1 ? "Application" : "Core"), 2);} else { OutputToConsole("Invalid logger argument, types include: core, application", 4); } } }
			else	if (command == "log")		{ if (CheckArgumentsNumber(arguments.size(), 2, -1))
					{
							if (arguments[0] == "trace"		) { std::string outPrint = ""; for (int i = 1; i < arguments.size(); i++) { outPrint += arguments[i] + " "; } OutputToConsole(outPrint, 1); }
						else if (arguments[0] == "info"		) { std::string outPrint = ""; for (int i = 1; i < arguments.size(); i++) { outPrint += arguments[i] + " "; } OutputToConsole(outPrint, 2); }
						else if (arguments[0] == "warn"		) { std::string outPrint = ""; for (int i = 1; i < arguments.size(); i++) { outPrint += arguments[i] + " "; } OutputToConsole(outPrint, 3); }
						else if (arguments[0] == "error"	) { std::string outPrint = ""; for (int i = 1; i < arguments.size(); i++) { outPrint += arguments[i] + " "; } OutputToConsole(outPrint, 4); }
						else if (arguments[0] == "critical"	) { std::string outPrint = ""; for (int i = 1; i < arguments.size(); i++) { outPrint += arguments[i] + " "; } OutputToConsole(outPrint, 5); }
						else
						{
								OutputToConsole("Invalid log type argument, types include: trace, info, warn, error, critical", 4);
						}
					} }
			else	if (command == "assert") { if (m_Logger == 1) { DY_ASSERT(false); } else { DY_CORE_ASSERT(false); } }
			else	if (command == "system") { std::string outPrint = ""; for (int i = 0; i < arguments.size(); i++) { outPrint += arguments[i] + " "; } SystemCommand(outPrint); }
			else	if (command == "clear") {
						system("CLS > nul"); std::ofstream dymaticLog; dymaticLog.open(m_LogPath); dymaticLog.clear(); dymaticLog.close(); }
			else	if (command == "exit") { Application::Get().Close(); }


			else { OutputToConsole("\'" + command + "\' is not recognized as a valid Dymatic console command!", 4); }

		}
		else
		{
			DY_CORE_INFO("");
		}
	}

	void ConsoleWindow::OutputToConsole(std::string string, int level)
	{
		if (m_Logger == 1)
		{
			switch (level)
			{
			case 0: {								break; }
			case 1: { DY_TRACE			(string);	break; }
			case 2: { DY_INFO			(string);	break; }
			case 3: { DY_WARN			(string);	break; }
			case 4: { DY_ERROR			(string);	break; }
			case 5: { DY_CRITICAL		(string);	break; }
			}
		}
		else
		{
			switch (level)
			{
			case 0: {								break; }
			case 1: { DY_CORE_TRACE		(string);	break; }
			case 2: { DY_CORE_INFO		(string);	break; }
			case 3: { DY_CORE_WARN		(string);	break; }
			case 4: { DY_CORE_ERROR		(string);	break; }
			case 5: { DY_CORE_CRITICAL	(string);	break; }
			}
		}
	}

	bool ConsoleWindow::CheckArgumentsNumber(int size, int min, int max)
	{
		bool MinVal = false;
		if (size >= min) { MinVal = true; }
		if (min == -1) { MinVal = true; }

		bool MaxVal = false;
		if (size <= max) { MaxVal = true; }
		if (max == -1) { MaxVal = true; }

		if (!MinVal) { OutputToConsole("Too few arguments for function!", 4); }
		if (!MaxVal) { OutputToConsole("Too many arguments for function!", 4); }

		return MinVal && MaxVal;
	}

	void ConsoleWindow::SystemCommand(std::string command)
	{
		system((command + " > " + m_SystemLogPath).c_str());

		std::ifstream ifs(m_SystemLogPath);
		std::string total((std::istreambuf_iterator<char>(ifs)),
			(std::istreambuf_iterator<char>()));

		std::vector<std::string> lines;

		while (total.find("\n") != std::string::npos)
		{
			lines.push_back(total.substr(0, total.find("\n")));
			total.erase(0, total.find("\n") + 1);
		}

		if (!lines.empty())
		{
			if (lines.size() != 1)
			{
				DY_CORE_INFO("System:");
			}
			for (int i = 0; i < lines.size(); i++)
			{
				DY_CORE_INFO(((lines.size() == 1 ? "System: " : "                                        ") + lines[i]));
			}
		}

		system("del /Q SystemConsole.log");
	}

}