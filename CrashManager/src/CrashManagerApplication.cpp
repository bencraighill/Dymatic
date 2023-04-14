//// Windows API
//#include "CrashManager.h"
//#include <windows.h>

// Dymatic
#include "Dymatic/Core/Base.h"
#include "Dymatic/Core/Application.h"
#include "Dymatic/Core/Layer.h"
#include "Dymatic/Core/Log.h"
#include "Dymatic/Core/Assert.h"
#include "Dymatic/Core/Timestep.h"
#include "Dymatic/Core/Input.h"
#include "Dymatic/Core/KeyCodes.h"
#include "Dymatic/Core/MouseCodes.h"

#include "Dymatic/Renderer/Texture.h"

#include "Dymatic/ImGui/ImGuiLayer.h"

// ImGui
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

VOID OpenApplication(LPCTSTR lpApplicationName)
{
	// additional information
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	// set the size of the structures
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	// start the program up
	CreateProcess(lpApplicationName,   // the path
		NULL,        // Command line
		NULL,           // Process handle not inheritable
		NULL,           // Thread handle not inheritable
		FALSE,          // Set handle inheritance to FALSE
		0,              // No creation flags
		NULL,           // Use parent's environment block
		NULL,           // Use parent's starting directory 
		&si,            // Pointer to STARTUPINFO structure
		&pi             // Pointer to PROCESS_INFORMATION structure (removed extra parentheses)
	);
	// Close process and thread handles. 
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
}

BOOL IsProcessRunning(DWORD pid)
{
	HANDLE process = OpenProcess(SYNCHRONIZE, FALSE, pid);
	DWORD ret = WaitForSingleObject(process, 0);
	CloseHandle(process);
	return ret == WAIT_TIMEOUT;
}

char* logData = nullptr;
int LogSize;

int main(int argc, char* argv[])
{
	auto id = strtoul(argv[0], NULL, 16);
	while (true)
	{
		if (!IsProcessRunning(id)) break;
		Sleep(1000);
	}
	auto log_path = argv[2];

	bool failed = false;
	std::string result;
	std::ifstream in(log_path, std::ios::in | std::ios::binary); // ifstream closes itself due to RAII
	if (in)
	{
		in.seekg(0, std::ios::end);
		size_t size = in.tellg();
		if (size != -1)
		{
			result.resize(size);
			in.seekg(0, std::ios::beg);
			in.read(&result[0], size);
		}
		else
			failed = true;
	}
	result += "\n[internal error] CRASH MANAGER: Unexpected termination of Dymatic Core process.\n[END OF LOG]";

	if (!failed)
	{
		LogSize = result.length();
		logData = new char[LogSize];
		memset(logData, 0, LogSize);
		std::strncpy(logData, result.c_str(), LogSize);
	}

	Dymatic::Log::Init();
	DY_CORE_ERROR("CRASH MANAGER: Unexpected termination of Dymatic Core process");

	auto app = Dymatic::CreateApplication({ argc, argv });
	app->Run();
	delete app;

	if (!failed)
		delete[] logData;
}

namespace Dymatic {

	class CrashManagerLayer : public Layer
	{
	public:
		CrashManagerLayer(const char* executable_path)
			: m_ExecutablePath(executable_path)
		{
		}
		virtual ~CrashManagerLayer() = default;

		virtual void OnAttach() override;
		virtual void OnDetach() override;

		void OnUpdate(Timestep ts) override;
		virtual void OnImGuiRender() override;
		void OnEvent(Event& e) override;
	private:
		Ref<Texture2D> m_DymaticLogo;
		bool m_SubmitLogInfo = true;
		bool m_SubmitProfileInfo = true;
		const char* m_ExecutablePath;
	};

	void CrashManagerLayer::OnAttach()
	{
		m_DymaticLogo = Texture2D::Create("Resources/Icons/Branding/DymaticLogo.png");

		Application::Get().GetWindow().MaximizeWindow();
		Application::Get().GetWindow().FocusWindow();

		// Setup Styling
		auto& colors = ImGui::GetStyle().Colors;
		colors[ImGuiCol_Text] = ImVec4{ 1.0, 1.0, 1.0, 1.0 };
		colors[ImGuiCol_TextDisabled] = ImVec4{ 0.5, 0.5, 0.5, 1.0 };
		colors[ImGuiCol_TextSelectedBg] = ImVec4{ 0.26, 0.59, 0.98, 0.35 };
		colors[ImGuiCol_MenuBarBg] = ImVec4{ 0.14, 0.14, 0.14, 1.0 };
		colors[ImGuiCol_MenuBarGrip] = ImVec4{ 0.15, 0.15, 0.15, 1.0 };
		colors[ImGuiCol_MenuBarGripBorder] = ImVec4{ 0.35, 0.35, 0.35, 1.0 };
		colors[ImGuiCol_WindowBg] = ImVec4{ 0.1, 0.105, 0.11, 1.0 };
		colors[ImGuiCol_Header] = ImVec4{ 0.2, 0.205, 0.21, 1.0 };
		colors[ImGuiCol_HeaderHovered] = ImVec4{ 0.3, 0.305, 0.31, 1.0 };
		colors[ImGuiCol_HeaderActive] = ImVec4{ 0.15, 0.1505, 0.151, 1.0 };
		colors[ImGuiCol_Tab] = ImVec4{ 0.15, 0.1505, 0.151, 1.0 };
		colors[ImGuiCol_TabHovered] = ImVec4{ 0.38, 0.3805, 0.381, 1.0 };
		colors[ImGuiCol_TabActive] = ImVec4{ 0.28, 0.2805, 0.281, 1.0 };
		colors[ImGuiCol_TabUnfocused] = ImVec4{ 0.15, 0.1505, 0.151, 1.0 };
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4{ 0.2, 0.205, 0.21, 1.0 };
		colors[ImGuiCol_TitleBg] = ImVec4{ 0.15, 0.1505, 0.151, 1.0 };
		colors[ImGuiCol_TitleBgActive] = ImVec4{ 0.15, 0.1505, 0.151, 1.0 };
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4{ 0.15, 0.1505, 0.151, 1.0 };
		colors[ImGuiCol_Button] = ImVec4{ 0.298147, 0.303514, 0.30888, 1.0 };
		colors[ImGuiCol_ButtonHovered] = ImVec4{ 0.423529, 0.576471, 0.807843, 1.0 };
		colors[ImGuiCol_ButtonActive] = ImVec4{ 0.32549, 0.47451, 0.705882, 1.0 };
		colors[ImGuiCol_ButtonToggled] = ImVec4{ 0.325, 0.475, 0.706, 1.0 };
		colors[ImGuiCol_ButtonToggledHovered] = ImVec4{ 0.425, 0.575, 0.806, 1.0 };
		colors[ImGuiCol_PopupBg] = ImVec4{ 0.08, 0.08, 0.08, 0.94 };
		colors[ImGuiCol_ModalWindowDimBg] = ImVec4{ 0.8, 0.8, 0.8, 0.35 };
		colors[ImGuiCol_Border] = ImVec4{ 0.43, 0.43, 0.5, 0.5 };
		colors[ImGuiCol_BorderShadow] = ImVec4{ 0.0, 0.0, 0.0, 0.0 };
		colors[ImGuiCol_FrameBg] = ImVec4{ 0.2, 0.205, 0.21, 1.0 };
		colors[ImGuiCol_FrameBgHovered] = ImVec4{ 0.3, 0.305, 0.31, 1.0 };
		colors[ImGuiCol_FrameBgActive] = ImVec4{ 0.15, 0.1505, 0.151, 1.0 };
		colors[ImGuiCol_ScrollbarBg] = ImVec4{ 0.0, 0.0, 0.0, 0.0 };
		colors[ImGuiCol_ScrollbarGrab] = ImVec4{ 0.31, 0.31, 0.31, 1.0 };
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4{ 0.41, 0.41, 0.41, 1.0 };
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4{ 0.51, 0.51, 0.51, 1.0 };
		colors[ImGuiCol_ScrollbarDots] = ImVec4{ 0.1, 0.105, 0.11, 1.0 };
		colors[ImGuiCol_ProgressBarBg] = ImVec4{ 0.5, 0.5, 0.5, 1.0 };
		colors[ImGuiCol_ProgressBarBorder] = ImVec4{ 0.8, 0.8, 0.8, 1.0 };
		colors[ImGuiCol_ProgressBarFill] = ImVec4{ 0.68, 0.82, 0.84, 1.0 };
		colors[ImGuiCol_SliderGrab] = ImVec4{ 0.24, 0.52, 0.88, 1.0 };
		colors[ImGuiCol_SliderGrabActive] = ImVec4{ 0.26, 0.59, 0.98, 1.0 };
		colors[ImGuiCol_Separator] = ImVec4{ 0.43, 0.43, 0.5, 0.5 };
		colors[ImGuiCol_SeparatorHovered] = ImVec4{ 0.1, 0.4, 0.75, 0.78 };
		colors[ImGuiCol_SeparatorActive] = ImVec4{ 0.1, 0.4, 0.75, 1.0 };
		colors[ImGuiCol_ResizeGrip] = ImVec4{ 0.26, 0.59, 0.98, 0.25 };
		colors[ImGuiCol_ResizeGripHovered] = ImVec4{ 0.26, 0.59, 0.98, 0.67 };
		colors[ImGuiCol_ResizeGripActive] = ImVec4{ 0.26, 0.59, 0.98, 0.95 };
		colors[ImGuiCol_CheckMark] = ImVec4{ 1.0, 1.0, 1.0, 1.0 };
		colors[ImGuiCol_Checkbox] = ImVec4{ 1.0, 1.0, 1.0, 1.0 };
		colors[ImGuiCol_CheckboxHovered] = ImVec4{ 1.0, 1.0, 1.0, 1.0 };
		colors[ImGuiCol_CheckboxActive] = ImVec4{ 1.0, 1.0, 1.0, 1.0 };
		colors[ImGuiCol_CheckboxTicked] = ImVec4{ 1.0, 1.0, 1.0, 1.0 };
		colors[ImGuiCol_CheckboxHoveredTicked] = ImVec4{ 1.0, 1.0, 1.0, 1.0 };
		colors[ImGuiCol_DockingPreview] = ImVec4{ 0.15, 0.1505, 0.151, 0.7 };
		colors[ImGuiCol_DockingEmptyBg] = ImVec4{ 0.2, 0.2, 0.2, 1.0 };
		colors[ImGuiCol_PlotLines] = ImVec4{ 0.61, 0.61, 0.61, 1.0 };
		colors[ImGuiCol_PlotLinesHovered] = ImVec4{ 1.0, 0.43, 0.35, 1.0 };
		colors[ImGuiCol_PlotHistogram] = ImVec4{ 0.9, 0.7, 0.0, 1.0 };
		colors[ImGuiCol_PlotHistogramHovered] = ImVec4{ 1.0, 0.6, 0.0, 1.0 };
		colors[ImGuiCol_DragDropTarget] = ImVec4{ 1.0, 1.0, 0.0, 0.9 };
		colors[ImGuiCol_NavHighlight] = ImVec4{ 0.26, 0.59, 0.98, 1.0 };
		colors[ImGuiCol_NavWindowingHighlight] = ImVec4{ 1.0, 1.0, 1.0, 0.7 };
		colors[ImGuiCol_NavWindowingDimBg] = ImVec4{ 0.8, 0.8, 0.8, 0.2 };

		Application::Get().GetWindow().ShowWindow();
	}

	void CrashManagerLayer::OnDetach()
	{
	}

	void CrashManagerLayer::OnUpdate(Timestep ts)
	{
	}

	void CrashManagerLayer::OnImGuiRender()
	{
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->Pos);
		ImGui::SetNextWindowSize(viewport->Size);
		ImGui::SetNextWindowViewport(viewport->ID);
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

		const float windowConstraintWidth = 850.0f;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::Begin("Crash Manager Window", NULL, window_flags);
		ImGui::PopStyleVar(2);

		ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4());

		auto& fonts = ImGui::GetIO().Fonts->Fonts;
		const float windowWidth = ImGui::GetWindowWidth();
		const auto windowPos = ImGui::GetWindowPos();
		const auto windowSize = ImGui::GetWindowSize();
		const auto drawList = ImGui::GetWindowDrawList();
		const float columnWidth = (windowWidth - windowConstraintWidth) * 0.5f;
		static const ImVec4 ProgressBarColorA = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
		static const ImVec4 ProgressBarColorB = ImVec4(0.97f, 0.97f, 0.97f, 1.0f);

		if (windowWidth > windowConstraintWidth)
		{
			ImGui::BeginColumns("##MainWindowColumns", 3, ImGuiColumnsFlags_NoPreserveWidths | ImGuiColumnsFlags_NoResize);
			ImGui::SetColumnWidth(0, columnWidth);
			ImGui::SetColumnWidth(1, windowConstraintWidth);

			ImGui::NextColumn();
		}

		ImGui::Dummy({0.0f, 10.0f});

		{
			float cPos = ImGui::GetCursorScreenPos().y;
			drawList->AddRectFilled(ImVec2(windowPos.x, cPos), ImVec2(windowPos.x + windowSize.x, cPos + 10.0f), ImGui::ColorConvertFloat4ToU32(ProgressBarColorA));
			unsigned int total = 0;
			float space = 0.0f;
			while (space < windowSize.x)
			{
				float value = fmod(10.0f * ImGui::GetTime(), 10.0f);
				ImVec2 points[4] = { ImVec2(windowPos.x + std::min(total * 10.0f + value, windowSize.x), cPos), ImVec2(windowPos.x + std::min(total * 10.0f + 5.0f + value, windowSize.x), cPos), ImVec2(windowPos.x + std::min(total * 10.0f + value, windowSize.x), cPos + 10.0f), ImVec2(windowPos.x + std::min(total * 10.0f - 5.0f + value, windowSize.x), cPos + 10.0f) };
				drawList->AddConvexPolyFilled(points, 4, ImGui::ColorConvertFloat4ToU32(ProgressBarColorB));

				space += 10.0f;
				total++;
			}
		}

		ImGui::Dummy({ 0.0f, 75.0f });

		const float radius = 120.0f;
		const float imageWidth = 90.0f;
		auto imageAndCirclePos = windowPos + ImGui::GetWindowContentRegionMin() + ImVec2(windowWidth * 0.5f, ImGui::GetCursorPos().y + radius * 0.5f);
		ImGui::Dummy({ 0.0f, radius * 1.5f });
		drawList->AddImage(ImTextureID(m_DymaticLogo->GetRendererID()), ImVec2(imageAndCirclePos.x - imageWidth, imageAndCirclePos.y - imageWidth), ImVec2(imageAndCirclePos.x + imageWidth, imageAndCirclePos.y + imageWidth), ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
		const float circleRadius = radius * (((std::sin(ImGui::GetTime() * 5.0f) + 1.0f) / 2.0f) * 0.25f + 0.8f);
		drawList->AddCircle(imageAndCirclePos, circleRadius, IM_COL32_WHITE, (int)circleRadius - 1, 3.0f);

		ImGui::Dummy({ 0.0f, 25.0f });

		ImGui::PushFont(fonts[2]);
		auto text = "DYMATIC CRASH MANAGER";
		auto textWidth = ImGui::CalcTextSize(text).x;
		ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
		ImGui::Text(text);
		ImGui::PopFont();

		ImGui::Dummy({ 0.0f, 25.0f });

		ImGui::PushFont(fonts[0]);
		ImGui::Text("A Dymatic Core process has crashed");
		ImGui::PopFont();
		ImGui::TextWrapped("Unfortunately a critical error occurred, and the Dymatic process was terminated. Providing log and profiling information can help to improve or fix any issues that Dymatic Engine has. All data is sent anonymously.");
		
		ImGui::Dummy({ 0.0f, 10.0f });
		
		if (logData != nullptr)
		{
			ImGui::PushFont(fonts[0]);
			ImGui::Text("The following log data will be included:");
			ImGui::PopFont();
			ImGui::SetNextItemWidth(-1);
			ImGui::InputTextMultiline("##LogMessage", logData, LogSize, {}, ImGuiInputTextFlags_ReadOnly);
		}
		else
			ImGui::Text("Log data associated with the application could not be found.");

		ImGui::Checkbox("##IncludeLogFiles", &m_SubmitLogInfo);
		ImGui::SameLine();
		ImGui::TextWrapped("Include log files with data submission. This data may contain personal information such as system or user names.");

		ImGui::Dummy({ 0.0f, 10.0f });

		ImGui::PushFont(fonts[0]);
		ImGui::Text("The following profiling files were found:");
		ImGui::PopFont();
		static char filesFound[85] = "DymaticProfile-Startup.json\nDymaticProfile-Runtime.json\nDymaticProfile-Shutdown.json";
		ImGui::SetNextItemWidth(-1);
		ImGui::InputTextMultiline("##ProfilingFiles", filesFound, 85);

		ImGui::Checkbox("##IncludeProfilingFiles", &m_SubmitProfileInfo);
		ImGui::SameLine();
		ImGui::TextWrapped("Include profiling files with data submission. These files provide information on internal timings and what code the program was executing.");

		ImGui::Dummy({ 0.0f, 50.0f });

		if (ImGui::Button("Close")) Application::Get().Close();
		ImGui::SameLine();
		ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x - 250.0f, 0.0f));
		ImGui::SameLine();
		if (ImGui::Button("Send and Close")) Application::Get().Close();
		ImGui::SameLine();
		if (ImGui::Button("Send and Restart"))
		{
			wchar_t* wString = new wchar_t[4096];
			MultiByteToWideChar(CP_ACP, 0, m_ExecutablePath, -1, wString, MAX_PATH);
			OpenApplication(wString);
			delete[] wString;
			Application::Get().Close();
		}

		ImGui::Dummy({0.0f, 10.0f});

		{
			float cPos = ImGui::GetCursorScreenPos().y;
			drawList->AddRectFilled(ImVec2(windowPos.x, cPos), ImVec2(windowPos.x + windowSize.x, cPos + 10.0f), ImGui::ColorConvertFloat4ToU32(ProgressBarColorA));
			unsigned int total = 0;
			float space = 0.0f;
			while (space < windowSize.x)
			{
				float value = fmod(10.0f * ImGui::GetTime(), 10.0f);
				ImVec2 points[4] = { ImVec2(windowPos.x + std::min(total * 10.0f + value, windowSize.x), cPos), ImVec2(windowPos.x + std::min(total * 10.0f + 5.0f + value, windowSize.x), cPos), ImVec2(windowPos.x + std::min(total * 10.0f + value, windowSize.x), cPos + 10.0f), ImVec2(windowPos.x + std::min(total * 10.0f - 5.0f + value, windowSize.x), cPos + 10.0f) };
				drawList->AddConvexPolyFilled(points, 4, ImGui::ColorConvertFloat4ToU32(ProgressBarColorB));

				space += 10.0f;
				total++;
			}
		}

		ImGui::Dummy({0.0f, 10.0f});

		if (windowWidth > windowConstraintWidth)
		{
			ImGui::EndColumns();
			const float columnPadding = 10.0f;

			drawList->AddRectFilled(windowPos, ImVec2(windowPos.x + columnWidth - columnPadding, windowPos.y + windowSize.y), IM_COL32(75, 75, 75, 255));
			drawList->AddRectFilled(ImVec2(windowPos.x + windowSize.x - columnWidth + columnPadding, windowPos.y), windowPos + windowSize, IM_COL32(75, 75, 75, 255));

			// Shadows
			const float shadowWidth = 75.0f;

			drawList->AddRectFilledMultiColor(ImVec2(windowPos.x + columnWidth - shadowWidth, windowPos.y), ImVec2(windowPos.x + columnWidth - columnPadding, windowPos.y + windowSize.y), IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 200), IM_COL32(0, 0, 0, 200), IM_COL32(0, 0, 0, 0));
			drawList->AddRectFilledMultiColor(ImVec2(windowPos.x + windowSize.x - columnWidth + shadowWidth, windowPos.y), ImVec2(windowPos.x + windowSize.x - columnWidth + columnPadding, windowPos.y + windowSize.y), IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 200), IM_COL32(0, 0, 0, 200), IM_COL32(0, 0, 0, 0));
		}
		ImGui::PopStyleColor();

		{
			const char* text = "Dymatic Crash Manager " DY_VERSION;
			ImGui::GetForegroundDrawList()->AddText(ImGui::GetWindowPos() + ImGui::GetWindowSize() - ImGui::CalcTextSize(text) - ImGui::GetStyle().WindowPadding, ImGui::GetColorU32(ImGuiCol_TextDisabled), text);
		}
		ImGui::End();
	}

	void CrashManagerLayer::OnEvent(Event& e)
	{
	}

	class CrashManager : public Application
	{
	public:
		CrashManager(const ApplicationSpecification& spec)
			: Application(spec)
		{
			PushLayer(new CrashManagerLayer(spec.CommandLineArgs[1]));
		}

		~CrashManager()
		{
		}
	};

	Application* CreateApplication(ApplicationCommandLineArgs args)
	{
		ApplicationSpecification spec;
		spec.Name = "Dymatic Crash Manager";
		spec.CommandLineArgs = args;
		spec.ConsoleVisible = false;
		spec.WindowStartHidden = true;
		spec.ApplicationIcon = "Resources/Icons/Branding/DymaticLogoBorderSmall.png";

		return new CrashManager(spec);
	}

}