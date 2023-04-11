#include "EditorLayer.h"

#include "Dymatic/Asset/AssetManager.h"

#include "Dymatic/Scene/SceneSerializer.h"

#include "Dymatic/Scripting/ScriptEngine.h"
#include "Dymatic/Audio/AudioEngine.h"

#include "Dymatic/Utils/PlatformUtils.h"
#include "Dymatic/Math/Math.h"
#include "Dymatic/Math/StringUtils.h"

#include "Settings/Preferences.h"
#include "Settings/ProjectSettings.h"
#include "TextSymbols.h"
#include "Dymatic/UI/UI.h"

#include "Dymatic/Core/Memory.h"
#include "Tools/PluginLoader.h"

#include <shellapi.h>
#include <ctime>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include "ImGuizmo.h"
#include <imgui/imgui_stdlib.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_query.hpp>

#include <stb_image/stb_image_write.h>

namespace Dymatic {

	PROCESS_INFORMATION s_CrashManagerProcessInformation;

	static void LoadApplicationCrashManager()
	{
		STARTUPINFO si;

		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		ZeroMemory(&s_CrashManagerProcessInformation, sizeof(s_CrashManagerProcessInformation));

		WCHAR path[MAX_PATH];
		GetModuleFileName(GetModuleHandle(NULL), path, sizeof(path));

		TCHAR s[MAX_PATH + 10 + 12];
		swprintf_s(s, (L"%X %s %S"), GetCurrentProcessId(), path, "Dymatic.log");

		CreateProcess(L"../bin/Release-windows-x86_64/CrashManager/CrashManager.exe", s, NULL, NULL, FALSE, 0, NULL, NULL, &si, &s_CrashManagerProcessInformation);
	}

	EditorLayer::EditorLayer()
		: Layer("EditorLayer")
	{		
	}

	// Window Button Hover Queries
	static bool s_TitlebarHovered;
	static void IsTitlebarHovered(int* hovered) { *hovered = s_TitlebarHovered; }
	static bool s_MinimizeHovered;
	static void IsMinimizeHovered(int* hovered) { *hovered = s_MinimizeHovered; }
	static bool s_MaximiseHovered;
	static void IsMaximiseHovered(int* hovered) { *hovered = s_MaximiseHovered; }
	static bool s_CloseHovered;
	static void IsCloseHovered(int* hovered) { *hovered = s_CloseHovered; }

	void EditorLayer::OnAttach()
	{
		DY_PROFILE_FUNCTION();

		LoadApplicationCrashManager();

		auto& window = Application::Get().GetWindow();
		window.SetTitlebarHoveredQueryCallback(&IsTitlebarHovered);
		window.SetMinimizeHoveredQueryCallback(&IsMinimizeHovered);
		window.SetMaximizeHoveredQueryCallback(&IsMaximiseHovered);
		window.SetCloseHoveredQueryCallback(&IsCloseHovered);

		Preferences::Init();

		PythonTools::Init(this);

		PluginLoader::Init();

		Notification::Init();

		AudioEngine::SetGlobalVolume(Preferences::GetData().EditorVolume);
		
		m_IconPlay = Texture2D::Create("Resources/Icons/Toolbar/PlayButton.png");
		m_IconSimulate = Texture2D::Create("Resources/Icons/Toolbar/SimulateButton.png");
		m_IconStop = Texture2D::Create("Resources/Icons/Toolbar/StopButton.png");
		m_IconPause = Texture2D::Create("Resources/Icons/Toolbar/PauseButton.png");
		m_IconStep = Texture2D::Create("Resources/Icons/Toolbar/StepButton.png");

		m_SoundPlay = Audio::Create("Resources/Audio/EditorStart.wav");
		m_SoundSimulate = Audio::Create("Resources/Audio/EditorSimulate.wav");
		m_SoundStop = Audio::Create("Resources/Audio/EditorStop.wav");
		m_SoundPause = Audio::Create("Resources/Audio/EditorPause.wav");

		m_EditIcon = Texture2D::Create("Resources/Icons/Info/EditIcon.png");
		m_LoadingCogAnimation[0] = Texture2D::Create("Resources/Icons/Info/LoadingCog1.png");
		m_LoadingCogAnimation[1] = Texture2D::Create("Resources/Icons/Info/LoadingCog2.png");
		m_LoadingCogAnimation[2] = Texture2D::Create("Resources/Icons/Info/LoadingCog3.png");

		Application::Get().GetImGuiLayer()->AddIconFont("assets/fonts/IconsFont.ttf", 25.0f, 0x700, 0x713);	// Window and Viewport Icons
		Application::Get().GetImGuiLayer()->AddIconFont("assets/fonts/IconsFont.ttf", 12.0f, 0x714, 0x71E); // Gizmo Icons
		Application::Get().GetImGuiLayer()->AddIconFont("assets/fonts/IconsFont.ttf", 25.0f, 0x71F, 0x75D); // Icons

		Application::Get().GetImGuiLayer()->AddIconFont("assets/fonts/IconsFont.ttf", 10.0f, 0xF7, 0xF7); // Division Symbol
		Application::Get().GetImGuiLayer()->AddIconFont("assets/fonts/IconsFont.ttf", 15.0f, 0x03BC, 0x03BC); // Mu Symbol
		Application::Get().GetImGuiLayer()->AddIconFont("assets/fonts/IconsFont.ttf", 15.0f, 0x3C0, 0x3C0); // PI Symbol
		Application::Get().GetImGuiLayer()->AddIconFont("assets/fonts/IconsFont.ttf", 20.0f, 0x2713, 0x2713); // Tick Symbol

		FramebufferSpecification fbSpec;
		fbSpec.Attachments = {
			TextureFormat::RGBA16F,			// Color
			TextureFormat::RED_INTEGER,		// EntityID
			TextureFormat::Depth,			// Depth
			TextureFormat::RGBA16F,			// Normal
			TextureFormat::RGBA16F,			// Emissive
			TextureFormat::RGBA8			// Roughness + Metallic + Specular + AO
		};
		fbSpec.Width = 1600;
		fbSpec.Height = 900;
		fbSpec.Samples = 1;
		m_Framebuffer = Framebuffer::Create(fbSpec);
		SceneRenderer::SetActiveFramebuffer(m_Framebuffer);

		m_EditorScene = CreateRef<Scene>();
		m_ActiveScene = m_EditorScene;

		SourceControl::Init();

		auto& commandLineArgs = Application::Get().GetSpecification().CommandLineArgs;
		if (commandLineArgs.Count > 2)
		{
			auto projectFilePath = commandLineArgs[2];
			OpenProject(projectFilePath);
			ShowEditorWindow();
		}
		else
			m_ProjectLauncher.Open();

		m_SceneHierarchyPanel.SetContext(m_ActiveScene);
		m_ContentBrowserPanel.SetOpenFileCallback([this](const std::filesystem::path& path) { this->OnOpenFile(path); });

		m_EditorCamera = EditorCamera(45.0f, 1.778f, 0.1f, 1000.0f);

		Renderer2D::SetLineWidth(4.0f);

		for (auto& pythonPluginPath : Preferences::GetData().PythonPluginPaths)
			PythonTools::LoadPlugin(pythonPluginPath);
	}
	
	void EditorLayer::OnDetach()
	{
		DY_PROFILE_FUNCTION();

		PythonTools::Shutdown();

		SourceControl::Shutdown();

		Preferences::Shutdown();

		// Close crash manager process and thread handles
		TerminateProcess(s_CrashManagerProcessInformation.hProcess, 1);
		TerminateProcess(s_CrashManagerProcessInformation.hThread, 1);
		CloseHandle(s_CrashManagerProcessInformation.hProcess);
		CloseHandle(s_CrashManagerProcessInformation.hThread);
	}

	void EditorLayer::OnUpdate(Timestep ts)
	{
		DY_PROFILE_FUNCTION();

		PluginLoader::OnUpdate(ts);
		PythonTools::OnUpdate();

		m_DeltaTime = ts;
		m_ProgramTime += ts;
		m_LastSaveTime += ts;

		if (Preferences::GetData().AutosaveEnabled)
		{
			//AutoSave
			if (m_LastSaveTime >= Preferences::GetData().AutosaveTime * 60.0f && !m_EditorScenePath.empty())
			{
				SaveScene();
				DY_CORE_INFO("Autosave Complete: Program Time - {0}", m_ProgramTime);
				Notification::Create("Autosave Completed", ("Autosaved current scene at program time: \n" + std::to_string((int)m_ProgramTime)), { { "Dismiss", [](){} } });
			}

			//Warning of autosave
			if (m_LastSaveTime >= (Preferences::GetData().AutosaveTime * 60.0f) - 10 && m_LastSaveTime <= (Preferences::GetData().AutosaveTime * 60.0f) - 10 + ts && !m_EditorScenePath.empty())
				Notification::Create("Autosave pending...", "Autosave of current scene will commence\n in 10 seconds.", { { "Cancel", [&]() { m_LastSaveTime = 1.0f; } }, { "Save Now", [&]() { m_LastSaveTime = Preferences::GetData().AutosaveTime * 60; } } }, 10.0f, false);
		}

		m_ActiveScene->OnViewportResize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);

		// Resize
		if (FramebufferSpecification spec = m_Framebuffer->GetSpecification();
			m_ViewportSize.x > 0.0f && m_ViewportSize.y > 0.0f && // zero sized framebuffer is invalid
			(spec.Width != m_ViewportSize.x || spec.Height != m_ViewportSize.y))
		{
			m_Framebuffer->Resize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
			SceneRenderer::Resize();
			m_EditorCamera.SetViewportSize(m_ViewportSize.x, m_ViewportSize.y);
		}

		// Render
		Renderer2D::ResetStats();
		SceneRenderer::ResetStats();
		m_Framebuffer->Bind();
		
		//RenderCommand::SetClearColor({ 0.28f, 0.28f, 0.28f, 1.0f });
		RenderCommand::SetClearColor({ 0.0f, 0.0f, 0.0f, 1.0f });
		RenderCommand::Clear();

		//Clear entity ID attachment to -1
		int value = -1;
		m_Framebuffer->ClearAttachment(1, &value);

		switch (m_SceneState)
		{
			case SceneState::Edit:
			{
				m_EditorCamera.OnUpdate(ts);
				m_ActiveScene->OnUpdateEditor(ts, m_EditorCamera);
				break;
			}
			case SceneState::Simulate:
			{
				m_EditorCamera.OnUpdate(ts);
				m_ActiveScene->OnUpdateSimulation(ts, m_EditorCamera);
				break;
			}
			case SceneState::Play:
			{
				m_ActiveScene->OnUpdateRuntime(ts);

				break;
			}
		}

		auto [mx, my] = ImGui::GetMousePos();
		mx -= m_ViewportBounds[0].x;
		my -= m_ViewportBounds[0].y;
		glm::vec2 viewportSize = m_ViewportBounds[1] - m_ViewportBounds[0];
		my = viewportSize.y - my;
		int mouseX = (int)mx;
		int mouseY = (int)my;

		if (mouseX >= 0 && mouseY >= 0 && mouseX < (int)viewportSize.x && mouseY < (int)viewportSize.y)
		{
			int pixelData;
			m_Framebuffer->ReadPixel(1, mouseX, mouseY, &pixelData);
			m_HoveredEntity = pixelData == -1 ? Entity{ entt::null, m_ActiveScene.get() } : Entity((entt::entity)pixelData, m_ActiveScene.get());
		}

		OnOverlayRender();

		m_Framebuffer->Unbind();

		// Prior to ImGui render, load the target workspace if there is one
		if (!m_WorkspaceTarget.empty())
		{
			Preferences::LoadWorkspace(m_WorkspaceTarget);
			m_WorkspaceTarget.clear();
		}
	}

	void EditorLayer::OnImGuiRender()
	{
		DY_PROFILE_FUNCTION();
		
		static const ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
		
		const bool maximized = Application::Get().GetWindow().IsWindowMaximized();
		
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;

		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->Pos);
		ImGui::SetNextWindowSize(viewport->Size);
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		
		if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
			window_flags |= ImGuiWindowFlags_NoBackground;
		
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(maximized ? 0.0f : 5.0f, maximized ? 0.0f : 5.0f));
		ImGui::Begin("Dymatic Editor Dockspace Window", nullptr, window_flags);
		ImVec2 dockspaceWindowPosition = ImGui::GetWindowPos();
		ImGui::PopStyleVar(1);

		// Draw Window Border
		if (!maximized)
		{
			const ImVec2 pos = ImGui::GetWindowPos();
			const ImVec2 size = ImGui::GetWindowSize();

			const float window_border_thickness = 3.0f;
			const float half_thickness = window_border_thickness * 0.5f;

			ImGuiCol coloridx = m_SceneState == SceneState::Edit ? (ImGuiCol_MainWindowBorderEdit)
				: (m_SceneState == SceneState::Play ? (ImGuiCol_MainWindowBorderPlay)
					: (ImGuiCol_MainWindowBorderSimulate));

			ImGui::GetForegroundDrawList()->AddRect({ pos.x + half_thickness, pos.y + half_thickness }, { pos.x + size.x - half_thickness, pos.y + size.y - half_thickness }, ImGui::GetColorU32(coloridx), 5.0f, 0, window_border_thickness);
		}
		
		ImGui::PopStyleVar(2);

		// DockSpace
		ImGuiIO& io = ImGui::GetIO();
		ImGuiStyle& style = ImGui::GetStyle();
		float minWinSizeX = style.WindowMinSize.x;
		style.WindowMinSize.x = 370.0f;
		if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
		{
			ImGuiID dockspace_id = ImGui::GetID("Dymatic Editor DockSpace");
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
		}

		style.WindowMinSize.x = minWinSizeX;

		if (ImGui::BeginMenuBar())
		{
			auto& window = Application::Get().GetWindow();
			
			if (window.GetWidth() > 700)
			{
				ImVec2 pos = ImVec2{ ImGui::GetWindowPos().x + ImGui::GetWindowSize().x / 2, ImGui::GetWindowPos().y + 10.0f };
				ImVec2 points[] = { {pos.x + ImGui::GetWindowSize().x * 0.3f, pos.y - 50.0f}, {pos.x + ImGui::GetWindowSize().x * 0.25f, pos.y + 5.0f}, {pos.x - ImGui::GetWindowSize().x * 0.25f, pos.y + 5.0f}, {pos.x - ImGui::GetWindowSize().x * 0.3f, pos.y - 50.0f} };

				ImGui::GetWindowDrawList()->AddConvexPolyFilled(points, 4, ImGui::GetColorU32(ImGuiCol_MenuBarGrip));
				ImGui::GetWindowDrawList()->AddPolyline(points, 4, ImGui::GetColorU32(ImGuiCol_MenuBarGripBorder), true, 2.0f);
			}

			if (ImGui::BeginMenu(CHARACTER_ICON_DYMATIC))
			{
				if (ImGui::MenuItem("Splash Screen")) { m_ShowSplash = true; }
				if (ImGui::MenuItem("About Dymatic")) { Popup::Create("Engine Information", "Dymatic Engine\nVersion 23.1.0 (Development)\n\n\nBranch Publication Date: 11th April 2023\nBranch: Development\n\n\nDymatic Engine is a free, open source engine developed by Dymatic Technologies.\nView source files for licenses from vendor libraries.", { { "Learn More", []() { ShellExecute(0, 0, L"https://www.dymaticengine.com", 0, 0, SW_SHOW); } }, { "Ok", []() {} } }, m_DymaticLogo); }
				ImGui::Separator();
				if (ImGui::MenuItem("Github")) { ShellExecute(0, 0, L"https://github.com/benc25/dymatic", 0, 0 , SW_SHOW ); }
				if (ImGui::MenuItem("Website")) { ShellExecute(0, 0, L"https://www.dymaticengine.com", 0, 0 , SW_SHOW ); }
				ImGui::Separator();
				if (ImGui::BeginMenu("System"))
				{
					ImGui::MenuItem("Performance Analyzer", "", &m_PerformanceAnalyser.GetPerformanceAnalyserVisible());
					ImGui::EndMenu();
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem(CHARACTER_ICON_NEW_FILE " New", Preferences::Keymap::GetBindString(Preferences::Keymap::KeyBindEvent::NewSceneBind).c_str())) NewScene();
				if (ImGui::MenuItem(CHARACTER_ICON_OPEN_FILE " Open...", Preferences::Keymap::GetBindString(Preferences::Keymap::KeyBindEvent::OpenSceneBind).c_str())) OpenScene();
				if (ImGui::BeginMenu(CHARACTER_ICON_RECENT " Open Recent", !ProjectSettings::GetData().RecentScenePaths.empty()))
				{
					for (auto& file : ProjectSettings::GetData().RecentScenePaths)
						if (ImGui::MenuItem((file.stem()).string().c_str()))
							OpenScene(file);
					ImGui::EndMenu();
				}
				if (ImGui::MenuItem(CHARACTER_ICON_APPEND " Append...", "")) AppendScene();

				ImGui::Separator();

				if (ImGui::MenuItem(CHARACTER_ICON_SAVE " Save", Preferences::Keymap::GetBindString(Preferences::Keymap::KeyBindEvent::SaveSceneBind).c_str())) SaveScene();
				if (ImGui::MenuItem(CHARACTER_ICON_SAVE_AS " Save As...", Preferences::Keymap::GetBindString(Preferences::Keymap::KeyBindEvent::SaveSceneAsBind).c_str())) SaveSceneAs();

				ImGui::Separator();

				if (ImGui::MenuItem(CHARACTER_ICON_NEW_PROJECT " New Project"))
					m_ProjectLauncher.Open(true);

				if (ImGui::MenuItem(CHARACTER_ICON_OPEN_PROJECT " Open Project"))
					m_ProjectLauncher.Open();

				ImGui::MenuItem(CHARACTER_ICON_PACKAGE " Package Project");

				ImGui::Separator();

				if (ImGui::BeginMenu(CHARACTER_ICON_WORKSPACE " Workspace"))
				{
					ImGuiSelectableFlags selectableFlags = ImGuiSelectableFlags_SpanAvailWidth | ImGuiSelectableFlags_AllowItemOverlap;

					ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2());

					if (ImGui::IsWindowAppearing())
						ReloadAvailableWorkspaces();

					ImGui::Dummy({ 250.0f, 0.0f });
					
					if (ImGui::Selectable(CHARACTER_ICON_ADD " Create", false, selectableFlags | ImGuiSelectableFlags_DontClosePopups))
					{
						std::filesystem::path newWorkspaceFilepath = std::filesystem::path("saved/workspaces") / FileManager::GetNextOfNameInDirectory("New Workspace.workspace", "saved/workspaces");
						Preferences::SaveWorkspace(newWorkspaceFilepath);
						ReloadAvailableWorkspaces();
						m_WorkspaceRenameContext = newWorkspaceFilepath;
					}

					if (ImGui::Selectable(CHARACTER_ICON_WORKSPACE " Default Workspace", false, selectableFlags))
						m_WorkspaceTarget = "saved/presets/workspaces/DefaultWorkspace.workspace";

					if (!m_AvailableWorkspaces.empty())
						ImGui::Separator();

					for (auto& workspace : m_AvailableWorkspaces)
					{
						ImGui::PushID(workspace.c_str());

						if (workspace == m_WorkspaceRenameContext)
						{
							ImGui::SetNextItemWidth(-75.0f);
							char buffer[256];
							memset(buffer, 0, sizeof(buffer));
							strncpy_s(buffer, sizeof(buffer), m_WorkspaceRenameContext.stem().string().c_str(), sizeof(buffer));
							if (GImGui->ActiveId != ImGui::GetID("##WorkspaceInputText"))
								ImGui::SetKeyboardFocusHere();
							ImGui::InputText("##WorkspaceInputText", buffer, sizeof(buffer));
							if (ImGui::IsItemActivePreviousFrame() && !ImGui::IsItemActive())
							{
								if (buffer != m_WorkspaceRenameContext.stem())
								{
									std::filesystem::path renameFilepath = m_WorkspaceRenameContext.parent_path() / FileManager::GetNextOfNameInDirectory(std::filesystem::path(std::string(buffer) + ".workspace"), m_WorkspaceRenameContext.parent_path(), m_WorkspaceRenameContext.filename());
									if (FileManager::IsFilenameValid(renameFilepath.filename()))
										std::filesystem::rename(m_WorkspaceRenameContext, renameFilepath);
								}
								ReloadAvailableWorkspaces();
							}
						}
						else
						{
							ImGui::SetNextItemWidth(50.0f);
							if (ImGui::Selectable(fmt::format(CHARACTER_ICON_WORKSPACE " {}", workspace.stem().string()).c_str(), false, selectableFlags))
								m_WorkspaceTarget = workspace;
						}

						ImGui::SameLine();
						ImGui::Dummy({ std::max(style.FramePadding.x, ImGui::GetContentRegionAvailWidth() - 66.0f), 0.0f });
						ImGui::SameLine();

						if (ImGui::Button(CHARACTER_ICON_EDIT))
							m_WorkspaceRenameContext = workspace;
						ImGui::SameLine();
						if (ImGui::Button(CHARACTER_ICON_DELETE))
						{
							std::filesystem::remove(workspace);
							ReloadAvailableWorkspaces();
						}

						ImGui::PopID();
					}

					ImGui::PopStyleVar();
					ImGui::EndMenu();
				}

				ImGui::Separator();

				if (ImGui::MenuItem(CHARACTER_ICON_RESTART " Restart"))
				{
					WCHAR filename[MAX_PATH];
					GetModuleFileName(GetModuleHandle(NULL), filename, sizeof(filename));
					Process::CreateApplicationProcess(filename, {});
					Application::Get().Close();
				}

				if (ImGui::MenuItem(CHARACTER_ICON_QUIT " Quit", Preferences::Keymap::GetBindString(Preferences::Keymap::KeyBindEvent::QuitBind).c_str())) SaveAndExit();
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Edit"))
			{
				if (ImGui::MenuItem(CHARACTER_ICON_PREFERENCES " Preferences")) 
					m_PreferencesPannel.GetPreferencesPanelVisible() = true;
				if (ImGui::MenuItem(CHARACTER_ICON_TEXT_EDIT " Open Solution"))
					;
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Window"))
			{
				ImGui::MenuItem(CHARACTER_ICON_VIEWPORT " Viewport", "",				&Preferences::GetEditorWindowVisible(Preferences::EditorWindow::Viewport));
				ImGui::MenuItem(CHARACTER_ICON_TOOLBAR " Toolbar", "",					&Preferences::GetEditorWindowVisible(Preferences::EditorWindow::Toolbar));
				ImGui::MenuItem(CHARACTER_ICON_STATISTICS " Statistics", "",			&Preferences::GetEditorWindowVisible(Preferences::EditorWindow::Statistics));
				ImGui::MenuItem(CHARACTER_ICON_INFO " Info", "",						&Preferences::GetEditorWindowVisible(Preferences::EditorWindow::Info));
				ImGui::MenuItem(CHARACTER_ICON_MEMORY " Profiler", "",					&Preferences::GetEditorWindowVisible(Preferences::EditorWindow::Profiler));
				ImGui::MenuItem(CHARACTER_ICON_NODES " Script Editor", "",				&Preferences::GetEditorWindowVisible(Preferences::EditorWindow::ScriptEditor));
				ImGui::MenuItem(CHARACTER_ICON_SCENE_HIERARCHY " Scene Hierarchy", "",	&Preferences::GetEditorWindowVisible(Preferences::EditorWindow::SceneHierarchy));
				ImGui::MenuItem(CHARACTER_ICON_PROPERTIES " Properties", "",			&Preferences::GetEditorWindowVisible(Preferences::EditorWindow::Properties));
				ImGui::MenuItem(CHARACTER_ICON_NOTIFICATIONS " Notifications", "",		&Preferences::GetEditorWindowVisible(Preferences::EditorWindow::Notifications));
				ImGui::MenuItem(CHARACTER_ICON_FOLDER " Content Browser", "",			&Preferences::GetEditorWindowVisible(Preferences::EditorWindow::ContentBrowser));
				ImGui::MenuItem(CHARACTER_ICON_TEXT_EDIT " Text Editor", "",			&Preferences::GetEditorWindowVisible(Preferences::EditorWindow::TextEditor));
				ImGui::MenuItem(CHARACTER_ICON_CURVE " Curve Editor", "",				&Preferences::GetEditorWindowVisible(Preferences::EditorWindow::CurveEditor));
				ImGui::MenuItem(CHARACTER_ICON_IMAGE " Image Editor", "",				&Preferences::GetEditorWindowVisible(Preferences::EditorWindow::ImageEditor));
				ImGui::MenuItem(CHARACTER_ICON_MATERIAL " Material Editor", "",			&Preferences::GetEditorWindowVisible(Preferences::EditorWindow::MaterialEditor));
				ImGui::MenuItem(CHARACTER_ICON_CONSOLE " Console", "",					&Preferences::GetEditorWindowVisible(Preferences::EditorWindow::Console));
				ImGui::MenuItem(CHARACTER_ICON_STATISTICS " Asset Manager", "",			&Preferences::GetEditorWindowVisible(Preferences::EditorWindow::AssetManager));

				ImGui::Separator();

				if (ImGui::MenuItem(CHARACTER_ICON_SYSTEM_CONSOLE " Toggle System Console", ""))
				{
					if (Log::IsConsoleVisible())
						Log::HideConsole();
					else
						Log::ShowConsole();
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("View"))
			{
				if (ImGui::MenuItem("Perspective/Orthographic", Preferences::Keymap::GetBindString(Preferences::Keymap::KeyBindEvent::ViewProjectionBind).c_str())) { m_ProjectionToggled = !m_EditorCamera.GetProjectionType(); m_EditorCamera.SetProjectionType(m_ProjectionToggled); }

				if (ImGui::BeginMenu("Set Perspective"))
				{
					if (ImGui::MenuItem("Front", Preferences::Keymap::GetBindString(Preferences::Keymap::KeyBindEvent::ViewFrontBind).c_str())) { m_YawUpdate = floor(glm::degrees(m_EditorCamera.GetYaw()) / 360) * 360; m_PitchUpdate = floor(glm::degrees(m_EditorCamera.GetYaw()) / 360) * 360; m_UpdateAngles = true; m_EditorCamera.SetProjectionType(1); }
					if (ImGui::MenuItem("Side", Preferences::Keymap::GetBindString(Preferences::Keymap::KeyBindEvent::ViewSideBind).c_str())) { m_YawUpdate = floor(glm::degrees(m_EditorCamera.GetYaw()) / 360) * 360 - 90.0f; m_PitchUpdate = floor(glm::degrees(m_EditorCamera.GetYaw()) / 360) * 360; m_UpdateAngles = true; m_EditorCamera.SetProjectionType(1); }
					if (ImGui::MenuItem("Top", Preferences::Keymap::GetBindString(Preferences::Keymap::KeyBindEvent::ViewTopBind).c_str())) { m_YawUpdate = floor(glm::degrees(m_EditorCamera.GetYaw()) / 360) * 360; m_PitchUpdate = floor(glm::degrees(m_EditorCamera.GetYaw()) / 360) * 360 + 90; m_UpdateAngles = true; m_EditorCamera.SetProjectionType(1); }
					ImGui::Separator();
					if (ImGui::MenuItem("Flip", Preferences::Keymap::GetBindString(Preferences::Keymap::KeyBindEvent::ViewFlipBind).c_str())) { m_YawUpdate = glm::degrees(m_EditorCamera.GetYaw()) + 180.0f; m_PitchUpdate = glm::degrees(m_EditorCamera.GetPitch()); m_UpdateAngles = true; m_EditorCamera.SetProjectionType(1); }

					ImGui::EndMenu();
				}

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Script"))
			{
				if (ImGui::MenuItem(CHARACTER_ICON_SCRIPT " Compile Assembly", "", nullptr, m_SceneState == SceneState::Edit))
					Compile();
				
				if (ImGui::MenuItem(CHARACTER_ICON_RESTART " Reload Assembly", Preferences::Keymap::GetBindString(Preferences::Keymap::KeyBindEvent::ReloadAssembly).c_str(), nullptr, m_SceneState == SceneState::Edit))
					ScriptEngine::ReloadAssembly();

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Help"))
			{
				if (ImGui::MenuItem(CHARACTER_ICON_DOCUMENT " Documentation"))
					ShellExecute(0, 0, L"https://docs.dymaticengine.com", 0, 0, SW_SHOW);
				ImGui::EndMenu();
			}

			auto drawList = ImGui::GetWindowDrawList();
			
			{
				ImGui::PushFont(io.Fonts->Fonts[0]);
				std::string projectName = "None";
				if (Project::GetActive())
					projectName = Project::GetName();
				
				auto minX = ImGui::GetWindowPos().x + ImGui::GetWindowWidth() * 0.785f;
				auto minY = ImGui::GetWindowPos().y;
				drawList->AddRect(ImVec2(minX, minY), ImVec2(minX + ImGui::CalcTextSize(projectName.c_str()).x + style.FramePadding.x * 4.0f, minY + ImGui::GetTextLineHeight() + style.FramePadding.y * 2.0f), 
					ImGui::GetColorU32(ImGuiCol_TextDisabled), 5.0f, ImDrawFlags_RoundCornersBottom
				);
				drawList->AddText(ImVec2(minX + style.FramePadding.x * 2.0f, minY + style.FramePadding.y), ImGui::GetColorU32(ImGuiCol_TextDisabled), projectName.c_str());
				ImGui::PopFont();
			}
			
			// Titlebar and window buttons
			{
				auto endMenuItemPosition = ImGui::GetItemRectMax();

				const ImVec4 WindowOperatorButtonCol = ImGui::GetStyleColorVec4(ImGuiCol_Text);
				const ImVec4 WindowOperatorCircleCol = ImVec4{ 0.0f, 0.0f, 0.0f, 0.0f };
				const ImVec4 WindowOperatorCircleColHovered = ImGui::GetStyleColorVec4(ImGuiCol_HeaderHovered);
				const float lineThickness = 1.0f;
				const ImVec2 buttonSize = ImVec2(45.0f, 32.0f);
				const float iconRadius = 5.0f;

				for (int i = 0; i < 3; i++)
				{
					ImGui::PushID(i);

					ImVec2 pos = ImVec2{ ImGui::GetWindowPos().x + ImGui::GetWindowSize().x - ((i + 1) * buttonSize.x), (ImGui::GetWindowPos().y) };

					const ImGuiID id = ImGui::GetCurrentWindow()->GetID("##WindowOperatorButton");
					const ImRect bb(pos, ImVec2(pos.x + buttonSize.x, pos.y + buttonSize.y));
					const ImVec2 center = ImVec2((bb.Min.x + bb.Max.x) * 0.5f, (bb.Min.y + bb.Max.y) * 0.5f);
					bool hovered, held;
					bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);
					ImU32 col = ImGui::ColorConvertFloat4ToU32(WindowOperatorButtonCol);
					drawList->AddRectFilled(pos, ImVec2(pos.x + buttonSize.x, pos.y + buttonSize.y), ImGui::ColorConvertFloat4ToU32(hovered ? WindowOperatorCircleColHovered : WindowOperatorCircleCol));
					switch (i)
					{
					case 0: {
						drawList->AddLine(ImVec2{ center.x - iconRadius, center.y + iconRadius }, ImVec2{ center.x + iconRadius, center.y - iconRadius }, col, lineThickness);
						drawList->AddLine(ImVec2{ center.x - iconRadius, center.y - iconRadius }, ImVec2{ center.x + iconRadius, center.y + iconRadius }, col, lineThickness);
						s_CloseHovered = hovered;
						break;
					}
					case 1: {
						drawList->AddRect(ImVec2{ center.x - iconRadius, center.y - iconRadius }, ImVec2{ center.x + iconRadius, center.y + iconRadius }, col, NULL, NULL, lineThickness);
						s_MaximiseHovered = !held && hovered;
						break;
					}
					case 2:
					{
						drawList->AddLine(ImVec2{ center.x - iconRadius, center.y }, ImVec2{ center.x + iconRadius, center.y }, col, lineThickness);
						s_MinimizeHovered = hovered;
						break;
					}
					}
					if (pressed)
					{
						switch (i)
						{
						case 0: { SaveAndExit(); break; }
						case 1: {window.IsWindowMaximized() ? window.RestoreWindow() : window.MaximizeWindow(); break; }
						case 2: {window.MinimizeWindow(); break; }
						}
					}

					ImGui::PopID();
				}

				//Window Move
				s_TitlebarHovered = false;
				if (GImGui->HoveredWindow == ImGui::GetCurrentWindow() || GImGui->HoveredWindow == nullptr)
				{
					auto& rect = ImGui::GetCurrentWindow()->Rect();
					const ImRect bb();
					if (ImGui::IsMouseHoveringRect(ImGui::GetWindowPos(), ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowSize().x, ImGui::GetWindowPos().y + 30), false)
						&& !ImGui::IsMouseHoveringRect(ImGui::GetWindowPos(), endMenuItemPosition, false))
						s_TitlebarHovered = true;
				}
			}

			ImGui::EndMenuBar();
		}

		//Toolbar Window
		if (auto& toolbarVisible = Preferences::GetEditorWindowVisible(Preferences::EditorWindow::Toolbar))
		{
			ImGui::Begin(CHARACTER_ICON_TOOLBAR " Toolbar", &toolbarVisible, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

			const float buttonHeight = 30.0f;
			auto drawList = ImGui::GetWindowDrawList();

			ImGui::PushStyleColor(ImGuiCol_Button, {});
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {});
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, {});

			// Project Management Controls
			{
				// Save Scene Button
				{
					if (ImGui::Button("##SaveSceneButton", ImVec2(buttonHeight, buttonHeight)))
						SaveScene();
					drawList->AddImage((ImTextureID)(uint64_t)((ImGui::IsItemHovered() ? m_SaveHoveredIcon : m_SaveIcon)->GetRendererID()), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), { 0, 1 }, { 1, 0 });
				}

				ImGui::SameLine();

				// Source Control Button
				{
					if (ImGui::Button("##SourceControlButton", ImVec2(buttonHeight, buttonHeight)))
						m_SourceControlPanel.GetSourceControlPannelVisible() = !m_SourceControlPanel.GetSourceControlPannelVisible();

					if (GImGui->HoveredIdTimer > 0.5f && ImGui::IsItemHovered())
						ImGui::SetTooltip(SourceControl::IsActive() ? "Source Control: Active" : "Source Control: Inactive");

					drawList->AddImage((ImTextureID)(uint64_t)((m_SourceControlIcon)->GetRendererID()), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), { 0, 1 }, { 1, 0 },
						ImGui::GetColorU32(ImGui::IsItemHovered() ? ImVec4(0.75f, 0.75f, 0.75f, 1.0f) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f))
					);

					drawList->AddCircle(ImGui::GetItemRectMax(), 4.0f, ImGui::GetColorU32(SourceControl::IsActive() ? ImVec4(0.2f, 0.7f, 0.3f, 1.0f) : ImVec4(0.8f, 0.2f, 0.3f, 1.0f)), 0, 3.0f);
				}

				ImGui::SameLine();

				// Compile Button
				{
					if (ImGui::Button("##CompileButton", ImVec2(buttonHeight, buttonHeight)) && m_SceneState == SceneState::Edit)
						Compile();
					drawList->AddImage((ImTextureID)(uint64_t)((ImGui::IsItemHovered() && m_SceneState == SceneState::Edit ? m_CompileHoveredIcon : m_CompileIcon)->GetRendererID()), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), { 0, 1 }, { 1, 0 }, 
						ImGui::GetColorU32(m_SceneState == SceneState::Edit ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f) : ImVec4(0.5f, 0.5f, 0.5f, 1.0f))
					);
				}

				ImGui::SameLine();

				// IDE Debugger Connection
				if (ScriptEngine::IsDebuggerAttached())
				{
					if (ImGui::Button("##IDEDebuggerButton", ImVec2(buttonHeight, buttonHeight)))
						;

					drawList->AddImage((ImTextureID)(uint64_t)((m_IDEIcon)->GetRendererID()), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), { 0, 1 }, { 1, 0 },
						ImGui::GetColorU32(ImGui::IsItemHovered() ? ImVec4(0.75f, 0.75f, 0.75f, 1.0f) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f)));

					if (GImGui->HoveredIdTimer > 0.5f && ImGui::IsItemHovered())
						ImGui::SetTooltip("IDE Debugger Connection Established");
				}
			}

			ImGui::SameLine();
			
			// Editor State Controls
			{				
				const int button_count = 4;
				ImGui::SetCursorPosX((ImGui::GetWindowContentRegionMax().x * 0.5f) - (buttonHeight * button_count * 0.5f));

				drawList->AddRectFilled(ImVec2(ImGui::GetCursorScreenPos().x - style.FramePadding.x, ImGui::GetCursorScreenPos().y - style.FramePadding.y), ImVec2(ImGui::GetCursorScreenPos().x + (buttonHeight + 2.0f * style.FramePadding.x) * button_count, ImGui::GetCursorScreenPos().y + buttonHeight + style.FramePadding.y), ImGui::GetColorU32(ImGuiCol_Header), style.FrameRounding);

				{
					const Ref<Texture2D> icon = m_SceneState == SceneState::Edit || m_SceneState == SceneState::Simulate ? m_IconPlay : (m_ActiveScene->IsPaused() ? m_IconPlay : m_IconPause);
					if (ImGui::Button("##PlayButton", { buttonHeight, buttonHeight }))
					{
						if (m_SceneState == SceneState::Edit || m_SceneState == SceneState::Simulate)
							OnScenePlay();
						else if (m_SceneState == SceneState::Play)
							OnScenePause();
					}
					const ImColor color = m_SceneState == SceneState::Edit || m_SceneState == SceneState::Simulate || (m_SceneState == SceneState::Play && m_ActiveScene->IsPaused()) ? (ImGui::IsItemActive() ? ImColor(101, 142, 52) : (ImGui::IsItemHovered() ? ImColor(169, 194, 150) : ImColor(139, 194, 74))) : (ImGui::IsItemActive() ? ImColor(117, 117, 117) : (ImGui::IsItemHovered() ? ImColor(255, 255, 255) : ImColor(192, 192, 192)));
					drawList->AddImage((ImTextureID)(uint64_t)icon->GetRendererID(), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), { 0, 1 }, { 1, 0 }, color);
				}
				ImGui::SameLine();
				{
					const Ref<Texture2D> icon = m_SceneState == SceneState::Edit || m_SceneState == SceneState::Play ? m_IconSimulate : (m_ActiveScene->IsPaused() ? m_IconSimulate : m_IconPause);
					if (ImGui::Button("##SimulateButton", { buttonHeight, buttonHeight }))
					{
						if (m_SceneState == SceneState::Edit || m_SceneState == SceneState::Play)
							OnSceneSimulate();
						else if (m_SceneState == SceneState::Simulate)
							OnScenePause();
					}
					const ImColor color = ImGui::IsItemActive() ? ImColor(117, 117, 117) : (ImGui::IsItemHovered() ? ImColor(255, 255, 255) : ImColor(192, 192, 192));
					drawList->AddImage((ImTextureID)(uint64_t)icon->GetRendererID(), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), { 0, 1 }, { 1, 0 }, color);
				}
				ImGui::SameLine();
				{
					if (ImGui::Button("##StopButton", { buttonHeight, buttonHeight }))
					{
						if (m_SceneState != SceneState::Edit)
							OnSceneStop();
					}
					const ImColor color = m_SceneState == SceneState::Edit ? ImColor(117, 117, 117) : (ImGui::IsItemActive() ? ImColor(188, 44, 44) : (ImGui::IsItemHovered() ? ImColor(255, 192, 192) : ImColor(255, 64, 64)));
					drawList->AddImage((ImTextureID)(uint64_t)m_IconStop->GetRendererID(), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), { 0, 1 }, { 1, 0 }, color);
				}
				ImGui::SameLine();
				{
					if (ImGui::Button("##NextFrameButton", { buttonHeight, buttonHeight }))
					{
						if (m_SceneState != SceneState::Edit && m_ActiveScene->IsPaused())
							m_ActiveScene->Step(Preferences::GetData().FrameStepCount);
					}
					const ImColor color = (m_SceneState != SceneState::Edit && m_ActiveScene->IsPaused()) ? (ImGui::IsItemHovered() ? ImColor(255, 255, 255) : ImColor(192, 192, 192)) : ImColor(117, 117, 117);
					drawList->AddImage((ImTextureID)(uint64_t)m_IconStep->GetRendererID(), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), { 0, 1 }, { 1, 0 }, color);
				}
			}

			ImGui::PopStyleColor(2);

			// Toolbar/Project Settings Dropdown
			{
				const char* text = "      Settings";
				const float textWidth = ImGui::CalcTextSize(text).x;
				const float width = buttonHeight + textWidth;

				ImGui::SameLine();
				ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvailWidth() - width - 2.0f * style.WindowPadding.x, 0.0f));
				ImGui::SameLine();

				if (ImGui::Button(text, ImVec2(width, buttonHeight)))
					ImGui::OpenPopup("##ProjectSettingsPopup");
				auto& min = ImGui::GetItemRectMin();
				drawList->AddImage((ImTextureID)(uint64_t)((m_SettingsIcon)->GetRendererID()), 
					ImVec2(min.x + style.FramePadding.x, min.y + style.FramePadding.y), 
					ImVec2(min.x + style.FramePadding.x + buttonHeight, min.y + style.FramePadding.y + buttonHeight),
					{0, 1}, {1, 0});

				if (ImGui::BeginPopup("##ProjectSettingsPopup"))
				{
					ImGui::MenuItem("Show Transform Gizmo", nullptr, &Preferences::GetData().ShowTransformGizmo);

					// Editor Volume
					{
						float volume = Preferences::GetData().EditorVolume;

						const char* volumeText;
						if (volume == 0.0f) volumeText = CHARACTER_ICON_AUDIO_OFF " Volume###EditorVolumeMenu";
						else if (volume < 0.4f) volumeText = CHARACTER_ICON_AUDIO_LOW " Volume###EditorVolumeMenu";
						else if (volume < 0.8f) volumeText = CHARACTER_ICON_AUDIO_MEDIUM " Volume###EditorVolumeMenu";
						else volumeText = CHARACTER_ICON_AUDIO_HIGH " Volume###EditorVolumeMenu";

						if (ImGui::BeginMenu(volumeText))
						{
							if (ImGui::SliderFloat("##ProjectSettingsVolumeInput", &volume, 0.0f, 1.0f, "%.2f"))
							{
								Preferences::GetData().EditorVolume = volume;
								AudioEngine::SetGlobalVolume(volume);
							}

							ImGui::EndMenu();
						}
					}
					
					ImGui::MenuItem("Show Viewport UI", nullptr, &Preferences::GetData().ShowViewportUI);

					ImGui::EndPopup();
				}
			}

			ImGui::PopStyleColor(1);

			ImGui::End();
		}

		if (auto& infoVisible = Preferences::GetEditorWindowVisible(Preferences::EditorWindow::Info))
		{
			ImVec4 color = ImGui::GetStyleColorVec4(
				m_SceneState == SceneState::Edit ? (ImGuiCol_MainWindowBorderEdit)
				: (m_SceneState == SceneState::Play ? (ImGuiCol_MainWindowBorderPlay)
					: (ImGuiCol_MainWindowBorderSimulate)));

			ImGui::PushStyleColor(ImGuiCol_WindowBg, color);
			ImGui::PushStyleColor(ImGuiCol_Border, color);
			ImGui::Begin(CHARACTER_ICON_INFO " Info", &infoVisible, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
			ImGui::PopStyleColor(2);

			static int s_LoadingFrameIndex;
			s_LoadingFrameIndex++;
			if (s_LoadingFrameIndex > 7)
				s_LoadingFrameIndex = 0;

			float size = ImGui::GetContentRegionAvail().y;
			ImGui::Image((ImTextureID)(uint64_t)((m_SceneState == SceneState::Edit ? m_EditIcon : m_LoadingCogAnimation[s_LoadingFrameIndex/3])->GetRendererID()), { size, size }, { 0, 1 }, { 1, 0 });

			{
				std::string text;
				if (m_SceneState == SceneState::Edit)
					text = "Editing";
				else if (m_SceneState == SceneState::Play)
					text = "Playing";
				else if (m_SceneState == SceneState::Simulate)
					text = "Simulating";

				if (m_SceneState != SceneState::Edit)
				{
					for (size_t i = 0; i < ((int)ImGui::GetTime())%4; i++)
						text += ".";
				}

				ImGui::SameLine();
				ImGui::Text(text.c_str());
			}

			if (Project::GetActive())
			{
				std::string currentSceneName = std::filesystem::relative(m_EditorScenePath, Project::GetAssetDirectory()).lexically_normal().replace_extension().string();

				ImGui::SameLine();
				ImGui::Dummy({ (ImGui::GetContentRegionAvailWidth() - ImGui::CalcTextSize(currentSceneName.c_str()).x) * 0.5f, 0.0f });
				ImGui::SameLine();
				ImGui::Text(currentSceneName.c_str());
			}

			ImGui::SameLine();
			std::string text = "23.1.0 (Development)";
			ImGui::Dummy(ImVec2{ ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(text.c_str()).x - 5, 0 });
			ImGui::SameLine();
			ImGui::Text(text.c_str());

			ImGui::End();
		}

		//Preferences Pannel
		m_PreferencesPannel.OnImGuiRender();

		// Update Viewport-Scene Hierarchy Picker
		{
			auto& pickingID = m_SceneHierarchyPanel.GetPickingID();
			if (pickingID == 1 && Input::IsMouseButtonPressed(Mouse::ButtonLeft) && m_ViewportHovered)
			{
				if ((entt::entity)m_HoveredEntity != entt::null)
					pickingID = m_HoveredEntity.GetUUID();
				else
					pickingID = 0;
			}
		}

		//Scene Hierarchy and properties pannel
		m_SceneHierarchyPanel.OnImGuiRender();

		// Check for external drag drop sources
		m_ContentBrowserPanel.OnImGuiRender(m_IsDragging);

		SceneRenderer::OnImGuiRender();

		m_PerformanceAnalyser.OnImGuiRender(m_DeltaTime);

		m_CurveEditor.OnImGuiRender();
		m_ImageEditor.OnImGuiRender();

		m_MaterialEditorPanel.OnImGuiRender();

		{
			std::filesystem::path projectLoadPath;
			m_ProjectLauncher.OnImGuiRender(&projectLoadPath);
			if (!projectLoadPath.empty())
			{
				// Show main window if no project was previously active
				if (!Project::GetActive())
					ShowEditorWindow();

				OpenProject(projectLoadPath);
			}
		}

		m_SourceControlPanel.OnImGuiRender();

		Notification::OnImGuiRender(m_DeltaTime);
		Popup::OnImGuiRender(m_DeltaTime);
		m_NotificationsPanel.OnImGuiRender(m_DeltaTime);

		PluginLoader::OnUIRender();
		
		PythonTools::OnImGuiRender();
		
		m_ProfilerPanel.OnImGuiRender();

		m_ConsoleWindow.OnImGuiRender(m_DeltaTime);
		m_AssetManagerPanel.OnImGuiRender();
		
		m_TextEditor.OnImGuiRender();
		
		m_NodeEditorPannel.OnImGuiRender();
		m_MaterialEditor.OnImGuiRender();

		if (auto& statisticsVisible = Preferences::GetEditorWindowVisible(Preferences::EditorWindow::Statistics))
		{
			ImGui::Begin(CHARACTER_ICON_STATISTICS " Statistics", &statisticsVisible);

			ImGui::Text("Frames Per Second: %d", (int)(1.0f / m_DeltaTime));
			ImGui::Text("Delta Time: %f ms", m_DeltaTime * 1000);

			ImGui::Separator();

			auto stats = Renderer2D::GetStats();
			ImGui::Text("Renderer2D Stats:");
			ImGui::Text("Draw Calls: %d", stats.DrawCalls);
			ImGui::Text("Quads: %d", stats.QuadCount);
			ImGui::Text("Vertices: %d", stats.GetTotalVertexCount());
			ImGui::Text("Indices: %d", stats.GetTotalIndexCount());

			ImGui::Separator();

			std::string name = "Null";
			if ((entt::entity)m_HoveredEntity != entt::null)
				name = m_HoveredEntity.GetComponent<TagComponent>().Tag;
			ImGui::Text("Hovered Entity: %s", name.c_str());
			ImGui::End();

			ImGui::Begin("Memory");
			for (auto& allocation : Memory::GetMemoryAllocationStats())
				ImGui::Text("%s: %d", allocation.first, allocation.second.TotalAllocated - allocation.second.TotalFreed);

			for (auto& allocation : Memory::GetMemoryAllocations())
			{
				std::stringstream sstream;
				sstream << std::hex << allocation.second.Memory;
				std::string result = sstream.str();
				ImGui::Text("%s : %s", result.c_str(), std::to_string(allocation.second.Size).c_str());
			}

			ImGui::Separator();

			auto& allocStats = Memory::GetAllocationStats();
			ImGui::Text("Total Allocated: %d", allocStats.TotalAllocated);
			ImGui::Text("Total Freed: %d", allocStats.TotalFreed);
			ImGui::Text("Current Usage: %d", allocStats.TotalAllocated - allocStats.TotalFreed);

			ImGui::End();

			ImGui::Begin("Settings");
			ImGui::Checkbox("Show physics colliders", &m_ShowPhysicsColliders);
			ImGui::End();
		}


		if (auto& viewportVisible = Preferences::GetEditorWindowVisible(Preferences::EditorWindow::Viewport))
		{
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
			ImGui::Begin(CHARACTER_ICON_VIEWPORT " Viewport", &viewportVisible, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
			auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
			auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
			auto viewportOffset = ImGui::GetWindowPos();
			m_ViewportBounds[0] = { viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y };
			m_ViewportBounds[1] = { viewportMaxRegion.x + viewportOffset.x, viewportMaxRegion.y + viewportOffset.y };

			m_ViewportFocused = ImGui::IsWindowFocused();
			m_ViewportHovered = ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered();
			//Application::Get().GetImGuiLayer()->BlockEvents(!m_ViewportFocused && !m_ViewportHovered);
			Application::Get().GetImGuiLayer()->BlockEvents(false);

			// Update logic to check if camera can be moved
			if (m_SceneState == SceneState::Play)
			{
				m_ViewportActive = false;
				m_EditorCamera.SetBlockEvents(true);
			}
			else
			{
				if (m_ViewportHovered && (ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Right) || ImGui::IsMouseClicked(ImGuiMouseButton_Middle)))
				{
					m_ViewportActive = true;
					m_EditorCamera.SetBlockEvents(false);
				}

				if (!m_EditorCamera.GetBlockEvents() && !ImGui::IsMouseDown(ImGuiMouseButton_Left) && !ImGui::IsMouseDown(ImGuiMouseButton_Right) && !ImGui::IsMouseDown(ImGuiMouseButton_Middle))
				{
					m_ViewportActive = false;
					m_EditorCamera.SetBlockEvents(true);
				}
			}
			
			ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
			m_ViewportSize = { viewportPanelSize.x, viewportPanelSize.y };
			
			ImGui::Image(reinterpret_cast<void*>(m_Framebuffer->GetColorAttachmentRendererID()), ImVec2{ m_ViewportSize.x, m_ViewportSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
			
			if (ImGui::BeginDragDropTarget())
			{
				// If the payload is accepted
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
				{
					const wchar_t* w_path = (const wchar_t*)payload->Data;
					std::filesystem::path path = w_path;
					if (path.extension() == ".dymatic")
						OpenScene(Project::GetAssetFileSystemPath(path));
					else if (FileManager::GetFileType(path) == FileType::FileTypeTexture)
					{
						auto texture = AssetManager::GetAsset<Texture2D>(path);
						if (texture)
						{
							auto entity = m_ActiveScene->CreateEntity(path.filename().stem().string());
							entity.AddComponent<SpriteRendererComponent>(texture);

							float image_width = texture->GetWidth();
							float image_height = texture->GetHeight();
							if (image_width > image_height)
							{
								image_width /= image_height;
								image_height = 1.0f;
							}
							else
							{
								image_height /= image_width;
								image_width = 1.0f;
							}

							entity.GetComponent<TransformComponent>().Scale = glm::vec3(image_width, image_height, 1.0f);
						}
					}
					else if (FileManager::GetFileType(path) == FileType::FileTypeMesh)
					{
						// Create a new entity with a static mesh
						auto entity = m_ActiveScene->CreateEntity(path.filename().stem().string());
						if (Ref <Model> model = AssetManager::GetAsset<Model>(path.string()))
						{
							entity.AddComponent<StaticMeshComponent>(model);

							// Calculate mouse pixel position
							auto [mx, my] = ImGui::GetMousePos();
							mx -= m_ViewportBounds[0].x;
							my -= m_ViewportBounds[0].y;
							glm::vec2 viewportSize = m_ViewportBounds[1] - m_ViewportBounds[0];
							my = viewportSize.y - my;
							int mouseX = (int)mx;
							int mouseY = (int)my;

							// Read depth from framebuffer
							m_Framebuffer->Bind();
							float depth = m_Framebuffer->ReadDepthPixel(mouseX, mouseY);
							m_Framebuffer->Unbind();

							if (depth == 1.0f)
								depth = 0.99f;

							// Get position from depth
							float z = depth * 2.0f - 1.0f;
							glm::vec2 texCoords = glm::vec2(mouseX, mouseY) / m_ViewportSize;
							glm::vec4 clipSpacePosition = glm::vec4(texCoords * 2.0f - 1.0f, z, 1.0f);
							glm::vec4 viewSpacePosition = glm::inverse(m_EditorCamera.GetViewProjection()) * clipSpacePosition;
							// Perspective division
							viewSpacePosition /= viewSpacePosition.w;

							// Update entity translation and selection
							entity.GetComponent<TransformComponent>().Translation = glm::vec3(viewSpacePosition);
							m_SceneHierarchyPanel.SelectedEntity(entity);
						}
					}
				}

				ImGui::EndDragDropTarget();
			}

			if (Preferences::GetData().ShowViewportUI)
			{

				ImGui::SetCursorPos(ImGui::GetWindowContentRegionMin() + ImVec2(5.0f, 5.0f));

				// Viewport settings dropdown
				{
					ImGui::SetNextItemWidth(ImGui::CalcTextSize(" Viewport Settings").x * 1.5f + style.FramePadding.x * 2.0f + 10.0f);
					ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.0f, 5.0f));
					ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.5f, 7.5f));
					bool open = ImGui::BeginCombo("##ViewportSettings", CHARACTER_ICON_PREFERENCES " Viewport Settings");
					ImGui::PopStyleVar();
					if (open)
					{
						if (ImGui::BeginMenu("Frame Step"))
						{
							ImGui::Text("Frame Count");
							ImGui::SameLine();
							ImGui::SetNextItemWidth(25.0f);
							ImGui::DragInt("##FrameStepCountInput", &Preferences::GetData().FrameStepCount, 0.25f, 1, 10, "%d", ImGuiSliderFlags_ClampOnInput);
							ImGui::EndMenu();
						}

						if (ImGui::BeginMenu("Camera View"))
						{
							if (ImGui::BeginMenu("FOV"))
							{
								float fov = m_EditorCamera.GetFOV();
								if (ImGui::DragFloat("##EditorCameraFOVInput", &fov, 0.25f, 1.0f, 180.0f, "%.2f", ImGuiSliderFlags_ClampOnInput))
									m_EditorCamera.SetFOV(fov);
								ImGui::EndMenu();
							}

							if (ImGui::BeginMenu("Near Clip"))
							{
								float nearClip = m_EditorCamera.GetNearClip();
								if (ImGui::DragFloat("##EditorCameraNearClipInput", &nearClip, 5.0f, 0.001f, 100000.0f, "%.2f", ImGuiSliderFlags_ClampOnInput | ImGuiSliderFlags_Logarithmic))
									m_EditorCamera.SetNearClip(nearClip);
								ImGui::EndMenu();
							}

							if (ImGui::BeginMenu("Far Clip"))
							{
								float farClip = m_EditorCamera.GetFarClip();
								if (ImGui::DragFloat("##EditorCameraFarClipInput", &farClip, 5.0f, 0.001f, 100000.0f, "%.2f", ImGuiSliderFlags_ClampOnInput | ImGuiSliderFlags_Logarithmic))
									m_EditorCamera.SetFarClip(farClip);
								ImGui::EndMenu();
							}

							ImGui::EndMenu();
						}

						ImGui::MenuItem("Show FPS", nullptr, &Preferences::GetData().ShowFPS);

						if (ImGui::MenuItem("Create Camera Here"))
						{
							auto entity = m_ActiveScene->CreateEntity("Camera");
							
							auto& camera = entity.AddComponent<CameraComponent>().Camera;
							camera.SetPerspectiveNearClip(m_EditorCamera.GetNearClip());
							camera.SetPerspectiveFarClip(m_EditorCamera.GetFarClip());
							camera.SetPerspectiveVerticalFOV(glm::radians(m_EditorCamera.GetFOV()));

							auto& transform = entity.GetComponent<TransformComponent>();
							transform.Translation = m_EditorCamera.GetPosition();
							transform.Rotation = glm::vec3(m_EditorCamera.GetPitch() * -1.0f, m_EditorCamera.GetYaw() * -1.0f, 0.0f);
						}

						ImGui::MenuItem("Game View");

						if (ImGui::BeginMenu("Bookmarks"))
						{
							for (uint32_t i = 0; i < 9; i++)
							{
								if (Preferences::GetData().ViewportBookmarks[i] == glm::vec3())
									continue;

								ImGui::PushID(i);
								if (ImGui::MenuItem("##GoToBookmark"))
									m_EditorCamera.SetFocalPoint(Preferences::GetData().ViewportBookmarks[i]);
								ImGui::SameLine(style.FramePadding.x * 2.0f);
								ImGui::Text("Jump to Bookmark %d", i);
								ImGui::PopID();
							}

							if (ImGui::BeginMenu("Set Bookmark"))
							{
								for (uint32_t i = 0; i < 9; i++)
								{
									ImGui::PushID(i);
									if (ImGui::MenuItem("##BookmarkButton"))
										Preferences::GetData().ViewportBookmarks[i] = m_EditorCamera.GetPosition();
									ImGui::SameLine(style.FramePadding.x * 2.0f);
									ImGui::Text("Bookmark %d", i);
									ImGui::PopID();
								}

								
								ImGui::EndMenu();
							}

							if (ImGui::MenuItem("Clear Bookmarks"))
								for (uint32_t i = 0; i < 9; i++)
									Preferences::GetData().ViewportBookmarks[i] = glm::vec3();

							ImGui::EndMenu();
						}

						if (ImGui::MenuItem("Save Screenshot"))
						{
							std::filesystem::path screenshotDirectory = Project::GetProjectDirectory() / "Saved" / "Screenshots";
							if (!std::filesystem::exists(screenshotDirectory))
								std::filesystem::create_directory(screenshotDirectory);

							char filename[256];
							time_t time = std::time(0);
							strftime(filename, sizeof(filename), "Screenshot %Y-%m-%d %H%M%S.png", localtime(&time));

							auto& spec = m_Framebuffer->GetSpecification();
							size_t size = spec.Width * spec.Height * 4;
							float* raw = new float[size];

							m_Framebuffer->Bind();
							m_Framebuffer->ReadPixels(0, 1, 1, spec.Width, spec.Height, raw);
							m_Framebuffer->Unbind();

							unsigned char* data = new unsigned char[size];
							for (uint32_t i = 0; i < size; i++)
								data[i] = raw[i] * 255;

							std::string filepath = (screenshotDirectory / filename).lexically_normal().string();

							stbi_flip_vertically_on_write(true);
							stbi_write_png(filepath.c_str(), spec.Width, spec.Height, 4, data, spec.Width * 4);
							delete[] raw;
							
							// Open popup to display screenshot
							{
								Ref<Texture2D> texture = Texture2D::Create(filepath);

								// Calculate image area size
								float aspectRatio = texture->GetWidth() / texture->GetHeight();
								float width = std::min((float)texture->GetWidth(), 300.0f);
								ImVec2 size = ImVec2(width, width/aspectRatio);

								Popup::Create("Screenshot Saved", std::string("Screenshot save to ") + filepath,
									{
										{ "Ok", []() {} },
										{ "Show In Explorer", [=]() { m_ContentBrowserPanel.OpenExternally(std::filesystem::path(filepath).parent_path()); } },
										{ "Open", [=]() { m_ContentBrowserPanel.OpenExternally(filepath); } }
									},
									nullptr, false,
									[texture, size]() {
										ImGui::Dummy({ (ImGui::GetContentRegionAvailWidth() - size.x) * 0.5f, 0.0f });
										ImGui::SameLine();
										ImGui::Image((ImTextureID)(uint64_t)(texture->GetRendererID()), size, { 0, 1 }, { 1, 0 });
									},
									glm::vec2(size.x, size.y)
								);
							}
						}

						ImGui::EndCombo();
					}
					ImGui::PopStyleVar();

					// Draw Frame Counter
					if (Preferences::GetData().ShowFPS)
					{
						auto drawList = ImGui::GetWindowDrawList();
						char buff[256];

						const float& dt = m_DeltaTime;
						const float& fps = 1.0f / m_DeltaTime;

						auto color = ImGui::GetColorU32(fps > 55.0f ? (ImVec4(0.1f, 0.8f, 0.2f, 1.0f)) : (fps > 25.0f ? (ImVec4(1.0f, 0.95f, 0.85f, 1.0f)) : (ImVec4(0.8f, 0.1f, 0.2f, 1.0f))));

						sprintf(buff, "%.2f FPS", fps);
						drawList->AddText(ImVec2(ImGui::GetWindowPos().x + style.FramePadding.x, ImGui::GetItemRectMax().y + style.FramePadding.y), color, buff);

						memset(buff, 0, 256);

						sprintf(buff, "%.2f ms", dt);
						drawList->AddText(ImVec2(ImGui::GetWindowPos().x + style.FramePadding.x, ImGui::GetItemRectMax().y + style.FramePadding.y * 3.0f + ImGui::GetTextLineHeight()), color, buff);
					}
				}

				ImGui::SameLine();

				// Viewport Render Settings
				{
					const char* visualizationModeNames[] = 
					{
						"Rendered", "Wireframe", "Lighting Only", "Pre Post Processing",
						"Albedo", "Depth", "Object ID", "Normal", "Emissive", "Roughness", "Metallic", "Specular", "Ambient Occlusion"
					};

					SceneRenderer::RendererVisualizationMode visualizationMode = SceneRenderer::GetVisualizationMode();

					ImGui::SetNextItemWidth(150.0f + style.FramePadding.x * 2.0f + 10.0f);
					ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.0f, 5.0f));
					ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.5f, 7.5f));
					bool open = ImGui::BeginCombo("##ViewportRenderSettings", (std::string(CHARACTER_ICON_SHADING_RENDERED) + visualizationModeNames[(int)visualizationMode]).c_str(), ImGuiComboFlags_HeightLargest);
					ImGui::PopStyleVar();
					if (open)
					{
						ImGui::TextDisabled("View Modes");
						if (ImGui::MenuItem(CHARACTER_ICON_SHADING_RENDERED " Rendered")) SetRendererVisualizationMode(SceneRenderer::RendererVisualizationMode::Rendered);
						if (ImGui::MenuItem(CHARACTER_ICON_SHADING_WIREFRAME " Wireframe")) SetRendererVisualizationMode(SceneRenderer::RendererVisualizationMode::Wireframe);
						if (ImGui::MenuItem(CHARACTER_ICON_SHADING_SOLID " Lighting Only")) SetRendererVisualizationMode(SceneRenderer::RendererVisualizationMode::LightingOnly);
						if (ImGui::MenuItem(CHARACTER_ICON_SHADING_UNLIT " Pre Post Processing")) SetRendererVisualizationMode(SceneRenderer::RendererVisualizationMode::PrePostProcessing);

						ImGui::Separator();

						ImGui::TextDisabled("Buffer Visualization");
						if (ImGui::MenuItem(CHARACTER_ICON_MEMORY " Albedo")) SetRendererVisualizationMode(SceneRenderer::RendererVisualizationMode::Albedo);
						if (ImGui::MenuItem(CHARACTER_ICON_MEMORY " Depth")) SetRendererVisualizationMode(SceneRenderer::RendererVisualizationMode::Depth);
						if (ImGui::MenuItem(CHARACTER_ICON_MEMORY " Object ID")) SetRendererVisualizationMode(SceneRenderer::RendererVisualizationMode::EntityID);
						if (ImGui::MenuItem(CHARACTER_ICON_MEMORY " Normal")) SetRendererVisualizationMode(SceneRenderer::RendererVisualizationMode::Normal);
						if (ImGui::MenuItem(CHARACTER_ICON_MEMORY " Emissive")) SetRendererVisualizationMode(SceneRenderer::RendererVisualizationMode::Emissive);
						if (ImGui::MenuItem(CHARACTER_ICON_MEMORY " Roughness")) SetRendererVisualizationMode(SceneRenderer::RendererVisualizationMode::Roughness);
						if (ImGui::MenuItem(CHARACTER_ICON_MEMORY " Metallic")) SetRendererVisualizationMode(SceneRenderer::RendererVisualizationMode::Metallic);
						if (ImGui::MenuItem(CHARACTER_ICON_MEMORY " Specular")) SetRendererVisualizationMode(SceneRenderer::RendererVisualizationMode::Specular);
						if (ImGui::MenuItem(CHARACTER_ICON_MEMORY " Ambient Occlusion")) SetRendererVisualizationMode(SceneRenderer::RendererVisualizationMode::AmbientOcclusion);

						ImGui::EndCombo();
					}
					ImGui::PopStyleVar();
				}

				ImGui::SameLine();
				ImGui::Dummy(ImVec2{ ImGui::GetContentRegionAvail().x - 480, 0 });
				ImGui::SameLine();

				// Viewport gizmo settings
				{
					const char* gizmo_type_items[4] = { CHARACTER_ICON_GIZMO_CURSOR, CHARACTER_ICON_GIZMO_TRANSLATE, CHARACTER_ICON_GIZMO_ROTATE, CHARACTER_ICON_GIZMO_SCALE };
					int currentValue = (int)(m_GizmoOperation)+1;
					if (ImGui::SwitchButtonEx("##GizmoTypeSwitch", gizmo_type_items, 4, &currentValue, ImVec2(120, 30)))
						m_GizmoOperation = currentValue - 1;

					ImGui::SameLine();

					if (ImGui::Button(m_GizmoMode == 0 ? CHARACTER_ICON_SPACE_LOCAL : CHARACTER_ICON_SPACE_WORLD, ImVec2(30, 30)))
						m_GizmoMode = !m_GizmoMode;

					ImGui::SameLine();

					{


						std::string number = String::FloatToString(m_TranslationSnapValue);
						const char* pchar = number.c_str();
						const char** items = new const char* [2] { CHARACTER_ICON_SNAP_TRANSLATION, pchar };
						int currentTranslationValue = m_TranslationSnap ? 0 : -1;
						if (ImGui::SwitchButtonEx("##TranslationSnapEnabledSwitch", items, 2, &currentTranslationValue, ImVec2(60, 30)))
						{
							if (currentTranslationValue == 1)
								ImGui::OpenPopup("TranslationSnapLevel");
							else if (currentTranslationValue == 0)
								m_TranslationSnap = !m_TranslationSnap;
						}
						delete[] items;
					}

					ImGui::SameLine();

					{
						std::string number = String::FloatToString(m_RotationSnapValue) + std::string(CHARACTER_SYMBOL_DEGREE);
						const char* pchar = number.c_str();
						const char** items = new const char* [2] { CHARACTER_ICON_SNAP_ROTATION, pchar };
						int currentRotationValue = m_RotationSnap ? 0 : -1;
						if (ImGui::SwitchButtonEx("##RotationSnapEnabledSwitch", items, 2, &currentRotationValue, ImVec2(60, 30)))
						{
							if (currentRotationValue == 1)
								ImGui::OpenPopup("RotationSnapLevel");
							else if (currentRotationValue == 0)
								m_RotationSnap = !m_RotationSnap;
						}
						delete[] items;
					}

					ImGui::SameLine();

					{
						std::string number = String::FloatToString(m_ScaleSnapValue);
						const char* pchar = number.c_str();
						const char** items = new const char* [2] { CHARACTER_ICON_SNAP_SCALING, pchar };
						int currentScalingValue = m_ScaleSnap ? 0 : -1;
						if (ImGui::SwitchButtonEx("##ScalingSnapEnabledSwitch", items, 2, &currentScalingValue, ImVec2(60, 30)))
						{
							if (currentScalingValue == 1)
								ImGui::OpenPopup("ScaleSnapLevel");
							else if (currentScalingValue == 0)
								m_ScaleSnap = !m_ScaleSnap;
						}
						delete[] items;
					}

					//Snapping Setter Events
					{
						if (ImGui::BeginPopup("TranslationSnapLevel"))
						{
							if (ImGui::MenuItem("0.01")) m_TranslationSnapValue = 0.01f;
							if (ImGui::MenuItem("0.05")) m_TranslationSnapValue = 0.05f;
							if (ImGui::MenuItem("0.1")) m_TranslationSnapValue = 0.1f;
							if (ImGui::MenuItem("0.5")) m_TranslationSnapValue = 0.5f;
							if (ImGui::MenuItem("1")) m_TranslationSnapValue = 1.0f;
							if (ImGui::MenuItem("5")) m_TranslationSnapValue = 5.0f;
							if (ImGui::MenuItem("10")) m_TranslationSnapValue = 10.0f;
							if (ImGui::MenuItem("50")) m_TranslationSnapValue = 50.0f;
							if (ImGui::MenuItem("100")) m_TranslationSnapValue = 100.0f;
							ImGui::EndPopup();
						}

						if (ImGui::BeginPopup("RotationSnapLevel"))
						{
							if (ImGui::MenuItem("1" CHARACTER_SYMBOL_DEGREE)) m_RotationSnap = 1.0f;
							if (ImGui::MenuItem("5" CHARACTER_SYMBOL_DEGREE)) m_RotationSnap = 5.0f;
							if (ImGui::MenuItem("10" CHARACTER_SYMBOL_DEGREE)) m_RotationSnap = 10.0f;
							if (ImGui::MenuItem("15" CHARACTER_SYMBOL_DEGREE)) m_RotationSnap = 15.0f;
							if (ImGui::MenuItem("30" CHARACTER_SYMBOL_DEGREE)) m_RotationSnap = 30.0f;
							if (ImGui::MenuItem("45" CHARACTER_SYMBOL_DEGREE)) m_RotationSnap = 45.0f;
							if (ImGui::MenuItem("60" CHARACTER_SYMBOL_DEGREE)) m_RotationSnap = 60.0f;
							if (ImGui::MenuItem("90" CHARACTER_SYMBOL_DEGREE)) m_RotationSnap = 90.0f;
							if (ImGui::MenuItem("120" CHARACTER_SYMBOL_DEGREE)) m_RotationSnap = 120.0f;
							if (ImGui::MenuItem("180" CHARACTER_SYMBOL_DEGREE)) m_RotationSnap = 180.0f;
							ImGui::Separator();
							if (ImGui::MenuItem(CHARACTER_SYMBOL_PI)) m_RotationSnap = 3.14159265358979323846;
							
							ImGui::EndPopup();
						}

						if (ImGui::BeginPopup("ScaleSnapLevel"))
						{
							if (ImGui::MenuItem("0.1")) m_ScaleSnapValue = 0.1f;
							if (ImGui::MenuItem("0.25")) m_ScaleSnapValue = 0.25f;
							if (ImGui::MenuItem("0.5")) m_ScaleSnapValue = 0.5f;
							if (ImGui::MenuItem("1")) m_ScaleSnapValue = 1.0f;
							if (ImGui::MenuItem("5")) m_ScaleSnapValue = 5.0f;
							if (ImGui::MenuItem("10")) m_ScaleSnapValue = 10.0f;
							ImGui::EndPopup();
						}
					}

					ImGui::SameLine();

					if (ImGui::Button((CHARACTER_ICON_CAMERA + std::to_string(m_CameraSpeedScale)).c_str(), ImVec2(50, 30)))
						ImGui::OpenPopup("##CameraSpeedPopup");

					ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.0f, 5.0f));
					if (ImGui::BeginPopup("##CameraSpeedPopup", ImGuiWindowFlags_NoMove))
					{
						ImGui::Text("Speed Scale");
						if (ImGui::SliderInt("##CameraSpeedScaleInput", &m_CameraSpeedScale, 1, 8))
							m_EditorCamera.SetMoveSpeed(m_CameraBaseSpeed * m_CameraSpeedScale);

						ImGui::Separator();

						ImGui::Text("Base Speed (m/s)");
						ImGui::SameLine();
						ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());
						if (ImGui::DragFloat("##CameraBaseSpeedInput", &m_CameraBaseSpeed, 0.1f, 0.001f))
						{
							if (m_CameraBaseSpeed < 0.001f)
								m_CameraBaseSpeed = 0.001f;
							m_EditorCamera.SetMoveSpeed(m_CameraBaseSpeed * m_CameraSpeedScale);
						}

						ImGui::Text("Smoothing Time");
						ImGui::SameLine();
						ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());
						float smoothingTime = m_EditorCamera.GetSmoothingTime();
						if (ImGui::DragFloat("##CameraSmoothingTimeInput", &smoothingTime, 0.025f, 0.0f, 1.0f))
						{
							if (smoothingTime < 0.0f)
								smoothingTime = 0.0f;
							m_EditorCamera.SetSmoothingTime(smoothingTime);
						}

						if (ImGui::BeginMenu("Camera Type"))
						{
							if (ImGui::MenuItem("First Person"))
							{
								m_EditorCamera.SetOrbitalEnabled(false);
								m_EditorCamera.SetFirstPersonEnabled(true);
							}
							if (ImGui::MenuItem("Orbital"))
							{
								m_EditorCamera.SetOrbitalEnabled(true);
								m_EditorCamera.SetFirstPersonEnabled(false);
							}
							if (ImGui::MenuItem("Hybrid"))
							{
								m_EditorCamera.SetOrbitalEnabled(true);
								m_EditorCamera.SetFirstPersonEnabled(true);
							}
								
							ImGui::EndMenu();
						}
						
						ImGui::EndPopup();
					}
					ImGui::PopStyleVar();

					ImGui::SameLine();

					if (ImGui::Button(CHARACTER_ICON_PREFERENCES, ImVec2(40, 30)))
						ImGui::OpenPopup("##GizmoSettingsPopup");

					ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.0f, 5.0f));
					if (ImGui::BeginPopup("##GizmoSettingsPopup", ImGuiWindowFlags_NoMove))
					{
						ImGui::TextDisabled("Gizmo Settings");
						
						ImGui::Separator();

						if (ImGui::BeginMenu("Gizmo Operation"))
						{
							if (ImGui::MenuItem("None", nullptr, m_GizmoOperation == -1))
								m_GizmoOperation = -1;
							if (ImGui::MenuItem("Translate", nullptr, m_GizmoOperation == ImGuizmo::OPERATION::TRANSLATE))
								m_GizmoOperation = ImGuizmo::OPERATION::TRANSLATE;
							if (ImGui::MenuItem("Rotate", nullptr, m_GizmoOperation == ImGuizmo::OPERATION::ROTATE))
								m_GizmoOperation = ImGuizmo::OPERATION::ROTATE;
							if (ImGui::MenuItem("Scale", nullptr, m_GizmoOperation == ImGuizmo::OPERATION::SCALE))
								m_GizmoOperation = ImGuizmo::OPERATION::SCALE;

							ImGui::EndMenu();
						}

						if (ImGui::BeginMenu("Gizmo Type"))
						{
							if (ImGui::MenuItem("Local", nullptr, m_GizmoMode == ImGuizmo::MODE::LOCAL))
								m_GizmoMode = ImGuizmo::MODE::LOCAL;
							if (ImGui::MenuItem("World", nullptr, m_GizmoMode == ImGuizmo::MODE::WORLD))
								m_GizmoMode = ImGuizmo::MODE::WORLD;

							ImGui::EndMenu();
						}
						
						if (ImGui::BeginMenu("Pivot Point"))
						{
							if (ImGui::MenuItem("Median Point", nullptr, m_GizmoPivotPoint == GizmoPivotPoint::MedianPoint))
								m_GizmoPivotPoint = GizmoPivotPoint::MedianPoint;
							if (ImGui::MenuItem("Individual Origins", nullptr, m_GizmoPivotPoint == GizmoPivotPoint::IndividualOrigins))
								m_GizmoPivotPoint = GizmoPivotPoint::IndividualOrigins;
							if (ImGui::MenuItem("Active Element", nullptr, m_GizmoPivotPoint == GizmoPivotPoint::ActiveElement))
								m_GizmoPivotPoint = GizmoPivotPoint::ActiveElement;

							ImGui::EndMenu();
						}

						ImGui::EndPopup();
					}
					ImGui::PopStyleVar();
				}

				//Scene View Gizmo
				if (m_SceneState != SceneState::Play)
				{
					float camYAngle = Math::NormalizeAngle(glm::degrees(m_EditorCamera.GetPitch()), -180) * -1;
					float camXAngle = Math::NormalizeAngle(glm::degrees(m_EditorCamera.GetYaw()), -180);

					ImU32 hoveredColor = ImGui::ColorConvertFloat4ToU32(ImVec4{ 1.0f, 1.0f, 1.0f, 1.0f });
					ImU32 fontColor = ImGui::ColorConvertFloat4ToU32(ImVec4{ 0.0f, 0.0f, 0.0f, 1.0f });
					ImU32 xCol = ImGui::ColorConvertFloat4ToU32(ImVec4{ 1.0f, 0.21f, 0.33f, 1.0f });
					ImU32 xColDisabled = ImGui::ColorConvertFloat4ToU32(ImVec4{ 0.6f, 0.22f, 0.28f, 1.0f });
					ImU32 yCol = ImGui::ColorConvertFloat4ToU32(ImVec4{ 0.17f, 0.56f, 1.0f, 1.0f });
					ImU32 yColDisabled = ImGui::ColorConvertFloat4ToU32(ImVec4{ 0.2f, 0.39f, 0.6f, 1.0f });
					ImU32 zCol = ImGui::ColorConvertFloat4ToU32(ImVec4{ 0.54f, 0.86f, 0.0f, 1.0f });
					ImU32 zColDisabled = ImGui::ColorConvertFloat4ToU32(ImVec4{ 0.38f, 0.54f, 0.13f, 1.0f });
					float thicknessVal = 2.0f;
					float armLength = 40.0f;
					float circleRadius = 9.0f;
					ImVec2 pos = ImVec2(ImGui::GetWindowPos().x + (float)ImGui::GetWindowWidth() - (armLength + circleRadius) * 1.1f, ImGui::GetCursorScreenPos().y + (armLength + circleRadius) * 1.1f);

					ImVec2 xPos = pos;
					ImVec2 yPos = pos;
					ImVec2 zPos = pos;

					if (m_UpdateAngles) { m_EditorCamera.SetYaw(glm::radians(Math::LerpAngle(glm::degrees(m_EditorCamera.GetYaw()), m_YawUpdate, 0.3f))); m_EditorCamera.SetPitch(glm::radians(Math::LerpAngle(glm::degrees(m_EditorCamera.GetPitch()), m_PitchUpdate, 0.3f))); if (Math::FloatDistance(Math::NormalizeAngle(m_YawUpdate, -180), glm::degrees(m_EditorCamera.GetYaw())) < 0.1f && Math::FloatDistance(Math::NormalizeAngle(m_PitchUpdate, -180), glm::degrees(m_EditorCamera.GetPitch())) < 0.1f) { m_UpdateAngles = false; } }

					if (camXAngle >= 0) { xPos = ImVec2{ pos.x + (armLength * ((camXAngle - 90) / 90)), pos.y + (armLength * (((camXAngle < 90 ? (camXAngle) : ((camXAngle - 90) * -1 + 90)) / 90) * (camYAngle > 0 ? (camYAngle < 90 ? (camYAngle / 90 * -1) : (((camYAngle - 90) * -1 + 90) * -1 / 90)) : (camYAngle > -90 ? (camYAngle * -1 / 90) : ((((camYAngle + 90) * -1 - 90) * -1) / 90))))) }; }
					else if (camXAngle < 0) { xPos = ImVec2{ pos.x + (armLength * ((camXAngle * -1 - 90) / 90)), pos.y + (armLength * (((camXAngle > -90 ? (camXAngle) : ((camXAngle + 90) * -1 - 90)) / 90) * (camYAngle > 0 ? (camYAngle < 90 ? (camYAngle / 90 * -1) : (((camYAngle - 90) * -1 + 90) * -1 / 90)) : (camYAngle > -90 ? (camYAngle * -1 / 90) : ((((camYAngle + 90) * -1 - 90) * -1) / 90))))) }; }

					if (camXAngle >= 0) { yPos = ImVec2{ pos.x + (armLength * ((camXAngle > 90 ? ((camXAngle - 90) * -1 + 90) : (camXAngle)) / 90)), pos.y + (armLength * (((camXAngle - 90) / 90 * -1) * (camYAngle > 0 ? (camYAngle < 90 ? (camYAngle / 90 * -1) : (((camYAngle - 90) * -1 + 90) * -1 / 90)) : (camYAngle > -90 ? (camYAngle * -1 / 90) : ((((camYAngle + 90) * -1 - 90) * -1) / 90))))) }; }
					else if (camXAngle < 0) { yPos = ImVec2{ pos.x + (armLength * ((camXAngle < -90 ? ((camXAngle + 90) * -1 - 90) : (camXAngle)) / 90)), pos.y + (armLength * (((camXAngle + 90) * -1 / 90 * -1) * (camYAngle > 0 ? (camYAngle < 90 ? (camYAngle / 90 * -1) : (((camYAngle - 90) * -1 + 90) * -1 / 90)) : (camYAngle > -90 ? (camYAngle * -1 / 90) : ((((camYAngle + 90) * -1 - 90) * -1) / 90))))) }; }

					zPos = ImVec2{ zPos.x, pos.y + (armLength * (camYAngle > 0 ? ((camYAngle - 90) / 90) : ((camYAngle + 90) * -1 / 90))) };

					//ImGui Draw
					static bool activeCirclePress = false;
					static bool activeCircleHovered = false;

					ImGui::GetWindowDrawList()->AddCircleFilled(pos, armLength + circleRadius, activeCircleHovered ? ImGui::ColorConvertFloat4ToU32(ImVec4{ 0.5f, 0.5f, 0.5f, 0.5f }) : ImGui::ColorConvertFloat4ToU32(ImVec4{ 0.5f, 0.5f, 0.5f, 0.0f }));
					static bool hovered, held, hoveredB0, hoveredB1, hoveredB2, hoveredB3, hoveredB4, hoveredB5;
					if (sqrt(pow(ImGui::GetMousePos().y - pos.y, 2) + pow(ImGui::GetMousePos().x - pos.x, 2)) < (armLength + circleRadius) && !activeCirclePress && (hovered || hoveredB0 || hoveredB1 || hoveredB2 || hoveredB3 || hoveredB4 || hoveredB5))
					{
						activeCircleHovered = true;
						if (Input::IsMouseButtonPressed(Mouse::ButtonLeft)) 
						{ 
							m_EditorCamera.SetFreePan(true);
							activeCirclePress = true;
						}
					}
					else
						activeCircleHovered = false;

					if (activeCirclePress)
						activeCircleHovered = true;

					if (!Input::IsMouseButtonPressed(Mouse::ButtonLeft)) { m_EditorCamera.SetFreePan(false); activeCirclePress = false; }

					ImVec2 xPosAlt = xPos;
					ImVec2 yPosAlt = yPos;
					ImVec2 zPosAlt = zPos;

					auto ang = camXAngle < 0 ? (camXAngle * -1) : (camXAngle);

					float a[6] = { Math::NormalizeAngle(camXAngle + 90, -180), Math::NormalizeAngle(camXAngle - 90, -180), Math::NormalizeAngle(camXAngle + 180, -180), Math::NormalizeAngle(camXAngle - 0, -180), Math::NormalizeAngle((camYAngle + 90), -180) < 20 && Math::NormalizeAngle((camYAngle + 90), -180) > -20 ? (0) : (Math::NormalizeAngle((camYAngle + 90), -180)),  Math::NormalizeAngle((camYAngle - 90), -180) };

					bool circleOverlayCheck = true;
					while (circleOverlayCheck)
					{
						int highestValueIndex = 0;
						for (int i = 0; i < sizeof(a) / sizeof(a[0]); i++)
						{
							if (a[i] == -1000.0f) { if (i == highestValueIndex) { highestValueIndex++; } }
							else if ((a[i] < 0 ? (a[i] * -1) : (a[i])) > (a[highestValueIndex] < 0 ? (a[highestValueIndex] * -1) : (a[highestValueIndex]))) { highestValueIndex = i; }
						}

						a[highestValueIndex] = -1000.0f;

						bool inFront = false;
						if (highestValueIndex == 0 && a[1] == -1000.0f) { inFront = true; }
						else if (highestValueIndex == 1 && a[0] == -1000.0f) { inFront = true; }
						else if (highestValueIndex == 2 && a[3] == -1000.0f) { inFront = true; }
						else if (highestValueIndex == 3 && a[2] == -1000.0f) { inFront = true; }
						else if (highestValueIndex == 4 && a[5] == -1000.0f) { inFront = true; }
						else if (highestValueIndex == 5 && a[4] == -1000.0f) { inFront = true; }


						if (highestValueIndex == 0)
						{
							xPos = ImVec2{ (xPos.x - pos.x) * -1 + pos.x, (xPos.y - pos.y) * -1 + pos.y };

							ImRect bb(ImVec2{ xPos.x - circleRadius, xPos.y - circleRadius }, ImVec2{ xPos.x + circleRadius, xPos.y + circleRadius });
							ImGuiID id = ImGui::GetCurrentWindow()->GetID("IdsMainX");
							bool held;
							bool pressed = ImGui::ButtonBehavior(bb, id, &hoveredB0, &held);

							if (pressed)
							{
								m_YawUpdate = floor(glm::degrees(m_EditorCamera.GetYaw()) / 360) * 360 - 90.0f;
								m_PitchUpdate = floor(glm::degrees(m_EditorCamera.GetYaw()) / 360) * 360;
								m_UpdateAngles = true;
							}

							ImGui::GetWindowDrawList()->AddLine(pos, xPos, hoveredB0 ? hoveredColor : (inFront ? xCol : xColDisabled), thicknessVal);
							ImGui::GetWindowDrawList()->AddCircleFilled(xPos, circleRadius, hoveredB0 ? hoveredColor : (inFront ? xCol : xColDisabled));
							ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
							ImGui::GetWindowDrawList()->AddText(ImVec2{ xPos.x - ImGui::CalcTextSize("W").x * 0.33f, xPos.y - ImGui::CalcTextSize("W").y / 2 }, fontColor, "X");
							ImGui::PopFont();
						}

						if (highestValueIndex == 1)
						{
							ImRect bb(ImVec2{ xPosAlt.x - circleRadius, xPosAlt.y - circleRadius }, ImVec2{ xPosAlt.x + circleRadius, xPosAlt.y + circleRadius });
							ImGuiID id = ImGui::GetCurrentWindow()->GetID("IdsSecondaryX");
							bool held;
							bool pressed = ImGui::ButtonBehavior(bb, id, &hoveredB1, &held);

							if (pressed)
							{
								m_YawUpdate = floor(glm::degrees(m_EditorCamera.GetYaw()) / 360) * 360 + 90.0f;
								m_PitchUpdate = floor(glm::degrees(m_EditorCamera.GetYaw()) / 360) * 360;
								m_UpdateAngles = true;
							}

							ImGui::GetWindowDrawList()->AddCircleFilled(xPosAlt, circleRadius, hoveredB1 ? hoveredColor : (inFront ? xCol : xColDisabled));
						}

						if (highestValueIndex == 2)
						{
							yPos = ImVec2{ (yPos.x - pos.x) * -1 + pos.x, (yPos.y - pos.y) * -1 + pos.y };

							ImRect bb(ImVec2{ yPos.x - circleRadius, yPos.y - circleRadius }, ImVec2{ yPos.x + circleRadius, yPos.y + circleRadius });
							ImGuiID id = ImGui::GetCurrentWindow()->GetID("IdsMainY");
							bool held;
							bool pressed = ImGui::ButtonBehavior(bb, id, &hoveredB2, &held);

							if (pressed)
							{
								m_YawUpdate = floor(glm::degrees(m_EditorCamera.GetYaw()) / 360) * 360 + 180.0f;
								m_PitchUpdate = floor(glm::degrees(m_EditorCamera.GetYaw()) / 360) * 360;
								m_UpdateAngles = true;
							}

							ImGui::GetWindowDrawList()->AddLine(pos, yPos, hoveredB2 ? hoveredColor : (inFront ? yCol : yColDisabled), thicknessVal);
							ImGui::GetWindowDrawList()->AddCircleFilled(yPos, circleRadius, hoveredB2 ? hoveredColor : (inFront ? yCol : yColDisabled));
							ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
							ImGui::GetWindowDrawList()->AddText(ImVec2{ yPos.x - ImGui::CalcTextSize("W").x * 0.33f, yPos.y - ImGui::CalcTextSize("W").y / 2 }, fontColor, "Z");
							ImGui::PopFont();
						}

						if (highestValueIndex == 3)
						{
							ImRect bb(ImVec2{ yPosAlt.x - circleRadius, yPosAlt.y - circleRadius }, ImVec2{ yPosAlt.x + circleRadius, yPosAlt.y + circleRadius });
							ImGuiID id = ImGui::GetCurrentWindow()->GetID("IdsSecondaryY");
							bool held;
							bool pressed = ImGui::ButtonBehavior(bb, id, &hoveredB3, &held);

							if (pressed)
							{
								m_YawUpdate = floor(glm::degrees(m_EditorCamera.GetYaw()) / 360) * 360;
								m_PitchUpdate = floor(glm::degrees(m_EditorCamera.GetYaw()) / 360) * 360;
								m_UpdateAngles = true;
							}
							ImGui::GetWindowDrawList()->AddCircleFilled(yPosAlt, circleRadius, hoveredB3 ? hoveredColor : (inFront ? yCol : yColDisabled));
						}

						if (highestValueIndex == 4)
						{
							ImRect bb(ImVec2{ zPos.x - circleRadius, zPos.y - circleRadius }, ImVec2{ zPos.x + circleRadius, zPos.y + circleRadius });
							ImGuiID id = ImGui::GetCurrentWindow()->GetID("IdsMainZ");
							bool held;
							bool pressed = ImGui::ButtonBehavior(bb, id, &hoveredB4, &held);

							if (pressed)
							{
								m_YawUpdate = floor(glm::degrees(m_EditorCamera.GetYaw()) / 360) * 360;
								m_PitchUpdate = floor(glm::degrees(m_EditorCamera.GetYaw()) / 360) * 360 + 90;
								m_UpdateAngles = true;
							}

							ImGui::GetWindowDrawList()->AddLine(pos, zPos, hoveredB4 ? hoveredColor : (inFront ? zCol : zColDisabled), thicknessVal);
							ImGui::GetWindowDrawList()->AddCircleFilled(zPos, circleRadius, hoveredB4 ? hoveredColor : (inFront ? zCol : zColDisabled));
							ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
							ImGui::GetWindowDrawList()->AddText(ImVec2{ zPos.x - ImGui::CalcTextSize("W").x * 0.33f, zPos.y - ImGui::CalcTextSize("W").y / 2 }, fontColor, "Y");
							ImGui::PopFont();
						}

						if (highestValueIndex == 5)
						{
							zPosAlt = ImVec2{ (zPosAlt.x - pos.x) * -1 + pos.x, (zPosAlt.y - pos.y) * -1 + pos.y };

							ImRect bb(ImVec2{ zPosAlt.x - circleRadius, zPosAlt.y - circleRadius }, ImVec2{ zPosAlt.x + circleRadius, zPosAlt.y + circleRadius });
							ImGuiID id = ImGui::GetCurrentWindow()->GetID("IdsSecondaryZ");
							bool held;
							bool pressed = ImGui::ButtonBehavior(bb, id, &hoveredB5, &held);

							if (pressed)
							{
								m_YawUpdate = floor(glm::degrees(m_EditorCamera.GetYaw()) / 360) * 360;
								m_PitchUpdate = floor(glm::degrees(m_EditorCamera.GetYaw()) / 360) * 360 - 90;
								m_UpdateAngles = true;
							}

							ImGui::GetWindowDrawList()->AddCircleFilled(zPosAlt, circleRadius, hoveredB5 ? hoveredColor : (inFront ? zCol : zColDisabled));
						}

						bool checkValSuccess = false;
						for (int checkVal : a)
							if (checkVal != -1000.0f) { checkValSuccess = true; }
						if (!checkValSuccess) { circleOverlayCheck = false; break; }

					}
					ImRect bb = ImRect(ImVec2(pos.x - (armLength + circleRadius), pos.y - (armLength + circleRadius)), ImVec2(pos.x + (armLength + circleRadius), pos.y + (armLength + circleRadius)));
					ImGuiID id = ImGui::GetID("##ViewGizmoHoverButtonID");
					ImGui::ButtonBehavior(bb, id, &hovered, &held);
					ImGui::ItemSize(ImVec2((armLength + circleRadius) * 2, (armLength + circleRadius) * 2), style.FramePadding.y);
				}
			}

			// Viewport Command Line
			{
				static float offset = 0.0f;
				bool setFocus = false;

				if ((float)m_ViewportCommandLineOpen != offset)
				{
					if (m_ViewportCommandLineOpen)
					{
						offset = std::min(offset + m_DeltaTime * 5.0f, 1.0f);
						if (offset == 1.0f)
							setFocus = true;
					}
					else
						offset = std::max(offset - m_DeltaTime * 5.0f, 0.0f);
				}

				if (offset != 0.0f)
				{
					const ImVec2 frameSize = ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetTextLineHeight() + style.FramePadding.y * 4.0f);
					auto& size = ImGui::GetWindowSize();

					ImGui::SetCursorPos(ImVec2(0.0f, size.y - frameSize.y * offset));

					// Draw Background
					{
						auto window = ImGui::GetCurrentWindow();

						const ImRect frame_bb(window->DC.CursorPos, ImVec2(window->DC.CursorPos.x + size.x, window->DC.CursorPos.y + size.y));
						ImGui::RenderFrame(frame_bb.Min, frame_bb.Max, ImGui::GetColorU32(ImGuiCol_FrameBg), true, style.FrameRounding);
					}

					ImGui::SetCursorPos(ImVec2(style.FramePadding.x, size.y - frameSize.y * offset + style.FramePadding.y));

					ImGui::SetNextItemWidth(150.0f);
					if (ImGui::BeginCombo("##CommandTypeSelection", m_ViewportCommandLineExecute == 0 ? "Dymatic Command" : (m_ViewportCommandLineExecute == 1 ? "Python Script" : "Python REPL")))
					{
						if (ImGui::MenuItem("Dymatic Command"))
							m_ViewportCommandLineExecute = 0;
						if (ImGui::MenuItem("Python Script"))
							m_ViewportCommandLineExecute = 1;
						if (ImGui::MenuItem("Python REPL"))
							m_ViewportCommandLineExecute = 2;
						ImGui::EndCombo();
					}

					ImGui::SameLine();

					ImGui::PushStyleColor(ImGuiCol_FrameBg, {});
					ImGui::PushStyleColor(ImGuiCol_FrameBgActive, {});
					ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, {});

					ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2());

					if (setFocus)
						ImGui::SetKeyboardFocusHere();

					char buffer[256];
					memset(buffer, 0, sizeof(buffer));
					std::strncpy(buffer, m_ViewportCommandLineBuffer.c_str(), sizeof(buffer));
					ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());
					if (ImGui::InputTextWithHint("##ViewportCommandLineInput", m_ViewportCommandLineExecute == 0 ? ">>> Enter Command:" : (m_ViewportCommandLineExecute == 1 ? ">>> Python Script Path:" : ">>> Python Command:"), buffer, sizeof(buffer),
						ImGuiInputTextFlags_CallbackCharFilter, [](ImGuiInputTextCallbackData* data) -> int
						{
							if (data->EventChar == '`')
								return 1;
							return 0;
						}
						))
						m_ViewportCommandLineBuffer = std::string(buffer);

					if (ImGui::IsItemDeactivated() && Input::IsKeyPressed(Key::Enter))
					{
						if (m_ViewportCommandLineExecute == 0)
							;
						else if (m_ViewportCommandLineExecute == 1)
						{
							PythonTools::RunScript(m_ViewportCommandLineBuffer);
						}
						else if (m_ViewportCommandLineExecute == 2)
						{
							PythonTools::RunGlobalCommand(m_ViewportCommandLineBuffer);
						}
						
						m_ViewportCommandLineBuffer.clear();
					}

					ImGui::PopStyleVar();
					ImGui::PopStyleColor(3);
				}
			}

			// Gizmos
			if (Preferences::GetData().ShowTransformGizmo)
			{
				Entity activeEntity = m_SceneHierarchyPanel.GetActiveEntity();

				if (activeEntity && m_GizmoOperation != -1 && activeEntity.HasComponent<TransformComponent>())
				{
					ImGuizmo::SetOrthographic(false);
					ImGuizmo::SetDrawlist();
					ImGuizmo::SetRect(m_ViewportBounds[0].x, m_ViewportBounds[0].y, m_ViewportBounds[1].x - m_ViewportBounds[0].x, m_ViewportBounds[1].y - m_ViewportBounds[0].y);

					const glm::mat4* cameraProjection;
					const glm::mat4* cameraView;

					if (m_SceneState == SceneState::Play)
					{
						// Scene Camera
						Entity cameraEntity = m_ActiveScene->GetPrimaryCameraEntity();
						const auto& camera = cameraEntity.GetComponent<CameraComponent>().Camera;
						cameraProjection = &camera.GetProjection();
						cameraView = &glm::inverse(cameraEntity.GetComponent<TransformComponent>().GetTransform());
					}
					else
					{
						//Editor camera
						cameraProjection = &m_EditorCamera.GetProjection();
						cameraView = &m_EditorCamera.GetViewMatrix();
					}

					// Entity Transform
					TransformComponent& activeTransformComponent = activeEntity.GetComponent<TransformComponent>();
					glm::mat4 modifiedTransform = m_ActiveScene->GetWorldTransform(activeEntity);

					//Snapping
					bool snap = m_GizmoOperation == ImGuizmo::OPERATION::TRANSLATE ? m_TranslationSnap : m_GizmoOperation == ImGuizmo::OPERATION::ROTATE ? m_RotationSnap : m_ScaleSnap;
					float snapValue = m_GizmoOperation == ImGuizmo::OPERATION::TRANSLATE ? m_TranslationSnapValue : m_GizmoOperation == ImGuizmo::OPERATION::ROTATE ? m_RotationSnapValue : m_ScaleSnapValue;

					float snapValues[3] = { snapValue, snapValue, snapValue };

					ImGuizmo::Manipulate(glm::value_ptr(*cameraView), glm::value_ptr(*cameraProjection),
						(ImGuizmo::OPERATION)m_GizmoOperation, (ImGuizmo::MODE)m_GizmoMode, glm::value_ptr(modifiedTransform),
						nullptr, ((Input::IsKeyPressed(Key::LeftControl) ? !snap : snap) ? snapValues : nullptr));

					if (ImGuizmo::IsUsing())
					{
						switch (m_GizmoPivotPoint)
						{
						case GizmoPivotPoint::MedianPoint:
						{
							glm::vec3 modifiedTranslation, modifiedRotation, modifiedScale;
							Math::DecomposeTransform(modifiedTransform, modifiedTranslation, modifiedRotation, modifiedScale);

							activeTransformComponent.Translation = modifiedTranslation;
							activeTransformComponent.Rotation = modifiedRotation;
							activeTransformComponent.Scale = modifiedScale;

							break;
						}
						case GizmoPivotPoint::IndividualOrigins:
						{
							glm::mat4 diff = glm::inverse(activeTransformComponent.GetTransform()) * modifiedTransform;

							// Update all selected entities' transform
							for (auto e : m_SceneHierarchyPanel.GetSelectedEntities())
							{
								Entity entity = { e, m_ActiveScene.get() };

								if (entity == activeEntity)
									continue;

								TransformComponent& tc = entity.GetComponent<TransformComponent>();

								glm::mat4 transform = tc.GetTransform() * diff;

								glm::vec3 translation, rotation, scale;
								Math::DecomposeTransform(transform, translation, rotation, scale);

								tc.Translation = translation;
								tc.Rotation = rotation;
								tc.Scale = scale;
							}

							// Update the active element's transform
							glm::vec3 modifiedTranslation, modifiedRotation, modifiedScale;
							Math::DecomposeTransform(modifiedTransform, modifiedTranslation, modifiedRotation, modifiedScale);

							activeTransformComponent.Translation = modifiedTranslation;
							activeTransformComponent.Rotation = modifiedRotation;
							activeTransformComponent.Scale = modifiedScale;

							break;
						}
						case GizmoPivotPoint::ActiveElement:
						{
							glm::vec3 pivotPoint = activeTransformComponent.Translation;

							glm::vec3 modifiedTranslation, modifiedRotation, modifiedScale;
							Math::DecomposeTransform(modifiedTransform, modifiedTranslation, modifiedRotation, modifiedScale);

							glm::vec3 deltaTranslation = modifiedTranslation - activeTransformComponent.Translation;
							glm::vec3 deltaRotation = modifiedRotation - activeTransformComponent.Rotation;
							glm::vec3 deltaScale = modifiedScale - activeTransformComponent.Scale;
							
							glm::mat4 diff = glm::inverse(activeTransformComponent.GetTransform()) * modifiedTransform;
							diff[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

							// Update all selected entities' transform
							for (auto e : m_SceneHierarchyPanel.GetSelectedEntities())
							{
								Entity entity = { e, m_ActiveScene.get() };

								if (entity == activeEntity)
									continue;

								TransformComponent& tc = entity.GetComponent<TransformComponent>();

								glm::mat4 transform = tc.GetTransform();
								transform *= glm::translate(glm::mat4(1.0f), pivotPoint);
								transform *= diff;
								transform *= glm::translate(glm::mat4(1.0f), -pivotPoint);

								glm::vec3 translation, rotation, scale;
								Math::DecomposeTransform(transform, translation, rotation, scale);

								tc.Translation = translation;
								tc.Rotation = rotation;
								tc.Scale = scale;
							}

							activeTransformComponent.Translation = modifiedTranslation;
							activeTransformComponent.Rotation = modifiedRotation;
							activeTransformComponent.Scale = modifiedScale;
							
							break;
						}
						}

						//glm::vec3 translation, rotation, scale;
						//Math::DecomposeTransform(m_ActiveScene->WorldToLocalTransform(activeEntity, transform), translation, rotation, scale);
						//
						//glm::vec3 deltaRotation = rotation - tc.Rotation;
						//tc.Translation = translation;
						//tc.Rotation += deltaRotation;
						//tc.Scale = scale;
					}
				}
			}

			ImGui::End();
			ImGui::PopStyleVar();
		}

		if (m_ShowSplash)
		{
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 20.0f);
			ImGui::Begin("Splash", nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);
			ImGui::SetWindowPos(ImVec2((io.DisplaySize.x - ImGui::GetWindowWidth()) * 0.5f + dockspaceWindowPosition.x, (io.DisplaySize.y - ImGui::GetWindowHeight()) * 0.5f + dockspaceWindowPosition.y));
			
			if (ImGui::IsWindowAppearing())
				ReloadAvailableWorkspaces();
			
			ImGui::Image((ImTextureID)m_DymaticSplash->GetRendererID(), ImVec2(488, 267), { 0, 1 }, { 1, 0 });
			ImGui::SameLine();
			const char* versionName = "Version 23.1.0";
			ImGui::SameLine( 488.0f -ImGui::CalcTextSize(versionName).x);
			ImGui::Text(versionName);

			ImGui::Columns(2);
			
			{
				ImGui::TextDisabled("Select a Workspace");
				ImGui::BeginChild("##WorkspacesList", ImVec2(0.0f, 100.0f));
				
				if (ImGui::MenuItem(CHARACTER_ICON_WORKSPACE " Default Workspace"))
					m_WorkspaceTarget = "saved/presets/workspaces/DefaultWorkspace.workspace";
				
				ImGui::Separator();

				for (auto& workspace : m_AvailableWorkspaces)
					if (ImGui::MenuItem(fmt::format(CHARACTER_ICON_WORKSPACE " {}", workspace.stem().string()).c_str()))
						m_WorkspaceTarget = workspace;
				
				ImGui::EndChild();
			}
			
			ImGui::NextColumn();
			
			{
				ImGui::TextDisabled("Select a Scene");
				ImGui::BeginChild("##RecentScenesList", ImVec2(0.0f, 100.0f));

				if (ImGui::MenuItem(CHARACTER_ICON_NEW_FILE " New Scene"))
					NewScene();

				ImGui::Separator();

				for (auto& file : ProjectSettings::GetData().RecentScenePaths)
					if (ImGui::MenuItem(((CHARACTER_ICON_OPEN_FILE " ") + file.stem().string()).c_str()))
						OpenScene(file);
				ImGui::EndChild();
			}
			
			ImGui::EndColumns();

			if (ImGui::IsWindowAppearing())
				ImGui::FocusWindow(ImGui::GetCurrentWindow());

			if (!ImGui::IsWindowFocused(ImGuiHoveredFlags_ChildWindows))
				m_ShowSplash = false;
			
			ImGui::End();
			ImGui::PopStyleVar(2);
		}	
		
		// Show debug log overlay
		{
			static bool s_DebugOpen = false;
			
			static bool pressedLastFrame = false;
			bool pressed = io.KeyCtrl && io.KeyShift && io.KeyAlt && Input::IsKeyPressed(Key::Z);
			if (pressed && !pressedLastFrame)
				s_DebugOpen = !s_DebugOpen;
			pressedLastFrame = pressed;

			if (s_DebugOpen)
			{
				ImGui::PushFont(io.Fonts->Fonts[1]);
				auto drawList = ImGui::GetForegroundDrawList();
				const float spacing = ImGui::GetTextLineHeight() + style.FramePadding.y * 2.0f;

				ImVec2 drawPos = ImVec2(ImGui::GetWindowPos().x + style.FramePadding.x, ImGui::GetWindowPos().y + ImGui::GetWindowSize().y - spacing);
				auto& messages = Log::GetMessages();
				for (uint32_t index = messages.size() - 1; index > 0; index--)
				{
					auto& message = messages[index];
				
					if (message.Level == 5 /*Critical*/)
					{
						ImVec2 size = ImGui::CalcTextSize(message.Text.c_str());
						drawList->AddRectFilled(
							ImVec2(drawPos.x - style.FramePadding.x, drawPos.y - style.FramePadding.y),
							ImVec2(drawPos.x + size.x + style.FramePadding.x, drawPos.y + size.y + style.FramePadding.y),
							ImGui::GetColorU32(ImGuiCol_LogCritical)
						);
					}

					drawList->AddText(drawPos,
						// Get log level color
						ImGui::GetColorU32(
							message.Level == 0 ? ImGuiCol_LogTrace :
							message.Level == 2 ? ImGuiCol_LogInfo :
							message.Level == 3 ? ImGuiCol_LogWarn :
							message.Level == 4 ? ImGuiCol_LogError :
							message.Level == 5 ? ImGuiCol_Text :
							ImGuiCol_TextDisabled
						),
						// Access log message text
						message.Text.c_str()
					);

					if (drawPos.y > ImGui::GetWindowPos().y)
						drawPos.y -= spacing;
					else
						break;
				}

				ImGui::PopFont();
			}

			
		}

		ImGui::End();
	}
	
	void EditorLayer::OnEvent(Event& e)
	{
		m_EditorCamera.OnEvent(e);

		m_NodeEditorPannel.OnEvent(e);
		m_PreferencesPannel.OnEvent(e);
		m_CurveEditor.OnEvent(e);
		m_ConsoleWindow.OnEvent(e);
		m_ImageEditor.OnEvent(e);
		m_TextEditor.OnEvent(e);

		EventDispatcher dispatcher(e);

		dispatcher.Dispatch<KeyPressedEvent>(DY_BIND_EVENT_FN(EditorLayer::OnKeyPressed));
		dispatcher.Dispatch<MouseButtonPressedEvent>(DY_BIND_EVENT_FN(EditorLayer::OnMouseButtonPressed));
		dispatcher.Dispatch<WindowDropEvent>(DY_BIND_EVENT_FN(EditorLayer::OnDropped));
		dispatcher.Dispatch<WindowDragEnterEvent>(DY_BIND_EVENT_FN(EditorLayer::OnDragEnter));
		dispatcher.Dispatch<WindowDragLeaveEvent>(DY_BIND_EVENT_FN(EditorLayer::OnDragLeave));
		dispatcher.Dispatch<WindowDragOverEvent>(DY_BIND_EVENT_FN(EditorLayer::OnDragOver));
		dispatcher.Dispatch<WindowCloseEvent>(DY_BIND_EVENT_FN(EditorLayer::OnClosed));
		dispatcher.Dispatch<GamepadConnectedEvent>(DY_BIND_EVENT_FN(EditorLayer::OnGamepadConnected));
		dispatcher.Dispatch<GamepadDisconnectedEvent>(DY_BIND_EVENT_FN(EditorLayer::OnGamepadDisconnected));
		dispatcher.Dispatch<GamepadButtonPressedEvent>(DY_BIND_EVENT_FN(EditorLayer::OnGamepadButtonPressed));
		dispatcher.Dispatch<GamepadButtonReleasedEvent>(DY_BIND_EVENT_FN(EditorLayer::OnGamepadButtonReleased));
		dispatcher.Dispatch<GamepadAxisMovedEvent>(DY_BIND_EVENT_FN(EditorLayer::OnGamepadAxisMoved));
	}

	void EditorLayer::UpdateKeymapEvents(std::vector<Preferences::Keymap::KeyBindEvent> events)
	{
		for (auto& event : events)
		{
			if (event == Preferences::Keymap::KeyBindEvent::INVALID_BIND)
				return;

			switch (event)
			{
			case Preferences::Keymap::INVALID_BIND: return;
			case Preferences::Keymap::NewSceneBind: NewScene(); break;
			case Preferences::Keymap::OpenSceneBind: OpenScene(); break;
			case Preferences::Keymap::SaveSceneBind: SaveScene(); break;
			case Preferences::Keymap::SaveSceneAsBind: SaveSceneAs(); break;
			case Preferences::Keymap::QuitBind: SaveAndExit(); break;
			case Preferences::Keymap::SelectObjectBind: { if (m_ViewportHovered && !ImGuizmo::IsUsing() && !ImGuizmo::IsOver() && !Input::IsKeyPressed(Key::LeftAlt) && m_SceneHierarchyPanel.GetPickingID() == 0) { m_SceneHierarchyPanel.SelectedEntity(m_HoveredEntity); } } break;
			case Preferences::Keymap::SceneStartBind: { if (m_SceneState == SceneState::Play) OnScenePause(); else OnScenePlay(); } break;
			case Preferences::Keymap::SceneSimulateBind: { if (m_SceneState == SceneState::Simulate) OnScenePause(); else OnSceneSimulate(); } break;
			case Preferences::Keymap::SceneStopBind: { if (m_SceneState != SceneState::Edit) OnSceneStop(); } break;
			case Preferences::Keymap::ReloadAssembly: { if (m_SceneState == SceneState::Edit) ScriptEngine::ReloadAssembly(); } break;
			case Preferences::Keymap::GizmoNoneBind: { if (ViewportKeyAllowed() && !ImGuizmo::IsUsing()) { m_GizmoOperation = -1; } } break;
			case Preferences::Keymap::GizmoTranslateBind: { if (ViewportKeyAllowed() && !ImGuizmo::IsUsing()) { m_GizmoOperation = ImGuizmo::OPERATION::TRANSLATE; } } break;
			case Preferences::Keymap::GizmoRotateBind: { if (ViewportKeyAllowed() && !ImGuizmo::IsUsing()) { m_GizmoOperation = ImGuizmo::OPERATION::ROTATE; } } break;
			case Preferences::Keymap::GizmoScaleBind: { if (ViewportKeyAllowed() && !ImGuizmo::IsUsing()) { m_GizmoOperation = ImGuizmo::OPERATION::SCALE; } } break;
			case Preferences::Keymap::CreateBind: { if (ViewportKeyAllowed()) { m_SceneHierarchyPanel.ShowCreateMenu(); } } break;
			case Preferences::Keymap::DuplicateBind: { if (ViewportKeyAllowed() && m_SceneHierarchyPanel.GetActiveEntity()) { m_SceneHierarchyPanel.DuplicateEntities(); } m_NodeEditorPannel.DuplicateNodes(); } break;
			case Preferences::Keymap::DeleteBind: { if (ViewportKeyAllowed() && m_SceneHierarchyPanel.GetActiveEntity()) { m_SceneHierarchyPanel.DeleteEntities(); } } break;
			case Preferences::Keymap::VisualizationRenderedBind: { if (ViewportKeyAllowed()) SetRendererVisualizationMode(SceneRenderer::RendererVisualizationMode::Rendered); } break;
			case Preferences::Keymap::VisualizationWireframeBind: { if (ViewportKeyAllowed()) SetRendererVisualizationMode(SceneRenderer::RendererVisualizationMode::Wireframe); } break;
			case Preferences::Keymap::VisualizationLightingOnlyBind: { if (ViewportKeyAllowed()) SetRendererVisualizationMode(SceneRenderer::RendererVisualizationMode::LightingOnly); } break;
			case Preferences::Keymap::VisualizationAlbedoBind: { if (ViewportKeyAllowed()) SetRendererVisualizationMode(SceneRenderer::RendererVisualizationMode::Albedo); } break;
			case Preferences::Keymap::VisualizationNormalBind: { if (ViewportKeyAllowed()) SetRendererVisualizationMode(SceneRenderer::RendererVisualizationMode::Normal); } break;
			case Preferences::Keymap::VisualizationEntityIDBind: { if (ViewportKeyAllowed()) SetRendererVisualizationMode(SceneRenderer::RendererVisualizationMode::EntityID); } break;
			case Preferences::Keymap::ToggleVisualizationBind: { if (ViewportKeyAllowed()) ToggleRendererVisualizationMode(); } break;
			case Preferences::Keymap::ViewFrontBind: { if (ViewportKeyAllowed()) { m_YawUpdate = floor(glm::degrees(m_EditorCamera.GetYaw()) / 360) * 360; m_PitchUpdate = floor(glm::degrees(m_EditorCamera.GetYaw()) / 360) * 360; m_UpdateAngles = true; m_EditorCamera.SetProjectionType(1); } } break;
			case Preferences::Keymap::ViewSideBind: { if (ViewportKeyAllowed()) { m_YawUpdate = floor(glm::degrees(m_EditorCamera.GetYaw()) / 360) * 360 - 90.0f; m_PitchUpdate = floor(glm::degrees(m_EditorCamera.GetYaw()) / 360) * 360; m_UpdateAngles = true; m_EditorCamera.SetProjectionType(1); } } break;
			case Preferences::Keymap::ViewTopBind: { if (ViewportKeyAllowed()) { m_YawUpdate = floor(glm::degrees(m_EditorCamera.GetYaw()) / 360) * 360; m_PitchUpdate = floor(glm::degrees(m_EditorCamera.GetYaw()) / 360) * 360 + 90; m_UpdateAngles = true; m_EditorCamera.SetProjectionType(1); } } break;
			case Preferences::Keymap::ViewFlipBind: { if (ViewportKeyAllowed()) { m_YawUpdate = glm::degrees(m_EditorCamera.GetYaw()) + 180.0f; m_PitchUpdate = glm::degrees(m_EditorCamera.GetPitch()); m_UpdateAngles = true; m_EditorCamera.SetProjectionType(1); } } break;
			case Preferences::Keymap::ViewProjectionBind: { if (ViewportKeyAllowed()) { m_ProjectionToggled = !m_EditorCamera.GetProjectionType(); m_EditorCamera.SetProjectionType(m_ProjectionToggled); } } break;
			case Preferences::Keymap::OpenCommandLineBind: { m_ViewportCommandLineOpen = !m_ViewportCommandLineOpen; } break;
			case Preferences::Keymap::ClosePopupBind: { Popup::RemoveTopmostPopup(); } break;
			case Preferences::Keymap::TextEditorDuplicate: { m_TextEditor.Duplicate(); } break;
			case Preferences::Keymap::TextEditorSwapLineUp: { m_TextEditor.SwapLineUp(); } break;
			case Preferences::Keymap::TextEditorSwapLineDown: { m_TextEditor.SwapLineDown(); } break;
			case Preferences::Keymap::TextEditorSwitchHeader: { m_TextEditor.SwitchCStyleHeader(); } break;
			}
		}

		// TODO: Move to on update
		if (Input::IsMouseButtonPressed(Mouse::ButtonLeft) && Input::IsKeyPressed(Key::LeftAlt)) { if (m_ProjectionToggled != m_EditorCamera.GetProjectionType()) { m_EditorCamera.SetProjectionType(m_ProjectionToggled); } }
	}

	bool EditorLayer::OnKeyPressed(KeyPressedEvent& e)
	{
		UpdateKeymapEvents(Preferences::Keymap::CheckKey(e));
		return false;
	}

	bool EditorLayer::OnMouseButtonPressed(MouseButtonPressedEvent& e)
	{
		UpdateKeymapEvents(Preferences::Keymap::CheckMouseButton(e));
		return false;
	}

	bool EditorLayer::OnDropped(WindowDropEvent& e)
	{
		auto& filepaths = e.GetFilepaths();
		m_ContentBrowserPanel.OnExternalFileDrop(filepaths);

		m_IsDragging = false;
		return false;
	}

	bool EditorLayer::OnDragEnter(WindowDragEnterEvent& e)
	{
		m_IsDragging = true;
		return false;
	}

	bool EditorLayer::OnDragLeave(WindowDragLeaveEvent& e)
	{
		m_IsDragging = false;
		return false;
	}

	bool EditorLayer::OnDragOver(WindowDragOverEvent& e)
	{
		return false;
	}

	bool EditorLayer::OnClosed(WindowCloseEvent& e)
	{
		auto& window = Application::Get().GetWindow();
		if (!window.IsWindowFocused())
			window.FocusWindow();

		if (window.IsWindowMinimized())
			window.RestoreWindow();

		SaveAndExit();
		return true;
	}

	bool EditorLayer::OnGamepadConnected(GamepadConnectedEvent& e)
	{
		Notification::Create("Gamepad Connected", fmt::format("Gamepad '{}' was connected at slot [{}]", Input::GetGamepadName(e.GetGamepad()), e.GetGamepad()), { { "Ok", []() {} } }, 10.0f);
		return true;
	}

	bool EditorLayer::OnGamepadDisconnected(GamepadDisconnectedEvent& e)
	{
		return false;
	}

	bool EditorLayer::OnGamepadButtonPressed(GamepadButtonPressedEvent& e)
	{
		return false;
	}

	bool EditorLayer::OnGamepadButtonReleased(GamepadButtonReleasedEvent& e)
	{
		return false;
	}

	bool EditorLayer::OnGamepadAxisMoved(GamepadAxisMovedEvent& e)
	{
		return false;
	}

	void EditorLayer::OnOverlayRender()
	{
		if (m_SceneState == SceneState::Play)
		{
			Entity camera = m_ActiveScene->GetPrimaryCameraEntity();
			if (!camera)
				return;

			Renderer2D::BeginScene(camera.GetComponent<CameraComponent>().Camera, camera.GetComponent<TransformComponent>().GetTransform());
		}
		else
		{
			Renderer2D::BeginScene(m_EditorCamera);
		}

		if (m_ShowPhysicsColliders)
		{
			// Box Colliders
			{
				auto view = m_ActiveScene->GetAllEntitiesWith<TransformComponent, BoxCollider2DComponent>();
				for (auto entity : view)
				{
					auto [tc, bc2d] = view.get<TransformComponent, BoxCollider2DComponent>(entity);

					glm::vec3 translation = tc.Translation + glm::vec3(bc2d.Offset, 0.001f);
					glm::vec3 scale = tc.Scale * glm::vec3(bc2d.Size * 2.0f, 1.0f);

					glm::mat4 transform = glm::translate(glm::mat4(1.0f), translation)
						* glm::rotate(glm::mat4(1.0f), tc.Rotation.z, glm::vec3(0.0f, 0.0f, 1.0f))
						* glm::scale(glm::mat4(1.0f), scale);

					Renderer2D::DrawRect(transform, glm::vec4(0, 1, 0, 1));
				}
			}

			// Circle Colliders
			{
				auto view = m_ActiveScene->GetAllEntitiesWith<TransformComponent, CircleCollider2DComponent>();
				for (auto entity : view)
				{
					auto [tc, cc2d] = view.get<TransformComponent, CircleCollider2DComponent>(entity);

					glm::vec3 translation = tc.Translation + glm::vec3(cc2d.Offset, 0.001f);
					glm::vec3 scale = tc.Scale * glm::vec3(cc2d.Radius * 2.0f);

					glm::mat4 transform = glm::translate(glm::mat4(1.0f), translation)
						* glm::scale(glm::mat4(1.0f), scale);

					Renderer2D::DrawCircle(transform, glm::vec4(0, 1, 0, 1), 0.01f);
				}
			}
		}

		// Draw selected entity outline
		auto& selectedEntities = m_SceneHierarchyPanel.GetSelectedEntities();
		for (auto e : selectedEntities)
		{
			Entity selectedEntity = { e, m_ActiveScene.get() };
			if (selectedEntity.HasComponent<SpriteRendererComponent>() || selectedEntity.HasComponent<CircleRendererComponent>())
			{
				TransformComponent transform = selectedEntity.GetComponent<TransformComponent>();
				Renderer2D::DrawRect(transform.GetTransform(), glm::vec4(0.82f, 0.62f, 0.13f, 1.0));
			}
		}

		Renderer2D::EndScene();
	}

	void EditorLayer::NewProject()
	{
		Project::New();
	}

	void EditorLayer::OpenProject(const std::filesystem::path& path)
	{
		if (!Project::GetActive())
			m_ShowSplash = Preferences::GetData().ShowSplashStartup;

		if (Project::Load(path))
		{
			AssetManager::Deserialize();
			ProjectSettings::Deserialize();

			ScriptEngine::ReloadAssembly(Project::GetScriptModulePath());
			
			auto startScenePath = Project::GetAssetFileSystemPath(Project::GetActive()->GetConfig().StartScene);
			OpenScene(startScenePath);
			m_ContentBrowserPanel.Init();
			m_ProjectLauncher.AddRecentProject(path);
			SourceControl::Connect();
		}
	}

	void EditorLayer::SaveProject()
	{
	}

	void EditorLayer::NewScene()
	{
		if (m_SceneState != SceneState::Edit)
			OnSceneStop();

		m_EditorScene = CreateRef<Scene>();
		m_SceneHierarchyPanel.SetContext(m_EditorScene);

		m_ActiveScene = m_EditorScene;
		m_EditorScenePath = std::filesystem::path();

		//Reset Scene time values
		m_LastSaveTime = 0;
		m_ProgramTime = 0;
		Notification::Clear();
	}

	void EditorLayer::AppendScene()
	{
		std::string filepath = FileDialogs::OpenFile("Dymatic Scene (*.dymatic)\0*.dymatic\0");
		if (!filepath.empty())
			AppendScene(filepath);
	}

	void EditorLayer::AppendScene(const std::filesystem::path& path)
	{
		if (path.extension().string() != ".dymatic")
		{
			DY_WARN("Could not load {0} - not a scene file", path.filename().string());
			return;
		}

		SceneSerializer serializer(m_EditorScene);
		serializer.Deserialize(path.string());
	}

	void EditorLayer::OpenScene()
	{
		std::string filepath = FileDialogs::OpenFile("Dymatic Scene (*.dymatic)\0*.dymatic\0");
		if (!filepath.empty())
			OpenScene(filepath);
	}

	void EditorLayer::OpenScene(const std::filesystem::path& path)
	{
		if (m_SceneState != SceneState::Edit)
			OnSceneStop();

		if (path.extension().string() != ".dymatic")
		{
			DY_WARN("Could not load {0} - not a scene file", path.filename().string());
			return;
		}

		Ref<Scene> newScene = CreateRef<Scene>();
		SceneSerializer serializer(newScene);
		if (serializer.Deserialize(path.string()))
		{
			m_EditorScene = newScene;
			m_SceneHierarchyPanel.SetContext(m_EditorScene);

			m_ActiveScene = m_EditorScene;
			m_EditorScenePath = path;

			ProjectSettings::AddRecentScenePath(m_EditorScenePath);

			//Reset Scene time values
			m_LastSaveTime = 0;
			m_ProgramTime = 0;
			Notification::Clear();
		}
	}

	bool EditorLayer::SaveScene()
	{
		if (m_EditorScenePath.empty())
			return SaveSceneAs();
		else
		{
			SerializeScene(m_EditorScene, m_EditorScenePath);
			return true;
		}
		return false;
	}

	bool EditorLayer::SaveSceneAs()
	{
		std::string filepath = FileDialogs::SaveFile("Dymatic Scene (*.dymatic)\0*.dymatic\0");
		if (!filepath.empty())
		{
			SerializeScene(m_EditorScene, filepath);
			m_EditorScenePath = filepath;

			ProjectSettings::AddRecentScenePath(m_EditorScenePath);
			
			return true;
		}
		return false;
	}

	void EditorLayer::SerializeScene(Ref<Scene> scene, const std::filesystem::path& path)
	{
		DY_CORE_ASSERT(!path.empty());

		SceneSerializer serializer(scene);
		serializer.Serialize(path.string());
		m_LastSaveTime = 0;

		// Update project screenshot
		{
			std::filesystem::path projectSavedDirectory = Project::GetProjectDirectory() / "Saved";
			if (!std::filesystem::exists(projectSavedDirectory))
				std::filesystem::create_directory(projectSavedDirectory);


			auto& spec = m_Framebuffer->GetSpecification();
			uint32_t width = std::min(spec.Width, spec.Height);
			
			uint32_t size = width * width * 4;
			float* raw = new float[size];

			m_Framebuffer->Bind();
			m_Framebuffer->ReadPixels(0, (spec.Width - width) / 2 + 1, (spec.Height - width) / 2 + 1, width, width, raw);
			m_Framebuffer->Unbind();

			unsigned char* data = new unsigned char[size];
			for (uint32_t i = 0; i < size; i++)
				data[i] = raw[i] * 255;

			stbi_flip_vertically_on_write(true);
			stbi_write_png((projectSavedDirectory / "DefaultScreenshot.png").string().c_str(), width, width, 4, data, width * 4);
			delete[] raw;
		}
	}

	void EditorLayer::Compile()
	{
		if (m_SceneState != SceneState::Edit)
			return;

		std::string devenvPath;
		if (Preferences::GetData().ManualDevenv)
			devenvPath = Preferences::GetData().DevenvPath;
		else
		{
			devenvPath = System::Execute("\"\"vendor/vswhere/vswhere.exe\" -property productPath\"");
			devenvPath.erase(devenvPath.find(".exe"));
			devenvPath += ".com";
		}

		if (devenvPath.empty())
		{
			Popup::Create("Compilation Failure", "Development Environment has not been specified.", { { "Ok", nullptr } }, m_ErrorIcon);
			Taskbar::FlashIcon();
		}
		else if (!std::filesystem::exists(devenvPath))
		{
			Popup::Create("Compilation Failure", "Cannot access Development Environment", { { "Ok", nullptr } }, m_ErrorIcon);
			Taskbar::FlashIcon();
		}
		else
		{
			std::string compileMessage = System::Execute("\"\"" + devenvPath + "\" \"" + std::filesystem::absolute(Project::GetAssetDirectory() / "Scripts/Sandbox.sln").string() + "\" /build "
#ifdef DY_DEBUG
				"Debug"
#else
				"Release"
#endif
				"\""
			);

			if (compileMessage.find("0 failed,") == std::string::npos)
			{
				Popup::Create("Compilation Failure", compileMessage, { { "Ok", nullptr } }, m_ErrorIcon);
				Taskbar::FlashIcon();
			}
		}
	}

	void EditorLayer::SaveAndExit()
	{
		Popup::Create("Unsaved Changes", "Save changes before closing?\nDocument: " + m_EditorScenePath.filename().string() + "\n",
			{ 
				{ "Cancel", [](){}}, 
				{ "Discard", [&]() { CloseProgramWindow(); } }, 
				{ "Save", [&]() { if (SaveScene()) { CloseProgramWindow(); } } } 
			}, m_QuestionMarkIcon);
	}

	void EditorLayer::CloseProgramWindow()
	{
		Application::Get().Close();
	}

	void EditorLayer::SetRendererVisualizationMode(SceneRenderer::RendererVisualizationMode visualizationMode)
	{
		m_PreviousVisualizationMode = SceneRenderer::GetVisualizationMode();
		SceneRenderer::SetVisualizationMode(visualizationMode);
	}

	void EditorLayer::ToggleRendererVisualizationMode()
	{
		auto visualizationMode = SceneRenderer::GetVisualizationMode();
		SceneRenderer::SetVisualizationMode(m_PreviousVisualizationMode);
		m_PreviousVisualizationMode = visualizationMode;
	}

	void EditorLayer::OnScenePlay()
	{
		if (m_SceneState == SceneState::Simulate)
			OnSceneStop();

		m_SceneState = SceneState::Play;
		m_ActiveScene->SetPaused(false);

		m_ActiveScene = Scene::Copy(m_EditorScene);
		m_ActiveScene->OnRuntimeStart();

		m_SceneHierarchyPanel.SetContext(m_ActiveScene);

		m_SoundPlay->Play();
	}

	void EditorLayer::OnSceneSimulate()
	{
		if (m_SceneState == SceneState::Play)
			OnSceneStop();

		m_SceneState = SceneState::Simulate;
		m_ActiveScene->SetPaused(false);

		m_ActiveScene = Scene::Copy(m_EditorScene);
		m_ActiveScene->OnSimulationStart();

		m_SceneHierarchyPanel.SetContext(m_ActiveScene);

		m_SoundSimulate->Play();
	}

	void EditorLayer::OnSceneStop()
	{
		DY_CORE_ASSERT(m_SceneState == SceneState::Play || m_SceneState == SceneState::Simulate);

		if (m_SceneState == SceneState::Play)
			m_ActiveScene->OnRuntimeStop();
		else if (m_SceneState == SceneState::Simulate)
			m_ActiveScene->OnSimulationStop();

		m_SceneState = SceneState::Edit;

		m_ActiveScene = m_EditorScene;

		m_SceneHierarchyPanel.SetContext(m_ActiveScene);

		m_SoundStop->Play();
	}

	void EditorLayer::OnScenePause()
	{
		if (m_SceneState == SceneState::Edit)
			return;

		bool paused = !m_ActiveScene->IsPaused();
		m_ActiveScene->SetPaused(paused);

		if (paused)
			m_SoundPause->Play();
		else
			m_SoundPlay->Play();
	}

	void EditorLayer::ShowEditorWindow()
	{
		Application::Get().GetWindow().ShowWindow();

		Taskbar::SetThumbnailButtons({
			{ m_IconPlay, "Play", 
				[&](Taskbar::ThumbnailButton& button) {
					if (m_SceneState == SceneState::Edit)
					{
						OnScenePlay();

						button.Icon = m_IconStop;
						button.Tooltip = "Stop";
					}
					else
					{
						OnSceneStop();

						button.Icon = m_IconPlay;
						button.Tooltip = "Play";
					}

					Taskbar::UpdateThumbnailButtons();
				}
			},
			{ m_SaveIcon, "Save Scene", 
				[&](Taskbar::ThumbnailButton& button) {
					SaveScene();
				}
			},
			{ m_CompileIcon, "Compile",
				[&](Taskbar::ThumbnailButton& button) {
					Compile();
				} 
			}
		});
	}

	void EditorLayer::ReloadAvailableWorkspaces()
	{
		m_AvailableWorkspaces.clear();
		m_WorkspaceRenameContext.clear();

		if (!std::filesystem::exists("saved/workspaces"))
			return;

		for (const auto& entry : std::filesystem::directory_iterator("saved/workspaces"))
		{
			if (!entry.is_directory() && entry.path().extension() == ".workspace")
				m_AvailableWorkspaces.push_back(entry.path());
		}
	}

	void EditorLayer::OnOpenFile(const std::filesystem::path& path)
	{
		FileType type = FileManager::GetFileType(path);

		switch (type)
		{
		case FileType::FileTypeScene: { OpenScene(Project::GetAssetFileSystemPath((path))); break; }
		case FileType::FileTypeMaterial: { m_MaterialEditorPanel.Open(path); break; }
		}
	}
}