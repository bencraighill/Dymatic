#include "EditorLayer.h"
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Dymatic/Scene/SceneSerializer.h"

#include "Dymatic/Utils/PlatformUtils.h"

#include "ImGuizmo.h"

#include "Dymatic/Math/Math.h"

#include "stb_image/stb_image.h"

#include <cmath>

namespace Dymatic {

	EditorLayer::EditorLayer()
		: Layer("EditorLayer"), m_CameraController(1280.0f / 720.0f), m_SquareColor({ 0.2f, 0.3f, 0.8f, 1.0f })
	{
	}


	void EditorLayer::OnAttach()
	{
		DY_PROFILE_FUNCTION();

		m_NodeEditorPannel.Application_Initialize();

		//Add Preference Data load in here
		auto& preferencesData = m_PreferencesPannel.GetPreferences().m_PreferenceData;

		m_CheckerboardTexture = Texture2D::Create("assets/textures/Checkerboard.png");


		FramebufferSpecification fbSpec;
		fbSpec.Width = 1280;
		fbSpec.Height = 720;
		m_Framebuffer = Framebuffer::Create(fbSpec);

		m_ActiveScene = CreateRef<Scene>();

		m_EditorCamera = EditorCamera(30.0f, 1.778f, 0.1f, 1000.0f);

		Application::Get().SetCloseWindowCallback(true);

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
# if 1

		auto testSquare = m_ActiveScene->CreateEntity("Test Square");
		testSquare.AddComponent<SpriteRendererComponent>(glm::vec4{ 1.0f, 1.0f, 1.0f, 1.0f });

		class BlueprintClass : public ScriptableEntity
		{
//#include "../DymaticNodeLibrary.h"
			//virtual void OnCreate() override
			//{
			//	//FunctionIn funcUpload_Branch_15;
			//	//FunctionIn funcUpload_Make_Bool_12;
			//	//
			//	//FunctionReturn absz;
			//	//FunctionReturn s_FunctionReturnA;
			//	//FunctionReturn s_FunctionReturnB;
			//	//
			//	//funcUpload_Make_Bool_12.PinValues[0].Bool = true;
			//	//funcUpload_Branch_15.PinValues[0].Bool = Make_Bool(funcUpload_Make_Bool_12).PinValues[0].Bool;
			//	//
			//	//
			//	////s_FunctionReturn = Branch(absz.PinValues[1].Bool = Make_Bool(funcUpload_Make_Bool_12).PinValues[1].Bool);
			//	//s_FunctionReturnA = Branch(funcUpload_Branch_15);
			//}
		};

		testSquare.AddComponent<NativeScriptComponent>().Bind<BlueprintClass>();
#endif

		m_SceneHierarchyPanel.SetContext(m_ActiveScene);
	}

	void EditorLayer::OnDetach()
	{
		DY_PROFILE_FUNCTION();
	}

	void EditorLayer::OnUpdate(Timestep ts)
	{
		DY_PROFILE_FUNCTION();

		m_DeltaTime = ts;

		if (m_PopupsAndNotifications.GetPopupOpen() == false) {
			m_ProgramTime += ts;
			m_LastSaveTime += ts;
		}

		if (Application::Get().GetCloseWindowButtonPressed())
		{
			SaveAndExit();
		}

		if (m_PreferencesPannel.GetPreferences().m_PreferenceData.autosaveEnabled)
		{
			//AutoSave
			if (m_LastSaveTime >= m_PreferencesPannel.GetPreferences().m_PreferenceData.autosaveTime * 60.0f && m_CurrentFilepath != "")
			{
				SaveScene();
				DY_CORE_INFO("Autosave Complete: Program Time - {0}", m_ProgramTime);
				//Add Autosave Complete Notification
				{
					m_PopupsAndNotifications.Notification("##AutosaveComplete", 1, "Autosave Completed", ("Autosaved current scene at program time: \n" + std::to_string((int)m_ProgramTime)), {"Dismiss"}, false);
				}
			}

			//Warning of autosave
			if (m_LastSaveTime >= (m_PreferencesPannel.GetPreferences().m_PreferenceData.autosaveTime * 60.0f) - 10 && m_LastSaveTime <= (m_PreferencesPannel.GetPreferences().m_PreferenceData.autosaveTime * 60.0f) - 10 + ts && m_CurrentFilepath != "")
			{
				m_PopupsAndNotifications.Notification("##AutosavePending", 0, "Autosave pending...", "Autosave of current scene will commence\n in 10 seconds.", {"Cancel", "Save Now"}, false, 10.0f);
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

		// Update
		if (m_ViewportFocused)
			m_CameraController.OnUpdate(ts);

		m_EditorCamera.OnUpdate(ts);


		// Render
		Renderer2D::ResetStats();
		m_Framebuffer->Bind();
		//RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.1f, 1 });
		RenderCommand::SetClearColor({ 0.28f, 0.28f, 0.28f, 1.0f });
		RenderCommand::Clear();
		m_Framebuffer->Bind();

		// Update scene
		m_ActiveScene->OnUpdateEditor(ts, m_EditorCamera);
		m_ActiveScene->OnUpdateRuntime(ts);

		auto [mx, my] = ImGui::GetMousePos();
		mx -= m_ViewportBounds[0].x;
		my -= m_ViewportBounds[0].y;
		auto viewportWidth = m_ViewportBounds[1].x - m_ViewportBounds[0].x;
		auto viewportHeight = m_ViewportBounds[1].y - m_ViewportBounds[0].y;
		my = viewportHeight - my;
		int mouseX = (int)mx;
		int mouseY = (int)my;
		if (mouseX >= 0 && mouseY >= 0 && mouseX < viewportWidth && mouseY < viewportHeight)
		{
			int pixel = m_ActiveScene->Pixel(mx, my);
			m_HoveredEntity = pixel == -1 ? Entity() : Entity((entt::entity)pixel, m_ActiveScene.get());
		}

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
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(maximized ? 0.0f : 5.0f, maximized ? 0.0f : 5.0f));
		ImGui::Begin("Dymatic Editor Dockspace Window", &dockspaceOpen, window_flags);
		ImVec2 dockspaceWindowPosition = ImGui::GetWindowPos();
		ImGui::PopStyleVar();

		if (opt_fullscreen)
			ImGui::PopStyleVar(2);

		//ImRect bb{ ImGui::GetWindowPos(), ImVec2{ImGui::GetWindowPos().x, ImGui::GetWindowPos().y + ImGui::GetWindowSize().y} };
		//const ImGuiID id = ImGui::GetCurrentWindow()->GetID("##LeftSplitter");
		//
		//float differenceX = 0.0f;
		//float differenceY = 10.0f;
		//
		//bool split_vertically = true;
		//float splitter_long_axis_size = ImGui::GetWindowSize().y;
		//float thickness = 10.0f;
		//
		//bb.Min = ImVec2{ ImGui::GetCurrentWindow()->DC.CursorPos.x + (split_vertically ? ImVec2(differenceX, 0.0f) : ImVec2(0.0f, differenceX)).x, ImGui::GetCurrentWindow()->DC.CursorPos.y + (split_vertically ? ImVec2(differenceX, 0.0f) : ImVec2(0.0f, differenceX)).y };
		//bb.Max = ImVec2{ bb.Min.x + ImGui::CalcItemSize(split_vertically ? ImVec2(thickness, splitter_long_axis_size) : ImVec2(splitter_long_axis_size, thickness), 0.0f, 0.0f).x, bb.Min.y + ImGui::CalcItemSize(split_vertically ? ImVec2(10.0f, splitter_long_axis_size) : ImVec2(splitter_long_axis_size, thickness), 0.0f, 0.0f).y };
		//
		//if (ImGui::SplitterBehavior(bb, id, ImGuiAxis_X, &differenceX, &differenceY, -10.0f, 10.0f))
		//{
		//	auto& window = Application::Get().GetWindow();
		//	window.SetSize(window.GetWidth() + differenceX, window.GetHeight());
		//}

		static bool windowResizeHeld = false;

		if (!Application::Get().GetWindow().IsWindowMaximized())
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
				case 2: {sizeX = window.GetWidth() + (ImGui::GetMousePos().x - prevMousePos.x); break; };
				}
				sizeY = window.GetHeight();

				sizeX = sizeX < 250 ? 250 : sizeX;
				sizeY = sizeY < 400 ? 400 : sizeY;

				window.SetSize(sizeX, sizeY);
				switch (selectionIndex)
				{
				case 0: {window.SetPosition(window.GetPositionX() + (prevWindowWidth - sizeX), window.GetPositionY()); break; }
				case 2: {break; }
				}
				

				prevMousePos = ImGui::GetMousePos();
				prevWindowWidth = window.GetWidth();
				prevWindowHeight = window.GetHeight();
				DY_CORE_INFO("Hovered");
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

			ImGui::GetWindowDrawList()->AddConvexPolyFilled(points, sizeof(points), ImGui::ColorConvertFloat4ToU32(ImVec4{ 0.15f, 0.15f, 0.15f, 1.0f }));
			ImGui::GetWindowDrawList()->AddPolyline(points, sizeof(points), ImGui::ColorConvertFloat4ToU32(ImVec4{ 0.35f, 0.35f, 0.35f, 1.0f }), true, 2.0f);

			if (ImGui::ImageButton(reinterpret_cast<void*>(m_DymaticLogo->GetRendererID()), ImVec2{ 20, 20 }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 }))
			{
				ImGui::OpenPopup("##MenuBarDymatic");
			}
			if (ImGui::BeginPopup("##MenuBarDymatic", ImGuiWindowFlags_NoMove))
			{
				if (ImGui::MenuItem("Splash Screen")) { m_ShowSplash = true; }
				if (ImGui::MenuItem("About Dymatic")) { m_PopupsAndNotifications.Popup("About Engine", "Dymatic Engine\nV1.2.1 (Development)\n\n\nDate: 2020 / 12 / 27\nHash: #O53X5g00PQ\nBranch: dev (repository up to date)\n\n\n Dymatic Engine is a free, open source software developed by BAS Solutions.\nView source files for licenses from vendor libraries.\nFor commercial projects a Dypermite License must be requested.", { "Learn More", "Ok" }); }
				ImGui::Separator();
				if (ImGui::MenuItem("Github")) {}
				if (ImGui::MenuItem("Uninstall")) {}
				ImGui::Separator();
				if (ImGui::BeginMenu("System"))
				{
					if (ImGui::MenuItem("View System Status")) {}
					ImGui::EndMenu();
				}
				ImGui::EndPopup();
			}
			if (ImGui::BeginMenu("File"))
			{
				// Disabling fullscreen would allow the window to be moved to the front of other windows, 
				// which we can't undo at the moment without finer window depth/z control.
				//ImGui::MenuItem("Fullscreen", NULL, &opt_fullscreen_persistant);1
				if (ImageMenuItem(m_IconSystemNew, "New", "Ctrl+N")) NewScene();
				if (ImageMenuItem(m_IconSystemOpen, "Open...", "Ctrl+O")) OpenScene();
				if (ImageMenuItem(m_IconSystemRecent, "Open Recent", "", true))
				{
					std::vector<std::string> recentFiles = GetRecentFiles();
					if (!recentFiles.empty())
					{
						for (int i = 0; i < recentFiles.size(); i++)
						{
							if (ImGui::MenuItem((m_ContentBrowser.GetFullFileNameFromPath(m_ContentBrowser.SwapStringSlashesDouble(recentFiles[i]))).c_str()))
							{
								OpenSceneByFilepath(recentFiles[i]);
							}
						}
					}
					ImGui::EndMenu();
				}

				ImGui::Separator();

				if (ImageMenuItem(m_IconSystemSave, "Save", "Ctrl+S")) SaveScene();
				if (ImageMenuItem(m_IconSystemSaveAs, "Save As...", "Ctrl+Shift+S")) SaveSceneAs();

				ImGui::Separator();

				if (ImageMenuItem(m_IconSystemImport, "Import", "", true))
				{
					ImGui::MenuItem("FBX", "(.fbx)"); ImGui::MenuItem("Wavefront", "(.obj"); ImGui::MenuItem("Theme", "(.dytheme)");
					ImGui::EndMenu();
				}
				if (ImageMenuItem(m_IconSystemExport, "Export", "", true))
				{
					ImGui::MenuItem("FBX", "(.fbx)"); ImGui::MenuItem("Wavefront", "(.obj"); ImGui::MenuItem("Theme", "(.dytheme)");
					ImGui::EndMenu();
				}

				ImGui::Separator();

				if (ImageMenuItem(m_IconSystemQuit, "Exit", "Ctrl+Q")) SaveAndExit();
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Edit"))
			{
				if (ImageMenuItem(m_IconSystemPreferences, "Preferences", "")) m_PreferencesPannel.SetShowWindow(true);
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Window"))
			{
				if (ImageMenuItem(m_IconSystemConsoleWindow, "Toggle System Console", ""))
				{
					if (Log::IsConsoleVisible())
						Log::HideConsole();
					else
						Log::ShowConsole();
				}
				ImGui::EndMenu();
			}

			DY_CORE_INFO("Position: {0}, {1} - Size: {2}, {3}", Application::Get().GetWindow().GetPositionX(), Application::Get().GetWindow().GetPositionY(), Application::Get().GetWindow().GetWidth(), Application::Get().GetWindow().GetHeight());

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
			bool hovered, held;
			bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

			static ImVec2 offset = {};
			static bool init = true;
			static ImVec2 previousMousePos = {};
			if (held && !windowResizeHeld)
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

		//Toolbar Window
		{
			ImGui::Begin("Toolbar", NULL, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

			float buttonHeight = 30;
			float buttonWidth = 30;
			float sideOffset = 450;
			ImGui::Dummy(ImVec2{ ImGui::GetContentRegionAvail().x / 2, (ImGui::GetContentRegionAvail().y / 2) - (buttonHeight)});
			ImGui::Spacing();

			//Gizmo type buttons
			ImGui::SameLine(ImGui::GetContentRegionAvail().x - sideOffset + buttonWidth * 1);
			if (ImGui::ImageButton(reinterpret_cast<void*>((m_GizmoType == ImGuizmo::OPERATION::TRANSLATE ? m_IconTranslationActive : m_IconTranslation)->GetRendererID()), ImVec2{ buttonWidth, buttonHeight }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 }, 0))
				m_GizmoType = ImGuizmo::OPERATION::TRANSLATE;
			ImGui::SameLine(ImGui::GetContentRegionAvail().x - sideOffset + buttonWidth * 2);
			if (ImGui::ImageButton(reinterpret_cast<void*>((m_GizmoType == ImGuizmo::OPERATION::ROTATE ? m_IconRotationActive : m_IconRotation)->GetRendererID()), ImVec2{ buttonWidth, buttonHeight }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 }, 0))
				m_GizmoType = ImGuizmo::OPERATION::ROTATE;
			ImGui::SameLine(ImGui::GetContentRegionAvail().x - sideOffset + buttonWidth * 3);
			if (ImGui::ImageButton(reinterpret_cast<void*>((m_GizmoType == ImGuizmo::OPERATION::SCALE ? m_IconScaleActive : m_IconScale)->GetRendererID()), ImVec2{ buttonWidth, buttonHeight }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 }, 0))
				m_GizmoType = ImGuizmo::OPERATION::SCALE;

			//Gizmo space button
			ImGui::SameLine(ImGui::GetContentRegionAvail().x - sideOffset + buttonWidth * 4.5);
			if (ImGui::ImageButton(reinterpret_cast<void*>((m_GizmoSpace == ImGuizmo::MODE::LOCAL ? m_IconSpaceLocal : m_IconSpaceWorld)->GetRendererID()), ImVec2{ buttonWidth, buttonHeight }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 }, 0))
			{
				m_GizmoSpace = m_GizmoSpace == ImGuizmo::MODE::LOCAL ? ImGuizmo::MODE::WORLD : ImGuizmo::MODE::LOCAL;
			}
			
			//Translation Snap Caller
			ImGui::SameLine(ImGui::GetContentRegionAvail().x - sideOffset + buttonWidth * 6);
			if (ImGui::ImageButton(reinterpret_cast<void*>((m_TranslationSnap ? m_IconTranslationSnapActive : m_IconTranslationSnap)->GetRendererID()), ImVec2{ buttonWidth, buttonHeight }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 }, 0))
				m_TranslationSnap = !m_TranslationSnap;
			ImGui::SameLine(ImGui::GetContentRegionAvail().x - sideOffset + buttonWidth * 7);
			ImGui::PushID("TranslationID");
			bool openTranslationPopup =  (ImGui::ImageButton(reinterpret_cast<void*>(m_IconSnapDropdown->GetRendererID()), ImVec2{ 45, buttonHeight }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 }, 0));
			ImGui::PopID();
			if (openTranslationPopup)
			{
				ImGui::OpenPopup("TranslationSnapLevel");
			}
			ImGui::SameLine(ImGui::GetContentRegionAvail().x - sideOffset + buttonWidth * 7 + (45 / 2) - (ImGui::CalcTextSize(Math::FloatToString(m_TranslationSnapValue).c_str()).x / 2));
			ImGui::Text(Math::FloatToString(m_TranslationSnapValue).c_str());

			//Rotation Snap Caller
			ImGui::SameLine(ImGui::GetContentRegionAvail().x - sideOffset + buttonWidth * 9);
			if (ImGui::ImageButton(reinterpret_cast<void*>((m_RotationSnap ? m_IconRotationSnapActive : m_IconRotationSnap)->GetRendererID()), ImVec2{ buttonWidth, buttonHeight }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 }, 0))
				m_RotationSnap = !m_RotationSnap;
			ImGui::SameLine(ImGui::GetContentRegionAvail().x - sideOffset + buttonWidth * 10);
			ImGui::PushID("RotationID");
			bool openRotationPopup(ImGui::ImageButton(reinterpret_cast<void*>(m_IconSnapDropdown->GetRendererID()), ImVec2{ 45, buttonHeight }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 }, 0));
			ImGui::PopID();
			if (openRotationPopup)
			{
				ImGui::OpenPopup("RotationSnapLevel");
			}
			ImGui::SameLine(ImGui::GetContentRegionAvail().x - sideOffset + buttonWidth * 10 + (45 / 2) - (ImGui::CalcTextSize(Math::FloatToString(m_RotationSnapValue).c_str()).x / 2));
			ImGui::Text(Math::FloatToString(m_RotationSnapValue).c_str());

			//Scale Snap Caller
			ImGui::SameLine(ImGui::GetContentRegionAvail().x - sideOffset + buttonWidth * 12);
			if (ImGui::ImageButton(reinterpret_cast<void*>((m_ScaleSnap ? m_IconScaleSnapActive : m_IconScaleSnap)->GetRendererID()), ImVec2{ buttonWidth, buttonHeight }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 }, 0))
				m_ScaleSnap = !m_ScaleSnap;
			ImGui::SameLine(ImGui::GetContentRegionAvail().x - sideOffset + buttonWidth * 13);
			ImGui::PushID("ScaleID");
			bool openScalePopup (ImGui::ImageButton(reinterpret_cast<void*>(m_IconSnapDropdown->GetRendererID()), ImVec2{ 45, buttonHeight }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 }, 0));
			ImGui::PopID();
			if (openScalePopup)
			{
				ImGui::OpenPopup("ScaleSnapLevel");
			}
			ImGui::SameLine(ImGui::GetContentRegionAvail().x - sideOffset + buttonWidth * 13 + (45 / 2) - (ImGui::CalcTextSize(Math::FloatToString(m_ScaleSnapValue).c_str()).x / 2));
			ImGui::Text(Math::FloatToString(m_ScaleSnapValue).c_str());

			//Snapping Setter Events
			{
				if (ImGui::BeginPopup("TranslationSnapLevel"))
				{
					float snapOptions[9] = { 0.01f, 0.05f, 0.1f, 0.5f, 1.0f, 5.0f, 10.0f, 50.0f, 100.0f };
					for (int i = 0; i < 9; i++)
					{
						if (ImGui::MenuItem((Math::FloatToString(snapOptions[i])).c_str()))
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
						if (ImGui::MenuItem(((Math::FloatToString(snapOptions[i])) + " degrees").c_str()))
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
						if (ImGui::MenuItem((Math::FloatToString(snapOptions[i])).c_str()))
						{
							m_ScaleSnapValue = snapOptions[i];
						}
					}
					ImGui::EndPopup();
				}
			}

			ImGui::End();
		}

		ImGui::Begin("Info", NULL, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		ImGui::Dummy(ImVec2{ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize("V1.2.1").x - 5, 0});
		ImGui::SameLine();
		ImGui::TextDisabled("V1.2.1");
		ImGui::End();

		{
		//Preferences Pannel
		m_PreferencesPannel.OnImGuiRender();
		auto& data = m_PreferencesPannel.m_PreferencesMessage;
		if (data.title != "" && data.message != "")
		{
			m_PopupsAndNotifications.Popup(data);
			data = {};
		}
		}

		//Content Browser Window
		m_ContentBrowser.OnImGuiRender(m_DeltaTime);
		auto& contentFileToOpen = m_ContentBrowser.GetFileToOpen();
		if (contentFileToOpen != "")
		{
			if (m_ContentBrowser.GetFileFormat(contentFileToOpen) == ".dymatic") { OpenSceneByFilepath(contentFileToOpen); }
			if (m_ContentBrowser.GetFileFormat(contentFileToOpen) == ".dytheme") { m_PopupsAndNotifications.Popup("Install Theme", "File: " + m_ContentBrowser.GetFullFileNameFromPath(contentFileToOpen) + "\n\nDo you wish to install this dytheme file? It will override you current theme.\nTo save your current theme, export it from Preferences.\n", { "Cancel", "Install" }); m_PreferencesPannel.SetRecentDythemePath(contentFileToOpen); }
			contentFileToOpen = "";
		}

		//Scene Hierarchy and properties pannel
		m_SceneHierarchyPanel.OnImGuiRender();

		ImGui::Begin("Editor Window");
		m_NodeEditorPannel.Application_Frame();
		ImGui::End();

		//Node Editor Update
		m_NodeEditor.OnImGuiRender();

		//Notifications Pannel Event Check
		auto popupData = m_PopupsAndNotifications.PopupUpdate();
		auto popupName = popupData.title;
		auto popupButton = popupData.buttonClicked;
		if (popupName == "Unsaved Changes")
		{
			switch (popupButton)
			{
			case 1: { CloseProgramWindow(); break; }
			case 2: {if (SaveScene()) { CloseProgramWindow(); break; }}
			}
		}
		if (popupName == "Dytheme Read Error")
		{
			switch (popupButton)
			{
			case 0: { m_PreferencesPannel.RetryDythemeFile(); break; }
			}
		}
		if (popupName == "Install Theme")
		{
			switch (popupButton)
			{
			case 1: { m_PreferencesPannel.RetryDythemeFile(); break; }
			}
		}
		if (popupName == "About Engine")
		{
			switch (popupButton)
			{
			case 0: { break; }
			}
		}

		NotificationData NotificationReturn = m_PopupsAndNotifications.NotificationUpdate(m_ProgramTime);
		if (NotificationReturn.nameID != "")
		{
			if (NotificationReturn.nameID == "##AutosavePending")
				switch (NotificationReturn.buttonClicked)
				{
				case 0: { m_LastSaveTime = 1.0f; break; }
				case 1: { m_LastSaveTime = m_PreferencesPannel.GetPreferences().m_PreferenceData.autosaveTime * 60; break; }
				}
		}

		ImGui::Begin("Stats");

		auto stats = Renderer2D::GetStats();
		ImGui::Text("Renderer2D Stats:");
		ImGui::Text("Draw Calls: %d", stats.DrawCalls);
		ImGui::Text("Quads: %d", stats.QuadCount);
		ImGui::Text("Vertices: %d", stats.GetTotalVertexCount());
		ImGui::Text("Indices: %d", stats.GetTotalIndexCount());

		std::string name = "Null";
		if ((entt::entity)m_HoveredEntity != entt::null)
			name = m_HoveredEntity.GetComponent<TagComponent>().Tag;
		ImGui::Text("Hovered Entity: %s", name.c_str());

		ImGui::End();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
		ImGui::Begin("Viewport");
		auto viewportOffset = ImGui::GetCursorPos(); // includes tab bar

		m_ViewportFocused = ImGui::IsWindowFocused();
		m_ViewportHovered = ImGui::IsWindowHovered();
		//TODO: fix the whole viewport hovered situation
		//Application::Get().GetImGuiLayer()->BlockEvents(!m_ViewportFocused && !m_ViewportHovered);
		Application::Get().GetImGuiLayer()->BlockEvents(false);

		ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
		m_ViewportSize = { viewportPanelSize.x, viewportPanelSize.y };

		uint64_t textureID = m_Framebuffer->GetColorAttachmentRendererID();
		//ImGui::Image(reinterpret_cast<void*>(textureID), ImVec2{ m_ViewportSize.x, m_ViewportSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
		ImGui::GetWindowDrawList()->AddImage(reinterpret_cast<void*>(textureID), ImGui::GetWindowPos(), ImVec2{ ImGui::GetWindowPos().x + m_ViewportSize.x, ImGui::GetWindowPos().y + m_ViewportSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

		auto windowSize = ImGui::GetWindowSize();
		ImVec2 minBound = ImGui::GetWindowPos();
		minBound.x += viewportOffset.x;
		minBound.y += viewportOffset.y;

		ImVec2 maxBound = { minBound.x + windowSize.x, minBound.y + windowSize.y };
		m_ViewportBounds[0] = { minBound.x, minBound.y };
		m_ViewportBounds[1] = { maxBound.x, maxBound.y };

		//Viewport View Settings
		ImGui::Dummy(ImVec2{ImGui::GetContentRegionAvail().x - 95, 0});
		ImGui::SameLine();
		static int currentKeyBindSearchItem = 3;
		ImTextureID shaderButtonTypes[4] = { reinterpret_cast<void*>(m_IconShaderWireframe->GetRendererID()), reinterpret_cast<void*>(m_IconShaderUnlit->GetRendererID()), reinterpret_cast<void*>(m_IconShaderSolid->GetRendererID()), reinterpret_cast<void*>(m_IconShaderRendered->GetRendererID()) };
		switch (ImGui::SwitchImageButtonEx("ShaderTypeButtonSwicth", shaderButtonTypes, 4, ImVec2{20, 20}, ImVec2{ 0, 1 }, ImVec2{ 1, 0 }, currentKeyBindSearchItem, ImVec2{ 85, 25 }))
		{
		case 0: {currentKeyBindSearchItem = 0; break; }
		case 1: {currentKeyBindSearchItem = 1; break; }
		case 2: {currentKeyBindSearchItem = 2; break; }
		case 3: {currentKeyBindSearchItem = 3; break; }
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

			static float yawUpdate;
			static float pitchUpdate;
			static bool updateAngles = false;

			if (updateAngles) { m_EditorCamera.SetYaw(glm::radians(Math::LerpAngle(glm::degrees(m_EditorCamera.GetYaw()), yawUpdate, 0.3f))); m_EditorCamera.SetPitch(glm::radians(Math::LerpAngle(glm::degrees(m_EditorCamera.GetPitch()), pitchUpdate, 0.3f))); if (Math::FloatDistance(Math::NormalizeAngle(yawUpdate, -180), glm::degrees(m_EditorCamera.GetYaw())) < 0.1f && Math::FloatDistance(Math::NormalizeAngle(pitchUpdate, -180), glm::degrees(m_EditorCamera.GetPitch())) < 0.1f) { updateAngles = false; } }

			if (camXAngle >= 0) { xPos = ImVec2{ pos.x + (armLength * ((camXAngle - 90) / 90)), pos.y + (armLength * (((camXAngle < 90 ? (camXAngle) : ((camXAngle - 90) * -1 + 90)) / 90) * (camYAngle > 0 ? (camYAngle < 90 ? (camYAngle / 90 * -1) : (((camYAngle - 90) * -1 + 90) * -1 / 90)) : (camYAngle > -90 ? (camYAngle * -1 / 90) : ((((camYAngle + 90) * -1 - 90) * -1) / 90))))) }; }
			else if (camXAngle < 0) { xPos = ImVec2{ pos.x + (armLength * ((camXAngle * -1 - 90) / 90)), pos.y + (armLength * (((camXAngle > -90 ? (camXAngle) : ((camXAngle + 90) * -1 - 90)) / 90) * (camYAngle > 0 ? (camYAngle < 90 ? (camYAngle / 90 * -1) : (((camYAngle - 90) * -1 + 90) * -1 / 90)) : (camYAngle > -90 ? (camYAngle * -1 / 90) : ((((camYAngle + 90) * -1 - 90) * -1) / 90))))) }; }

			if (camXAngle >= 0) { yPos = ImVec2{ pos.x + (armLength * ((camXAngle > 90 ? ((camXAngle - 90) * -1 + 90) : (camXAngle)) / 90)), pos.y + (armLength * (((camXAngle - 90) / 90 * -1) * (camYAngle > 0 ? (camYAngle < 90 ? (camYAngle / 90 * -1) : (((camYAngle - 90) * -1 + 90) * -1 / 90)) : (camYAngle > -90 ? (camYAngle * -1 / 90) : ((((camYAngle + 90) * -1 - 90) * -1) / 90))))) }; }
			else if (camXAngle < 0) { yPos = ImVec2{ pos.x + (armLength * ((camXAngle < -90 ? ((camXAngle + 90) * -1 - 90) : (camXAngle)) / 90)), pos.y + (armLength * (((camXAngle + 90) * -1 / 90 * -1) * (camYAngle > 0 ? (camYAngle < 90 ? (camYAngle / 90 * -1) : (((camYAngle - 90) * -1 + 90) * -1 / 90)) : (camYAngle > -90 ? (camYAngle * -1 / 90) : ((((camYAngle + 90) * -1 - 90) * -1) / 90))))) }; }

			zPos = ImVec2{ zPos.x, pos.y + (armLength * (camYAngle > 0 ? ((camYAngle - 90) / 90) : ((camYAngle + 90) * -1 / 90))) };

			//ImGui Draw

			static bool activeCirclePress = false;
			static bool activeCircleHovered = false;

			ImGui::GetWindowDrawList()->AddCircleFilled(pos, armLength + circleRadius, activeCircleHovered ? ImGui::ColorConvertFloat4ToU32(ImVec4{ 0.5f, 0.5f, 0.5f, 0.5f }) : ImGui::ColorConvertFloat4ToU32(ImVec4{ 0.5f, 0.5f, 0.5f, 0.0f }));
			if (sqrt(pow(ImGui::GetMousePos().y - pos.y, 2) + pow(ImGui::GetMousePos().x - pos.x, 2)) < (armLength + circleRadius) && !activeCirclePress)
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
					bool hovered, held;
					bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

					if (pressed)
					{
						yawUpdate = floor(glm::degrees(m_EditorCamera.GetYaw()) / 360) * 360 - 90.0f;
						pitchUpdate = floor(glm::degrees(m_EditorCamera.GetYaw()) / 360) * 360;
						updateAngles = true;
					}

					ImGui::GetWindowDrawList()->AddLine(pos, xPos, hovered ? hoveredColor : (inFront ? xCol : xColDisabled), thicknessVal);
					ImGui::GetWindowDrawList()->AddCircleFilled(xPos, circleRadius, hovered ? hoveredColor : (inFront ? xCol : xColDisabled));
					ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
					ImGui::GetWindowDrawList()->AddText(ImVec2{ xPos.x - ImGui::CalcTextSize("W").x * 0.33f, xPos.y - ImGui::CalcTextSize("W").y / 2 }, fontColor, "X");
					ImGui::PopFont();
				}

				if (highestValueIndex == 1)
				{
					ImRect bb(ImVec2{ xPosAlt.x - circleRadius, xPosAlt.y - circleRadius }, ImVec2{ xPosAlt.x + circleRadius, xPosAlt.y + circleRadius });
					ImGuiID id = ImGui::GetCurrentWindow()->GetID("IdsSecondaryX");
					bool hovered, held;
					bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

					if (pressed)
					{
						yawUpdate = floor(glm::degrees(m_EditorCamera.GetYaw()) / 360) * 360 + 90.0f;
						pitchUpdate = floor(glm::degrees(m_EditorCamera.GetYaw()) / 360) * 360;
						updateAngles = true;
					}

					ImGui::GetWindowDrawList()->AddCircleFilled(xPosAlt, circleRadius, hovered ? hoveredColor : (inFront ? xCol : xColDisabled));
				}

				if (highestValueIndex == 2)
				{
					yPos = ImVec2{ (yPos.x - pos.x) * -1 + pos.x, (yPos.y - pos.y) * -1 + pos.y };

					ImRect bb(ImVec2{ yPos.x - circleRadius, yPos.y - circleRadius }, ImVec2{ yPos.x + circleRadius, yPos.y + circleRadius });
					ImGuiID id = ImGui::GetCurrentWindow()->GetID("IdsMainY");
					bool hovered, held;
					bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

					if (pressed)
					{
						yawUpdate = floor(glm::degrees(m_EditorCamera.GetYaw()) / 360) * 360 + 180.0f;
						pitchUpdate = floor(glm::degrees(m_EditorCamera.GetYaw()) / 360) * 360;
						updateAngles = true;
					}

					ImGui::GetWindowDrawList()->AddLine(pos, yPos, hovered ? hoveredColor : (inFront ? yCol : yColDisabled), thicknessVal);
					ImGui::GetWindowDrawList()->AddCircleFilled(yPos, circleRadius, hovered ? hoveredColor : (inFront ? yCol : yColDisabled));
					ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
					ImGui::GetWindowDrawList()->AddText(ImVec2{ yPos.x - ImGui::CalcTextSize("W").x * 0.33f, yPos.y - ImGui::CalcTextSize("W").y / 2 }, fontColor, "Z");
					ImGui::PopFont();
				}

				if (highestValueIndex == 3)
				{
					ImRect bb(ImVec2{ yPosAlt.x - circleRadius, yPosAlt.y - circleRadius }, ImVec2{ yPosAlt.x + circleRadius, yPosAlt.y + circleRadius });
					ImGuiID id = ImGui::GetCurrentWindow()->GetID("IdsSecondaryY");
					bool hovered, held;
					bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

					if (pressed)
					{
						yawUpdate = floor(glm::degrees(m_EditorCamera.GetYaw()) / 360) * 360;
						pitchUpdate = floor(glm::degrees(m_EditorCamera.GetYaw()) / 360) * 360;
						updateAngles = true;
					}
					ImGui::GetWindowDrawList()->AddCircleFilled(yPosAlt, circleRadius, hovered ? hoveredColor : (inFront ? yCol : yColDisabled));
				}

				if (highestValueIndex == 4)
				{
					ImRect bb(ImVec2{ zPos.x - circleRadius, zPos.y - circleRadius }, ImVec2{ zPos.x + circleRadius, zPos.y + circleRadius });
					ImGuiID id = ImGui::GetCurrentWindow()->GetID("IdsMainZ");
					bool hovered, held;
					bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

					if (pressed)
					{
						yawUpdate = floor(glm::degrees(m_EditorCamera.GetYaw()) / 360) * 360;
						pitchUpdate = floor(glm::degrees(m_EditorCamera.GetYaw()) / 360) * 360 + 90;
						updateAngles = true;
					}

					ImGui::GetWindowDrawList()->AddLine(pos, zPos, hovered ? hoveredColor : (inFront ? zCol : zColDisabled), thicknessVal);
					ImGui::GetWindowDrawList()->AddCircleFilled(zPos, circleRadius, hovered ? hoveredColor : (inFront ? zCol : zColDisabled));
					ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
					ImGui::GetWindowDrawList()->AddText(ImVec2{ zPos.x - ImGui::CalcTextSize("W").x * 0.33f, zPos.y - ImGui::CalcTextSize("W").y / 2 }, fontColor, "Y");
					ImGui::PopFont();
				}

				if (highestValueIndex == 5)
				{
					zPosAlt = ImVec2{ (zPosAlt.x - pos.x) * -1 + pos.x, (zPosAlt.y - pos.y) * -1 + pos.y };

					ImRect bb(ImVec2{ zPosAlt.x - circleRadius, zPosAlt.y - circleRadius }, ImVec2{ zPosAlt.x + circleRadius, zPosAlt.y + circleRadius });
					ImGuiID id = ImGui::GetCurrentWindow()->GetID("IdsSecondaryZ");
					bool hovered, held;
					bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

					if (pressed)
					{
						yawUpdate = floor(glm::degrees(m_EditorCamera.GetYaw()) / 360) * 360;
						pitchUpdate = floor(glm::degrees(m_EditorCamera.GetYaw()) / 360) * 360 - 90;
						updateAngles = true;
					}

					ImGui::GetWindowDrawList()->AddCircleFilled(zPosAlt, circleRadius, hovered ? hoveredColor : (inFront ? zCol : zColDisabled));
				}

				bool checkValSuccess = false;
				for (int checkVal : a)
				{
					if (checkVal != -1000.0f) { checkValSuccess = true; }
				}
				if (!checkValSuccess) { circleOverlayCheck = false; break; }
			}
		}


		// Gizmos
		Entity selectedEntity = m_SceneHierarchyPanel.GetSelectedEntity();

		if (selectedEntity && m_GizmoType != -1)
		{
			ImGuizmo::SetOrthographic(false);
			ImGuizmo::SetDrawlist();

			float windowWidth = (float)ImGui::GetWindowWidth();
			float windowHeight = (float)ImGui::GetWindowHeight();
			ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, windowWidth, windowHeight);
			

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

		if (m_ShowSplash)
		{
			bool focused = false;
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 20.0f);
			ImGui::Begin("##Editor Splash", &m_ShowSplash, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);
			if (ImGui::IsWindowFocused()) { focused = true; }
			ImGui::SetWindowPos(ImVec2((((io.DisplaySize.x * 0.5f) - (ImGui::GetWindowWidth() / 2)) + dockspaceWindowPosition.x), (((io.DisplaySize.y * 0.5f) - (ImGui::GetWindowHeight() / 2)) + dockspaceWindowPosition.y)));
			ImGui::Image(reinterpret_cast<void*>(m_DymaticSplash->GetRendererID()), ImVec2{ 488, 267 }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
			ImGui::SameLine();
			ImGui::Dummy(ImVec2{ 0, 40 - (ImGui::CalcTextSize("W").y / 2) });
			ImGui::SameLine();
			std::string versionName = "V1.2.1";
			ImGui::SameLine( 488 -ImGui::CalcTextSize(versionName.c_str()).x);
			ImGui::Text(versionName.c_str());
			ImGui::Columns(2);
			ImGui::TextDisabled("New File");
			ImGui::Image(reinterpret_cast<void*>(m_IconSystemNew->GetRendererID()), ImVec2{ 15, 15 }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
			ImGui::SameLine();
			if (ImGui::Selectable("General Workspace")) { focused = false; }
			if (ImGui::IsItemFocused()) { focused = true; }
			ImGui::Image(reinterpret_cast<void*>(m_IconSystemNew->GetRendererID()), ImVec2{ 15, 15 }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
			ImGui::SameLine();
			if (ImGui::Selectable("Scultping Environment")) { focused = false; }
			if (ImGui::IsItemFocused()) { focused = true; }
			ImGui::Image(reinterpret_cast<void*>(m_IconSystemNew->GetRendererID()), ImVec2{ 15, 15 }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
			ImGui::SameLine();
			if (ImGui::Selectable("Animation Workspace")) { focused = false; }
			if (ImGui::IsItemFocused()) { focused = true; }
			ImGui::Image(reinterpret_cast<void*>(m_IconSystemNew->GetRendererID()), ImVec2{ 15, 15 }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
			ImGui::SameLine();
			if (ImGui::Selectable("VFX Workspace")) { focused = false; }
			if (ImGui::IsItemFocused()) { focused = true; }
			ImGui::NextColumn();
			ImGui::TextDisabled("Recent Files");
			ImGui::SetNextWindowSize(ImVec2{488 / 2, 600});
			ImGui::BeginChild("##RecentFilesList");
			if (ImGui::IsWindowFocused()) { focused = true; }
			std::vector<std::string> recentFiles = GetRecentFiles();
			if (!recentFiles.empty())
			{
				for (int i = 0; i < recentFiles.size(); i++)
				{
					ImGui::Image(reinterpret_cast<void*>(m_DymaticLogo->GetRendererID()), ImVec2{ 15, 15 }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
					ImGui::SameLine();
					if (ImGui::Selectable(m_ContentBrowser.GetFullFileNameFromPath(m_ContentBrowser.SwapStringSlashesDouble(recentFiles[i])).c_str()))
					{
						focused = false;
						OpenSceneByFilepath(recentFiles[i]);
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

		////Splash Popup
		//if (m_ShowSplash == true)
		//{
		//	m_ShowSplash = false;
		//	ImGui::OpenPopup("##SplashPopup");
		//}
		//
		//ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 5.0f);
		//if (ImGui::BeginPopup("##SplashPopup"))
		//{
		//	ImGui::Image(reinterpret_cast<void*>(m_DymaticSplash->GetRendererID()), ImVec2{ 200, 125 }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
		//	ImGui::EndPopup();
		//}
		//ImGui::PopStyleVar();

		ImGui::End();
	}

	void EditorLayer::OnEvent(Event& e)
	{
		m_CameraController.OnEvent(e);
		m_EditorCamera.OnEvent(e);
		m_NodeEditor.OnEvent(e);
		m_NodeEditorPannel.OnEvent(e);

		EventDispatcher dispatcher(e);

		m_KeyPressed.GetKeyCode();

		dispatcher.Dispatch<KeyPressedEvent>(DY_BIND_EVENT_FN(EditorLayer::OnKeyPressed));
		dispatcher.Dispatch<MouseButtonPressedEvent>(DY_BIND_EVENT_FN(EditorLayer::OnMouseButtonPressed));
	}

	void EditorLayer::UpdateKeys(BindCatagory bindCatagory)
	{
		bool control = Input::IsKeyPressed(Key::LeftControl) || Input::IsKeyPressed(Key::RightControl);
		bool shift = Input::IsKeyPressed(Key::LeftShift) || Input::IsKeyPressed(Key::RightShift);
		bool alt = Input::IsKeyPressed(Key::LeftAlt) || Input::IsKeyPressed(Key::RightAlt);

		auto repeats = m_KeyPressed.GetRepeatCount();

		auto& keyBinds = m_PreferencesPannel.GetPreferences().m_PreferenceData.keyBinds;

		if (keyBinds.IsKey(NewSceneBind, m_KeyPressed.GetKeyCode(), m_MouseButtonPressed.GetMouseButton(), bindCatagory, control, shift, alt, repeats))				{ NewScene(); }
		else if (keyBinds.IsKey(OpenSceneBind, m_KeyPressed.GetKeyCode(), m_MouseButtonPressed.GetMouseButton(), bindCatagory, control, shift, alt, repeats))		{ OpenScene(); }
		else if (keyBinds.IsKey(SaveSceneBind, m_KeyPressed.GetKeyCode(), m_MouseButtonPressed.GetMouseButton(), bindCatagory, control, shift, alt, repeats))		{ SaveScene(); }
		else if (keyBinds.IsKey(SaveSceneAsBind, m_KeyPressed.GetKeyCode(), m_MouseButtonPressed.GetMouseButton(), bindCatagory, control, shift, alt, repeats))		{ SaveSceneAs(); }
		else if (keyBinds.IsKey(QuitBind, m_KeyPressed.GetKeyCode(), m_MouseButtonPressed.GetMouseButton(), bindCatagory, control, shift, alt, repeats))				{ SaveAndExit(); }
		else if (keyBinds.IsKey(SelectObjectBind, m_KeyPressed.GetKeyCode(), m_MouseButtonPressed.GetMouseButton(), bindCatagory, control, shift, alt, repeats))		{ if (!ImGuizmo::IsUsing() && !ImGuizmo::IsOver() && !Input::IsKeyPressed(Key::LeftAlt)) m_SceneHierarchyPanel.SetSelectedEntity(m_HoveredEntity); }
		else if (keyBinds.IsKey(GizmoNoneBind, m_KeyPressed.GetKeyCode(), m_MouseButtonPressed.GetMouseButton(), bindCatagory, control, shift, alt, repeats))		{ m_GizmoType = -1; }
		else if (keyBinds.IsKey(GizmoTranslateBind, m_KeyPressed.GetKeyCode(), m_MouseButtonPressed.GetMouseButton(), bindCatagory, control, shift, alt, repeats))	{ m_GizmoType = ImGuizmo::OPERATION::TRANSLATE; }
		else if (keyBinds.IsKey(GizmoRotateBind, m_KeyPressed.GetKeyCode(), m_MouseButtonPressed.GetMouseButton(), bindCatagory, control, shift, alt, repeats))		{ m_GizmoType = ImGuizmo::OPERATION::ROTATE; }
		else if (keyBinds.IsKey(GizmoScaleBind, m_KeyPressed.GetKeyCode(), m_MouseButtonPressed.GetMouseButton(), bindCatagory, control, shift, alt, repeats))		{ m_GizmoType = ImGuizmo::OPERATION::SCALE; }
		else if (keyBinds.IsKey(DuplicateBind, m_KeyPressed.GetKeyCode(), m_MouseButtonPressed.GetMouseButton(), bindCatagory, control, shift, alt, repeats))		{ if (m_SceneHierarchyPanel.GetSelectedEntity()) m_SceneHierarchyPanel.SetSelectedEntity(m_ActiveScene->DuplicateEntity(m_SceneHierarchyPanel.GetSelectedEntity())); }
		else if (keyBinds.IsKey(RenameBind, m_KeyPressed.GetKeyCode(), m_MouseButtonPressed.GetMouseButton(), bindCatagory, control, shift, alt, repeats))			{ if (m_ContentBrowser.GetSelectedFile().filename != "") m_ContentBrowser.OpenRenamePopup(m_ContentBrowser.GetSelectedFile()); }
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

	void EditorLayer::NewScene()
	{
		m_ActiveScene = CreateRef<Scene>();
		m_ActiveScene->OnViewportResize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
		m_SceneHierarchyPanel.SetContext(m_ActiveScene);
		m_CurrentFilepath = "";

		//Reset Scene time values
		m_LastSaveTime = 0;
		m_ProgramTime = 0;
		m_PopupsAndNotifications.ClearNotificationList();
	}

	void EditorLayer::OpenScene()
	{
		std::optional<std::string> filepath = FileDialogs::OpenFile("Dymatic Scene (*.dymatic)\0*.dymatic\0");
		if (filepath)
		{
			OpenSceneByFilepath(*filepath);
		}
	}

	void EditorLayer::OpenSceneByFilepath(const std::string& filepath)
	{
		m_ActiveScene = CreateRef<Scene>();
		m_ActiveScene->OnViewportResize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
		m_SceneHierarchyPanel.SetContext(m_ActiveScene);

		SceneSerializer serializer(m_ActiveScene);
		serializer.Deserialize(filepath);
		m_CurrentFilepath = filepath;

		AddRecentFile(filepath);

		//Reset Scene time values
		m_LastSaveTime = 0;
		m_ProgramTime = 0;
		m_PopupsAndNotifications.ClearNotificationList();
	}

	void EditorLayer::SaveSceneToFilepath(const std::string& filepath)
	{
		SceneSerializer serializer(m_ActiveScene);
		serializer.Serialize(filepath);
		m_CurrentFilepath = filepath;
		m_LastSaveTime = 0;
	}

	bool EditorLayer::SaveScene()
	{
		if (m_CurrentFilepath == "")
			return SaveSceneAs();
		else
		{
			SaveSceneToFilepath(m_CurrentFilepath);
			return true;
		}
		return false;
	}

	bool EditorLayer::SaveSceneAs()
	{
		std::optional<std::string> filepath = FileDialogs::SaveFile("Dymatic Scene (*.dymatic)\0*.dymatic\0");
		if (filepath)
		{
			SaveSceneToFilepath(*filepath);
			AddRecentFile(*filepath);
			return true;
		}
		return false;
	}

	void EditorLayer::SaveAndExit()
	{
		m_PopupsAndNotifications.Popup("Unsaved Changes", "Would you like to save changes before exiting?\nWARNING: Any unsaved changes will be lost!\n", { "Cancel", "Discard", "Save" });
	}

	void EditorLayer::CloseProgramWindow()
	{
		Application::Get().Close();
	}

	void EditorLayer::AddRecentFile(std::string filepath)
	{
		std::ifstream file("saved/RecentPaths.txt", std::ios::binary);
		std::string fileStr;

		std::istreambuf_iterator<char> inputIt(file), emptyInputIt;
		std::back_insert_iterator<std::string> stringInsert(fileStr);

		copy(inputIt, emptyInputIt, stringInsert);

		//Check for existance
		if (fileStr.find(filepath + "\r") != -1)
		{
			fileStr = fileStr.erase(fileStr.find(filepath + "\r"), filepath.length() + 1);
		}

		std::ofstream fout("saved/RecentPaths.txt");
		fout << (filepath + "\r" + fileStr).c_str();
	}

	

	std::vector<std::string> EditorLayer::GetRecentFiles()
	{
		std::vector<std::string> recentFiles;

		std::ifstream file("saved/RecentPaths.txt", std::ios::binary);
		std::string fileStr;

		std::istreambuf_iterator<char> inputIt(file), emptyInputIt;
		std::back_insert_iterator<std::string> stringInsert(fileStr);

		copy(inputIt, emptyInputIt, stringInsert);

		while (fileStr.find_first_of("\r") != -1)
		{
			recentFiles.push_back(fileStr.substr(0, fileStr.find_first_of("\r")));
			//if (fileStr.find_first_of("\r") != fileStr.length() - 1)
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

	//ImGui Menu Item with Dymatic Image
	bool EditorLayer::ImageMenuItem(Ref<Texture2D> texture, std::string item, std::string shortcut, bool menu, ImVec2 imageSize)
	{
		ImGui::Image(reinterpret_cast<void*>(texture->GetRendererID()), imageSize, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
		ImGui::SameLine();
		if (menu)
		{
			return (ImGui::BeginMenu(item.c_str()));
		}
		else
		{
			return (ImGui::MenuItem(item.c_str(), shortcut.c_str()));
		}
	}

}


