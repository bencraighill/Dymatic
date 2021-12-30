#include "EditorLayer.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include "ImGuizmo.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Dymatic/Scene/SceneSerializer.h"

#include "Dymatic/Utils/PlatformUtils.h"
#include "Dymatic/Math/Math.h"
#include "Dymatic/Math/StringUtils.h"

#include "stb_image/stb_image.h"

#include <cmath>

#include "TextSymbols.h"

// To Open URL
#include <shellapi.h>

#include "Nodes/UnrealBlueprintClass.h"

namespace Dymatic {

	PROCESS_INFORMATION crash_manager_pi;

	VOID LoadApplicationCrashManager()
	{
		// additional information
		STARTUPINFO si;

		// set the size of the structures
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		ZeroMemory(&crash_manager_pi, sizeof(crash_manager_pi));

		WCHAR path[MAX_PATH];
		GetModuleFileName(GetModuleHandle(NULL), path, sizeof(path));

		TCHAR s[100];
		swprintf_s(s, (L"%X %s %S"), GetCurrentProcessId(), path, "Dymatic.log");

		// start the program up
		CreateProcess(L"../bin/Debug-windows-x86_64/CrashManager/CrashManager.exe",   // the path
			s,        // Command line
			NULL,           // Process handle not inheritable
			NULL,           // Thread handle not inheritable
			FALSE,          // Set handle inheritance to FALSE
			0,              // No creation flags
			NULL,           // Use parent's environment block
			NULL,           // Use parent's starting directory 
			&si,            // Pointer to STARTUPINFO structure
			&crash_manager_pi             // Pointer to PROCESS_INFORMATION structure (removed extra parentheses)
		);

		// Handles are closed and destroyed when the engine terminates
	}

	extern const std::filesystem::path g_AssetPath;

	EditorLayer::EditorLayer()
		: Layer("EditorLayer"), m_CameraController(1280.0f / 720.0f), m_SquareColor({ 0.2f, 0.3f, 0.8f, 1.0f })
	{
	}

	void EditorLayer::OnAttach()
	{
		DY_PROFILE_FUNCTION();

		LoadApplicationCrashManager();

		OpenWindowLayout("saved/presets/GeneralWorkspace.layout");
		OpenWindowLayout("saved/SavedLayout.layout");
		m_NodeEditorPannel.Application_Initialize();

		//Add Preference Data load in here
		auto& preferencesData = m_PreferencesPannel.GetPreferences().m_PreferenceData;

		//m_CheckerboardTexture = Texture2D::Create("assets/textures/Checkerboard.png");
		m_IconPlay = Texture2D::Create("Resources/Icons/PlayButton.png");
		m_IconStop = Texture2D::Create("Resources/Icons/StopButton.png");

		Application::Get().GetImGuiLayer()->AddIconFont("assets/fonts/IconsFont.ttf", 25.0f, 0x700, 0x70F);	// Window Icons
		Application::Get().GetImGuiLayer()->AddIconFont("assets/fonts/IconsFont.ttf", 12.0f, 0x710, 0x71E); // Viewport Shading Icons
		Application::Get().GetImGuiLayer()->AddIconFont("assets/fonts/IconsFont.ttf", 25.0f, 0x71F, 0x72E); // Icons
		Application::Get().GetImGuiLayer()->AddIconFont("assets/fonts/IconsFont.ttf", 10.0f, 0x72F, 0x72F); // Small Icons

		Application::Get().GetImGuiLayer()->AddIconFont("assets/fonts/IconsFont.ttf", 10.0f, 0xF7, 0xF8); // Division Symbol

		FramebufferSpecification fbSpec;
		fbSpec.Attachments = { FramebufferTextureFormat::RGBA8, FramebufferTextureFormat::RED_INTEGER, FramebufferTextureFormat::Depth };
		fbSpec.Width = 1600;
		fbSpec.Height = 900;
		m_Framebuffer = Framebuffer::Create(fbSpec);

		m_ActiveScene = CreateRef<Scene>();

		auto commandLineArgs = Application::Get().GetCommandLineArgs();
		if (commandLineArgs.Count > 1)
		{
			auto sceneFilePath = commandLineArgs[1];
			SceneSerializer serializer(m_ActiveScene);
			serializer.Deserialize(sceneFilePath);
		}

		m_EditorCamera = EditorCamera(30.0f, 1.778f, 0.1f, 1000.0f);
#if 0
		// Entity
		auto square = m_ActiveScene->CreateEntity("Green Square");
		square.AddComponent<SpriteRendererComponent>(glm::vec4{ 0.0f, 1.0f, 0.0f, 1.0f });

		auto redSquare = m_ActiveScene->CreateEntity("Red Square");
		redSquare.AddComponent<SpriteRendererComponent>(glm::vec4{ 1.0f, 0.0f, 0.0f, 1.0f });

		m_SquareEntity = square;

		m_CameraEntity = m_ActiveScene->CreateEntity("Camera A");
		m_CameraEntity.AddComponent<CameraComponent>();

		m_SecondCamera = m_ActiveScene->CreateEntity("Camera B");
		auto& cc = m_SecondCamera.AddComponent<CameraComponent>();
		cc.Primary = false;

		class CameraController : public ScriptableEntity
		{
		public:
			virtual void OnCreate() override
			{
				auto& translation = GetComponent<TransformComponent>().Translation;
				translation.x = rand() % 10 - 5.0f;
			}

			virtual void OnDestroy() override
			{
			}

			virtual void OnUpdate(Timestep ts) override
			{
				auto& translation = GetComponent<TransformComponent>().Translation;

				float speed = 5.0f;

				if (Input::IsKeyPressed(Key::A))
					translation.x -= speed * ts;
				if (Input::IsKeyPressed(Key::D))
					translation.x += speed * ts;
				if (Input::IsKeyPressed(Key::W))
					translation.y += speed * ts;
				if (Input::IsKeyPressed(Key::S))
					translation.y -= speed * ts;
			}
		};

		m_CameraEntity.AddComponent<NativeScriptComponent>().Bind<CameraController>();
		m_SecondCamera.AddComponent<NativeScriptComponent>().Bind<CameraController>();
#endif

		auto entity = m_ActiveScene->CreateEntity("Entity");
		entity.AddComponent<NativeScriptComponent>().Bind<Dymatic::UnrealBlueprintClass>();
	}

	void EditorLayer::OnDetach()
	{
		DY_PROFILE_FUNCTION();
		SaveWindowLayout("saved/SavedLayout.layout");


		// Close crash manager process and thread handles.
		TerminateProcess(crash_manager_pi.hProcess, 1);
		TerminateProcess(crash_manager_pi.hThread, 1);

		CloseHandle(crash_manager_pi.hProcess);
		CloseHandle(crash_manager_pi.hThread);
	}

	void EditorLayer::OnUpdate(Timestep ts)
	{
		DY_PROFILE_FUNCTION();

		if (Input::IsKeyPressed(Key::Q))
			abort();

		static bool created = false;
		if (Input::IsKeyPressed(Key::B) && !created)
		{
			auto entity = m_ActiveScene->CreateEntity("Entity");
			entity.AddComponent<NativeScriptComponent>().Bind<Dymatic::UnrealBlueprintClass>();
			created = true;
		}

		m_DeltaTime = ts;

		if (m_PopupsAndNotifications.GetPopupOpen() == false) {
			m_ProgramTime += ts;
			m_LastSaveTime += ts;
		}

		if (m_PreferencesPannel.GetPreferences().m_PreferenceData.autosaveEnabled)
		{
			//AutoSave
			if (m_LastSaveTime >= m_PreferencesPannel.GetPreferences().m_PreferenceData.autosaveTime * 60.0f && !m_EditorScenePath.empty())
			{
				SaveScene();
				DY_CORE_INFO("Autosave Complete: Program Time - {0}", m_ProgramTime);
				//Add Autosave Complete Notification
				{
					m_PopupsAndNotifications.Notification(1, "Autosave Completed", ("Autosaved current scene at program time: \n" + std::to_string((int)m_ProgramTime)), { { m_PopupsAndNotifications.GetNextNotificationId(), "Dismiss", [](){} } });
				}
			}

			//Warning of autosave
			if (m_LastSaveTime >= (m_PreferencesPannel.GetPreferences().m_PreferenceData.autosaveTime * 60.0f) - 10 && m_LastSaveTime <= (m_PreferencesPannel.GetPreferences().m_PreferenceData.autosaveTime * 60.0f) - 10 + ts && !m_EditorScenePath.empty())
			{
				m_PopupsAndNotifications.Notification(0, "Autosave pending...", "Autosave of current scene will commence\n in 10 seconds.", { { m_PopupsAndNotifications.GetNextNotificationId(), "Cancel", [&]() { m_LastSaveTime = 1.0f; } }, { m_PopupsAndNotifications.GetNextNotificationId(), "Save Now", [&]() { m_LastSaveTime = m_PreferencesPannel.GetPreferences().m_PreferenceData.autosaveTime * 60; } } }, false, 10.0f);
			}
		}

		// Resize
		if (FramebufferSpecification spec = m_Framebuffer->GetSpecification();
			m_ViewportSize.x > 0.0f && m_ViewportSize.y > 0.0f && // zero sized framebuffer is invalid
			(spec.Width != m_ViewportSize.x || spec.Height != m_ViewportSize.y))
		{
			m_Framebuffer->Resize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
			m_CameraController.OnResize(m_ViewportSize.x, m_ViewportSize.y);
			m_EditorCamera.SetViewportSize(m_ViewportSize.x, m_ViewportSize.y);
			m_ActiveScene->OnViewportResize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
		}

		// Render
		Renderer2D::ResetStats();
		m_Framebuffer->Bind();
		//RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.1f, 1 });
		RenderCommand::SetClearColor({ 0.28f, 0.28f, 0.28f, 1.0f });
		RenderCommand::Clear();

		//Clear entity ID attachment to -1
		m_Framebuffer->ClearAttachment(1, -1);

		switch (m_SceneState)
		{
			case SceneState::Edit:
			{
				if (m_ViewportFocused)
					m_CameraController.OnUpdate(ts);

				m_EditorCamera.OnUpdate(ts);

				m_ActiveScene->OnUpdateEditor(ts, m_EditorCamera);
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
			int pixelData = m_Framebuffer->ReadPixel(1, mouseX, mouseY);
			m_HoveredEntity = pixelData == -1 ? Entity() : Entity((entt::entity)pixelData, m_ActiveScene.get());
		}

		OnOverlayRender();

		m_Framebuffer->Unbind();
	}

	void EditorLayer::OnImGuiRender()
	{
		DY_PROFILE_FUNCTION();

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
		//ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1.0f, 20.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(maximized ? 0.0f : 5.0f, maximized ? 0.0f : 5.0f));
		ImGui::Begin("Dymatic Editor Dockspace Window", &dockspaceOpen, window_flags);
		ImVec2 dockspaceWindowPosition = ImGui::GetWindowPos();
		ImGui::PopStyleVar(1);

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
			ImGuiID dockspace_id = ImGui::GetID("Dymatic Editor DockSpace");
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

			if (ImGui::BeginMenu(CHARACTER_ICON_DYMATIC))
			{
				if (ImGui::MenuItem("Splash Screen")) { m_ShowSplash = true; }
				if (ImGui::MenuItem("About Dymatic")) { m_PopupsAndNotifications.Popup("Engine Information", "Dymatic Engine\nV1.2.2 (Development)\n\n\nDate: 2021 / 07 / 25\nHash: #D65Y4h11QR\nBranch: Development\n\n\n Dymatic Engine is a free, open source software developed by BAS Solutions.\nView source files for licenses from vendor libraries.", { { m_PopupsAndNotifications.GetNextNotificationId(), "Learn More", []() { ShellExecute(0, 0, L"https://github.com/benc25/dymatic", 0, 0, SW_SHOW); } }, { m_PopupsAndNotifications.GetNextNotificationId(), "Ok", []() {} } }); }
				ImGui::Separator();
				if (ImGui::MenuItem("Github")) { ShellExecute(0, 0, L"https://github.com/benc25/dymatic", 0, 0 , SW_SHOW ); }
				if (ImGui::MenuItem("Uninstall")) { m_PopupsAndNotifications.Popup("Uninstall Message", "Current Version:\nV1.2.2 (Development)\n\n\nUnfortunately Dymatic doesn't currently have an uninstaller.\nAll files must be deleted manually from the root directory.", { { m_PopupsAndNotifications.GetNextNotificationId(), "Ok", [](){} } }); }
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
				// Disabling fullscreen would allow the window to be moved to the front of other windows, 
				// which we can't undo at the moment without finer window depth/z control.
				//ImGui::MenuItem("Fullscreen", NULL, &opt_fullscreen_persistant);1
				if (ImGui::MenuItem((std::string(CHARACTER_SYSTEM_ICON_NEW_FILE) + " New").c_str(), GetBindAsString(NewSceneBind).c_str())) NewScene();
				if (ImGui::MenuItem((std::string(CHARACTER_SYSTEM_ICON_OPEN_FILE) + " Open...").c_str(), GetBindAsString(OpenSceneBind).c_str())) OpenScene();
				if (ImGui::BeginMenu((std::string(CHARACTER_SYSTEM_ICON_RECENT) + " Open Recent").c_str(), !GetRecentFiles().empty()))
				{
					std::vector<std::filesystem::path> recentFiles = GetRecentFiles();
					if (!recentFiles.empty())
					{
						for (int i = 0; i < recentFiles.size(); i++)
						{
							if (ImGui::MenuItem((m_ContentBrowser.GetFullFileNameFromPath(m_ContentBrowser.SwapStringSlashesSingle(recentFiles[i].string()))).c_str()))
							{
								OpenScene(recentFiles[i]);
							}
						}
					}
					ImGui::EndMenu();
				}

				ImGui::Separator();

				if (ImGui::MenuItem((std::string(CHARACTER_SYSTEM_ICON_SAVE) + " Save").c_str(), GetBindAsString(SaveSceneBind).c_str())) SaveScene();
				if (ImGui::MenuItem((std::string(CHARACTER_SYSTEM_ICON_SAVE_AS) + " Save As...").c_str(), GetBindAsString(SaveSceneAsBind).c_str())) SaveSceneAs();

				ImGui::Separator();

				if (ImGui::BeginMenu((std::string(CHARACTER_SYSTEM_ICON_IMPORT) + " Import").c_str()))
				{
					ImGui::MenuItem("FBX", "(.fbx)"); ImGui::MenuItem("Wavefront", "(.obj"); ImGui::MenuItem("Theme", "(.dytheme)");
					ImGui::EndMenu();
				}
				if (ImGui::BeginMenu((std::string(CHARACTER_SYSTEM_ICON_EXPORT) + " Export").c_str()))
				{
					ImGui::MenuItem("FBX", "(.fbx)"); ImGui::MenuItem("Wavefront", "(.obj"); ImGui::MenuItem("Theme", "(.dytheme)");
					ImGui::EndMenu();
				}

				if (ImGui::MenuItem((std::string(CHARACTER_SYSTEM_ICON_APPEND) + " Append...").c_str(), "")) AppendScene();

				ImGui::Separator();

				if (ImGui::MenuItem((std::string(CHARACTER_SYSTEM_ICON_QUIT) + " Quit").c_str(), GetBindAsString(QuitBind).c_str())) SaveAndExit();
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Edit"))
			{
				if (ImGui::MenuItem((std::string(CHARACTER_WINDOW_ICON_PREFERENCES) + " Preferences").c_str(), "")) m_PreferencesPannel.GetPreferencesPanelVisible() = true;
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Window"))
			{
				ImGui::MenuItem(		(std::string(CHARACTER_WINDOW_ICON_VIEWPORT)			+ " Viewport").c_str(), "",				&m_ViewportVisible);
				ImGui::MenuItem(		(std::string(CHARACTER_WINDOW_ICON_TOOLBAR)				+ " Toolbar").c_str(), "",				&m_ToolbarVisible);
				ImGui::MenuItem(		(std::string(CHARACTER_WINDOW_ICON_STATS)				+ " Stats").c_str(), "",				&m_StatsVisible);
				ImGui::MenuItem(		(std::string(CHARACTER_WINDOW_ICON_INFO)				+ " Info").c_str(), "",					&m_InfoVisible);
				ImGui::MenuItem(		(std::string(CHARACTER_WINDOW_ICON_MEMORY_EDITOR)		+ " Memory").c_str(), "",				&m_MemoryEditorVisible);
				ImGui::MenuItem(		(std::string(CHARACTER_WINDOW_ICON_NODE_EDITOR)			+ " Node Editor").c_str(), "",			&m_NodeEditorPannel.GetNodeEditorVisible());
				ImGui::MenuItem(		(std::string(CHARACTER_WINDOW_ICON_SCENE_HIERARCHY)		+ " Scene Hierarchy").c_str(), "",		&m_SceneHierarchyPanel.GetSceneHierarchyVisible());
				ImGui::MenuItem(		(std::string(CHARACTER_WINDOW_ICON_PROPERTIES)			+ " Properties").c_str(), "",			&m_SceneHierarchyPanel.GetPropertiesVisible());
				ImGui::MenuItem(		(std::string(CHARACTER_WINDOW_ICON_NOTIFICATIONS)		+ " Notifications").c_str(), "",		&m_PopupsAndNotifications.GetNotificationsVisible());
				ImGui::MenuItem(		(std::string(CHARACTER_WINDOW_ICON_CONTENT_BROWSER)		+ " Content Browser").c_str(), "",		&m_ContentBrowser.GetContentBrowserVisible());
				ImGui::MenuItem(		(std::string(CHARACTER_WINDOW_ICON_TEXT_EDITOR)			+ " Text Editor").c_str(), "",			&m_TextEditor.GetTextEditorVisible());
				ImGui::MenuItem(		(std::string(CHARACTER_WINDOW_ICON_CURVE_EDITOR)		+ " Curve Editor").c_str(), "",			&m_CurveEditor.GetCurveEditorVisible());
				ImGui::MenuItem(		(std::string(CHARACTER_WINDOW_ICON_IMAGE_EDITOR)		+ " Image Editor").c_str(), "",			&m_ImageEditor.GetImageEditorVisible());
				ImGui::MenuItem(		(std::string(CHARACTER_WINDOW_ICON_CONSOLE)				+ " Console").c_str(), "",				&m_ConsoleWindow.GetConsoleWindowVisible());

				ImGui::Separator();

				if (ImGui::MenuItem((std::string(CHARACTER_WINDOW_ICON_SYSTEM_CONSOLE) + " Toggle System Console").c_str(), ""))
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
				if (ImGui::MenuItem("Perspective/Orthographic", GetBindAsString(ViewProjectionBind).c_str())) { m_ProjectionToggled = !m_EditorCamera.GetProjectionType(); m_EditorCamera.SetProjectionType(m_ProjectionToggled); }

				if (ImGui::BeginMenu("Set Perspective"))
				{
					if (ImGui::MenuItem("Front", GetBindAsString(ViewFrontBind).c_str())) { m_YawUpdate = floor(glm::degrees(m_EditorCamera.GetYaw()) / 360) * 360; m_PitchUpdate = floor(glm::degrees(m_EditorCamera.GetYaw()) / 360) * 360; m_UpdateAngles = true; m_EditorCamera.SetProjectionType(1); }
					if (ImGui::MenuItem("Side", GetBindAsString(ViewSideBind).c_str())) { m_YawUpdate = floor(glm::degrees(m_EditorCamera.GetYaw()) / 360) * 360 - 90.0f; m_PitchUpdate = floor(glm::degrees(m_EditorCamera.GetYaw()) / 360) * 360; m_UpdateAngles = true; m_EditorCamera.SetProjectionType(1); }
					if (ImGui::MenuItem("Top", GetBindAsString(ViewTopBind).c_str())) { m_YawUpdate = floor(glm::degrees(m_EditorCamera.GetYaw()) / 360) * 360; m_PitchUpdate = floor(glm::degrees(m_EditorCamera.GetYaw()) / 360) * 360 + 90; m_UpdateAngles = true; m_EditorCamera.SetProjectionType(1); }
					ImGui::Separator();
					if (ImGui::MenuItem("Flip", GetBindAsString(ViewFlipBind).c_str())) { m_YawUpdate = glm::degrees(m_EditorCamera.GetYaw()) + 180.0f; m_PitchUpdate = glm::degrees(m_EditorCamera.GetPitch()); m_UpdateAngles = true; m_EditorCamera.SetProjectionType(1); }

					ImGui::EndMenu();
				}

				ImGui::EndMenu();
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
					case 0: {SaveAndExit(); break; }
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
			if (pressed)
			{
				if (Input::GetMouseY() + (window.GetPositionY() > 99999 ? 0.0f : window.GetPositionY()) <= Input::GetMouseY())
				{
					 window.MaximizeWindow();
				}
			}

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
			}
			else
			{
				init = true;
			}

			ImGui::EndMenuBar();
		}

		//Toolbar Window
		if (m_ToolbarVisible)
		{
			ImGui::Begin((std::string(CHARACTER_WINDOW_ICON_TOOLBAR) + " Toolbar").c_str(), &m_ToolbarVisible, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
			
			ImGui::Dummy(ImVec2{ 0, ((ImGui::GetContentRegionAvail().y - 30) / 2) });

			float size = 30.0f;
			Ref<Texture2D> icon = m_SceneState == SceneState::Edit ? m_IconPlay : m_IconStop;
			ImGui::SetCursorPosX((ImGui::GetWindowContentRegionMax().x * 0.5f) - (size * 0.5f));
			if (ImGui::ImageButton((ImTextureID)icon->GetRendererID(), ImVec2(size, size), ImVec2(0, 0), ImVec2(1, 1), 0))
			{
				if (m_SceneState == SceneState::Edit)
					OnScenePlay();
				else if (m_SceneState == SceneState::Play)
					OnSceneStop();
			}

			ImGui::SameLine();

			ImGui::Dummy(ImVec2{ ImGui::GetContentRegionAvail().x - 370, 0 });
			ImGui::SameLine();

			const char* gizmo_type_items[4] = { CHARACTER_VIEWPORT_ICON_GIZMO_CURSOR, CHARACTER_VIEWPORT_ICON_GIZMO_TRANSLATE, CHARACTER_VIEWPORT_ICON_GIZMO_ROTATE, CHARACTER_VIEWPORT_ICON_GIZMO_SCALE };
			int currentValue = (int)(m_GizmoType) + 1;
			if (ImGui::SwitchButtonEx("##GizmoTypeSwitch", gizmo_type_items, 4, &currentValue, ImVec2(120, 30)))
				m_GizmoType = currentValue - 1;

			ImGui::SameLine();

			if (ImGui::Button(m_GizmoSpace == 0 ? CHARACTER_VIEWPORT_ICON_SPACE_LOCAL : CHARACTER_VIEWPORT_ICON_SPACE_WORLD, ImVec2(30, 30)))
				m_GizmoSpace = !m_GizmoSpace;

			ImGui::SameLine();

			{
				std::string number = String::FloatToString(m_TranslationSnapValue);
				const char* pchar = number.c_str();
				const char** items = new const char* [2]{ CHARACTER_VIEWPORT_ICON_SNAP_TRANSLATION, pchar };
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
				const char** items = new const char* [2]{ CHARACTER_VIEWPORT_ICON_SNAP_ROTATION, pchar };
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
				const char** items = new const char* [2]{ CHARACTER_VIEWPORT_ICON_SNAP_SCALING, pchar };
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
					float snapOptions[9] = { 0.01f, 0.05f, 0.1f, 0.5f, 1.0f, 5.0f, 10.0f, 50.0f, 100.0f };
					for (int i = 0; i < 9; i++)
					{
						if (ImGui::MenuItem((String::FloatToString(snapOptions[i])).c_str()))
						{
							m_TranslationSnapValue = snapOptions[i];
						}
					}
					ImGui::EndPopup();
				}

				if (ImGui::BeginPopup("RotationSnapLevel"))
				{
					float snapOptions[9] = { 5.0f, 10.0f, 15.0f, 30.0f, 45.0f, 60.0f, 90.0f, 120.0f, 180.0f };
					for (int i = 0; i < 9; i++)
					{
						if (ImGui::MenuItem(((String::FloatToString(snapOptions[i])) + std::string(CHARACTER_SYMBOL_DEGREE)).c_str()))
						{
							m_RotationSnapValue = snapOptions[i];
						}
					}
					ImGui::EndPopup();
				}

				if (ImGui::BeginPopup("ScaleSnapLevel"))
				{
					float snapOptions[6] = { 0.1f, 0.25f, 0.5f, 1.0f, 5.0f, 10.0f };
					for (int i = 0; i < 6; i++)
					{
						if (ImGui::MenuItem((String::FloatToString(snapOptions[i])).c_str()))
						{
							m_ScaleSnapValue = snapOptions[i];
						}
					}
					ImGui::EndPopup();
				}
			}

			ImGui::End();
		}

		if (m_InfoVisible)
		{
			ImGui::Begin((std::string(CHARACTER_WINDOW_ICON_INFO) + " Info").c_str(), &m_InfoVisible, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
			ImGui::TextDisabled(GetCurrentFileName().c_str());
			ImGui::SameLine();
			std::string text = "1.2.2 - (Development)";
			ImGui::Dummy(ImVec2{ ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(text.c_str()).x - 5, 0 });
			ImGui::SameLine();
			ImGui::TextDisabled(text.c_str());
			ImGui::End();
		}

		//Preferences Pannel
		m_PreferencesPannel.OnImGuiRender();

		//Content Browser Window
		m_ContentBrowser.OnImGuiRender(m_DeltaTime);
		auto& contentFileToOpen = m_ContentBrowser.GetFileToOpen();
		if (contentFileToOpen != "")
		{
			if (m_ContentBrowser.GetFileFormat(contentFileToOpen) == ".dymatic") { OpenScene(contentFileToOpen); }
			if (m_ContentBrowser.GetFileFormat(contentFileToOpen) == ".dytheme") { m_PopupsAndNotifications.Popup("Install Theme", "File: " + m_ContentBrowser.GetFullFileNameFromPath(contentFileToOpen) + "\n\nDo you wish to install this dytheme file? It will override you current theme.\nTo save your current theme, export it from Preferences.\n", { { m_PopupsAndNotifications.GetNextNotificationId(), "Cancel", [](){} }, { m_PopupsAndNotifications.GetNextNotificationId(), "Install", [&]() { m_PreferencesPannel.RetryDythemeFile(); } } }); m_PreferencesPannel.SetRecentDythemePath(contentFileToOpen); }
			contentFileToOpen = "";
		}

		//Scene Hierarchy and properties pannel
		m_SceneHierarchyPanel.OnImGuiRender();
		//m_ContentBrowserPanel.OnImGuiRender();

		m_PerformanceAnalyser.OnImGuiRender(m_DeltaTime);

		m_CurveEditor.OnImGuiRender();
		m_ImageEditor.OnImGuiRender();

		if (m_MemoryEditorVisible)
		{
			//Memory Editor
			//SIZE_T MinSize;
			//SIZE_T MaxSize;
			//GetProcessWorkingSetSize(GetCurrentProcess(), &MinSize, &MaxSize);

			PROCESS_MEMORY_COUNTERS_EX pmc;
			GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
			SIZE_T MaxSize = pmc.PrivateUsage;

			static int test_int = 5;
			//test_int++;
			DY_CORE_INFO((unsigned int)&test_int);

			static char* buffer = new char[MaxSize];
			//static char buffer[200000000];
			SIZE_T NumberRead;
			SIZE_T NumberWritten;

			auto baseAddress = m_MemoryEditor.GetProcessBaseAddress(GetCurrentProcessId());
			ReadProcessMemory(GetCurrentProcess(), (LPCVOID)baseAddress, buffer, MaxSize, &NumberRead);
			m_MemoryEditor.DrawWindow((std::string(CHARACTER_WINDOW_ICON_MEMORY_EDITOR) + " Memory").c_str(), &m_MemoryEditorVisible, buffer, NumberRead, (SIZE_T)baseAddress);
			DY_CORE_INFO(WriteProcessMemory(GetCurrentProcess(), (LPVOID)baseAddress, buffer, MaxSize, &NumberWritten));
			DY_CORE_ERROR(GetLastError());
			DY_CORE_INFO(test_int);
			//---------------------//
		}

		m_ConsoleWindow.OnImGuiRender(m_DeltaTime);

		//m_FilePrompt.OnImGuiRender(m_DeltaTime);

		//m_AgentSimulation.Update(m_DeltaTime);
		//m_MandelbrotSet.OnImGuiRender();
		//m_SandSimulation.OnImGuiRender();
		//m_RopeSimulation.OnImGuiRender(m_DeltaTime);
		//m_ChessAI.OnImGuiRender(m_DeltaTime);
		
		m_TextEditor.OnImGuiRender();
		m_NodeEditorPannel.OnImGuiRender();

		
		m_PopupsAndNotifications.NotificationUpdate(m_DeltaTime, m_ProgramTime);

		//Notifications Pannel Event Check
		m_PopupsAndNotifications.PopupUpdate();


		if (m_StatsVisible)
		{
			ImGui::Begin((std::string(CHARACTER_WINDOW_ICON_STATS) + " Stats").c_str(), &m_StatsVisible);

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

			ImGui::Begin("Settings");
			ImGui::Checkbox("Show physics colliders", &m_ShowPhysicsColliders);
			ImGui::End();
		}


		if (m_ViewportVisible)
		{
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
			ImGui::Begin((std::string(CHARACTER_WINDOW_ICON_VIEWPORT) + " Viewport").c_str(), &m_ViewportVisible);
			auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
			auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
			auto viewportOffset = ImGui::GetWindowPos();
			m_ViewportBounds[0] = { viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y };
			m_ViewportBounds[1] = { viewportMaxRegion.x + viewportOffset.x, viewportMaxRegion.y + viewportOffset.y };

			m_ViewportFocused = ImGui::IsWindowFocused();
			m_ViewportHovered = ImGui::IsWindowHovered();
			//TODO: fix the whole viewport hovered situation
			//Application::Get().GetImGuiLayer()->BlockEvents(!m_ViewportFocused && !m_ViewportHovered);
			Application::Get().GetImGuiLayer()->BlockEvents(false);

			ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
			m_ViewportSize = { viewportPanelSize.x, viewportPanelSize.y };

			uint64_t textureID = m_Framebuffer->GetColorAttachmentRendererID();
			ImGui::Image(reinterpret_cast<void*>(textureID), ImVec2{ m_ViewportSize.x, m_ViewportSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
				{
					const wchar_t* path = (const wchar_t*)payload->Data;
					OpenScene(std::filesystem::path(g_AssetPath) / path);
				}
				ImGui::EndDragDropTarget();
			}

			ImGui::SetCursorPos(ImVec2());

			//Viewport View Settings
			ImGui::Dummy(ImVec2{ ImGui::GetContentRegionAvail().x - 110, 0 });
			ImGui::SameLine();
			const char* shaderButtonTypes[4] = { CHARACTER_VIEWPORT_ICON_SHADING_WIREFRAME, CHARACTER_VIEWPORT_ICON_SHADING_UNLIT, CHARACTER_VIEWPORT_ICON_SHADING_SOLID, CHARACTER_VIEWPORT_ICON_SHADING_RENDERED };
			int index = m_ShadingIndex;
			if (ImGui::SwitchButtonEx("##ShaderTypeButtonSwitch", shaderButtonTypes, 4, &index, ImVec2{ 95, 25 }))
			{
				SetShadingIndex(index);
			}

			//Scene View Gizmo
			{

				auto camYAngle = Math::NormalizeAngle(glm::degrees(m_EditorCamera.GetPitch()), -180) * -1;
				auto camXAngle = Math::NormalizeAngle(glm::degrees(m_EditorCamera.GetYaw()), -180);

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
				ImVec2 pos = ImVec2(ImGui::GetWindowPos().x + (float)ImGui::GetWindowWidth() - (armLength + circleRadius) * 1.1f, ImGui::GetWindowPos().y + (armLength + circleRadius) * 1.1f + 30);

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
					if (Input::IsMouseButtonPressed(Mouse::ButtonLeft)) { m_EditorCamera.SetFreePan(true); m_EditorCamera.SetInitialMousePosition(Input::GetMousePosition()); activeCirclePress = true; }
				}
				else
				{
					activeCircleHovered = false;
				}

				if (activeCirclePress) { activeCircleHovered = true; }

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
					{
						if (checkVal != -1000.0f) { checkValSuccess = true; }
					}
					if (!checkValSuccess) { circleOverlayCheck = false; break; }

				}
				ImRect bb = ImRect(ImVec2(pos.x - (armLength + circleRadius), pos.y - (armLength + circleRadius)), ImVec2(pos.x + (armLength + circleRadius), pos.y + (armLength + circleRadius)));
				ImGuiID id = ImGui::GetID("##ViewGizmoHoverButtonID");
				ImGui::ButtonBehavior(bb, id, &hovered, &held);
				ImGui::ItemSize(ImVec2((armLength + circleRadius) * 2, (armLength + circleRadius) * 2), style.FramePadding.y);
			}

			ImGui::Dummy(ImVec2{ ImGui::GetContentRegionAvail().x - 45, 0 });
			ImGui::SameLine();
			if (ImGui::Button(std::string((bool)m_EditorCamera.GetProjectionType() ? CHARACTER_VIEWPORT_ICON_PROJECTION_ORTHOGRAPHIC : CHARACTER_VIEWPORT_ICON_PROJECTION_PERSPECTIVE).c_str(), ImVec2(30, 30)))
			{
				m_ProjectionToggled = !m_EditorCamera.GetProjectionType();
				m_EditorCamera.SetProjectionType(m_ProjectionToggled);
			}

			//static Ref<Texture2D> tempTex = Texture2D::Create(32, 32);
			//char* data = new char[32 * 32 * 4];
			//
			//if (m_ProgramTime > 5.0f)
			//{
			//	//CURSORINFO info;
			//	//info.cbSize = sizeof(CURSORINFO);
			//	//GetCursorInfo(&info);
			//	//if ((int)info.hCursor == 0x0000000008e211e1)
			//	//{
			//	//	DY_CORE_INFO("Dragging File");
			//	//	CopyCursor(info.hCursor);
			//	//}
			//
			//	HBITMAP hbitmap;
			//	BITMAP bitmap;
			//	BITMAPINFO bmi;
			//	HDC hdcScreen = GetDC(NULL);
			//	HDC hdcMem = CreateCompatibleDC(hdcScreen);
			//
			//	CURSORINFO cursorInfo = { 0 };
			//	cursorInfo.cbSize = sizeof(cursorInfo);
			//
			//	GetCursorInfo(&cursorInfo);
			//
			//	ICONINFO ii = {};
			//	GetIconInfo(cursorInfo.hCursor, &ii);
			//
			//	hbitmap = ii.hbmColor;
			//	SelectObject(hdcMem, hbitmap);
			//	GetObject(hbitmap, sizeof(BITMAP), &bitmap);
			//
			//	GetDIBits(hdcMem, hbitmap, 0, 32, NULL, &bmi, DIB_RGB_COLORS);
			//
			//	for (int i = 0; i < 1024; i++) {
			//		data[i * 4 + 0] = bmi.bmiColors[i].rgbRed;
			//		data[i * 4 + 1] = bmi.bmiColors[i].rgbGreen;
			//		data[i * 4 + 2] = bmi.bmiColors[i].rgbBlue;
			//		data[i * 4 + 3] = 255.0f;
			//		//DY_CORE_INFO(a);
			//	}
			//}
			//tempTex->SetData(data, 32 * 32 * 4);
			//ImGui::Image((ImTextureID)tempTex->GetRendererID(), ImVec2(200.0f, 200.0f));
			//delete[] data;

			// Gizmos
			Entity selectedEntity = m_SceneHierarchyPanel.GetSelectedEntity();

			if (selectedEntity && m_GizmoType != -1)
			{
				ImGuizmo::SetOrthographic(false);
				ImGuizmo::SetDrawlist();

				ImGuizmo::SetRect(m_ViewportBounds[0].x, m_ViewportBounds[0].y, m_ViewportBounds[1].x - m_ViewportBounds[0].x, m_ViewportBounds[1].y - m_ViewportBounds[0].y);


				// Camera
				//Runtime camera from entity
				//auto cameraEntity = m_ActiveScene->GetPrimaryCameraEntity();
				//const auto& camera = cameraEntity.GetComponent<CameraComponent>().Camera;
				//const glm::mat4& cameraProjection = camera.GetProjection();
				//glm::mat4 cameraView = glm::inverse(cameraEntity.GetComponent<TransformComponent>().GetTransform());

				//Editor camera
				const glm::mat4& cameraProjection = m_EditorCamera.GetProjection();
				glm::mat4 cameraView = m_EditorCamera.GetViewMatrix();

				// Entity Transform
				auto& tc = selectedEntity.GetComponent<TransformComponent>();
				glm::mat4 transform = tc.GetTransform();

				//Snapping
				bool snap = m_GizmoType == ImGuizmo::OPERATION::TRANSLATE ? m_TranslationSnap : m_GizmoType == ImGuizmo::OPERATION::ROTATE ? m_RotationSnap : m_ScaleSnap;
				float snapValue = m_GizmoType == ImGuizmo::OPERATION::TRANSLATE ? m_TranslationSnapValue : m_GizmoType == ImGuizmo::OPERATION::ROTATE ? m_RotationSnapValue : m_ScaleSnapValue;

				float snapValues[3] = { snapValue, snapValue, snapValue };


				ImGuizmo::Manipulate(glm::value_ptr(cameraView), glm::value_ptr(cameraProjection),
					(ImGuizmo::OPERATION)m_GizmoType, (ImGuizmo::MODE)m_GizmoSpace, glm::value_ptr(transform),
					nullptr, ((Input::IsKeyPressed(Key::LeftControl) ? !snap : snap) ? snapValues : nullptr));

				if (ImGuizmo::IsUsing())
				{
					glm::vec3 translation, rotation, scale;
					Math::DecomposeTransform(transform, translation, rotation, scale);

					glm::vec3 deltaRotation = rotation - tc.Rotation;
					tc.Translation = translation;
					tc.Rotation += deltaRotation;
					tc.Scale = scale;

				}

			}

			ImGui::End();
			ImGui::PopStyleVar();
		}

		if (m_ShowSplash)
		{
			bool focused = false;
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 20.0f);
			ImGui::Begin("Splash", &m_ShowSplash, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);
			if (ImGui::IsWindowFocused()) { focused = true; }
			ImGui::SetWindowPos(ImVec2((((io.DisplaySize.x * 0.5f) - (ImGui::GetWindowWidth() / 2)) + dockspaceWindowPosition.x), (((io.DisplaySize.y * 0.5f) - (ImGui::GetWindowHeight() / 2)) + dockspaceWindowPosition.y)));
			ImGui::Image(reinterpret_cast<void*>(m_DymaticSplash->GetRendererID()), ImVec2{ 488, 267 }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
			ImGui::SameLine();
			ImGui::Dummy(ImVec2{ 0, 40 - (ImGui::CalcTextSize("W").y / 2) });
			ImGui::SameLine();
			std::string versionName = "V1.2.2";
			ImGui::SameLine( 488 -ImGui::CalcTextSize(versionName.c_str()).x);
			ImGui::Text(versionName.c_str());
			ImGui::Columns(2);
			ImGui::TextDisabled("New File");
			if (ImGui::Selectable((std::string(CHARACTER_SYSTEM_ICON_NEW_FILE) + "General Workspace").c_str())) { focused = false; OpenWindowLayout("saved/presets/layouts/GeneralWorkspace.layout"); NewScene(); AppendScene("saved/presets/scenes/DefaultCubeScene.dymatic"); }
			if (ImGui::IsItemFocused()) { focused = true; }
			if (ImGui::Selectable((std::string(CHARACTER_SYSTEM_ICON_NEW_FILE) + "Scripting Workspace").c_str())) { focused = false; OpenWindowLayout("saved/presets/layouts/ScriptingWorkspace.layout"); NewScene(); AppendScene("saved/presets/scenes/DefaultCubeScene.dymatic"); }
			if (ImGui::IsItemFocused()) { focused = true; }
			if (ImGui::Selectable((std::string(CHARACTER_SYSTEM_ICON_NEW_FILE) + "Environment Design Workspace").c_str())) { focused = false; OpenWindowLayout("saved/presets/layouts/EnvironmentDesignWorkspace.layout"); NewScene(); AppendScene("saved/presets/scenes/DefaultCubeScene.dymatic"); }
			if (ImGui::IsItemFocused()) { focused = true; }
			if (ImGui::Selectable((std::string(CHARACTER_SYSTEM_ICON_NEW_FILE) + "Animation Workspace").c_str())) { focused = false; OpenWindowLayout("saved/presets/layouts/AnimationWorkspace.layout"); }
			if (ImGui::IsItemFocused()) { focused = true; }
			if (ImGui::Selectable((std::string(CHARACTER_SYSTEM_ICON_NEW_FILE) + "VFX Workspace").c_str())) { focused = false; OpenWindowLayout("saved/presets/layouts/VFXWorkspace.layout"); }
			if (ImGui::IsItemFocused()) { focused = true; }
			ImGui::NextColumn();
			ImGui::TextDisabled("Recent Files");
			ImGui::SetNextWindowSize(ImVec2{488 / 2, 600});
			ImGui::BeginChild("##RecentFilesList");
			if (ImGui::IsWindowFocused()) { focused = true; }
			std::vector<std::filesystem::path> recentFiles = GetRecentFiles();
			if (!recentFiles.empty())
			{
				for (int i = 0; i < recentFiles.size(); i++)
				{
					if (ImGui::Selectable((std::string(CHARACTER_SYSTEM_ICON_NEW_FILE) + m_ContentBrowser.GetFullFileNameFromPath(recentFiles[i].string())).c_str()))
					{
						focused = false;
						OpenScene(recentFiles[i]);
					}
					if (ImGui::IsItemFocused()) { focused = true; }
				}
			}
			ImGui::EndChild();
			ImGui::EndColumns();
			ImGui::End();
			ImGui::PopStyleVar(2);
			m_ShowSplash = focused;
		}

		ImGui::End();
	}

	void EditorLayer::OnEvent(Event& e)
	{
		m_CameraController.OnEvent(e);
		m_EditorCamera.OnEvent(e);
		m_NodeEditorPannel.OnEvent(e);
		m_CurveEditor.OnEvent(e);
		m_ConsoleWindow.OnEvent(e);
		m_ImageEditor.OnEvent(e);
		m_TextEditor.OnEvent(e);

		EventDispatcher dispatcher(e);

		dispatcher.Dispatch<KeyPressedEvent>(DY_BIND_EVENT_FN(EditorLayer::OnKeyPressed));
		dispatcher.Dispatch<MouseButtonPressedEvent>(DY_BIND_EVENT_FN(EditorLayer::OnMouseButtonPressed));
		dispatcher.Dispatch<WindowDropEvent>(DY_BIND_EVENT_FN(EditorLayer::OnDropped));
		dispatcher.Dispatch<WindowCloseEvent>(DY_BIND_EVENT_FN(EditorLayer::OnClosed));
	}

	void EditorLayer::UpdateKeys(BindCatagory bindCatagory)
	{
		if (KeyBindCheck(NewSceneBind, bindCatagory))					{ NewScene(); }
		else if (KeyBindCheck(OpenSceneBind, bindCatagory))				{ OpenScene(); }
		else if (KeyBindCheck(SaveSceneBind, bindCatagory))				{ SaveScene(); }
		else if (KeyBindCheck(SaveSceneAsBind, bindCatagory))			{ SaveSceneAs(); }
		else if (KeyBindCheck(QuitBind, bindCatagory))					{ SaveAndExit(); }
		else if (KeyBindCheck(SelectObjectBind, bindCatagory))			{ if (m_ViewportHovered && !ImGuizmo::IsUsing() && !ImGuizmo::IsOver() && !Input::IsKeyPressed(Key::LeftAlt)) m_SceneHierarchyPanel.SetSelectedEntity(m_HoveredEntity); }
		else if (KeyBindCheck(GizmoNoneBind, bindCatagory))				{ if (ViewportKeyAllowed() && !ImGuizmo::IsUsing()) { m_GizmoType = -1; } }
		else if (KeyBindCheck(GizmoTranslateBind, bindCatagory))		{ if (ViewportKeyAllowed() && !ImGuizmo::IsUsing()) { m_GizmoType = ImGuizmo::OPERATION::TRANSLATE; } }
		else if (KeyBindCheck(GizmoRotateBind, bindCatagory))			{ if (ViewportKeyAllowed() && !ImGuizmo::IsUsing()) { m_GizmoType = ImGuizmo::OPERATION::ROTATE; } }
		else if (KeyBindCheck(GizmoScaleBind, bindCatagory))			{ if (ViewportKeyAllowed() && !ImGuizmo::IsUsing()) { m_GizmoType = ImGuizmo::OPERATION::SCALE; } }
		else if (KeyBindCheck(ShadingTypeWireframeBind, bindCatagory))	{ if (ViewportKeyAllowed()) SetShadingIndex(0); }
		else if (KeyBindCheck(ShadingTypeUnlitBind, bindCatagory))		{ if (ViewportKeyAllowed()) SetShadingIndex(1); }
		else if (KeyBindCheck(ShadingTypeSolidBind, bindCatagory))		{ if (ViewportKeyAllowed()) SetShadingIndex(2); }
		else if (KeyBindCheck(ShadingTypeRenderedBind, bindCatagory))	{ if (ViewportKeyAllowed()) SetShadingIndex(3); }
		else if (KeyBindCheck(ToggleShadingTypeBind, bindCatagory))		{ if (ViewportKeyAllowed()) SetShadingIndex(m_ShadingIndex == 0 ? m_PreviousToggleIndex : 0); }
		else if (KeyBindCheck(ViewFrontBind, bindCatagory))				{ if (ViewportKeyAllowed()) { m_YawUpdate = floor(glm::degrees(m_EditorCamera.GetYaw()) / 360) * 360; m_PitchUpdate = floor(glm::degrees(m_EditorCamera.GetYaw()) / 360) * 360; m_UpdateAngles = true; m_EditorCamera.SetProjectionType(1); } }
		else if (KeyBindCheck(ViewSideBind, bindCatagory))				{ if (ViewportKeyAllowed()) { m_YawUpdate = floor(glm::degrees(m_EditorCamera.GetYaw()) / 360) * 360 - 90.0f; m_PitchUpdate = floor(glm::degrees(m_EditorCamera.GetYaw()) / 360) * 360; m_UpdateAngles = true; m_EditorCamera.SetProjectionType(1); } }
		else if (KeyBindCheck(ViewTopBind, bindCatagory))				{ if (ViewportKeyAllowed()) { m_YawUpdate = floor(glm::degrees(m_EditorCamera.GetYaw()) / 360) * 360; m_PitchUpdate = floor(glm::degrees(m_EditorCamera.GetYaw()) / 360) * 360 + 90; m_UpdateAngles = true; m_EditorCamera.SetProjectionType(1); } }
		else if (KeyBindCheck(ViewFlipBind, bindCatagory))				{ if (ViewportKeyAllowed()) { m_YawUpdate = glm::degrees(m_EditorCamera.GetYaw()) + 180.0f; m_PitchUpdate = glm::degrees(m_EditorCamera.GetPitch()); m_UpdateAngles = true; m_EditorCamera.SetProjectionType(1); } }
		else if (KeyBindCheck(ViewProjectionBind, bindCatagory))		{ if (ViewportKeyAllowed()) { m_ProjectionToggled = !m_EditorCamera.GetProjectionType(); m_EditorCamera.SetProjectionType(m_ProjectionToggled); } }
		else if (KeyBindCheck(DuplicateBind, bindCatagory))				{ if (ViewportKeyAllowed() && m_SceneHierarchyPanel.GetSelectedEntity()) { OnDuplicateEntity(); } m_NodeEditorPannel.DuplicateNodes(); }
		else if (KeyBindCheck(RenameBind, bindCatagory))				{ if (m_ContentBrowser.GetSelectedFile().filename != "") m_ContentBrowser.OpenRenamePopup(m_ContentBrowser.GetSelectedFile()); }
		else if (KeyBindCheck(ClosePopupBind, bindCatagory))			{ m_PopupsAndNotifications.RemoveTopmostPopup(); }
		// Text Editor Keybinds
		else if (KeyBindCheck(TextEditorDuplicate, bindCatagory))		{ m_TextEditor.Duplicate(); }
		else if (KeyBindCheck(TextEditorSwapLineUp, bindCatagory))		{ m_TextEditor.SwapLineUp(); }
		else if (KeyBindCheck(TextEditorSwapLineDown, bindCatagory))	{ m_TextEditor.SwapLineDown(); }
		else if (KeyBindCheck(TextEditorSwitchHeader, bindCatagory))	{ m_TextEditor.SwitchCStyleHeader(); }
	
		else if (Input::IsMouseButtonPressed(Mouse::ButtonLeft) && Input::IsKeyPressed(Key::LeftAlt)) { if (m_ProjectionToggled != m_EditorCamera.GetProjectionType()) { m_EditorCamera.SetProjectionType(m_ProjectionToggled); } }
	}

	bool EditorLayer::OnKeyPressed(KeyPressedEvent& e)
	{
		m_KeyPressed = e;
		UpdateKeys(Keyboard);

		return false;
		// Shortcuts
		//if (e.GetRepeatCount() > 0)
		//	return false;
		//
		//bool control = Input::IsKeyPressed(Key::LeftControl) || Input::IsKeyPressed(Key::RightControl);
		//bool shift = Input::IsKeyPressed(Key::LeftShift) || Input::IsKeyPressed(Key::RightShift);
		//bool alt = Input::IsKeyPressed(Key::LeftAlt) || Input::IsKeyPressed(Key::RightAlt);
		//
		//
		//
		//switch (e.GetKeyCode())
		//{
		//	//case Key::N:
		//	//{
		//	//	if (control)
		//	//		NewScene();
		//	//
		//	//	break;
		//	//}
		////case Key::O:
		////{
		////	if (control)
		////		OpenScene();
		////
		////	break;
		////}
		//case Key::S:
		//{
		//	if (control && shift)
		//		SaveSceneAs();
		//	else if (control)
		//		SaveScene();
		//
		//	break;
		//}
		//case Key::Q:
		//{
		//	if (control)
		//		SaveAndExit();
		//	else
		//		//Blank Gizmo
		//		m_GizmoType = -1;
		//
		//	break;
		//}
		//
		////Gizmos
		//case Key::W:
		//	m_GizmoType = ImGuizmo::OPERATION::TRANSLATE;
		//	break;
		//case Key::E:
		//	m_GizmoType = ImGuizmo::OPERATION::ROTATE;
		//	break;
		//case Key::R:
		//	m_GizmoType = ImGuizmo::OPERATION::SCALE;
		//	break;
		//case Key::D:
		//	if (shift)
		//		if (m_SceneHierarchyPanel.GetSelectedEntity())
		//			m_SceneHierarchyPanel.SetSelectedEntity(m_ActiveScene->DuplicateEntity(m_SceneHierarchyPanel.GetSelectedEntity()));
		//	break;
		//case Key::F2:
		//{
		//	if (m_ContentBrowser.GetSelectedFile().filename != "")
		//	{
		//		m_ContentBrowser.OpenRenamePopup(m_ContentBrowser.GetSelectedFile());
		//	}
		//}
		//}
	}

	bool EditorLayer::OnMouseButtonPressed(MouseButtonPressedEvent& e)
	{
		m_MouseButtonPressed = e;
		UpdateKeys(MouseButton);
		//if (e.GetMouseButton() == Mouse::ButtonLeft && !ImGuizmo::IsUsing() && !ImGuizmo::IsOver() && !Input::IsKeyPressed(Key::LeftAlt))
		//	m_SceneHierarchyPanel.SetSelectedEntity(m_HoveredEntity);

		return false;
	}

	bool EditorLayer::OnDropped(WindowDropEvent& e)
	{
		auto paths = e.GetPaths();
		for (auto path : paths)
		{
			m_ContentBrowser.CopyFileToDirectory(path, m_ContentBrowser.GetBrowseDirectory() + "\\" + m_ContentBrowser.GetFullFileNameFromPath(path), m_ContentBrowser.IsDirectory(path) ? Folder : File);
		}
		m_PopupsAndNotifications.Notification(1, "Files Imported", (std::to_string(paths.size()) + " files have been imported."), { { m_PopupsAndNotifications.GetNextNotificationId(), "Dismiss", []() {} } });
		return false;
	}

	bool EditorLayer::OnClosed(WindowCloseEvent& e)
	{
		SaveAndExit();
		return true;
	}

	void EditorLayer::OnOverlayRender()
	{
		if (m_SceneState == SceneState::Play)
		{
			Entity camera = m_ActiveScene->GetPrimaryCameraEntity();
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

		Renderer2D::EndScene();
	}

	void EditorLayer::NewScene()
	{
		m_ActiveScene = CreateRef<Scene>();
		m_ActiveScene->OnViewportResize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
		m_SceneHierarchyPanel.SetContext(m_ActiveScene);
		
		m_EditorScenePath = std::filesystem::path();

		//Reset Scene time values
		m_LastSaveTime = 0;
		m_ProgramTime = 0;
		m_PopupsAndNotifications.ClearNotificationList();
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
			m_EditorScene->OnViewportResize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
			m_SceneHierarchyPanel.SetContext(m_EditorScene);

			m_ActiveScene = m_EditorScene;
			m_EditorScenePath = path;

			AddRecentFile(path.string());

			//Reset Scene time values
			m_LastSaveTime = 0;
			m_ProgramTime = 0;
			m_PopupsAndNotifications.ClearNotificationList();
		}
	}

	bool EditorLayer::SaveScene()
	{
		if (m_EditorScenePath.empty())
			return SaveSceneAs();
		else
		{
			SerializeScene(m_ActiveScene, m_EditorScenePath);
			return true;
		}
		return false;
	}

	bool EditorLayer::SaveSceneAs()
	{
		std::string filepath = FileDialogs::SaveFile("Dymatic Scene (*.dymatic)\0*.dymatic\0");
		if (!filepath.empty())
		{
			SerializeScene(m_ActiveScene, filepath);
			m_EditorScenePath = filepath;

			AddRecentFile(filepath);
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
	}

	void EditorLayer::SaveAndExit()
	{
		m_PopupsAndNotifications.Popup("Unsaved Changes", "Save changes before closing?\nDocument: " + GetCurrentFileName() + "\n", { { m_PopupsAndNotifications.GetNextNotificationId(), "Cancel", [](){}}, { m_PopupsAndNotifications.GetNextNotificationId(), "Discard", [&]() { CloseProgramWindow(); } }, { m_PopupsAndNotifications.GetNextNotificationId(), "Save", [&]() { if (SaveScene()) { CloseProgramWindow(); } } } });
	}

	void EditorLayer::CloseProgramWindow()
	{
		Application::Get().Close();
	}

	void EditorLayer::OnScenePlay()
	{
		m_SceneState = SceneState::Play;

		m_ActiveScene = Scene::Copy(m_EditorScene);
		m_ActiveScene->OnRuntimeStart();

		m_SceneHierarchyPanel.SetContext(m_ActiveScene);
	}

	void EditorLayer::OnSceneStop()
	{
		m_SceneState = SceneState::Edit;

		m_ActiveScene->OnRuntimeStop();
		m_ActiveScene = m_EditorScene;

		m_SceneHierarchyPanel.SetContext(m_ActiveScene);
	}

	void EditorLayer::OnDuplicateEntity()
	{
		if (m_SceneState != SceneState::Edit)
			return;

		Entity selectedEntity = m_SceneHierarchyPanel.GetSelectedEntity();
		if (selectedEntity)
			m_EditorScene->DuplicateEntity(selectedEntity);
	}

	void EditorLayer::UI_Toolbar()
	{

	}

	void EditorLayer::AddRecentFile(const std::filesystem::path& path)
	{
		std::ifstream file("saved/RecentPaths.txt", std::ios::binary);
		std::string fileStr;

		std::istreambuf_iterator<char> inputIt(file), emptyInputIt;
		std::back_insert_iterator<std::string> stringInsert(fileStr);

		copy(inputIt, emptyInputIt, stringInsert);

		//Check for existance
		if (fileStr.find(path.string() + "\r") != -1)
		{
			fileStr = fileStr.erase(fileStr.find(path.string() + "\r"), path.string().length() + 1);
		}

		fileStr = path.string() + "\r" + fileStr;

		int numberOfSaves = 0;
		for (int i = 0; i < fileStr.length(); i++)
		{
			if (fileStr[i] == "\r"[0]) { numberOfSaves++; }
		}

		if (m_PreferencesPannel.GetPreferences().m_PreferenceData.recentFiles < 1) { fileStr.clear(); }
		else
		{
			while (numberOfSaves > m_PreferencesPannel.GetPreferences().m_PreferenceData.recentFiles)
			{
				int secondLastIndex = fileStr.erase(fileStr.find_last_of("\r"), 1).find_last_of("\r");
				fileStr = fileStr.erase(secondLastIndex, fileStr.find_last_of("\r") - secondLastIndex);
				numberOfSaves--;
			}
		}

		std::ofstream fout("saved/RecentPaths.txt");
		fout << (fileStr).c_str();
	}

	

	std::vector<std::filesystem::path> EditorLayer::GetRecentFiles()
	{
		std::vector<std::filesystem::path> recentFiles;

		std::ifstream file("saved/RecentPaths.txt", std::ios::binary);
		std::string fileStr;

		std::istreambuf_iterator<char> inputIt(file), emptyInputIt;
		std::back_insert_iterator<std::string> stringInsert(fileStr);

		copy(inputIt, emptyInputIt, stringInsert);

		while (fileStr.find_first_of("\r") != -1)
		{
			recentFiles.push_back(fileStr.substr(0, fileStr.find_first_of("\r")));
			{
				int a = fileStr.find_first_of("\r") + 1;
				int b = fileStr.length() - 1 - fileStr.find("\r");
				fileStr = fileStr.substr(a, b);
			}
		}

		for (int i = 0; i < recentFiles.size(); i++)
		{
			if (recentFiles[i] == "" || recentFiles[i] == "\r")
			{
				recentFiles.erase(recentFiles.begin() + i);
				i--;
			}
		}

		return recentFiles;
	}

	void EditorLayer::SetShadingIndex(int index)
	{
		m_ShadingIndex = index;
		if (index != 0) { m_PreviousToggleIndex = m_ShadingIndex; }
		switch (m_ShadingIndex)
		{
		case 0: { RenderCommand::SetWireframe(true); break; }
		case 1: { RenderCommand::SetWireframe(false); break; }
		case 2: { RenderCommand::SetWireframe(false); break; }
		case 3: { RenderCommand::SetWireframe(false); break; }
		}
	}

	bool EditorLayer::KeyBindCheck(KeyBindEvent bindEvent, BindCatagory bindCatagory)
	{
			bool control = Input::IsKeyPressed(Key::LeftControl) || Input::IsKeyPressed(Key::RightControl);
			bool shift = Input::IsKeyPressed(Key::LeftShift) || Input::IsKeyPressed(Key::RightShift);
			bool alt = Input::IsKeyPressed(Key::LeftAlt) || Input::IsKeyPressed(Key::RightAlt);

			auto repeats = m_KeyPressed.GetRepeatCount();

			auto& prefs = m_PreferencesPannel.GetPreferences().m_PreferenceData;

			return (prefs.keyBinds.IsKey(bindEvent, m_KeyPressed.GetKeyCode(), m_MouseButtonPressed.GetMouseButton(), bindCatagory, control, shift, alt, repeats, prefs.emulateNumpad));
	}

	std::string EditorLayer::GetBindAsString(KeyBindEvent bindEvent)
	{
		return m_PreferencesPannel.GetPreferences().m_PreferenceData.keyBinds.GetBindAsString(bindEvent);
	}

	std::string EditorLayer::GetCurrentFileName()
	{
		std::string filename = m_EditorScenePath.string();
		return(filename != "" ? (filename.find_last_of("\\") != std::string::npos ? (filename.erase(0, filename.find_last_of("\\") + 1)) : (filename.find_last_of("/") != std::string::npos ? (filename.erase(0, filename.find_last_of("/") + 1)) : filename)) : "Unsaved Dymatic Document");
	}

	// Load Layout
	void EditorLayer::OpenWindowLayout(const std::filesystem::path& path)
	{
		std::string result;
		std::ifstream in(path, std::ios::in | std::ios::binary); // ifstream closes itself due to RAII
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
				DY_CORE_ERROR("Could not read from layout file '{0}'", path);
				return;
			}
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
				if (CurrentValueName == "Viewport Open") { m_ViewportVisible = (CurrentValue == "true" ? true : false); }
				else if (CurrentValueName == "Toolbar Open") { m_ToolbarVisible = (CurrentValue == "true" ? true : false); }
				else if (CurrentValueName == "Stats Open") { m_StatsVisible = (CurrentValue == "true" ? true : false); }
				else if (CurrentValueName == "Info Open") { m_InfoVisible = (CurrentValue == "true" ? true : false); }
				else if (CurrentValueName == "Memory Open") { m_MemoryEditorVisible = (CurrentValue == "true" ? true : false); }
				else if (CurrentValueName == "Node Editor Open") { m_NodeEditorPannel.GetNodeEditorVisible() = (CurrentValue == "true" ? true : false); }
				else if (CurrentValueName == "Scene Hierarchy Open") { m_SceneHierarchyPanel.GetSceneHierarchyVisible() = (CurrentValue == "true" ? true : false); }
				else if (CurrentValueName == "Properties Open") { m_SceneHierarchyPanel.GetPropertiesVisible() = (CurrentValue == "true" ? true : false); }
				else if (CurrentValueName == "Notifications Open") { m_PopupsAndNotifications.GetNotificationsVisible() = (CurrentValue == "true" ? true : false); }
				else if (CurrentValueName == "Content Browser Open") { m_ContentBrowser.GetContentBrowserVisible() = (CurrentValue == "true" ? true : false); }
				else if (CurrentValueName == "Text Editor Open") { m_TextEditor.GetTextEditorVisible() = (CurrentValue == "true" ? true : false); }
				else if (CurrentValueName == "Curve Editor Open") { m_CurveEditor.GetCurveEditorVisible() = (CurrentValue == "true" ? true : false); }
				else if (CurrentValueName == "Image Editor Open") { m_ImageEditor.GetImageEditorVisible() = (CurrentValue == "true" ? true : false); }
				else if (CurrentValueName == "Console Open") { m_ConsoleWindow.GetConsoleWindowVisible() = (CurrentValue == "true" ? true : false); }
				else if (CurrentValueName == "Ini Contents") { ImGui::LoadIniSettingsFromMemory(CurrentValue.c_str(), strlen(CurrentValue.c_str())); }
			}

		}
	}

	void EditorLayer::SaveWindowLayout(const std::filesystem::path& path)
	{
		std::string out = "Current Windows Open:";

		out = out + "<Viewport Open> {" + (m_ViewportVisible ? "true" : "false") + "}\r";
		out = out + "<Toolbar Open> {" + (m_ToolbarVisible ? "true" : "false") + "}\r";
		out = out + "<Stats Open> {" + (m_StatsVisible ? "true" : "false") + "}\r";
		out = out + "<Info Open> {" + (m_InfoVisible ? "true" : "false") + "}\r";
		out = out + "<Memory Open> {" + (m_MemoryEditorVisible ? "true" : "false") + "}\r";
		out = out + "<Node Editor Open> {" + (m_NodeEditorPannel.GetNodeEditorVisible() ? "true" : "false") + "}\r";
		out = out + "<Scene Hierarchy Open> {" + (m_SceneHierarchyPanel.GetSceneHierarchyVisible() ? "true" : "false") + "}\r";
		out = out + "<Properties Open> {" + (m_SceneHierarchyPanel.GetPropertiesVisible() ? "true" : "false") + "}\r";
		out = out + "<Notifications Open> {" + (m_PopupsAndNotifications.GetNotificationsVisible() ? "true" : "false") + "}\r";
		out = out + "<Content Browser Open> {" + (m_ContentBrowser.GetContentBrowserVisible() ? "true" : "false") + "}\r";
		out = out + "<Text Editor Open> {" + (m_TextEditor.GetTextEditorVisible() ? "true" : "false") + "}\r";
		out = out + "<Curve Editor Open> {" + (m_CurveEditor.GetCurveEditorVisible() ? "true" : "false") + "}\r";
		out = out + "<Image Editor Open> {" + (m_ImageEditor.GetImageEditorVisible() ? "true" : "false") + "}\r";
		out = out + "<Console Open> {" + (m_ConsoleWindow.GetConsoleWindowVisible() ? "true" : "false") + "}\r";

		//Load Ini File
		std::string result;
		std::ifstream in("imgui.ini", std::ios::in | std::ios::binary);
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
				DY_CORE_ERROR("Could not read from Ini file");
				return;
			}
		}

		out = out + "<Ini Contents> {" + result + "}\r";

		std::ofstream fout(path);
		fout << out.c_str();
	}

}


