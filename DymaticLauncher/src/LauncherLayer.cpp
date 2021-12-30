#include "LauncherLayer.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

#include "Dymatic/Utils/PlatformUtils.h"
#include "Dymatic/Math/Math.h"
#include "stb_image/stb_image.h"

#include "TextSymbols.h"

#include <iostream>

//---- INTERNET CHECK ----//
#include <wininet.h>
#pragma comment(lib,"Wininet.lib")
//------------------------//

namespace Dymatic {

	LauncherLayer::LauncherLayer()
		: Layer("LauncherLayer")
	{
		Log::ShowConsole();
		//Log::HideConsole();

		m_Online = InternetCheckConnection(L"https://www.google.com/", FLAG_ICC_FORCE_CONNECTION, 0);

		if (m_Online)
		{
			//SystemCommand("rmdir / s / q  \"info\"");
			//SystemCommand("git clone --recursive https://github.com/BenC25/DymaticDownloadSource.git info");
		}

		UpdateLauncherInformation();
		CheckUpdate();

		auto& colors = ImGui::GetStyle().Colors;
		
		colors[ImGuiCol_Text] = ImVec4 {1.0, 1.0, 1.0, 1.0 };

		colors[ImGuiCol_TextDisabled] = ImVec4 {0.5, 0.5, 0.5, 1.0 };

		colors[ImGuiCol_TextSelectedBg] = ImVec4 {0.26, 0.59, 0.98, 0.35 };

		colors[ImGuiCol_MenuBarBg] = ImVec4 {0.14, 0.14, 0.14, 1.0 };

		colors[ImGuiCol_MenuBarGrip] = ImVec4 {0.15, 0.15, 0.15, 1.0 };

		colors[ImGuiCol_MenuBarGripBorder] = ImVec4 {0.35, 0.35, 0.35, 1.0 };

		colors[ImGuiCol_WindowBg] = ImVec4 {0.1, 0.105, 0.11, 1.0 };

		colors[ImGuiCol_Header] = ImVec4 {0.2, 0.205, 0.21, 1.0 };

		colors[ImGuiCol_HeaderHovered] = ImVec4 {0.3, 0.305, 0.31, 1.0 };

		colors[ImGuiCol_HeaderActive] = ImVec4 {0.15, 0.1505, 0.151, 1.0 };

		colors[ImGuiCol_Tab] = ImVec4 {0.15, 0.1505, 0.151, 1.0 };

		colors[ImGuiCol_TabHovered] = ImVec4 {0.38, 0.3805, 0.381, 1.0 };

		colors[ImGuiCol_TabActive] = ImVec4 {0.28, 0.2805, 0.281, 1.0 };

		colors[ImGuiCol_TabUnfocused] = ImVec4 {0.15, 0.1505, 0.151, 1.0 };

		colors[ImGuiCol_TabUnfocusedActive] = ImVec4 {0.2, 0.205, 0.21, 1.0 };

		colors[ImGuiCol_TitleBg] = ImVec4 {0.15, 0.1505, 0.151, 1.0 };

		colors[ImGuiCol_TitleBgActive] = ImVec4 {0.15, 0.1505, 0.151, 1.0 };

		colors[ImGuiCol_TitleBgCollapsed] = ImVec4 {0.15, 0.1505, 0.151, 1.0 };

		colors[ImGuiCol_Button] = ImVec4 {0.298147, 0.303514, 0.30888, 1.0 };

		colors[ImGuiCol_ButtonHovered] = ImVec4 {0.423529, 0.576471, 0.807843, 1.0 };

		colors[ImGuiCol_ButtonActive] = ImVec4 {0.32549, 0.47451, 0.705882, 1.0 };

		colors[ImGuiCol_ButtonToggled] = ImVec4 {0.325, 0.475, 0.706, 1.0 };

		colors[ImGuiCol_ButtonToggledHovered] = ImVec4 {0.425, 0.575, 0.806, 1.0 };

		colors[ImGuiCol_PopupBg] = ImVec4 {0.08, 0.08, 0.08, 0.94 };

		colors[ImGuiCol_ModalWindowDimBg] = ImVec4 {0.8, 0.8, 0.8, 0.35 };

		colors[ImGuiCol_Border] = ImVec4 {0.43, 0.43, 0.5, 0.5 };

		colors[ImGuiCol_BorderShadow] = ImVec4 {0.0, 0.0, 0.0, 0.0 };

		colors[ImGuiCol_FrameBg] = ImVec4 {0.2, 0.205, 0.21, 1.0 };

		colors[ImGuiCol_FrameBgHovered] = ImVec4 {0.3, 0.305, 0.31, 1.0 };

		colors[ImGuiCol_FrameBgActive] = ImVec4 {0.15, 0.1505, 0.151, 1.0 };

		colors[ImGuiCol_ScrollbarBg] = ImVec4 {0.0, 0.0, 0.0, 0.0 };

		colors[ImGuiCol_ScrollbarGrab] = ImVec4 {0.31, 0.31, 0.31, 1.0 };

		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4 {0.41, 0.41, 0.41, 1.0 };

		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4 {0.51, 0.51, 0.51, 1.0 };

		colors[ImGuiCol_ScrollbarDots] = ImVec4 {0.1, 0.105, 0.11, 1.0 };

		colors[ImGuiCol_ProgressBarBg] = ImVec4 {0.5, 0.5, 0.5, 1.0 };

		colors[ImGuiCol_ProgressBarBorder] = ImVec4 {0.8, 0.8, 0.8, 1.0 };

		colors[ImGuiCol_ProgressBarFill] = ImVec4 {0.68, 0.82, 0.84, 1.0 };

		colors[ImGuiCol_SliderGrab] = ImVec4 {0.24, 0.52, 0.88, 1.0 };

		colors[ImGuiCol_SliderGrabActive] = ImVec4 {0.26, 0.59, 0.98, 1.0 };

		colors[ImGuiCol_Separator] = ImVec4 {0.43, 0.43, 0.5, 0.5 };

		colors[ImGuiCol_SeparatorHovered] = ImVec4 {0.1, 0.4, 0.75, 0.78 };

		colors[ImGuiCol_SeparatorActive] = ImVec4 {0.1, 0.4, 0.75, 1.0 };

		colors[ImGuiCol_ResizeGrip] = ImVec4 {0.26, 0.59, 0.98, 0.25 };

		colors[ImGuiCol_ResizeGripHovered] = ImVec4 {0.26, 0.59, 0.98, 0.67 };

		colors[ImGuiCol_ResizeGripActive] = ImVec4 {0.26, 0.59, 0.98, 0.95 };

		//colors[ImGuiCol_FileBg] = ImVec4 {0.48, 0.48, 0.48, 1.0 };

		//colors[ImGuiCol_FileIcon] = ImVec4 {0.48, 0.48, 0.48, 1.0 };
		//
		//colors[ImGuiCol_FileHovered] = ImVec4 {0.34902, 0.34902, 0.34902, 0.294118 };
		//
		//colors[ImGuiCol_FileSelected] = ImVec4 {0.682353, 0.682353, 0.682353, 0.517647 };

		colors[ImGuiCol_CheckMark] = ImVec4 {1.0, 1.0, 1.0, 1.0 };

		//colors[ImGuiCol_Checkbox] = ImVec4 {0.4, 0.4, 0.4, 1.0 };
		//
		//colors[ImGuiCol_CheckboxHovered] = ImVec4 {0.48, 0.48, 0.48, 1.0 };
		//
		//colors[ImGuiCol_CheckboxActive] = ImVec4 {0.4, 0.4, 0.4, 1.0 };
		//
		//colors[ImGuiCol_CheckboxTicked] = ImVec4 {0.325, 0.475, 0.706, 1.0 };
		//
		//colors[ImGuiCol_CheckBoxHoveredTicked] = ImVec4 {0.425, 0.575, 0.806, 1.0 };

		colors[ImGuiCol_DockingPreview] = ImVec4 {0.15, 0.1505, 0.151, 0.7 };

		colors[ImGuiCol_DockingEmptyBg] = ImVec4 {0.2, 0.2, 0.2, 1.0 };

		colors[ImGuiCol_PlotLines] = ImVec4{ 0.61, 0.61, 0.61, 1.0 };

		colors[ImGuiCol_PlotLinesHovered] = ImVec4{ 1.0, 0.43, 0.35, 1.0 };

		colors[ImGuiCol_PlotHistogram] = ImVec4{ 0.9, 0.7, 0.0, 1.0 };

		colors[ImGuiCol_PlotHistogramHovered] = ImVec4{ 1.0, 0.6, 0.0, 1.0 };

		colors[ImGuiCol_DragDropTarget] = ImVec4 {1.0, 1.0, 0.0, 0.9 };

		colors[ImGuiCol_NavHighlight] = ImVec4 {0.26, 0.59, 0.98, 1.0 };

		colors[ImGuiCol_NavWindowingHighlight] = ImVec4 {1.0, 1.0, 1.0, 0.7 };

		colors[ImGuiCol_NavWindowingDimBg] = ImVec4 {0.8, 0.8, 0.8, 0.2 };

		// create own program
		//std::ofstream f("tmp.cpp");
		//f << "#include<stdlib.h>\n #include<Dymatic.h>\nextern \"C\" void myfunc() { DY_CORE_INFO(\"Core Runtime Intergration Success!\"); }\n";
		//f.close();
		//
		//// create library
		//system("/usr/bin/gcc -shared tmp.cpp -o libtmp.so");

		//Load the DLL
		//HMODULE lib = LoadLibrary(L"DymaticLauncher.dll");
		//
		////Create the function
		//typedef void (*FNPTR)();
		//FNPTR myfunc = (FNPTR)GetProcAddress(lib, "myfunc");
		//
		////EDIT: For additional safety, check to see if it loaded
		//if (!myfunc) 
		//{
		//	//ERROR
		//	FreeLibrary(lib);
		//}
		//
		////Call it!
		//myfunc();

		//HINSTANCE hDLL;               // Handle to DLL
		//LPFNDLLFUNC1 lpfnDllFunc1;    // Function pointer
		//
		//hDLL = LoadLibrary(L"DymaticLauncher.dll");
		//if (hDLL != NULL)
		//{
		//	lpfnDllFunc1 = (LPFNDLLFUNC1)GetProcAddress(hDLL,
		//		"ImGuiRuntime");
		//	if (!lpfnDllFunc1)
		//	{
		//		// handle the error
		//		FreeLibrary(hDLL);
		//		DY_CORE_ASSERT(false, "Failed To Load ImGui Runtime DLL");
		//	}
		//	else
		//	{
		//		m_ImGuiRuntimeExecution = lpfnDllFunc1;
		//	}
		//}

		
	}

	void LauncherLayer::OnAttach()
	{
		DY_PROFILE_FUNCTION();

		Application::Get().GetWindow().MaximizeWindow();

		Application::Get().GetImGuiLayer()->AddIconFont("assets/fonts/IconsFont.ttf", 50.0f, 0x700, 0x705);
	}

	

	void LauncherLayer::OnDetach()
	{
		DY_PROFILE_FUNCTION();
	}

	void LauncherLayer::OnUpdate(Timestep ts)
	{
		DY_PROFILE_FUNCTION();

		m_DeltaTime = ts;
	}

	static bool lock_windows = false;

	static bool BeginWindow(const char* name, bool* p_open = (bool*)0, ImGuiWindowFlags flags = 0)
	{
		if (lock_windows)
		{
			ImGuiWindowClass window_class;
			window_class.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoTabBar | ImGuiDockNodeFlags_NoCloseButton | ImGuiDockNodeFlags_NoResize | ImGuiDockNodeFlags_NoSplit;
			ImGui::SetNextWindowClass(&window_class);
		}
		else
		{
			ImGuiWindowClass window_class;
			window_class.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_None;
			ImGui::SetNextWindowClass(&window_class);
		}
		return ImGui::Begin(name, p_open, lock_windows ? (flags | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar) : flags);
	}

	static void DrawNoInternetMessage()
	{
		auto drawList = ImGui::GetWindowDrawList();
		float offset = 0.0f;
		static float spacing = 10.0f;
		drawList->AddText(ImGui::GetWindowPos() + (ImGui::GetWindowSize() - ImGui::CalcTextSize(CHARACTER_LAUNCHER_ICON_NO_INTERNET)) / 2.0 + ImVec2(0, offset), ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_Text)), (std::string(CHARACTER_LAUNCHER_ICON_NO_INTERNET)).c_str());
		offset += ImGui::CalcTextSize(CHARACTER_LAUNCHER_ICON_NO_INTERNET).y + spacing;
		ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
		drawList->AddText(ImGui::GetWindowPos() + (ImGui::GetWindowSize() - ImGui::CalcTextSize("Uh oh, there was supposed to be something here.")) / 2.0 + ImVec2(0, offset), ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_Text)), (std::string("Uh oh, there was supposed to be something here.")).c_str());
		ImGui::PopFont();
		offset += ImGui::CalcTextSize("Uh oh, there was supposed to be something here.").y + spacing;
		drawList->AddText(ImGui::GetWindowPos() + (ImGui::GetWindowSize() - ImGui::CalcTextSize("Check your internet connection and try again")) / 2.0 + ImVec2(0, offset), ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_Text)), (std::string("Check your internet connection and try again")).c_str());
	}

	void LauncherLayer::OnImGuiRender()
	{
		DY_PROFILE_FUNCTION();

		if (Input::IsKeyPressed(Key::D1)) { lock_windows = false; }
		else if (Input::IsKeyPressed(Key::D2)) { lock_windows = true; }

		// Note: Switch this to true to enable dockspace
		static bool dockspaceOpen = true;
		static bool opt_fullscreen_persistant = true;
		bool opt_fullscreen = opt_fullscreen_persistant;
		static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

		bool maximized = Application::Get().GetWindow().IsWindowMaximized();

		// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
		// because it would be confusing to have two docking targets within each others.
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
		if (opt_fullscreen)
		{
			ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->Pos);
			ImGui::SetNextWindowSize(viewport->Size);
			ImGui::SetNextWindowViewport(viewport->ID);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, maximized ? 0.0f : 3.0f);
			window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
			window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		}

		// When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background and handle the pass-thru hole, so we ask Begin() to not render a background.
		if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
			window_flags |= ImGuiWindowFlags_NoBackground;

		// Important: note that we proceed even if Begin() returns false (aka window is collapsed).
		// This is because we want to keep our DockSpace() active. If a DockSpace() is inactive, 
		// all active windows docked into it will lose their parent and become undocked.
		// We cannot preserve the docking relationship between an active window and an inactive docking, otherwise 
		// any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(maximized ? 0.0f : 5.0f, maximized ? 0.0f : 5.0f));
		ImGui::Begin("Dymatic Launcher Dockspace Window", &dockspaceOpen, window_flags);
		ImVec2 dockspaceWindowPosition = ImGui::GetWindowPos();
		ImGui::PopStyleVar();

		if (opt_fullscreen)
			ImGui::PopStyleVar(2);

		static bool windowResizeHeld = false;
		static bool windowMoveHeld = false;

		if (!Application::Get().GetWindow().IsWindowMaximized() && !windowMoveHeld )
		{
			auto& window = Application::Get().GetWindow();

			

			static int selectionIndex = -1;

			static float mouseDistance = 5.0f;

			if (!windowResizeHeld)
			{
				if (Math::NearlyEqual(ImGui::GetMousePos().x, window.GetPositionX(), mouseDistance) && Math::NearlyEqual(ImGui::GetMousePos().y, window.GetPositionY(), mouseDistance)) { selectionIndex = 4; }
				else if (Math::NearlyEqual(ImGui::GetMousePos().x, window.GetPositionX() + window.GetWidth(), mouseDistance) && Math::NearlyEqual(ImGui::GetMousePos().y, window.GetPositionY(), mouseDistance)) { selectionIndex = 5; }
				else if (Math::NearlyEqual(ImGui::GetMousePos().x, window.GetPositionX() + window.GetWidth(), mouseDistance) && Math::NearlyEqual(ImGui::GetMousePos().y, window.GetPositionY() + window.GetHeight(), mouseDistance)) { selectionIndex = 6; }
				else if (Math::NearlyEqual(ImGui::GetMousePos().x, window.GetPositionX(), mouseDistance) && Math::NearlyEqual(ImGui::GetMousePos().y, window.GetPositionY() + window.GetHeight(), mouseDistance)) { selectionIndex = 7; }

				else if (Math::NearlyEqual(ImGui::GetMousePos().x, window.GetPositionX(), mouseDistance)) { selectionIndex = 0; }
				else if (Math::NearlyEqual(ImGui::GetMousePos().y, window.GetPositionY(), mouseDistance)) { selectionIndex = 1; }
				else if (Math::NearlyEqual(ImGui::GetMousePos().x, window.GetPositionX() + window.GetWidth(), mouseDistance)) { selectionIndex = 2; }
				else if (Math::NearlyEqual(ImGui::GetMousePos().y, window.GetPositionY() + window.GetHeight(), mouseDistance)) { selectionIndex = 3; }
			}


			if (selectionIndex != -1 || windowResizeHeld)
			{
				if (Input::IsMouseButtonPressed(Mouse::ButtonLeft))
				{
					windowResizeHeld = true;
				}
				switch (selectionIndex)
				{
				case 0: {window.SetCursor(4); break; }
				case 1: {window.SetCursor(5); break; }
				case 2: {window.SetCursor(4); break; }
				case 3: {window.SetCursor(5); break; }
				case 4: {window.SetCursor(6); break; }
				case 5: {window.SetCursor(7); break; }
				case 6: {window.SetCursor(6); break; }
				case 7: {window.SetCursor(7); break; }
				}
				
			}
			if (!Input::IsMouseButtonPressed(Mouse::ButtonLeft))
			{
				windowResizeHeld = false;
				selectionIndex = -1;
			} 

			static ImVec2 prevMousePos = ImGui::GetMousePos();
			static int prevWindowWidth = window.GetWidth();
			static int prevWindowHeight = window.GetHeight();
			static bool init = true;
			if (windowResizeHeld)
			{
				if (init)
				{
					prevMousePos = ImGui::GetMousePos();
					prevWindowWidth = window.GetWidth();
					prevWindowHeight = window.GetHeight();
					init = false;
				}

				int sizeX = window.GetWidth();
				int sizeY = window.GetHeight();

				switch (selectionIndex)
				{
				case 0: {sizeX = window.GetWidth() - (ImGui::GetMousePos().x - prevMousePos.x); break; };
				case 1: {sizeY = window.GetHeight() - (ImGui::GetMousePos().y - prevMousePos.y); break; };
				case 2: {sizeX = window.GetWidth() + (ImGui::GetMousePos().x - prevMousePos.x); break; };
				case 3: {sizeY = window.GetHeight() + (ImGui::GetMousePos().y - prevMousePos.y); break; };
				case 4: {sizeX = window.GetWidth() - (ImGui::GetMousePos().x - prevMousePos.x); sizeY = window.GetHeight() - (ImGui::GetMousePos().y - prevMousePos.y); break; };
				case 5: {sizeX = window.GetWidth() + (ImGui::GetMousePos().x - prevMousePos.x); sizeY = window.GetHeight() - (ImGui::GetMousePos().y - prevMousePos.y); break; };
				case 6: {sizeX = window.GetWidth() + (ImGui::GetMousePos().x - prevMousePos.x); sizeY = window.GetHeight() + (ImGui::GetMousePos().y - prevMousePos.y); break; };
				case 7: {sizeX = window.GetWidth() - (ImGui::GetMousePos().x - prevMousePos.x); sizeY = window.GetHeight() + (ImGui::GetMousePos().y - prevMousePos.y); break; };
				}
				

				sizeX = sizeX < 250 ? 250 : sizeX;
				sizeY = sizeY < 250 ? 250 : sizeY;

				window.SetSize(sizeX, sizeY);
				switch (selectionIndex)
				{
				case 0: {window.SetPosition(window.GetPositionX() + (prevWindowWidth - sizeX), window.GetPositionY()); break; }
				case 1: {window.SetPosition(window.GetPositionX(), window.GetPositionY() + (prevWindowHeight - sizeY)); break; }
				case 2: {break; }
				case 3: {break; }
				case 4: {window.SetPosition(window.GetPositionX() + (prevWindowWidth - sizeX), window.GetPositionY() + (prevWindowHeight - sizeY)); break; }
				case 5: {window.SetPosition(window.GetPositionX(), window.GetPositionY() + (prevWindowHeight - sizeY)); break; }
				case 6: {break; }
				case 7: {window.SetPosition(window.GetPositionX() + (prevWindowWidth - sizeX), window.GetPositionY()); break; }
				}
				

				prevMousePos = ImGui::GetMousePos();
				prevWindowWidth = window.GetWidth();
				prevWindowHeight = window.GetHeight();
			}
			else
			{
				init = true;
			}
		}

		// DockSpace
		ImGuiIO& io = ImGui::GetIO();
		ImGuiStyle& style = ImGui::GetStyle();
		float minWinSizeX = style.WindowMinSize.x;
		style.WindowMinSize.x = 370.0f;
		if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
		{
			ImGuiID dockspace_id = ImGui::GetID("Dymatic Launcher DockSpace");
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
		}

		style.WindowMinSize.x = minWinSizeX;

		if (ImGui::BeginMenuBar())
		{

			auto& window = Application::Get().GetWindow();

			ImVec2 pos = ImVec2{ ImGui::GetWindowPos().x + ImGui::GetWindowSize().x / 2, ImGui::GetWindowPos().y + 10.0f };
			ImVec2 points[4] = { {pos.x + ImGui::GetWindowSize().x * 0.3f, pos.y - 50.0f}, {pos.x + ImGui::GetWindowSize().x * 0.25f, pos.y + 5.0f}, {pos.x - ImGui::GetWindowSize().x * 0.25f, pos.y + 5.0f}, {pos.x - ImGui::GetWindowSize().x * 0.3f, pos.y - 50.0f} };

			if (window.GetWidth() > 700)
			{
				ImGui::GetWindowDrawList()->AddConvexPolyFilled(points, sizeof(points), ImGui::GetColorU32(ImGuiCol_MenuBarGrip));
				ImGui::GetWindowDrawList()->AddPolyline(points, sizeof(points), ImGui::GetColorU32(ImGuiCol_MenuBarGripBorder), true, 2.0f);
			}

			ImVec4 WindowOperatorButtonCol = ImVec4{1.0f, 1.0f, 1.0f, 1.0f};
			ImVec4 WindowOperatorCircleCol = ImVec4{ 0.5f, 0.5f, 0.5f, 0.0f };
			ImVec4 WindowOperatorCircleColHovered = ImVec4{ 0.5f, 0.5f, 0.5f, 1.0f };
			float lineThickness = 1.0f;
			float buttonSize = 5.0f;

			for (int i = 0; i < 3; i++)
			{
				ImVec2 pos = ImVec2{ ImGui::GetWindowPos().x + ImGui::GetWindowSize().x - ((i) * ((buttonSize + 12.5f) * 2.0f)) - buttonSize * 2.5f, (ImGui::GetWindowPos().y + 10.0f) };

				const ImGuiID id = ImGui::GetCurrentWindow()->GetID(("##WindowOperatorButton" + std::to_string(i)).c_str());
				const ImRect bb(ImVec2{pos.x - buttonSize - 5.0f, pos.y - buttonSize - 5.0f }, ImVec2{pos.x + buttonSize + 5.0f, pos.y + buttonSize + 5.0f });
				bool hovered, held;
				bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);
				ImU32 col = ImGui::ColorConvertFloat4ToU32(WindowOperatorButtonCol);
				ImGui::GetWindowDrawList()->AddCircleFilled(pos, buttonSize + 4.0f, ImGui::ColorConvertFloat4ToU32(hovered ? WindowOperatorCircleColHovered : WindowOperatorCircleCol));
				switch (i)
				{
				case 0: {ImGui::GetWindowDrawList()->AddLine(ImVec2{ pos.x - buttonSize, pos.y + buttonSize }, ImVec2{ pos.x + buttonSize, pos.y - buttonSize }, col, lineThickness);
					ImGui::GetWindowDrawList()->AddLine(ImVec2{ pos.x - buttonSize, pos.y - buttonSize }, ImVec2{ pos.x + buttonSize, pos.y + buttonSize }, col, lineThickness); break; }
				case 1: {ImGui::GetWindowDrawList()->AddRect(ImVec2{ pos.x - buttonSize, pos.y - buttonSize }, ImVec2{ pos.x + buttonSize, pos.y + buttonSize }, col, NULL, NULL, lineThickness); break; }
				case 2: {ImGui::GetWindowDrawList()->AddLine(ImVec2{pos.x - buttonSize, pos.y}, ImVec2{ pos.x + buttonSize, pos.y }, col, lineThickness); break; }
				}
				if (pressed)
				{
					switch (i)
					{
					case 0: {CloseProgramWindow(); break; }
					case 1: {window.IsWindowMaximized() ? window.ReturnWindow() : window.MaximizeWindow(); break; }
					case 2: {window.MinimizeWindow(); break; }
					}
				}

			}

			//Window Move

			const ImGuiID id = ImGui::GetCurrentWindow()->GetID(("##WindowMoveTab"));
			const ImRect bb(ImGui::GetWindowPos(), ImVec2{ ImGui::GetWindowPos().x + ImGui::GetWindowSize().x, ImGui::GetWindowPos().y + 30 });
			bool hovered;
			bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &windowMoveHeld);

			static ImVec2 offset = {};
			static bool init = true;
			static ImVec2 previousMousePos = {};
			if (windowMoveHeld && !windowResizeHeld)
			{
				if (init)
				{
					offset = ImVec2{ ImGui::GetMousePos().x - ImGui::GetWindowPos().x,  ImGui::GetMousePos().y - ImGui::GetWindowPos().y };
					init = false;
					previousMousePos = ImGui::GetMousePos();
				}
				if (previousMousePos.x != ImGui::GetMousePos().x || previousMousePos.y != ImGui::GetMousePos().y)
				{
					if (window.IsWindowMaximized()) { window.ReturnWindow(); }
				}
				previousMousePos = ImGui::GetMousePos();

				window.SetPosition(ImGui::GetMousePos().x - offset.x, ImGui::GetMousePos().y - offset.y);
				//window.SetPosition(window.GetPositionX() + (ImGui::GetMousePos().x - previousMousePos.x), window.GetPositionY() + (ImGui::GetMousePos().y - previousMousePos.y));
			}
			else
			{
				init = true;
			}

			ImGui::EndMenuBar();
		}

		BeginWindow("Navigation SideBar");

		const char* itemsMain[4] = { CHARACTER_LAUNCHER_ICON_HOME, CHARACTER_LAUNCHER_ICON_ENGINE, CHARACTER_LAUNCHER_ICON_STORE, CHARACTER_LAUNCHER_ICON_LIBRARY };
		int currentPageMain = m_CurrentPage;
		if (ImGui::ButtonStackEx("MainPageButtonStack", itemsMain, 4, &currentPageMain, ImVec2(ImGui::GetContentRegionAvail().x, 400.0f), 5.0f))
		{
			m_CurrentPage = (LauncherPage)currentPageMain;
		}

		ImGui::Dummy(ImGui::GetContentRegionAvail() - ImVec2(0, 110));

		const char* itemsUser[1] = { CHARACTER_LAUNCHER_ICON_SETTINGS };
		int currentPageUser = m_CurrentPage - 4;
		if (ImGui::ButtonStackEx("UserPageButtonStack", itemsUser, 1, &currentPageUser, ImVec2(ImGui::GetContentRegionAvail().x, 100.0f), 5.0f))
		{
			m_CurrentPage = (LauncherPage)(currentPageUser + 4);
		}

		ImGui::End();

		BeginWindow("Main Area", NULL, dockspace_flags);
		ImGuiID dockspace_id_area = ImGui::GetID("Area Docking");
		ImGui::DockSpace(dockspace_id_area, ImVec2(0.0f, 0.0f), dockspace_flags);

		if (m_CurrentPage == Home)
		{
			BeginWindow("Home");

			auto drawList = ImGui::GetWindowDrawList();

			if (m_Online)
			{
			}
			else
			{
				DrawNoInternetMessage();
			}
			ImGui::End();
		}

		else if (m_CurrentPage == Engine)
		{
			BeginWindow("Engine");
			if (ImGui::Button("Download"))
			{
				CheckUpdate();
			}
			ImGui::End();
		}

		else if (m_CurrentPage == Store)
		{
			BeginWindow("Store");
			ImGui::End();
		}

		else if (m_CurrentPage == Library)
		{
			BeginWindow("Library");
			ImGui::End();
		}

		static ImVec4 colorA = ImVec4(0.0f, 0.47f, 0.95f, 1.0f); //ImVec4(0.87f, 0.64f, 0.035f, 1.0f);
		static ImVec4 colorB = ImVec4(0.38f, 0.67f, 0.97f, 1.0f);//ImVec4(0.92f, 0.78f, 0.4f, 1.0f);
		ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetWindowPos() + ImVec2(0, ImGui::GetContentRegionMax().y), ImGui::GetWindowPos() + ImGui::GetContentRegionMax() - ImVec2(0, 10.0f), ImGui::ColorConvertFloat4ToU32(colorA));
		int total = 0;
		float space = 0.0f;
		while (space < ImGui::GetContentRegionAvail().x + 20.0f)
		{
			float value = fmod(10.0f * ImGui::GetTime(), 10.0f);
			ImVec2 points[4] = { ImGui::GetWindowPos() + ImVec2(0, ImGui::GetContentRegionMax().y - 10.0f) + ImVec2(std::min(total * 10.0f + value, ImGui::GetContentRegionMax().x), 0), ImGui::GetWindowPos() + ImVec2(0, ImGui::GetContentRegionMax().y - 10.0f) + ImVec2(std::min(total * 10.0f + 5.0f + value, ImGui::GetContentRegionMax().x), 0), ImGui::GetWindowPos() + ImVec2(0, ImGui::GetContentRegionMax().y) + ImVec2(std::min(total * 10.0f + value, ImGui::GetContentRegionMax().x), 0.0f), ImGui::GetWindowPos() + ImVec2(0, ImGui::GetContentRegionMax().y) + ImVec2(std::min(total * 10.0f - 5.0f + value, ImGui::GetContentRegionMax().x), 0.0f) };
			ImGui::GetWindowDrawList()->AddConvexPolyFilled(points, 4, ImGui::ColorConvertFloat4ToU32(colorB));
			space+= 10.0f;
			total++;
		}

		ImGui::End();
		ImGui::End();
				
	}

	void LauncherLayer::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);

		dispatcher.Dispatch<KeyPressedEvent>(DY_BIND_EVENT_FN(LauncherLayer::OnKeyPressed));
		dispatcher.Dispatch<MouseButtonPressedEvent>(DY_BIND_EVENT_FN(LauncherLayer::OnMouseButtonPressed));
	}

	bool LauncherLayer::OnKeyPressed(KeyPressedEvent& e)
	{
		return false;
	}

	bool LauncherLayer::OnMouseButtonPressed(MouseButtonPressedEvent& e)
	{
		return false;
	}

	void LauncherLayer::CloseProgramWindow()
	{
		Application::Get().Close();
	}

	void LauncherLayer::CheckUpdate()
	{
		if (m_Online && m_LatestVersion > m_CurrentVersion)
		{
			system("curl -L \"https://docs.google.com/uc?export=download&id=16597IDRWoi-BnkJgbB3z3xPRqbo2SSWN\" > DymaticLauncherInstallationPackage.exe");
			system("curl -L \"https://docs.google.com/uc?export=download&id=1DCVdgSLYe4vOb0s3d0sqGp4rQith231w\" > DownloadSequencer.bat");
			CloseProgramWindow();
			system("start /min DownloadSequencer.bat");
		}
	}

	void LauncherLayer::UpdateLauncherInformation()
	{
		if (m_Online)
		{
			system("curl -L \"https://docs.google.com/uc?export=download&id=1nm41iVXhwYCoc9zotWBP75jqCrPekVFh\" > LauncherInfoPackage.dypackage");

			std::string result;
			std::ifstream in("LauncherInfoPackage.dypackage", std::ios::in | std::ios::binary); // ifstream closes itself due to RAII
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
				{
					DY_CORE_ASSERT(false);
				}
			}
			in.close();


			while (result.find_first_of("\n") != std::string::npos)
			{
				result = result.erase(result.find_first_of("\n"), 1);
			}

			while (result.find_first_of("\r") != std::string::npos)
			{
				result = result.erase(result.find_first_of("\r"), 1);
			}

			bool openValueName = false;
			bool openValue = false;
			std::string CurrentValueName = "";
			std::string CurrentValue = "";
			for (int i = 0; i < result.length(); i++)
			{
				std::string character = result.substr(i, 1);

				if (character == ">") { openValueName = false; }
				if (openValueName) { CurrentValueName = CurrentValueName + character; }
				if (character == "<") { openValueName = true; CurrentValueName = ""; }

				if (character == "}") { openValue = false; }
				if (openValue) { CurrentValue = CurrentValue + character; }
				if (character == "{") { openValue = true; CurrentValue = ""; }

				if (character == "}" && CurrentValue != "")
				{
					if (CurrentValueName == "LatestVersion") { m_LatestVersion = std::stof(CurrentValue); }
				}

			}

			system("del / Q LauncherInfoPackage.dypackage");
		}
	}

	int LauncherLayer::SystemCommand(std::string command)
	{
		return system((command).c_str());
	}

	std::string LauncherLayer::ReadFile(std::string path)
	{
		std::string total;

		std::string string;
		std::ifstream infile;
		infile.open(path);
		while (!infile.eof()) // To get you all the lines.
		{
			getline(infile, string); // Saves the line in STRING.

			total += string + "\n"; // Prints our STRING.
		}
		infile.close();

		return total;
	}
	
	void LauncherLayer::WriteFile(std::string path, std::string string)
	{
		std::ofstream myfile;
		myfile.open(path);

		myfile << string;

		myfile.close();
	}

	std::string LauncherLayer::ReadInEncryptedFile(std::string path)
	{
		std::string readFile = ReadFile(path);
		readFile.erase(readFile.length() - 1, 1);
		return DecryptString(readFile);
	}

	std::string LauncherLayer::EncryptString(std::string string)
	{
		static int authCode = 53987;

		for (int i = 0; i < string.length(); i++)
		{
			string[i] = string[i] + Hash(authCode + 1);
			string[i] = string[i] + Hash(authCode + i);
			//string[i] = string[i] + Hash(authCode * i);
		}

		return string;
	}

	std::string LauncherLayer::DecryptString(std::string string)
	{
		static int authCode = 53987;

		for (int i = 0; i < string.length(); i++)
		{
			string[i] = string[i] - Hash(authCode + 1);
			string[i] = string[i] - Hash(authCode + i);
			//string[i] = string[i] - Hash(authCode * i);
		}

		return string;
	}

	int LauncherLayer::Hash(int state)
	{
		state ^= 2747636419;
		state *= 2654435769;
		state ^= state >> 16;
		state *= 2654435769;
		state ^= state >> 16;
		state *= 2654435769;
		return state;
	}

}

