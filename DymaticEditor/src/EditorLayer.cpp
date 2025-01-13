#include "EditorLayer.h"

#include "Dymatic/Asset/AssetManager.h"
#include "Dymatic/Asset/AssetThread.h"
#include "Dymatic/Scripting/ScriptEngine.h"
#include "Dymatic/Scripting/ScriptGlue.h"
#include "Dymatic/Audio/AudioEngine.h"

#include "EditorResources.h"

#include "Dymatic/Core/TransactionManager.h"
#include "Transactions/TransformTransaction.h"
#include "Transactions/EntityTransaction.h"

#include "Dymatic/Utils/PlatformUtils.h"
#include "Dymatic/Math/Math.h"
#include "Dymatic/Math/StringUtils.h"

#include "Settings/Preferences.h"
#include "Settings/ProjectSettings.h"

#include "Dymatic/Networking/NetworkManager.h"

#include "Tools/PluginLoader.h"
#include "Tools/VisualStudioInterface.h"
#include "Tools/LiveLink.h"

// Viewer panels
#include "Panels/Assets/TextureViewerPanel.h"
#include "Panels/Assets/FontViewerPanel.h"
#include "Panels/Assets/MeshViewerPanel.h"
#include "Panels/Assets/SkeletonViewerPanel.h"
#include "Panels/Assets/MaterialPanel.h"
#include "Panels/Assets/MaterialInstancePanel.h"
#include "Panels/Assets/ParticleSystemPanel.h"
#include "Panels/Assets/AnimationGraphPanel.h"
#include "Panels/Assets/VirtualTexturePanel.h"
#include "Panels/Assets/VideoPlayerPanel.h"

#include "Dymatic/Core/Memory.h"
#include "Dymatic/Video/VideoReader.h"

#include "Fonts.h"
#include "TextSymbols.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "Dymatic/UI/UI.h"
#include "Panels/UI.h"

#include <ctime>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include "ImGuizmo.h"
#include <imgui/imgui_stdlib.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_query.hpp>

#include <stb_image/stb_image_write.h>

#include <spdlog/fmt/chrono.h>

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

	namespace Utils {
	
		static std::string GetFileSafeTimestamp()
		{
			char filename[MAX_PATH];
			time_t time = std::time(0);
			strftime(filename, sizeof(filename), "%Y-%m-%d %H%M%S", localtime(&time));

			return filename;
		}

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

		EditorResources::Init();
		Scene::InitEditorResources();

		LoadApplicationCrashManager();

		auto& window = Application::Get().GetWindow();
		window.SetTitlebarHoveredQueryCallback(&IsTitlebarHovered);
		window.SetMinimizeHoveredQueryCallback(&IsMinimizeHovered);
		window.SetMaximizeHoveredQueryCallback(&IsMaximiseHovered);
		window.SetCloseHoveredQueryCallback(&IsCloseHovered);

		Log::SetCallback([&](const Log::Message& message)
		{
			m_DebugMessages.push_back({ message.FormattedText, message.Level });
			m_LogPanel.OnLog(message);
		});

		ScriptGlue::SetOpenSceneCallback([&](UUID handle)
		{
			m_PostUpdateQueue.push_back(handle);
		});

		ScriptEngine::SetAssemblyReloadCallback([&]()
		{
			if (m_SceneState == SceneState::Play)
				OnSceneStop();

			Notification::Create("Assembly Reloaded", "A build for this project's C# application assembly was completed\nand all changes were reloaded.", {}, 5.0f);
			EditorResources::SoundCompileSuccess->Play();
		});

		Splash::Update("Initializing Preferences...", 95);
		Preferences::Init();

		Splash::Update("Initializing Python Interpreter...", 96);
		PythonTools::Init(this);

		PluginLoader::Init();

		Notification::Init();

		AudioEngine::SetGlobalVolume(Preferences::GetData().EditorVolume);

		Splash::Update("Initializing Editor Resources...", 97);
		m_EditorFont = Font::Create("Resources/Fonts/OpenSans-Regular.ttf");

		m_EditIcon = Texture2D::Create("Resources/Icons/Info/EditIcon.png");
		m_LoadingCogAnimation[0] = Texture2D::Create("Resources/Icons/Info/LoadingCog1.png");
		m_LoadingCogAnimation[1] = Texture2D::Create("Resources/Icons/Info/LoadingCog2.png");
		m_LoadingCogAnimation[2] = Texture2D::Create("Resources/Icons/Info/LoadingCog3.png");

		// Initialize ImGui
		{
			auto& io = ImGui::GetIO();
			io.InfinityString = CHARACTER_SYMBOL_INFINITY;
			io.NegativeInfinityString = "-" CHARACTER_SYMBOL_INFINITY;

			ImGuiLayer* imGuiLayer = Application::Get().GetImGuiLayer();

			// Load Icons Fonts (these will be merged with the default)
			imGuiLayer->AddIconFont("Resources/Fonts/fontawesome/Font Awesome 6 Pro-Solid-900.otf", 25.0f, 0xE000, 0xF8FF); // Font Awesome Solid (Dymatic Default)
			imGuiLayer->AddIconFont("Resources/Fonts/fontawesome/Font Awesome 6 Brands-Regular-400.otf", 25.0f, 0xE007, 0xF8E8); // Font Awesome Brands

			imGuiLayer->AddIconFont("Resources/Fonts/IconsFont.ttf", 25.0f, 0x700, 0x713);	// Window and Viewport Icons
			imGuiLayer->AddIconFont("Resources/Fonts/IconsFont.ttf", 12.0f, 0x714, 0x71C); // Gizmo Icons
			imGuiLayer->AddIconFont("Resources/Fonts/IconsFont.ttf", 25.0f, 0x71D, 0x763); // Main Icons

			imGuiLayer->AddIconFont("Resources/Fonts/IconsFont.ttf", 20.0f, 0x00A9, 0x00A9); // Copyright Symbol
			imGuiLayer->AddIconFont("Resources/Fonts/IconsFont.ttf", 20.0f, 0x00AE, 0x00AE); // Registered Symbol
			imGuiLayer->AddIconFont("Resources/Fonts/IconsFont.ttf", 10.0f, 0xF7, 0xF7); // Division Symbol
			imGuiLayer->AddIconFont("Resources/Fonts/IconsFont.ttf", 15.0f, 0x03BC, 0x03BC); // Mu Symbol
			imGuiLayer->AddIconFont("Resources/Fonts/IconsFont.ttf", 15.0f, 0x3C0, 0x3C0); // PI Symbol
			imGuiLayer->AddIconFont("Resources/Fonts/IconsFont.ttf", 20.0f, 0x2713, 0x2713); // Tick Symbol
			imGuiLayer->AddIconFont("Resources/Fonts/IconsFont.ttf", 25.0f, 0x221E, 0x221E); // Infinity Symbol
			imGuiLayer->AddIconFont("Resources/Fonts/IconsFont.ttf", 16.0f, 0x0384, 0x03D6); // Greek Alphabet

			// Load additional font styles/sizes
			io.Fonts->AddFontFromFileTTF("Resources/Fonts/opensans/OpenSans-Bold.ttf", 18.0f);
			io.Fonts->AddFontFromFileTTF("Resources/Fonts/opensans/OpenSans-Regular.ttf", 13.0f);
			io.Fonts->AddFontFromFileTTF("Resources/Fonts/opensans/OpenSans-Regular.ttf", 60.0f);

			// Font Awesome Regular (Separate font to avoid conflicts with Solid codepoints)
			// Note: This only currently loads the FA_PLAYER icon
			imGuiLayer->AddFontRanges("Resources/Fonts/fontawesome/Font Awesome 6 Pro-Solid-900.otf", 35.0f, 0xF183, 0xF183);
			imGuiLayer->AddFontRanges("Resources/Fonts/fontawesome/Font Awesome 6 Pro-Regular-400.otf", 35.0f, 0xF183, 0xF183);

			// Setup ImGuizmo
			auto& style = ImGuizmo::GetStyle();
			style.Colors[ImGuizmo::COLOR::DIRECTION_X] = ImGui::ColorConvertU32ToFloat4(0xFF715ED8);
			style.Colors[ImGuizmo::COLOR::DIRECTION_Y] = ImGui::ColorConvertU32ToFloat4(0xFF25AA25);
			style.Colors[ImGuizmo::COLOR::DIRECTION_Z] = ImGui::ColorConvertU32ToFloat4(0xFFCC532C);
			style.Colors[ImGuizmo::COLOR::PLANE_X] = ImGui::ColorConvertU32ToFloat4(0xFF7A68D8);
			style.Colors[ImGuizmo::COLOR::PLANE_Y] = ImGui::ColorConvertU32ToFloat4(0xFF55AB55);
			style.Colors[ImGuizmo::COLOR::PLANE_Z] = ImGui::ColorConvertU32ToFloat4(0xFFD96742);
			style.Colors[ImGuizmo::COLOR::SELECTION] = ImGui::ColorConvertU32ToFloat4(0xFF20AACC);
			style.Colors[ImGuizmo::COLOR::SCALE_LINE] = ImGui::ColorConvertU32ToFloat4(0xFF404040);
			style.RotationLineThickness = 6.0f;
			style.RotationOuterLineThickness = 6.0f;
			style.ScaleLineThickness = 6.0f;
			style.ScaleLineCircleSize = 12.0f;
			style.TranslationLineThickness = 6.0f;
			style.TranslationLineArrowSize = 12.0f;
			ImGuizmo::SetGizmoSizeClipSpace(0.15f);
		}

		UI::SetEditorContext(this);

		// Setup the renderer contexts (Note: Preview context shares main scene context)
		const glm::vec2 defaultViewportSize = glm::vec2(1600, 900);
		m_SceneRendererContext = SceneRendererContext::Create(defaultViewportSize);
		m_PreviewSceneRendererContext = SceneRendererContext::Create(defaultViewportSize, m_SceneRendererContext->SceneContext);

		m_EditorScene = AssetManager::CreateMemoryOnlyAsset<Scene>();
		m_ActiveScene = m_EditorScene;

		Splash::Update("Connecting to source control...", 98);
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

		m_EditorCamera = EditorCamera();

		Renderer2D::SetLineWidth(4.0f);

		// Add the buttons
		Taskbar::SetThumbnailButtons({
			{ EditorResources::IconPlay, "Play", 
				[&](Taskbar::ThumbnailButton& button) {
					if (m_SceneState == SceneState::Edit)
					{
						OnScenePlay();

						button.Icon = EditorResources::IconStop;
						button.Tooltip = "Stop";
					}
					else
					{
						OnSceneStop();

						button.Icon = EditorResources::IconPlay;
						button.Tooltip = "Play";
					}

					Taskbar::UpdateThumbnailButtons();
				}
			},
			{ EditorResources::SaveIcon, "Save Scene",
				[&](Taskbar::ThumbnailButton& button) {
					SaveScene();
				}
			},
			{ EditorResources::CompileIcon, "Compile",
				[&](Taskbar::ThumbnailButton& button) {
					Compile();
				} 
			}
		});

		Splash::Update("Initializing Plugins...", 99);
		for (auto& pythonPlugin : Preferences::GetData().PythonPlugins)
			if (pythonPlugin.Enabled)
				PythonTools::LoadPlugin(pythonPlugin.PluginPath);

#ifdef TODO
		Popup::Create(FA_FLOPPY_DISK " Save as", {}, {}, nullptr, false, []()
		{
			static std::string s_Filename;

			ImGui::Text(FA_TAG " File name");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(-1);
			ImGui::InputText("##FilenameInput", &s_Filename);

			const float buttonWidth = 150.0f;
			const auto& style = ImGui::GetStyle();
			ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvailWidth() - 2.0f * buttonWidth - 4.0f * style.FramePadding.x, 0.0f));
			ImGui::SameLine();

			bool close = false;
			if (close = ImGui::Button(FA_FLOPPY_DISK " Save", ImVec2(buttonWidth, 0.0f)))
				;

			ImGui::SameLine();

			close |= ImGui::Button(FA_CIRCLE_XMARK " Cancel", ImVec2(buttonWidth, 0.0f));

			if (close)
				Popup::RemoveTopmostPopup();

		}, UI::GetScreenSizeRatio(0.5f));
#endif
	}
	
	void EditorLayer::OnDetach()
	{
		DY_PROFILE_FUNCTION();

		LiveLink::Shutdown();

		PythonTools::Shutdown();

		SourceControl::Shutdown();

		Preferences::Shutdown();

		// Close crash manager process and thread handles
		TerminateProcess(s_CrashManagerProcessInformation.hProcess, 1);
		TerminateProcess(s_CrashManagerProcessInformation.hThread, 1);
		CloseHandle(s_CrashManagerProcessInformation.hProcess);
		CloseHandle(s_CrashManagerProcessInformation.hThread);

		EditorResources::Shutdown();
	}

	void EditorLayer::OnUpdate(Timestep ts)
	{
		DY_PROFILE_FUNCTION();

		PluginLoader::OnUpdate(ts);
		PythonTools::OnUpdate(ts);

#ifdef TODO
		if (ImGui::IsKeyPressed(ImGuiKey_J))
		{
			TransactionManager::BeginTransaction("Create Fancy Camera");
			TransactionManager::Execute(CreateRef<CreateEntityTransaction>("Fancy Camera Entity", m_ActiveScene));
			TransactionManager::Execute(CreateRef<AddComponentTransaction<CameraComponent>>(m_ActiveScene->FindEntityByName("Fancy Camera Entity")));
			TransactionManager::EndTransaction();
		}

		if (ImGui::IsKeyPressed(ImGuiKey_Y))
		{
			TransactionManager::Execute(CreateRef<RemoveComponentTransaction<CameraComponent>>(m_ActiveScene->FindEntityByName("Fancy Camera Entity")));
		}

		if (ImGui::IsKeyPressed(ImGuiKey_R))
		{
			TransactionManager::Execute(CreateRef<DeleteEntityTransaction>(m_ActiveScene->FindEntityByName("Fancy Camera Entity")));
		}
#endif

		m_DeltaTime = ts;
		m_ProgramTime += ts;
		m_LastSaveTime += ts;

		if (Preferences::GetData().AutosaveEnabled)
		{
			//AutoSave
			if (m_LastSaveTime >= Preferences::GetData().AutosaveTime * 60.0f && !m_EditorScenePath.empty())
			{
				SaveScene();
				DY_CORE_INFO("Autosave Complete: Program Time - {}", m_ProgramTime);
				Notification::Create("Autosave Completed", fmt::format(FA_ALARM_CLOCK " Autosaved scene '{}' at \n{:%H:%M:%S} ({})", m_EditorScenePath.stem().string(), std::chrono::system_clock::now(), (int)m_ProgramTime), { {"Dismiss", []() {}} });
			}

			//Warning of autosave
			if (m_LastSaveTime >= (Preferences::GetData().AutosaveTime * 60.0f) - 10 && m_LastSaveTime <= (Preferences::GetData().AutosaveTime * 60.0f) - 10 + ts && !m_EditorScenePath.empty())
				Notification::Create("Autosave pending...", FA_ALARM_CLOCK " Autosave of the current scene will commence\n in 10 seconds.", { { "Cancel", [&]() { m_LastSaveTime = 1.0f; } }, { "Save Now", [&]() { m_LastSaveTime = Preferences::GetData().AutosaveTime * 60; } } }, 10.0f, false);
		}

		m_ActiveScene->OnViewportResize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);

		// Handle Resizing
		if (const FramebufferSpecification& spec = m_SceneRendererContext->ActiveFramebuffer->GetSpecification();
			m_ViewportSize.x > 0.0f && m_ViewportSize.y > 0.0f && // zero sized framebuffer is invalid
			(spec.Width != m_ViewportSize.x || spec.Height != m_ViewportSize.y))
		{
			m_SceneRendererContext->Resize(m_ViewportSize);

			// TODO: Camera preview does not need to be this large. We can just calculate it's desired size based off the viewport size and it taking up approx a quarter of the viewport
			m_PreviewSceneRendererContext->Resize(m_ViewportSize);
			
			m_EditorCamera.SetViewportSize(m_ViewportSize.x, m_ViewportSize.y);
		}

		// Render
		Renderer2D::ResetStats();
		SceneRenderer::ResetStats();

		m_SceneRendererContext->ActiveFramebuffer->Bind();
		SceneRenderer::SetActiveContext(m_SceneRendererContext);
		
		//RenderCommand::SetClearColor({ 0.28f, 0.28f, 0.28f, 1.0f });
		RenderCommand::SetClearColor({ 0.0f, 0.0f, 0.0f, 1.0f });
		RenderCommand::Clear();

		// Clear entity ID  and submesh attachment to -1
		int value = -1;
		m_SceneRendererContext->ActiveFramebuffer->ClearAttachment(1, &value);
		m_SceneRendererContext->ActiveFramebuffer->ClearAttachment(5, &value);

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

		const glm::ivec2 mouse = GetMouseViewportPosition();
		const glm::ivec2 viewportSize = GetViewportSize();

		if (mouse.x >= 0 && mouse.y >= 0 && mouse.x < (int)viewportSize.x && mouse.y < (int)viewportSize.y)
		{
			int pixelData;
			m_SceneRendererContext->ActiveFramebuffer->ReadPixel(1, mouse.x, mouse.y, &pixelData);
			m_HoveredEntity = pixelData == -1 ? Entity{ entt::null, m_ActiveScene.get() } : Entity((entt::entity)pixelData, m_ActiveScene.get());
		}

		OnOverlayRender();

		m_SceneRendererContext->ActiveFramebuffer->Unbind();

		// Handle video writing if we are capturing the screen
		if (!m_VideoCaptureOutputPath.empty())
		{
			if (!m_VideoWriter)
			{
				VideoWriterSpecification writerSpecification;
				writerSpecification.Path = m_VideoCaptureOutputPath;
				writerSpecification.Framebuffer = m_SceneRendererContext->ActiveFramebuffer;
				writerSpecification.FPS = 60;
				writerSpecification.Bitrate = 6000000;

				m_VideoWriter = CreateRef<VideoWriter>(writerSpecification);
			}

			m_VideoWriter->WriteFrame(ts);
		}
		else if (m_VideoWriter)
			m_VideoWriter = nullptr;

		// Render the active camera preview to a framebuffer if one is selected.
		m_PreviewCamera = false;
		if (Preferences::GetData().ShowCameraPreview && m_SceneState != SceneState::Play)
		{
			const auto& selectedEntities = m_SceneHierarchyPanel.GetSelectedEntities();

			if (selectedEntities.size() == 1)
			{
				Entity entity = { *selectedEntities.begin(), m_ActiveScene.get() };
				if (entity.HasComponent<CameraComponent>())
				{
					m_PreviewCamera = true;

					SceneRenderer::SetActiveContext(m_PreviewSceneRendererContext);
					m_PreviewSceneRendererContext->ActiveFramebuffer->Bind();

					RenderCommand::SetClearColor({ 0.0f, 0.0f, 0.0f, 1.0f });
					RenderCommand::Clear();
					m_ActiveScene->RenderSceneRuntime(0.0f, &entity.GetComponent<CameraComponent>().Camera, m_ActiveScene->GetWorldTransformMatrix(entity));

					SceneRenderer::SetActiveContext(m_SceneRendererContext);
					m_PreviewSceneRendererContext->ActiveFramebuffer->Unbind();
				}
			}
		}

		// Trigger content browser OnUpdate (used to render/generate thumbnails).
		m_ContentBrowserPanel.OnUpdate();

		// Trigger updates for all other editor panels (for rendering/timestep purposes)
		for (auto& [handle, panel] : m_AssetEditorPanels)
			panel->OnUpdate(ts);

		// After other systems have finished rendering, return to the original framebuffer
		SceneRenderer::SetActiveContext(m_SceneRendererContext);

		// Ensure that no framebuffer is bound after rendering
		RenderCommand::UnbindFramebuffers();

		// Prior to ImGui render, load the target workspace if there is one
		if (!m_WorkspaceTarget.empty())
		{
			Preferences::LoadWorkspace(m_WorkspaceTarget);
			m_WorkspaceTarget.clear();
		}

		// Iterate through the post update command list
		if (!m_PostUpdateQueue.empty())
		{
			for (auto& command : m_PostUpdateQueue)
			{
				if (Ref<Scene> scene = AssetManager::GetAsset<Scene>(command))
				{
					m_ActiveScene->OnRuntimeStop();
					m_ActiveScene = Scene::Copy(scene);
					m_ActiveScene->OnRuntimeStart();
					m_SceneHierarchyPanel.SetContext(m_ActiveScene);
				}
			}

			m_PostUpdateQueue.clear();
		}

		if (Input::IsMouseButtonPressed(Mouse::ButtonLeft))
		{
			const float radius = 0.1f;
			const float strength = 5.0f;

			auto view = m_EditorScene->GetRegistry().view<TransformComponent, LandscapeComponent>();
			for (auto e : view)
			{
				auto& [tc, lc] = view.get<TransformComponent, LandscapeComponent>(e);

				Entity entity = { e, m_EditorScene.get() };

				const glm::vec3 worldPosition = GetHoveredWorldPositionUnbounded();
				const glm::vec3 localPosition = glm::inverse(entity.GetWorldTransform().GetMatrix()) * glm::vec4(worldPosition, 1.0f);

				for (uint32_t y = 0; y < lc.Resolution.y; y++)
				{
					for (uint32_t x = 0; x < lc.Resolution.x; x++)
					{
						const glm::vec2 coord = glm::vec2((float)x / (float)lc.Resolution.x, (float)y / (float)lc.Resolution.y);
						const float triangleDistance = glm::distance({ localPosition.x, localPosition.z }, coord);

						if (triangleDistance > radius)
							continue;

						// weight = (1 - x^3)^3
						float weight = triangleDistance / radius;
						weight = weight * weight * weight;
						weight = 1.0f - weight;
						weight = weight * weight * weight;

						const uint32_t index = x + lc.Resolution.x * y;
						lc.Data->Set<float>(index, lc.Data->Get<float>(index) + (weight * strength * ts.GetSeconds()));
					}
				}

				lc.Build();
			}
		}
	}

	namespace Utils {

		static bool BeginMenuWithAlpha(const char* label)
		{
			auto& style = ImGui::GetStyle();

			// Hovered
			ImVec4 hoveredColor = ImGui::GetStyleColorVec4(ImGuiCol_HeaderHovered);
			hoveredColor.w = 0.55f;
			ImGui::PushStyleColor(ImGuiCol_HeaderHovered, hoveredColor);

			// Active
			ImVec4 activeColor = ImGui::GetStyleColorVec4(ImGuiCol_Header);
			activeColor.w = 0.55f;
			ImGui::PushStyleColor(ImGuiCol_Header, activeColor);


			bool open = ImGui::BeginMenu(label);
			ImGui::PopStyleColor(2);

			return open;
		}

		struct SnapValues
		{
			const char* Label;
			float Value;
		};

		static void DrawGizmoSnappingMenu(const char* id, const char* icon, const SnapValues snapValues[], const uint32_t snapValueCount, bool& snap, float& snapValue, const char* trailer = nullptr)
		{
			std::string value = String::FloatToString(snapValue);

			if (trailer)
				value += trailer;

			const char* items[] = { icon, value.c_str() };
			int currentScalingValue = snap ? 0 : -1;
			if (ImGui::SwitchButtonEx(id, items, IM_ARRAYSIZE(items), &currentScalingValue, ImVec2(60, 30)))
			{
				if (currentScalingValue == 1)
					ImGui::OpenPopup(id);
				else if (currentScalingValue == 0)
					snap = !snap;
			}

			if (ImGui::BeginPopup(id))
			{
				for (uint32_t valueIndex = 0; valueIndex < snapValueCount; valueIndex++)
					if (ImGui::MenuItem(snapValues[valueIndex].Label))
						snapValue = snapValues[valueIndex].Value;

				ImGui::EndPopup();
			}
		}

	}

	void EditorLayer::OnImGuiRender()
	{
		DY_PROFILE_FUNCTION();
	
		if (Preferences::GetData().LockViewportMouse)
		{
			// Handle mouse locking
			// Disable this in preferences for certain remote connection tools
			// Check if the cursor should be locked
			if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && m_ViewportHovered && !m_LockMouse)
			{
				m_LockMouse = true;
				Application::Get().GetWindow().LockCursor(true);
				ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouse;
			}

			// Check if the mouse button should be unlocked
			if (m_LockMouse && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
			{
				m_LockMouse = false;
				Application::Get().GetWindow().LockCursor(false);
				ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
			}
		}

		// Project manager and default background UI (displayed while windows are loading at startup)
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

			// Background text if windows are still being generated on start up
			{
				ImDrawList* backDrawList = ImGui::GetBackgroundDrawList();

				const ImVec2 halfSize = ImVec2(100.0f, 100.0f);
				const ImVec2 center = (ImGui::GetWindowViewport()->Pos + ImGui::GetWindowViewport()->Size * 0.5f) - ImVec2(0.0f, halfSize.y + 75.0f);
				backDrawList->AddImage((ImTextureID)EditorResources::DymaticLogo->GetRendererID(), center - halfSize, center + halfSize, { 0, 1 }, { 1, 0 }, ImGui::GetColorU32(ImGuiCol_TextDisabled));

				ImVec2 pos;
				const char* text = "DYMATIC ENGINE";
				UI::PushFont(FontType::ExtraLarge);
				pos = ImGui::GetWindowViewport()->Pos + (ImGui::GetWindowViewport()->Size - ImGui::CalcTextSize(text)) * 0.5f;
				backDrawList->AddText(pos, IM_COL32_WHITE, text);
				UI::PopFont();


				pos = ImGui::GetWindowViewport()->Pos + (ImGui::GetWindowViewport()->Size - ImGui::CalcTextSize(DY_VERSION)) * 0.5f + ImVec2(0.0f, 100.0f);
				backDrawList->AddText(pos, IM_COL32_WHITE, DY_VERSION);

				pos = ImGui::GetWindowViewport()->Pos + (ImGui::GetWindowViewport()->Size - ImGui::CalcTextSize(DY_VERSION_COPYRIGHT)) * 0.5f + ImVec2(0.0f, 120.0f);
				backDrawList->AddText(pos, IM_COL32_WHITE, u8"" DY_VERSION_COPYRIGHT);
			}

			if (!Project::GetActive())
				return;
		}

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
		{
			const ImVec2 pos = ImGui::GetWindowPos();
			const ImVec2 size = ImGui::GetWindowSize();

			const float window_border_thickness = 3.0f;
			const float half_thickness = window_border_thickness * 0.5f;

			ImGuiCol coloridx = m_SceneState == SceneState::Edit ? (ImGuiCol_MainWindowBorderEdit)
				: (m_SceneState == SceneState::Play ? (ImGuiCol_MainWindowBorderPlay)
					: (ImGuiCol_MainWindowBorderSimulate));

			{
				ImU32 fullColor = ImGui::GetColorU32(coloridx);
				ImU32 fadedColor = fullColor & (~IM_COL32_A_MASK);

				auto drawList = ImGui::GetWindowDrawList();
				drawList->PushClipRectFullScreen();
				drawList->AddRectFilledMultiColor(pos, pos + ImVec2(300.0f, 30.0f), fullColor, fadedColor, fadedColor, fullColor);
				drawList->PopClipRect();
			}

			if (!maximized)
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
				const ImVec2 pos = ImVec2{ ImGui::GetWindowPos().x + ImGui::GetWindowSize().x / 2, ImGui::GetWindowPos().y + 10.0f };
				const ImVec2 points[] = { {pos.x + ImGui::GetWindowSize().x * 0.3f, pos.y - 50.0f}, {pos.x + ImGui::GetWindowSize().x * 0.25f, pos.y + 15.0f}, {pos.x - ImGui::GetWindowSize().x * 0.25f, pos.y + 15.0f}, {pos.x - ImGui::GetWindowSize().x * 0.3f, pos.y - 50.0f} };

				ImGui::GetWindowDrawList()->AddConvexPolyFilled(points, 4, ImGui::GetColorU32(ImGuiCol_MenuBarGrip));
				ImGui::GetWindowDrawList()->AddPolyline(points, 4, ImGui::GetColorU32(ImGuiCol_MenuBarGripBorder), true, 2.0f);
			}

			if (Utils::BeginMenuWithAlpha(CHARACTER_ICON_DYMATIC))
			{
				if (ImGui::MenuItem(FA_HOUSE " Splash Screen")) { m_ShowSplash = true; }
				if (ImGui::MenuItem(FA_CIRCLE_QUESTION " About Dymatic"))
				{
					Popup::Create("Engine Information", "Dymatic Engine\nVersion " DY_VERSION "\n\n\Build Date: " __DATE__ "\nBranch: Development\n\n\nDymatic Engine is a free, open source engine developed by Dymatic Technologies.\nView source files for licenses from vendor libraries.",
						{ { "Learn More", []() { Network::OpenURL("https://www.dymaticengine.com"); }}, {"Ok", []() {}} }, EditorResources::DymaticLogo);
				}
				ImGui::Separator();
				if (ImGui::MenuItem(CHARACTER_ICON_GITHUB " Github"))
					Network::OpenURL("https://github.com/bencraighill/Dymatic");
				if (ImGui::MenuItem(FA_PAGER " Website"))
					Network::OpenURL("https://www.dymaticengine.com");
				ImGui::Separator();
				if (ImGui::BeginMenu(FA_WAVEFORM " System"))
				{
					ImGui::MenuItem(FA_CHART_BAR " Performance Analyzer", "", &m_PerformanceAnalyser.GetPerformanceAnalyserVisible());
					ImGui::EndMenu();
				}
				ImGui::EndMenu();
			}

			if (Utils::BeginMenuWithAlpha("File"))
			{
				if (ImGui::MenuItem(CHARACTER_ICON_NEW_FILE " New", Preferences::Keymap::GetBindString(Preferences::Keymap::KeyBindEvent::NewSceneBind).c_str())) NewScene();
				if (ImGui::MenuItem(CHARACTER_ICON_OPEN_FILE " Open...", Preferences::Keymap::GetBindString(Preferences::Keymap::KeyBindEvent::OpenSceneBind).c_str())) OpenScene();
				if (ImGui::BeginMenu(CHARACTER_ICON_RECENT " Open Recent", !ProjectSettings::GetData().RecentScenePaths.empty()))
				{
					for (auto& file : ProjectSettings::GetData().RecentScenePaths)
						if (ImGui::MenuItem(fmt::format(FILE_ICON_SCENE " {}", file.stem().string()).c_str()))
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

				if (ImGui::BeginMenu(CHARACTER_ICON_PACKAGE " Package Project"))
				{
					if (ImGui::MenuItem(FA_HAMMER " Build All"))
					{
						Renderer::GetShaderLibrary()->SerializeShaderPack(Project::GetProjectDirectory() / "Packaged" / "Resources" / "ShaderPack.dysp");
						AssetManager::SerializeAssetPack(Project::GetProjectDirectory() / "Packaged" / "Assets" / "AssetPack.dyap");
					}

					ImGui::Separator();

					if (ImGui::MenuItem(FA_BRUSH " Build Shader Pack"))
						Renderer::GetShaderLibrary()->SerializeShaderPack(Project::GetProjectDirectory() / "Packaged" / "Resources" / "ShaderPack.dysp");

					if (ImGui::MenuItem(FA_CUBES " Build Asset Pack"))
						AssetManager::SerializeAssetPack(Project::GetProjectDirectory() / "Packaged" / "Assets" / "AssetPack.dyap");
					
					ImGui::EndMenu();
				}

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
						const std::filesystem::path workspaceSavePath = std::filesystem::path("saved/workspaces");

						if (!std::filesystem::exists(workspaceSavePath))
							std::filesystem::create_directories(workspaceSavePath);

						const std::filesystem::path newWorkspaceFilepath = workspaceSavePath / FileManager::GetNextOfNameInDirectory("New Workspace.workspace", "saved/workspaces");
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

				PythonTools::OnImGuiRender(PythonUIRenderStage::MenuBar_File);

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

			if (Utils::BeginMenuWithAlpha("Edit"))
			{
				if (ImGui::MenuItem(FA_UNDO " Undo", Preferences::Keymap::GetBindString(Preferences::Keymap::KeyBindEvent::UndoBind).c_str(), nullptr, TransactionManager::CanUndo()))
					Undo();

				if (ImGui::MenuItem(FA_REDO " Redo", Preferences::Keymap::GetBindString(Preferences::Keymap::KeyBindEvent::RedoBind).c_str(), nullptr, TransactionManager::CanRedo()))
					Redo();

				ImGui::Separator();

				if (ImGui::MenuItem(CHARACTER_ICON_PREFERENCES " Preferences"))
					m_PreferencesPannel.GetPreferencesPanelVisible() = true;
				if (ImGui::MenuItem(FA_FOLDER_GEAR " Project Settings"))
					m_ProjectSettingsPanel.GetVisible() = true;
				if (ImGui::MenuItem(CHARACTER_ICON_VISUAL_STUDIO " Open Solution"))
					VisualStudioInterface::OpenSolution();

				PythonTools::OnImGuiRender(PythonUIRenderStage::MenuBar_Edit);

				ImGui::EndMenu();
			}

			if (Utils::BeginMenuWithAlpha("Window"))
			{
				ImGui::MenuItem(CHARACTER_ICON_VIEWPORT " Viewport", "", &Preferences::GetEditorWindowVisible(Preferences::EditorWindow::Viewport));
				ImGui::MenuItem(CHARACTER_ICON_TOOLBAR " Toolbar", "", &Preferences::GetEditorWindowVisible(Preferences::EditorWindow::Toolbar));
				ImGui::MenuItem(FA_CHART_MIXED " Statistics", "", &Preferences::GetEditorWindowVisible(Preferences::EditorWindow::Statistics));
				ImGui::MenuItem(CHARACTER_ICON_INFO " Info", "", &Preferences::GetEditorWindowVisible(Preferences::EditorWindow::Info));
				ImGui::MenuItem(CHARACTER_ICON_MEMORY " Profiler", "", &Preferences::GetEditorWindowVisible(Preferences::EditorWindow::Profiler));
				ImGui::MenuItem(CHARACTER_ICON_NODES " Script Editor", "", &Preferences::GetEditorWindowVisible(Preferences::EditorWindow::ScriptEditor));
				ImGui::MenuItem(FILE_ICON_SCENE " Scene Settings", "", &Preferences::GetEditorWindowVisible(Preferences::EditorWindow::SceneSettings));
				ImGui::MenuItem(CHARACTER_ICON_SCENE_HIERARCHY " Scene Hierarchy", "", &Preferences::GetEditorWindowVisible(Preferences::EditorWindow::SceneHierarchy));
				ImGui::MenuItem(CHARACTER_ICON_PROPERTIES " Properties", "", &Preferences::GetEditorWindowVisible(Preferences::EditorWindow::Properties));
				ImGui::MenuItem(CHARACTER_ICON_NOTIFICATIONS " Notifications", "", &Preferences::GetEditorWindowVisible(Preferences::EditorWindow::Notifications));
				ImGui::MenuItem(CHARACTER_ICON_FOLDER " Content Browser", "", &Preferences::GetEditorWindowVisible(Preferences::EditorWindow::ContentBrowser));
				ImGui::MenuItem(CHARACTER_ICON_TEXT_EDIT " Text Editor", "", &Preferences::GetEditorWindowVisible(Preferences::EditorWindow::TextEditor));
				ImGui::MenuItem(CHARACTER_ICON_CURVE " Curve Editor", "", &Preferences::GetEditorWindowVisible(Preferences::EditorWindow::CurveEditor));
				ImGui::MenuItem(CHARACTER_ICON_IMAGE " Image Editor", "", &Preferences::GetEditorWindowVisible(Preferences::EditorWindow::ImageEditor));
				ImGui::MenuItem(CHARACTER_ICON_CONSOLE " Log", "", &Preferences::GetEditorWindowVisible(Preferences::EditorWindow::Log));
				ImGui::MenuItem(FA_RECTANGLE_LIST " Asset Manager", "", &Preferences::GetEditorWindowVisible(Preferences::EditorWindow::AssetManager));

				PythonTools::OnImGuiRender(PythonUIRenderStage::MenuBar_Window);

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

			if (Utils::BeginMenuWithAlpha("View"))
			{
				if (ImGui::MenuItem(m_EditorCamera.GetProjectionType() == 0 ? CHARACTER_ICON_PROJECTION_ORTHOGRAPHIC " Orthographic" : CHARACTER_ICON_PROJECTION_PERSPECTIVE " Perspective", Preferences::Keymap::GetBindString(Preferences::Keymap::KeyBindEvent::ViewProjectionBind).c_str())) { m_ProjectionToggled = !m_EditorCamera.GetProjectionType(); m_EditorCamera.SetProjectionType(m_ProjectionToggled); }

				if (ImGui::BeginMenu(FA_EYE " Set Perspective"))
				{
					if (ImGui::MenuItem("Front", Preferences::Keymap::GetBindString(Preferences::Keymap::KeyBindEvent::ViewFrontBind).c_str())) { m_YawUpdate = floor(glm::degrees(m_EditorCamera.GetYaw()) / 360) * 360; m_PitchUpdate = floor(glm::degrees(m_EditorCamera.GetYaw()) / 360) * 360; m_UpdateAngles = true; m_EditorCamera.SetProjectionType(1); }
					if (ImGui::MenuItem("Side", Preferences::Keymap::GetBindString(Preferences::Keymap::KeyBindEvent::ViewSideBind).c_str())) { m_YawUpdate = floor(glm::degrees(m_EditorCamera.GetYaw()) / 360) * 360 - 90.0f; m_PitchUpdate = floor(glm::degrees(m_EditorCamera.GetYaw()) / 360) * 360; m_UpdateAngles = true; m_EditorCamera.SetProjectionType(1); }
					if (ImGui::MenuItem("Top", Preferences::Keymap::GetBindString(Preferences::Keymap::KeyBindEvent::ViewTopBind).c_str())) { m_YawUpdate = floor(glm::degrees(m_EditorCamera.GetYaw()) / 360) * 360; m_PitchUpdate = floor(glm::degrees(m_EditorCamera.GetYaw()) / 360) * 360 + 90; m_UpdateAngles = true; m_EditorCamera.SetProjectionType(1); }
					ImGui::Separator();
					if (ImGui::MenuItem("Flip", Preferences::Keymap::GetBindString(Preferences::Keymap::KeyBindEvent::ViewFlipBind).c_str())) { m_YawUpdate = glm::degrees(m_EditorCamera.GetYaw()) + 180.0f; m_PitchUpdate = glm::degrees(m_EditorCamera.GetPitch()); m_UpdateAngles = true; m_EditorCamera.SetProjectionType(1); }

					ImGui::EndMenu();
				}

				if (ImGui::MenuItem(FA_UNDO " Reset to Origin"))
					m_EditorCamera.SetTransform(EditorCamera::EditorCameraTransform());

				PythonTools::OnImGuiRender(PythonUIRenderStage::MenuBar_View);

				ImGui::EndMenu();
			}

			if (Utils::BeginMenuWithAlpha("Script"))
			{
				if (ImGui::MenuItem(FILE_ICON_SCRIPT " Compile Assembly", "", nullptr, m_SceneState == SceneState::Edit))
					Compile();

				if (ImGui::MenuItem(CHARACTER_ICON_RESTART " Reload Assembly", Preferences::Keymap::GetBindString(Preferences::Keymap::KeyBindEvent::ReloadAssembly).c_str(), nullptr, m_SceneState == SceneState::Edit))
					ScriptEngine::ReloadAssembly();

				PythonTools::OnImGuiRender(PythonUIRenderStage::MenuBar_Script);

				ImGui::EndMenu();
			}

			if (Utils::BeginMenuWithAlpha("Help"))
			{
				if (ImGui::MenuItem(FA_BOOK " Documentation"))
					Network::OpenURL("https://docs.dymaticengine.com");

				PythonTools::OnImGuiRender(PythonUIRenderStage::MenuBar_Help);

				ImGui::EndMenu();
			}

			PythonTools::OnImGuiRender(PythonUIRenderStage::MenuBar);

			auto drawList = ImGui::GetWindowDrawList();

			// Draw the project name tag.
			{
				UI::PushFont(FontType::Bold);
				std::string projectName = "None";
				if (Project::GetActive())
					projectName = Project::GetName();

				auto minX = ImGui::GetWindowPos().x + ImGui::GetWindowWidth() * 0.785f;
				auto minY = ImGui::GetWindowPos().y;
				drawList->AddRect(ImVec2(minX, minY), ImVec2(minX + ImGui::CalcTextSize(projectName.c_str()).x + style.FramePadding.x * 4.0f, minY + ImGui::GetTextLineHeight() + style.FramePadding.y * 2.0f),
					ImGui::GetColorU32(ImGuiCol_TextDisabled), 5.0f, ImDrawFlags_RoundCornersBottom
				);
				drawList->AddText(ImVec2(minX + style.FramePadding.x * 2.0f, minY + style.FramePadding.y), ImGui::GetColorU32(ImGuiCol_TextDisabled), projectName.c_str());
				UI::PopFont();
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
					case 0:
					{
						drawList->AddLine(ImVec2{ center.x - iconRadius, center.y + iconRadius }, ImVec2{ center.x + iconRadius, center.y - iconRadius }, col, lineThickness);
						drawList->AddLine(ImVec2{ center.x - iconRadius, center.y - iconRadius }, ImVec2{ center.x + iconRadius, center.y + iconRadius }, col, lineThickness);
						s_CloseHovered = hovered;
						break;
					}
					case 1:
					{
						if (maximized)
						{
							const float offset = 1.5f;
							const float scale = 0.85f;
							drawList->AddRect(ImVec2(center.x - iconRadius * scale + offset, center.y - iconRadius * scale - offset), ImVec2(center.x + iconRadius * scale + offset, center.y + iconRadius * scale - offset), col, 0.0f, 0, lineThickness);

							const ImVec2 min = ImVec2(center.x - iconRadius * scale - offset, center.y - iconRadius * scale + offset);
							const ImVec2 max = ImVec2(center.x + iconRadius * scale - offset, center.y + iconRadius * scale + offset);
							drawList->AddRectFilled(min, max, ImGui::GetColorU32(hovered ? ImGuiCol_HeaderHovered : ImGuiCol_MenuBarBg), 0.0f, 0);
							drawList->AddRect(min, max, col, 0.0f, 0, lineThickness);
						}
						else
						{
							drawList->AddRect(ImVec2{ center.x - iconRadius, center.y - iconRadius }, ImVec2{ center.x + iconRadius, center.y + iconRadius }, col, 0.0f, 0, lineThickness);
						}

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

				// Window Title
				const char* text = "Dymatic Editor [" DY_VERSION_STRING "] (Windows - OpenGL)";
				drawList->AddText(ImGui::GetWindowPos() + ImVec2((ImGui::GetWindowSize().x - ImGui::CalcTextSize(text).x) * 0.5f, style.FramePadding.y), ImGui::GetColorU32(ImGuiCol_TextDisabled), text);
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
					drawList->AddImage((ImTextureID)(ImGui::IsItemHovered() ? EditorResources::SaveHoveredIcon : EditorResources::SaveIcon)->GetRendererID(), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), { 0, 1 }, { 1, 0 });
				}

				ImGui::SameLine();

				// Source Control Button
				{
					if (ImGui::Button("##SourceControlButton", ImVec2(buttonHeight, buttonHeight)))
						m_SourceControlPanel.GetSourceControlPannelVisible() = !m_SourceControlPanel.GetSourceControlPannelVisible();

					if (GImGui->HoveredIdTimer > 0.5f && ImGui::IsItemHovered())
						ImGui::SetTooltip(SourceControl::IsActive() ? "Source Control: Active" : "Source Control: Inactive");

					drawList->AddImage((ImTextureID)EditorResources::SourceControlIcon->GetRendererID(), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), { 0, 1 }, { 1, 0 },
						ImGui::GetColorU32(ImGui::IsItemHovered() ? ImVec4(0.75f, 0.75f, 0.75f, 1.0f) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f))
					);

					drawList->AddCircle(ImGui::GetItemRectMax(), 4.0f, ImGui::GetColorU32(SourceControl::IsActive() ? ImVec4(0.2f, 0.7f, 0.3f, 1.0f) : ImVec4(0.8f, 0.2f, 0.3f, 1.0f)), 0, 3.0f);
				}

				ImGui::SameLine();

				// Compile Button
				{
					if (ImGui::Button("##CompileButton", ImVec2(buttonHeight, buttonHeight)) && m_SceneState == SceneState::Edit)
						Compile();
					drawList->AddImage((ImTextureID)(ImGui::IsItemHovered() && m_SceneState == SceneState::Edit ? EditorResources::CompileHoveredIcon : EditorResources::CompileIcon)->GetRendererID(), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), { 0, 1 }, { 1, 0 },
						ImGui::GetColorU32(m_SceneState == SceneState::Edit ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f) : ImVec4(0.5f, 0.5f, 0.5f, 1.0f))
					);
				}

				ImGui::SameLine();

				// Live Link Button
				{
					if (ImGui::Button("##LiveLinkButton", ImVec2(buttonHeight, buttonHeight)))
						LiveLink::ToggleVisibility();
					drawList->AddImage((ImTextureID)(ImGui::IsItemHovered() ? EditorResources::LiveLinkHoveredIcon : EditorResources::LiveLinkIcon)->GetRendererID(), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), { 0, 1 }, { 1, 0 });
				}

				ImGui::SameLine();

				// IDE Debugger Connection
				if (ScriptEngine::IsDebuggerAttached())
				{
					if (ImGui::Button("##IDEDebuggerButton", ImVec2(buttonHeight, buttonHeight)))
						;

					drawList->AddImage((ImTextureID)(EditorResources::IDEIcon)->GetRendererID(), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), { 0, 1 }, { 1, 0 },
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
					const Ref<Texture2D> icon = m_SceneState == SceneState::Edit || m_SceneState == SceneState::Simulate ? EditorResources::IconPlay : (m_ActiveScene->IsPaused() ? EditorResources::IconPlay : EditorResources::IconPause);
					if (ImGui::Button("##PlayButton", { buttonHeight, buttonHeight }))
					{
						if (m_SceneState == SceneState::Edit || m_SceneState == SceneState::Simulate)
							OnScenePlay();
						else if (m_SceneState == SceneState::Play)
							OnScenePause();
					}
					const ImColor color = m_SceneState == SceneState::Edit || m_SceneState == SceneState::Simulate || (m_SceneState == SceneState::Play && m_ActiveScene->IsPaused()) ? (ImGui::IsItemActive() ? ImColor(101, 142, 52) : (ImGui::IsItemHovered() ? ImColor(169, 194, 150) : ImColor(139, 194, 74))) : (ImGui::IsItemActive() ? ImColor(117, 117, 117) : (ImGui::IsItemHovered() ? ImColor(255, 255, 255) : ImColor(192, 192, 192)));
					drawList->AddImage((ImTextureID)icon->GetRendererID(), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), { 0, 1 }, { 1, 0 }, color);
				}
				ImGui::SameLine();
				{
					const Ref<Texture2D> icon = m_SceneState == SceneState::Edit || m_SceneState == SceneState::Play ? EditorResources::IconSimulate : (m_ActiveScene->IsPaused() ? EditorResources::IconSimulate : EditorResources::IconPause);
					if (ImGui::Button("##SimulateButton", { buttonHeight, buttonHeight }))
					{
						if (m_SceneState == SceneState::Edit || m_SceneState == SceneState::Play)
							OnSceneSimulate();
						else if (m_SceneState == SceneState::Simulate)
							OnScenePause();
					}
					const ImColor color = ImGui::IsItemActive() ? ImColor(117, 117, 117) : (ImGui::IsItemHovered() ? ImColor(255, 255, 255) : ImColor(192, 192, 192));
					drawList->AddImage((ImTextureID)icon->GetRendererID(), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), { 0, 1 }, { 1, 0 }, color);
				}
				ImGui::SameLine();
				{
					if (ImGui::Button("##StopButton", { buttonHeight, buttonHeight }))
					{
						if (m_SceneState != SceneState::Edit)
							OnSceneStop();
					}
					const ImColor color = m_SceneState == SceneState::Edit ? ImColor(117, 117, 117) : (ImGui::IsItemActive() ? ImColor(188, 44, 44) : (ImGui::IsItemHovered() ? ImColor(255, 192, 192) : ImColor(255, 64, 64)));
					drawList->AddImage((ImTextureID)EditorResources::IconStop->GetRendererID(), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), { 0, 1 }, { 1, 0 }, color);
				}
				ImGui::SameLine();
				{
					if (ImGui::Button("##NextFrameButton", { buttonHeight, buttonHeight }))
					{
						if (m_SceneState != SceneState::Edit && m_ActiveScene->IsPaused())
						{
							m_ActiveScene->Step(Preferences::GetData().FrameStepCount);
							EditorResources::SoundStep->Play();
						}
					}
					const ImColor color = (m_SceneState != SceneState::Edit && m_ActiveScene->IsPaused()) ? (ImGui::IsItemHovered() ? ImColor(255, 255, 255) : ImColor(192, 192, 192)) : ImColor(117, 117, 117);
					drawList->AddImage((ImTextureID)EditorResources::IconStep->GetRendererID(), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), { 0, 1 }, { 1, 0 }, color);
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
				drawList->AddImage((ImTextureID)EditorResources::ToolbarSettingsIcon->GetRendererID(),
					ImVec2(min.x + style.FramePadding.x, min.y + style.FramePadding.y),
					ImVec2(min.x + style.FramePadding.x + buttonHeight, min.y + style.FramePadding.y + buttonHeight),
					{ 0, 1 }, { 1, 0 });

				if (ImGui::BeginPopup("##ProjectSettingsPopup"))
				{
					ImGui::TextDisabled("Editor");

					ImGui::MenuItem(CHARACTER_ICON_TRANSFORM " Show Transform Gizmo", nullptr, &Preferences::GetData().ShowTransformGizmo);

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
							volume *= 100.0f;
							if (ImGui::SliderFloat("##ProjectSettingsVolumeInput", &volume, 0.0f, 100.0f, "%.0f%%"))
							{
								volume /= 100.0f;
								Preferences::GetData().EditorVolume = volume;
								AudioEngine::SetGlobalVolume(volume);
							}

							ImGui::EndMenu();
						}
					}

					ImGui::MenuItem(FA_WINDOW_MAXIMIZE " Show Viewport UI", nullptr, &Preferences::GetData().ShowViewportUI);

					ImGui::Separator();

					ImGui::TextDisabled("Networking");
					if (ImGui::MenuItem(FA_SERVER " Start Server"))
						NetworkManager::StartServer(8192);
					if (ImGui::MenuItem(FA_NETWORK_WIRED " Connect to Server"))
						NetworkManager::StartClient("127.0.0.1:8192");

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
			ImGui::Image((ImTextureID)(uint64_t)((m_SceneState == SceneState::Edit ? m_EditIcon : m_LoadingCogAnimation[s_LoadingFrameIndex / 3])->GetRendererID()), { size, size }, { 0, 1 }, { 1, 0 });

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
					for (size_t i = 0; i < ((int)ImGui::GetTime()) % 4; i++)
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
			std::string text = DY_VERSION;
			ImGui::Dummy(ImVec2{ ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(text.c_str()).x - 5, 0 });
			ImGui::SameLine();
			ImGui::Text(text.c_str());

			ImGui::End();
		}

		// Preferences and Settings
		m_PreferencesPannel.OnImGuiRender();
		m_ProjectSettingsPanel.OnImGuiRender();

		// Update Viewport-Scene Hierarchy Picker
		{
			auto& pickingID = UI::GetEntityPickingID();
			if (pickingID == 1 && Input::IsMouseButtonPressed(Mouse::ButtonLeft) && m_ViewportHovered)
			{
				if ((entt::entity)m_HoveredEntity != entt::null)
					pickingID = m_HoveredEntity.GetUUID();
				else
					pickingID = 0;
			}
		}

		// Scene Hierarchy and properties panel
		m_SceneHierarchyPanel.OnImGuiRender();

		// Check for external drag drop sources
		m_ContentBrowserPanel.OnImGuiRender(m_IsDragging);

		// Render UI for all open generic editor panels (e.g. asset viewer panels)
		AssetHandle handleToRemove = 0;
		for (auto& [handle, panel] : m_AssetEditorPanels)
		{
			bool open = true;
			panel->OnImGuiRender(open);

			if (!open)
				handleToRemove = handle;
		}

		if (handleToRemove)
			m_AssetEditorPanels.erase(handleToRemove);

#ifdef TODO
		static bool init = true;
		static Ref<VideoReader> s_VideoReader = nullptr;
		if (init)
		{
			VideoReaderSpecification specification;
			specification.VideoStream = CreateRef<FileStreamReader>("Example.mp4");
			specification.SubtitleStream = CreateRef<FileStreamReader>("ExampleSubtitles.srt");
			s_VideoReader = CreateRef<VideoReader>(specification);

			init = false;
		}
		
		Ref<Texture2D> frame = s_VideoReader->GetNextFrame(m_DeltaTime);
		if (ImGui::IsKeyPressed(ImGuiKey_RightAlt))
			if (Entity testVideo = m_ActiveScene->FindEntityByName("Test Video"))
				testVideo.GetComponent<SpriteRendererComponent>().Texture = s_VideoReader->GetCurrentFrame();

		if (Entity testSubtitle = m_ActiveScene->FindEntityByName("Test Subtitle"))
			testSubtitle.GetComponent<TextComponent>().TextString = s_VideoReader->GetCurrentSubtitle();

		if (ImGui::IsKeyPressed(ImGuiKey_1))
			s_VideoReader->SetTime(25.1);
#endif

		SceneRenderer::OnImGuiRender();

		m_PerformanceAnalyser.OnImGuiRender(m_DeltaTime);

		m_CurveEditor.OnImGuiRender();
		m_ImageEditor.OnImGuiRender();

		m_SourceControlPanel.OnImGuiRender();
		LiveLink::OnImGuiRender(m_DeltaTime, m_ActiveScene);

		Notification::OnImGuiRender(m_DeltaTime);
		Popup::OnImGuiRender(m_DeltaTime);
		m_NotificationsPanel.OnImGuiRender(m_DeltaTime);

		PluginLoader::OnUIRender();

		PythonTools::OnImGuiRender(PythonUIRenderStage::Main);
		
		m_ProfilerPanel.OnImGuiRender();

		m_LogPanel.OnImGuiRender(m_DeltaTime);
		m_AssetManagerPanel.OnImGuiRender();
		
		m_TextEditor.OnImGuiRender();
		
		m_NodeEditorPannel.OnImGuiRender();

		if (auto& statisticsVisible = Preferences::GetEditorWindowVisible(Preferences::EditorWindow::Statistics))
		{
			ImGui::Begin(FA_CHART_MIXED " Statistics", &statisticsVisible);

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
			if (Memory::GetAllocationStats().TotalAllocated != 0)
			{
				for (auto& allocation : Memory::GetMemoryAllocationStats())
					ImGui::Text("%s: %d", allocation.first, allocation.second.TotalAllocated - allocation.second.TotalFreed);

				for (auto& allocation : Memory::GetMemoryAllocations())
				{
					std::stringstream sstream;
					sstream << std::hex << allocation.second.Memory;
					std::string result = sstream.str();
					ImGui::Text("%s : %s", result.c_str(), std::to_string(allocation.second.Size).c_str());
				}

			}

			ImGui::Separator();

			auto& allocStats = Memory::GetAllocationStats();
			ImGui::Text("Total Allocated: %d", allocStats.TotalAllocated);
			ImGui::Text("Total Freed: %d", allocStats.TotalFreed);
			ImGui::Text("Current Usage: %d", allocStats.TotalAllocated - allocStats.TotalFreed);

			ImGui::End();
		}

		if (auto& viewportVisible = Preferences::GetEditorWindowVisible(Preferences::EditorWindow::Viewport))
		{
			const bool viewportLocked = (bool)m_VideoWriter;

			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
			ImGui::Begin(CHARACTER_ICON_VIEWPORT " Viewport", &viewportVisible, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | (viewportLocked ? ImGuiWindowFlags_NoResize : 0));
			auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
			auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
			auto viewportOffset = ImGui::GetWindowPos();
			m_ViewportBounds[0] = { viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y };
			m_ViewportBounds[1] = { viewportMaxRegion.x + viewportOffset.x, viewportMaxRegion.y + viewportOffset.y };

			m_ViewportFocused = ImGui::IsWindowFocused();
			m_ViewportHovered = ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered();
			//Application::Get().GetImGuiLayer()->BlockEvents(!m_ViewportFocused && !m_ViewportHovered);
			//Application::Get().GetImGuiLayer()->BlockEvents(false);
			Application::Get().GetImGuiLayer()->BlockEvents(io.WantTextInput);

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

				if (m_ViewportHovered && m_RulerMode == 1)
				{
					ImGui::SetMouseCursor(ImGuiMouseCursor_Crosshair);

					if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
					{
						m_RulerLines.emplace_back();
						m_RulerLines.back().Start = GetHoveredWorldPosition();
						m_RulerMode = 2;
					}

					if (Input::IsKeyPressed(Key::Escape))
					{
						m_RulerLines.pop_back();
						m_RulerMode = 0;
					}
				}

				if (m_RulerMode == 2)
				{
					ImGui::SetMouseCursor(ImGuiMouseCursor_Crosshair);

					if (m_ViewportHovered)
						m_RulerLines.back().End = GetHoveredWorldPosition();

					if (Input::IsKeyPressed(Key::Escape))
					{
						m_RulerLines.pop_back();
						m_RulerMode = 0;
					}

					if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
						m_RulerMode = 0;
				}
			}
			
			ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
			m_ViewportSize = { viewportPanelSize.x, viewportPanelSize.y };
			
			// Draw the actual viewport framebuffer
			ImGui::Image((ImTextureID)m_SceneRendererContext->ActiveFramebuffer->GetColorAttachmentRendererID(), m_ViewportSize, { 0, 1 }, { 1, 0 });
			
			if (ImGui::BeginDragDropTarget())
			{
				// If the payload is accepted
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
				{
					const wchar_t* w_path = (const wchar_t*)payload->Data;
					const std::filesystem::path path = w_path;
					FileType fileType = FileManager::GetFileType(path);

					Entity entity;
					
					if (fileType == FileType::FileTypeScene)
					{
						OpenScene(Project::GetAssetFileSystemPath(path));
					}
					else if (fileType == FileType::FileTypeTexture || fileType == FileType::FileTypeVirtualTexture)
					{
						if (Ref<Texture2D> texture = AssetManager::GetAsset<Texture2D>(path))
						{
							entity = m_ActiveScene->CreateEntity(path.filename().stem().string());
							
							entity.AddComponent<SpriteRendererComponent>(texture);

							float width = texture->GetWidth();
							float height = texture->GetHeight();
							if (width > height)
							{
								width /= height;
								height = 1.0f;
							}
							else
							{
								height /= width;
								width = 1.0f;
							}

							auto& transform = entity.GetComponent<TransformComponent>().Transform;
							transform.Scale = glm::vec3(width, height, 1.0f);
						}
					}
					else if (fileType == FileType::FileTypeMesh)
					{
						// Create a new entity with a static mesh
						if (const Ref<Model> model = AssetManager::GetAsset<Model>(path.string()))
						{
							entity = m_ActiveScene->CreateEntity(path.filename().stem().string());
							entity.AddComponent<StaticMeshComponent>(model);
						}
					}
					else if (fileType == FileType::FileTypePrefab)
					{
						Ref<Prefab> prefab = AssetManager::GetAsset<Prefab>(path);
						entity = m_ActiveScene->Instantiate(prefab);
					}
					else if (fileType == FileType::FileTypeFont)
					{
						Ref<Font> font = AssetManager::GetAsset<Font>(path);
						entity = m_ActiveScene->CreateEntity("Text");
						TextComponent& tc = entity.AddComponent<TextComponent>();
						tc.Font = font;
						tc.TextString = "Lorem ipsum";
					}
					else if (fileType == FileType::FileTypeEnvironmentMap)
					{
						Ref<EnvironmentMap> environmentMap = AssetManager::GetAsset<EnvironmentMap>(path);
						entity = m_ActiveScene->CreateEntity("Environment Map");
						SkyLightComponent& slc = entity.AddComponent<SkyLightComponent>();
						slc.EnvironmentMap = environmentMap;
					}
					else if (fileType == FileType::FileTypeAudio)
					{
						Ref<Audio> audio = AssetManager::GetAsset<Audio>(path);
						entity = m_ActiveScene->CreateEntity(path.filename().string());
						AudioComponent& ac = entity.AddComponent<AudioComponent>();
						ac.AudioSound = audio;
					}
					else if (fileType == FileType::FileTypeParticleSystem)
					{
						Ref<ParticleSystem> particleSystem = AssetManager::GetAsset<ParticleSystem>(path);
						entity = m_ActiveScene->CreateEntity(path.filename().string());
						ParticleSystemComponent& psc = entity.AddComponent<ParticleSystemComponent>();
						psc.SetParticleSystem(particleSystem);
					}
					else if (fileType == FileType::FileTypeMaterial || fileType == FileType::FileTypeMaterialInstance)
					{
						// Retrieve the dropped material asset
						if (Ref<MaterialAsset> material = AssetManager::GetAsset<MaterialAsset>(path))
						{
							const MaterialAsset::MaterialUsage usage = material->GetProperties().Usage;

							// Check if the hovered entity exists and has a StaticMeshComponent
							if (usage == MaterialAsset::MaterialUsage::Surface && m_HoveredEntity && m_HoveredEntity.HasComponent<StaticMeshComponent>())
							{
								// Read the submesh index
								int submeshIndex;
								const glm::vec2 mouse = GetMouseViewportPosition();
								m_SceneRendererContext->ActiveFramebuffer->Bind();
								m_SceneRendererContext->ActiveFramebuffer->ReadPixel(5, mouse.x, mouse.y, &submeshIndex);
								m_SceneRendererContext->ActiveFramebuffer->Unbind();

								// Assign the material asset to the hovered static mesh at the specified submesh index
								StaticMeshComponent& smc = m_HoveredEntity.GetComponent<StaticMeshComponent>();

								if (submeshIndex < smc.m_Materials.size())
									smc.m_Materials[submeshIndex] = material;
							}
							else if (usage == MaterialAsset::MaterialUsage::PostProcessing)
							{
								Entity entity = m_ActiveScene->CreateEntity("Post Processing Volume");
								PostProcessVolumeComponent& ppvc = entity.AddComponent<PostProcessVolumeComponent>();
								ppvc.Material = material;
							}
						}
					}
					else if (fileType == FileType::FileTypeAnimationGraph)
					{
						if (m_HoveredEntity && m_HoveredEntity.HasComponent<StaticMeshComponent>())
						{
							auto& smc = m_HoveredEntity.GetComponent<StaticMeshComponent>();
							if (smc.GetSkeleton())
							{
								const Ref<AnimationGraph> animationGraph = AssetManager::GetAsset<AnimationGraph>(path);
								smc.SetAnimationGraph(animationGraph);
							}
						}
					}

					if (entity)
					{
						// If we created a new entity, move it to the cursor position in screen space and update selection
						m_SceneHierarchyPanel.SelectedEntity(entity);
						m_ActiveScene->SetEntityTranslation(entity, GetHoveredWorldPosition());
					}
				}

				ImGui::EndDragDropTarget();
			}

			if (m_VideoWriter)
				UI::DrawWindowInnerShadows(ImVec4(0.9f, 0.1f, 0.2f, 1.0f), 25.0f);

			if (Preferences::GetData().ShowViewportUI)
			{
				ImGui::SetCursorPos(ImGui::GetWindowContentRegionMin() + ImVec2(5.0f, 5.0f));

				// Viewport settings dropdown
				{
					ImGui::SetNextItemWidth(ImGui::CalcTextSize(" Viewport Settings").x * 1.5f + style.FramePadding.x * 2.0f + 10.0f);
					ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.0f, 5.0f));
					ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.5f, 7.5f));
					bool open = ImGui::BeginCombo("##ViewportSettings", CHARACTER_ICON_PREFERENCES " Viewport Settings", ImGuiComboFlags_HeightLargest);
					ImGui::PopStyleVar();
					if (open)
					{
						if (ImGui::BeginMenu(FA_FORWARD_STEP " Frame Step"))
						{
							ImGui::Text("Frame Count");
							ImGui::SameLine();
							ImGui::SetNextItemWidth(25.0f);
							ImGui::DragInt("##FrameStepCountInput", &Preferences::GetData().FrameStepCount, 0.25f, 1, 10, "%d", ImGuiSliderFlags_AlwaysClamp);
							ImGui::EndMenu();
						}

						if (ImGui::BeginMenu(FA_CAMCORDER " Camera View"))
						{
							if (ImGui::BeginMenu(FA_BINOCULARS " FOV"))
							{
								float fov = m_EditorCamera.GetFOV();
								if (ImGui::DragFloat("##EditorCameraFOVInput", &fov, 0.25f, 1.0f, 180.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp))
									m_EditorCamera.SetFOV(fov);
								ImGui::EndMenu();
							}

							if (ImGui::BeginMenu(FA_MOUSE_FIELD " Near Clip"))
							{
								float nearClip = m_EditorCamera.GetNearClip();
								if (ImGui::DragFloat("##EditorCameraNearClipInput", &nearClip, 5.0f, 0.001f, 100000.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_Logarithmic))
									m_EditorCamera.SetNearClip(nearClip);
								ImGui::EndMenu();
							}

							if (ImGui::BeginMenu(FA_MOUNTAINS " Far Clip"))
							{
								float farClip = m_EditorCamera.GetFarClip();
								if (ImGui::DragFloat("##EditorCameraFarClipInput", &farClip, 5.0f, 0.001f, 100000.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_Logarithmic))
									m_EditorCamera.SetFarClip(farClip);
								ImGui::EndMenu();
							}

							ImGui::EndMenu();
						}

						ImGui::MenuItem(FA_CLOCK " Show FPS", nullptr, &Preferences::GetData().ShowFPS);

						ImGui::MenuItem(FA_CAMERA_MOVIE " Show Camera Preview", nullptr, &Preferences::GetData().ShowCameraPreview);

						if (ImGui::MenuItem(CHARACTER_ICON_CAMERA " Create Camera Here"))
						{
							Entity entity = m_ActiveScene->CreateEntity("Camera");

							auto& camera = entity.AddComponent<CameraComponent>().Camera;
							camera.SetPerspectiveNearClip(m_EditorCamera.GetNearClip());
							camera.SetPerspectiveFarClip(m_EditorCamera.GetFarClip());
							camera.SetPerspectiveVerticalFOV(glm::radians(m_EditorCamera.GetFOV()));

							auto& transform = entity.GetComponent<TransformComponent>().Transform;
							transform.Translation = m_EditorCamera.GetPosition();
							transform.Rotation = glm::vec3(m_EditorCamera.GetPitch() * -1.0f, m_EditorCamera.GetYaw() * -1.0f, 0.0f);
						}

						ImGui::Separator();

						bool showColliders = m_ActiveScene->GetShowColliders();
						if (ImGui::MenuItem(CHARACTER_ICON_BOX_COLLIDER " Show Colliders", nullptr, &showColliders))
						{
							m_ActiveScene->SetShowColliders(showColliders);
							m_EditorScene->SetShowColliders(showColliders);
						}

						if (ImGui::MenuItem(FA_RULER " Ruler"))
							m_RulerMode = 1;

						if (ImGui::MenuItem(FA_ERASER " Clear Ruler Lines"))
							m_RulerLines.clear();

						ImGui::MenuItem(FA_GAME_BOARD " Game View");

						ImGui::Separator();

						if (ImGui::BeginMenu(FA_BOOKMARK " Bookmarks"))
						{
							ImGuiSelectableFlags selectableFlags = ImGuiSelectableFlags_SpanAvailWidth | ImGuiSelectableFlags_AllowItemOverlap;

							ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2());

							ImGui::Dummy(ImVec2(250.0f, 0.0f));

							auto& bookmarks = Preferences::GetData().ViewportBookmarks;

							if (ImGui::Selectable(CHARACTER_ICON_ADD " Create", false, selectableFlags | ImGuiSelectableFlags_DontClosePopups))
							{
								Preferences::ViewportBookmark newBookmark;
								newBookmark.Name = "New Viewport Bookmark";
								newBookmark.Transform = m_EditorCamera.GetTransform();

								bookmarks.push_back(newBookmark);
								m_ViewportBookmarkRenameIndex = bookmarks.size() - 1;
							}

							if (ImGui::Selectable(FA_TRASH " Clear", false, selectableFlags | ImGuiSelectableFlags_DontClosePopups))
								bookmarks.clear();

							if (!bookmarks.empty())
								ImGui::Separator();

							uint32_t index = 0;
							for (auto& bookmark : bookmarks)
							{
								ImGui::PushID(index);

								if (index == m_ViewportBookmarkRenameIndex)
								{
									ImGui::SetNextItemWidth(-75.0f);
									if (GImGui->ActiveId != ImGui::GetID("##BookmarkInputText"))
										ImGui::SetKeyboardFocusHere();
									std::string buffer = bookmark.Name;
									ImGui::InputText("##BookmarkInputText", &buffer);
									if (ImGui::IsItemActivePreviousFrame() && !ImGui::IsItemActive())
									{
										bookmark.Name = buffer;
										m_ViewportBookmarkRenameIndex = -1;
									}
								}
								else
								{
									ImGui::SetNextItemWidth(50.0f);
									if (ImGui::Selectable(fmt::format(FA_BOOKMARK " {}", bookmark.Name).c_str(), false, selectableFlags))
										m_EditorCamera.SetTransform(bookmark.Transform);
								}

								ImGui::SameLine();
								ImGui::Dummy(ImVec2(std::max(style.FramePadding.x, ImGui::GetContentRegionAvailWidth() - 66.0f), 0.0f));
								ImGui::SameLine();

								if (ImGui::Button(CHARACTER_ICON_EDIT))
									m_ViewportBookmarkRenameIndex = index;
								ImGui::SameLine();
								if (ImGui::Button(CHARACTER_ICON_DELETE))
									bookmarks.erase(bookmarks.begin() + index);

								ImGui::PopID();
								index++;
							}

							ImGui::PopStyleVar();
							ImGui::EndMenu();
						}

						if (ImGui::MenuItem(FA_CAMERA " Save Screenshot"))
						{
							const std::filesystem::path screenshotDirectory = Project::GetProjectDirectory() / "Saved" / "Screenshots";
							if (!std::filesystem::exists(screenshotDirectory))
								std::filesystem::create_directory(screenshotDirectory);

							std::filesystem::path filename = fmt::format("Screenshot {}.png", Utils::GetFileSafeTimestamp());

							const FramebufferSpecification& spec = m_SceneRendererContext->ActiveFramebuffer->GetSpecification();
							size_t size = spec.Width * spec.Height * 4;
							float* raw = new float[size];

							m_SceneRendererContext->ActiveFramebuffer->Bind();
							m_SceneRendererContext->ActiveFramebuffer->ReadPixels(0, 1, 1, spec.Width, spec.Height, raw);
							m_SceneRendererContext->ActiveFramebuffer->Unbind();

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
								ImVec2 size = ImVec2(width, width / aspectRatio);

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

						if (ImGui::MenuItem(m_VideoCaptureOutputPath.empty() ? FA_VIDEO " Video Capture Viewport" : FA_RECORD_VINYL " Stop Video Capture"))
						{
							if (m_VideoCaptureOutputPath.empty())
							{
								const std::filesystem::path videoCaptureDirectory = Project::GetProjectDirectory() / "Saved" / "Video Captures";
								if (!std::filesystem::exists(videoCaptureDirectory))
									std::filesystem::create_directory(videoCaptureDirectory);

								m_VideoCaptureOutputPath = videoCaptureDirectory  / fmt::format("Video Capture {}.mp4", Utils::GetFileSafeTimestamp());
							}
							else
								m_VideoCaptureOutputPath.clear();
						}

						ImGui::EndCombo();
					}
					ImGui::PopStyleVar();

					// Draw Frame Counters
					if (Preferences::GetData().ShowFPS)
					{
						auto drawList = ImGui::GetWindowDrawList();
						char buff[256];

						const float& fps = 1.0f / m_DeltaTime;

						const ImU32 color = ImGui::GetColorU32(fps > 55.0f ? (ImVec4(0.1f, 0.8f, 0.2f, 1.0f)) : (fps > 25.0f ? (ImVec4(1.0f, 0.95f, 0.85f, 1.0f)) : (ImVec4(0.8f, 0.1f, 0.2f, 1.0f))));

						sprintf(buff, "%.2f FPS", fps);
						drawList->AddText(ImVec2(ImGui::GetWindowPos().x + style.FramePadding.x, ImGui::GetItemRectMax().y + style.FramePadding.y), color, buff);

						memset(buff, 0, 256);

						sprintf(buff, "%.2f ms", m_DeltaTime * 1000.0f);
						drawList->AddText(ImVec2(ImGui::GetWindowPos().x + style.FramePadding.x, ImGui::GetItemRectMax().y + style.FramePadding.y * 3.0f + ImGui::GetTextLineHeight()), color, buff);

						const uint32_t assetThreadCount = AssetThread::QueuedWorkCount();
						if (assetThreadCount)
						{
							const std::string assetThreadMessage = fmt::format("Waiting for assets... ({})", assetThreadCount);
							drawList->AddText(ImVec2(ImGui::GetWindowPos().x + style.FramePadding.x, ImGui::GetItemRectMax().y + style.FramePadding.y * 5.0f + ImGui::GetTextLineHeight() * 2.0f), ImGui::GetColorU32(ImVec4(0.8f, 0.1f, 0.2f, 1.0f)), assetThreadMessage.c_str());
						}
						
						if (!m_VideoCaptureOutputPath.empty())
						{
							const std::string text = fmt::format("{} Framebuffer capture in progress.", glm::fract(ImGui::GetTime()) < 0.5 ? FA_RECORD_VINYL : "        ");
							drawList->AddText(ImGui::GetWindowPos() + ImVec2(style.FramePadding.x, ImGui::GetWindowSize().y - style.WindowPadding.y - style.FramePadding.y - ImGui::GetTextLineHeight()), ImGui::GetColorU32(ImVec4(0.8f, 0.1f, 0.2f, 1.0f)), text.c_str());
						}
					}
				}

				ImGui::SameLine();

				// Viewport Render Settings
				SceneRendererContext::RendererVisualizationMode rendererVisualizationMode = m_SceneRendererContext->VisualizationMode;
				UI::DrawViewportRenderSettingsButton(rendererVisualizationMode);
				if (rendererVisualizationMode != m_SceneRendererContext->VisualizationMode)
					SetRendererVisualizationMode(rendererVisualizationMode);

				ImGui::SameLine();
				ImGui::Dummy(ImVec2{ ImGui::GetContentRegionAvail().x - 512, 0 });
				ImGui::SameLine();

				// Viewport gizmo settings
				{
					const bool activeOperations[] = {
						IsGizmoEnabled(GizmoOperation::None),
						IsGizmoEnabled(GizmoOperation::Translate),
						IsGizmoEnabled(GizmoOperation::Rotate),
						IsGizmoEnabled(GizmoOperation::Scale),
						IsGizmoEnabled(GizmoOperation::Universal),
					};

					int selectedOperation = -1;
					const char* gizmoTypeIcons[] = { CHARACTER_ICON_GIZMO_CURSOR, CHARACTER_ICON_GIZMO_TRANSLATE, CHARACTER_ICON_GIZMO_ROTATE, CHARACTER_ICON_GIZMO_SCALE, FA_GROUP_ARROWS_ROTATE };
					const GizmoOperation gizmoOperations[] = { GizmoOperation::None, GizmoOperation::Translate, GizmoOperation::Rotate, GizmoOperation::Scale, GizmoOperation::Universal };
					if (ImGui::SwitchButtonEx("##GizmoTypeSwitch", gizmoTypeIcons, IM_ARRAYSIZE(gizmoTypeIcons), &selectedOperation, activeOperations, ImVec2(150, 30)))
						SetGizmoOperation(gizmoOperations[selectedOperation]);

					ImGui::SameLine();

					if (ImGui::Button(m_GizmoMode == GizmoMode::Local ? CHARACTER_ICON_SPACE_LOCAL : CHARACTER_ICON_SPACE_WORLD, ImVec2(30, 30)))
						m_GizmoMode = (m_GizmoMode == GizmoMode::Local) ? GizmoMode::World : GizmoMode::Local;

					constexpr Utils::SnapValues translationSnapValues[] = {
						{ "0.01",	0.01f	},
						{ "0.05",	0.05f	},
						{ "0.1",	0.1f	},
						{ "0.5",	0.5f	},
						{ "1",		1.0f	},
						{ "5",		5.0f	},
						{ "10",		10.0f	},
						{ "50",		50.0f	},
						{ "100",	100.0f	}
					};

					constexpr Utils::SnapValues rotationSnapValues[] = {
						{ "1" CHARACTER_SYMBOL_DEGREE,		1.0f					},
						{ "5" CHARACTER_SYMBOL_DEGREE,		5.0f					},
						{ "10" CHARACTER_SYMBOL_DEGREE,		10.0f					},
						{ "15" CHARACTER_SYMBOL_DEGREE,		15.0f					},
						{ "30" CHARACTER_SYMBOL_DEGREE,		30.0f					},
						{ "45" CHARACTER_SYMBOL_DEGREE,		45.0f					},
						{ "60" CHARACTER_SYMBOL_DEGREE,		60.0f					},
						{ "90" CHARACTER_SYMBOL_DEGREE,		90.0f					},
						{ "120" CHARACTER_SYMBOL_DEGREE,	120.0f					},
						{ "180" CHARACTER_SYMBOL_DEGREE,	180.0f					},
						{ CHARACTER_SYMBOL_PI,				3.14159265358979323846	}
					};

					constexpr Utils::SnapValues scaleSnapValues[] = {
						{ "0.1",	0.1f	},
						{ "0.25",	0.25f	},
						{ "0.5",	0.5f	},
						{ "1",		1.0f	},
						{ "5",		5.0f	},
						{ "10",		10.0f	}
					};

					ImGui::SameLine();
					Utils::DrawGizmoSnappingMenu("##TranslationSnapMenu", CHARACTER_ICON_SNAP_TRANSLATION, translationSnapValues, IM_ARRAYSIZE(translationSnapValues), m_TranslationSnap, m_TranslationSnapValue);
					ImGui::SameLine();
					Utils::DrawGizmoSnappingMenu("##RotationSnapMenu", CHARACTER_ICON_SNAP_ROTATION, rotationSnapValues, IM_ARRAYSIZE(rotationSnapValues), m_RotationSnap, m_RotationSnapValue, CHARACTER_SYMBOL_DEGREE);
					ImGui::SameLine();
					Utils::DrawGizmoSnappingMenu("##ScaleSnapMenu", CHARACTER_ICON_SNAP_SCALING, scaleSnapValues, IM_ARRAYSIZE(scaleSnapValues), m_ScaleSnap, m_ScaleSnapValue);

					ImGui::SameLine();

					UI::DrawCameraSpeedButton(m_EditorCamera, m_CameraBaseSpeed, m_CameraSpeedScale);

					ImGui::SameLine();

					if (ImGui::Button(CHARACTER_ICON_PREFERENCES, ImVec2(40, 30)))
						ImGui::OpenPopup("##GizmoSettingsPopup");

					ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.0f, 5.0f));
					if (ImGui::BeginPopup("##GizmoSettingsPopup", ImGuiWindowFlags_NoMove))
					{
						ImGui::TextDisabled("Gizmo Settings");
						
						ImGui::Separator();

						const char* gizmoOperationText;
						switch (m_GizmoOperation)
						{
						case GizmoOperation::None: gizmoOperationText = FA_ARROW_POINTER " Gizmo Operation"; break;
						case GizmoOperation::Translate: gizmoOperationText = FA_UP_DOWN_LEFT_RIGHT " Gizmo Operation"; break;
						case GizmoOperation::Rotate: gizmoOperationText = FA_ROTATE " Gizmo Operation"; break;
						case GizmoOperation::Scale: gizmoOperationText = FA_EXPAND " Gizmo Operation"; break;
						case GizmoOperation::Universal: gizmoOperationText = FA_GROUP_ARROWS_ROTATE " Gizmo Operation"; break;
						default: gizmoOperationText = "Gizmo Operation"; break;
						}

						if (ImGui::BeginMenu(gizmoOperationText))
						{
							if (ImGui::MenuItem(FA_ARROW_POINTER " None", nullptr, IsGizmoEnabled(GizmoOperation::None)))
								SetGizmoOperation(GizmoOperation::None);
							if (ImGui::MenuItem(FA_UP_DOWN_LEFT_RIGHT " Translate", nullptr, IsGizmoEnabled(GizmoOperation::Translate)))
								SetGizmoOperation(GizmoOperation::Translate);
							if (ImGui::MenuItem(FA_ROTATE " Rotate", nullptr, IsGizmoEnabled(GizmoOperation::Rotate)))
								SetGizmoOperation(GizmoOperation::Rotate);
							if (ImGui::MenuItem(FA_EXPAND " Scale", nullptr, IsGizmoEnabled(GizmoOperation::Scale)))
								SetGizmoOperation(GizmoOperation::Scale);
							if (ImGui::MenuItem(FA_GROUP_ARROWS_ROTATE " Universal", nullptr, IsGizmoEnabled(GizmoOperation::Universal)))
								SetGizmoOperation(GizmoOperation::Universal);

							ImGui::EndMenu();
						}

						if (ImGui::BeginMenu(m_GizmoMode == GizmoMode::Local ? CHARACTER_ICON_CUBE " Gizmo Type" : FA_GLOBE " World"))
						{
							if (ImGui::MenuItem(CHARACTER_ICON_CUBE " Local", nullptr, m_GizmoMode == GizmoMode::Local))
								m_GizmoMode = GizmoMode::Local;
							if (ImGui::MenuItem(FA_GLOBE " World", nullptr, m_GizmoMode == GizmoMode::World))
								m_GizmoMode = GizmoMode::World;

							ImGui::EndMenu();
						}
						
						if (ImGui::BeginMenu(FA_BULLSEYE " Pivot Point"))
						{
							if (ImGui::MenuItem(FA_BULLSEYE " Median Point", nullptr, m_GizmoPivotPoint == GizmoPivotPoint::MedianPoint))
								m_GizmoPivotPoint = GizmoPivotPoint::MedianPoint;
							if (ImGui::MenuItem(FA_SHAPES " Individual Origins", nullptr, m_GizmoPivotPoint == GizmoPivotPoint::IndividualOrigins))
								m_GizmoPivotPoint = GizmoPivotPoint::IndividualOrigins;
							if (ImGui::MenuItem(FA_CIRCLE_DOT " Active Element", nullptr, m_GizmoPivotPoint == GizmoPivotPoint::ActiveElement))
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
							UI::PushFont(FontType::Bold);
							ImGui::GetWindowDrawList()->AddText(ImVec2{ xPos.x - ImGui::CalcTextSize("W").x * 0.33f, xPos.y - ImGui::CalcTextSize("W").y / 2 }, fontColor, "X");
							UI::PopFont();
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
							UI::PushFont(FontType::Bold);
							ImGui::GetWindowDrawList()->AddText(ImVec2{ yPos.x - ImGui::CalcTextSize("W").x * 0.33f, yPos.y - ImGui::CalcTextSize("W").y / 2 }, fontColor, "Z");
							UI::PopFont();
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
							UI::PushFont(FontType::Bold);
							ImGui::GetWindowDrawList()->AddText(ImVec2{ zPos.x - ImGui::CalcTextSize("W").x * 0.33f, zPos.y - ImGui::CalcTextSize("W").y / 2 }, fontColor, "Y");
							UI::PopFont();
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

			// Gizmos
			if (Preferences::GetData().ShowTransformGizmo)
			{
				Entity activeEntity = m_SceneHierarchyPanel.GetActiveEntity();

				if (activeEntity && !m_SceneHierarchyPanel.IsEntityLocked(activeEntity) && m_GizmoOperation != GizmoOperation::None && activeEntity.HasComponent<TransformComponent>())
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
						cameraView = &glm::inverse(cameraEntity.GetComponent<TransformComponent>().Transform.GetMatrix());
					}
					else
					{
						// Editor camera
						cameraProjection = &m_EditorCamera.GetProjection();
						cameraView = &m_EditorCamera.GetViewMatrix();
					}

					// Entity Transform
					TransformComponent& activeTransformComponent = activeEntity.GetComponent<TransformComponent>();
					glm::mat4 modifiedTransformMatrix = m_ActiveScene->GetWorldTransformMatrix(activeEntity);

					// Snapping
					const bool snap = IsGizmoEnabled(GizmoOperation::Translate) ? m_TranslationSnap : IsGizmoEnabled(GizmoOperation::Rotate) ? m_RotationSnap : m_ScaleSnap;
					const float snapValue = IsGizmoEnabled(GizmoOperation::Translate) ? m_TranslationSnapValue : IsGizmoEnabled(GizmoOperation::Rotate) ? m_RotationSnapValue : m_ScaleSnapValue;

					const float snapValues[3] = { snapValue, snapValue, snapValue };

					ImGuizmo::Manipulate(glm::value_ptr(*cameraView), glm::value_ptr(*cameraProjection),
						Utils::GetImGuizmoOperation(m_GizmoOperation), Utils::GetImGuizmoMode(m_GizmoMode), glm::value_ptr(modifiedTransformMatrix),
						nullptr, ((Input::IsKeyPressed(Key::LeftControl) ? !snap : snap) ? snapValues : nullptr));

					if (ImGuizmo::IsUsing())
					{
						switch (m_GizmoPivotPoint)
						{
						case GizmoPivotPoint::MedianPoint:
						{
							// TODO: Allow for multiple entities to be moved at once
							Transform localModifiedTransform = m_ActiveScene->GetLocalTransform(activeEntity, Transform::ConstructTransformFromMatrix(modifiedTransformMatrix));

							Scene* scene = activeEntity.GetScene();
							scene->SetEntityTransform(activeEntity, localModifiedTransform);

							break;
						}
						}

					}
				}

				// Check if a gizmo edit action has been completed, and if so, register a transaction for the changes made to the transform component.
				static bool s_UsingGuizmoLastFrame = false;
				bool usingGizmo = ImGuizmo::IsUsing();

				static TransformComponent s_OriginalTransformComponent;

				// If we have just started using the gizmo, make a copy of the transform of the entity.
				if (usingGizmo && !s_UsingGuizmoLastFrame)
					s_OriginalTransformComponent = activeEntity.GetComponent<TransformComponent>();

				// If we were using the gizmo last frame but not this frame we have finished editing
				// Register a transaction for the changes made to the transform component
				if (s_UsingGuizmoLastFrame && !usingGizmo)
				{
					const TransformComponent& activeTransformComponent = activeEntity.GetComponent<TransformComponent>();

					if (s_OriginalTransformComponent != activeTransformComponent)
					{
						Ref<TransformTransaction> transaction = CreateRef<TransformTransaction>(activeEntity, s_OriginalTransformComponent, activeTransformComponent);
						TransactionManager::Execute(transaction);
					}
				}

				s_UsingGuizmoLastFrame = usingGizmo;
			}

			// Draw the rendered camera preview if a camera is selected.
			if (m_PreviewCamera)
			{
				ImDrawList* drawList = ImGui::GetWindowDrawList();

				const float rounding = 10.0f;
				const ImVec2 min = ImGui::GetWindowPos() + ImGui::GetWindowSize() * 0.5f;
				const ImVec2 max = ImGui::GetWindowPos() + ImGui::GetWindowSize() - style.FramePadding * 4.0f;

				drawList->AddImageRounded((ImTextureID)m_PreviewSceneRendererContext->ActiveFramebuffer->GetColorAttachmentRendererID(), min, max, ImVec2{ 0, 1 }, ImVec2{ 1, 0 }, IM_COL32_WHITE, rounding);
				drawList->AddRect(min, max, ImGui::GetColorU32(ImVec4(0.82f, 0.62f, 0.13f, 1.0f)), rounding, ImDrawFlags_RoundCornersAll, 3.0f);
				drawList->AddText(min + style.FramePadding * 2.0f, IM_COL32_WHITE, FA_CAMERA " Camera Preview");

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
					if (ImGui::BeginCombo("##CommandTypeSelection", m_ViewportCommandLineExecute == 0 ? CHARACTER_ICON_DYMATIC " Dymatic Command" : (m_ViewportCommandLineExecute == 1 ? CHARACTER_ICON_PYTHON " Python Script" : CHARACTER_ICON_PYTHON " Python REPL")))
					{
						if (ImGui::MenuItem(CHARACTER_ICON_DYMATIC " Dymatic Command"))
							m_ViewportCommandLineExecute = 0;
						if (ImGui::MenuItem(CHARACTER_ICON_PYTHON " Python Script"))
							m_ViewportCommandLineExecute = 1;
						if (ImGui::MenuItem(CHARACTER_ICON_PYTHON " Python REPL"))
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

					// Ensure we have an empty history to get from
					if (m_ViewportCommandLineBuffers.empty())
						m_ViewportCommandLineBuffers.push_back(std::string());

					// Ensure the input text doesn't receive the enter event (loosing focus)
					const bool enterDown = ImGui::GetKeyData(ImGuiKey_Enter)->Down;
					ImGui::GetKeyData(ImGuiKey_Enter)->Down = false;

					char buffer[256];
					memset(buffer, 0, sizeof(buffer));
					std::strncpy(buffer, m_ViewportCommandLineBuffers[m_ViewportCommandLineBufferIndex].c_str(), sizeof(buffer));
					ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());
					if (ImGui::InputTextWithHint("##ViewportCommandLineInput", m_ViewportCommandLineExecute == 0 ? ">>> Enter Command:" : (m_ViewportCommandLineExecute == 1 ? ">>> Python Script Path:" : ">>> Python Command:"), buffer, sizeof(buffer),
						ImGuiInputTextFlags_CallbackCharFilter | ImGuiInputTextFlags_CallbackHistory, [](ImGuiInputTextCallbackData* data) -> int
						{
							EditorLayer* editor = ((EditorLayer*)data->UserData);

							if (!editor->ViewportKeyAllowed())
								return 1;

							if (data->EventFlag == ImGuiInputTextFlags_CallbackCharFilter)
							{
								if (data->EventChar == '`')
								{
									editor->m_ViewportCommandLineOpen = !editor->m_ViewportCommandLineOpen;
									return 1;
								}
							}
							else if (data->EventFlag == ImGuiInputTextFlags_CallbackHistory)
							{
								if (data->EventKey == ImGuiKey_DownArrow || data->EventFlag == ImGuiInputTextFlags_CallbackHistory)
								{
									if (data->EventKey == ImGuiKey_UpArrow)
										editor->m_ViewportCommandLineBufferIndex = std::max(editor->m_ViewportCommandLineBufferIndex - 1, 0);

									if (data->EventKey == ImGuiKey_DownArrow)
										editor->m_ViewportCommandLineBufferIndex = std::min(editor->m_ViewportCommandLineBufferIndex + 1, (int)editor->m_ViewportCommandLineBuffers.size() - 1);

									data->DeleteChars(0, data->BufTextLen);
									data->InsertChars(0, editor->m_ViewportCommandLineBuffers[editor->m_ViewportCommandLineBufferIndex].c_str());

									return 1;
								}
							}
					
							return 0;
						}, this)
					)
					{
						m_ViewportCommandLineBuffers[m_ViewportCommandLineBufferIndex] = std::string(buffer);
					}

					// Restore the previous state of the enter key in ImGui's context
					ImGui::GetKeyData(ImGuiKey_Enter)->Down = enterDown;

					// Display a command preview if one is known
					if (ImGui::IsItemActive())
					{
						const ImVec2 commandLinePosition = ImVec2(ImGui::GetItemRectMin().x, ImGui::GetItemRectMax().y);

						static const std::string commandList[] = {
							"editor.freezefrustum",
							"editor.save",
							"editor.quit"
						};

						ImGui::OpenPopup("##ViewportCommandLinePopup");
						ImGui::BeginPopup("##ViewportCommandLinePopup", ImGuiWindowFlags_NoFocusOnAppearing);
						ImGui::SetWindowPos(commandLinePosition);

						for (const auto& command : commandList)
							if (command.find(m_ViewportCommandLineBuffers[m_ViewportCommandLineBufferIndex]) != std::string::npos)
								if (ImGui::MenuItem(command.c_str()))
									m_ViewportCommandLineBuffers[m_ViewportCommandLineBufferIndex] = command;

						ImGui::EndPopup();
					}

					if (ImGui::IsItemActive() && ImGui::IsKeyPressed(ImGuiKey_Enter) && !m_ViewportCommandLineBuffers[m_ViewportCommandLineBufferIndex].empty())
					{
						// Add the current index to the history and reset the buffers to the history
						m_ViewportCommandLineHistory.push_back(m_ViewportCommandLineBuffers[m_ViewportCommandLineBufferIndex]);
						m_ViewportCommandLineBuffers = m_ViewportCommandLineHistory;
						m_ViewportCommandLineBuffers.push_back(std::string());
						m_ViewportCommandLineBufferIndex = m_ViewportCommandLineBuffers.size() - 1;
						
						ImGui::GetInputTextState(ImGui::GetItemID())->ClearText();

						// The command executed was the most recent in history.
						const std::string& command = m_ViewportCommandLineHistory.back();

						// Convert the command to a list of arguments
						std::istringstream iss(command);
						std::vector<std::string> arguments;
						std::copy(std::istream_iterator<std::string>(iss), std::istream_iterator<std::string>(), std::back_inserter(arguments));

						// Execute the command
						if (m_ViewportCommandLineExecute == 0)
						{
							if (arguments.size() == 2 && arguments[0] == "editor.freezefrustum")
								m_SceneRendererContext->FreezeFrustumUpdate = String::ToInteger(arguments[1]) != 0;
						}
						else if (m_ViewportCommandLineExecute == 1)
						{
							PythonTools::RunScript(command);
						}
						else if (m_ViewportCommandLineExecute == 2)
						{
							PythonTools::RunGlobalCommand(command);
						}
					}

					ImGui::PopStyleVar();
					ImGui::PopStyleColor(3);
				}
			}

			ImGui::End();
			ImGui::PopStyleVar();
		}

		if (m_ShowSplash)
		{
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 20.0f);
			ImGui::Begin(FA_HOUSE " Splash", nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);
			ImGui::SetWindowPos(ImVec2((io.DisplaySize.x - ImGui::GetWindowWidth()) * 0.5f + dockspaceWindowPosition.x, (io.DisplaySize.y - ImGui::GetWindowHeight()) * 0.5f + dockspaceWindowPosition.y));
			
			if (ImGui::IsWindowAppearing())
				ReloadAvailableWorkspaces();
			
			ImGui::Image((ImTextureID)EditorResources::DymaticSplash->GetRendererID(), ImVec2(488, 267), { 0, 1 }, { 1, 0 });
			ImGui::SameLine();
			const char* versionName = CHARACTER_ICON_DYMATIC " Version " DY_VERSION_STRING;
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
		
			if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows) && (ImGui::IsAnyMouseDown() || ImGui::IsAnyItemActive() || ImGui::IsAnyItemFocused() || GImGui->ActiveIdWindow))
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
				UI::PushFont(FontType::Small);
				auto drawList = ImGui::GetForegroundDrawList();

				ImVec2 drawPos = ImVec2(ImGui::GetWindowPos().x + style.FramePadding.x, ImGui::GetWindowPos().y + ImGui::GetWindowSize().y);
				for (auto it = std::rbegin(m_DebugMessages); it != std::rend(m_DebugMessages); ++it)
				{
					auto& message = *it;

					if (drawPos.y > ImGui::GetWindowPos().y)
						drawPos.y -= ImGui::CalcTextSize(message.Text.c_str()).y + style.FramePadding.y * 2.0f;
					else
						break;
				
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
				}

				UI::PopFont();
			}

			
		}

		ImGui::End();

		UI::PostUIUpdate();
	}
	
	void EditorLayer::OnEvent(Event& e)
	{
		m_EditorCamera.OnEvent(e);

		m_NodeEditorPannel.OnEvent(e);
		m_PreferencesPannel.OnEvent(e);
		m_CurveEditor.OnEvent(e);
		m_LogPanel.OnEvent(e);
		m_ImageEditor.OnEvent(e);
		m_TextEditor.OnEvent(e);

		// Pass events to editor asset panels
		for (auto& [handle, panel] : m_AssetEditorPanels)
			panel->OnEvent(e);

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
			case Preferences::Keymap::SelectObjectBind: { if (m_ViewportHovered && !ImGuizmo::IsUsing() && !ImGuizmo::IsOver() && !Input::IsKeyPressed(Key::LeftAlt) && UI::GetEntityPickingID() == 0) { m_SceneHierarchyPanel.SelectedEntity(m_HoveredEntity); } } break;
			case Preferences::Keymap::SceneStartBind: { if (m_SceneState == SceneState::Play) OnScenePause(); else OnScenePlay(); } break;
			case Preferences::Keymap::SceneSimulateBind: { if (m_SceneState == SceneState::Simulate) OnScenePause(); else OnSceneSimulate(); } break;
			case Preferences::Keymap::SceneStopBind: { if (m_SceneState != SceneState::Edit) OnSceneStop(); } break;
			case Preferences::Keymap::FocusBind: if (ViewportKeyAllowed() && !ImGuizmo::IsUsing()) OnFocus(); break;
			case Preferences::Keymap::ReloadAssembly: { if (m_SceneState == SceneState::Edit) ScriptEngine::ReloadAssembly(); } break;
			case Preferences::Keymap::GizmoNoneBind: { if (ViewportKeyAllowed() && !ImGuizmo::IsUsing()) { SetGizmoOperation(GizmoOperation::None); } } break;
			case Preferences::Keymap::GizmoTranslateBind: { if (ViewportKeyAllowed() && !ImGuizmo::IsUsing()) { SetGizmoOperation(GizmoOperation::Translate); } } break;
			case Preferences::Keymap::GizmoRotateBind: { if (ViewportKeyAllowed() && !ImGuizmo::IsUsing()) { SetGizmoOperation(GizmoOperation::Rotate); } } break;
			case Preferences::Keymap::GizmoScaleBind: { if (ViewportKeyAllowed() && !ImGuizmo::IsUsing()) { SetGizmoOperation(GizmoOperation::Scale); } } break;
			case Preferences::Keymap::CreateBind: { if (ViewportKeyAllowed()) { m_SceneHierarchyPanel.ShowCreateMenu(); } } break;
			case Preferences::Keymap::DuplicateBind: { if (ViewportKeyAllowed() && m_SceneHierarchyPanel.GetActiveEntity()) { m_SceneHierarchyPanel.DuplicateEntities(); } m_NodeEditorPannel.DuplicateNodes(); } break;
			case Preferences::Keymap::DeleteBind: { if (ViewportKeyAllowed() && m_SceneHierarchyPanel.GetActiveEntity()) { m_SceneHierarchyPanel.DeleteEntities(); } } break;
			case Preferences::Keymap::UndoBind: Undo(); break;
			case Preferences::Keymap::RedoBind: Redo(); break;
			case Preferences::Keymap::VisualizationRenderedBind: { if (ViewportKeyAllowed()) SetRendererVisualizationMode(SceneRendererContext::RendererVisualizationMode::Rendered); } break;
			case Preferences::Keymap::VisualizationWireframeBind: { if (ViewportKeyAllowed()) SetRendererVisualizationMode(SceneRendererContext::RendererVisualizationMode::Wireframe); } break;
			case Preferences::Keymap::VisualizationLightingOnlyBind: { if (ViewportKeyAllowed()) SetRendererVisualizationMode(SceneRendererContext::RendererVisualizationMode::LightingOnly); } break;
			case Preferences::Keymap::VisualizationAlbedoBind: { if (ViewportKeyAllowed()) SetRendererVisualizationMode(SceneRendererContext::RendererVisualizationMode::Albedo); } break;
			case Preferences::Keymap::VisualizationNormalBind: { if (ViewportKeyAllowed()) SetRendererVisualizationMode(SceneRendererContext::RendererVisualizationMode::Normal); } break;
			case Preferences::Keymap::VisualizationEntityIDBind: { if (ViewportKeyAllowed()) SetRendererVisualizationMode(SceneRendererContext::RendererVisualizationMode::EntityID); } break;
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

	glm::vec2 EditorLayer::GetMouseViewportPosition()
	{
		// Calculate mouse pixel position
		auto [mx, my] = ImGui::GetMousePos();
		mx -= m_ViewportBounds[0].x;
		my -= m_ViewportBounds[0].y;
		glm::vec2 viewportSize = GetViewportSize();
		my = viewportSize.y - my;

		return glm::vec2(mx, my);
	}

	glm::vec3 EditorLayer::GetHoveredWorldPositionBounded(const float bound)
	{
		const glm::vec2 mouse = GetMouseViewportPosition();

		// Read depth from framebuffer
		m_SceneRendererContext->ActiveFramebuffer->Bind();
		float depth = m_SceneRendererContext->ActiveFramebuffer->ReadDepthPixel(mouse.x, mouse.y);
		m_SceneRendererContext->ActiveFramebuffer->Unbind();

		depth = std::min(depth, bound);

		// Get position from depth
		float z = depth * 2.0f - 1.0f;
		glm::vec2 texCoords = mouse / m_ViewportSize;
		glm::vec4 clipSpacePosition = glm::vec4(texCoords * 2.0f - 1.0f, z, 1.0f);
		glm::vec4 viewSpacePosition = glm::inverse(m_EditorCamera.GetViewProjection()) * clipSpacePosition;
		
		// Perspective division
		viewSpacePosition /= viewSpacePosition.w;
		return viewSpacePosition;
	}

	glm::vec3 EditorLayer::GetHoveredWorldPositionUnbounded()
	{
		return GetHoveredWorldPositionBounded(1.0f);
	}

	glm::vec3 EditorLayer::GetHoveredWorldPosition()
	{
		return GetHoveredWorldPositionBounded(0.99f);
	}

	glm::vec2 EditorLayer::WorldToViewportPosition(const glm::vec3& worldPosition)
	{
		glm::vec4 clipPos = m_EditorCamera.GetViewProjection() * glm::vec4(worldPosition, 1.0f);
		clipPos /= clipPos.w;

		glm::vec2 screenPosition;
		screenPosition.x = (clipPos.x + 1.0f) / 2.0f * m_ViewportSize.x;
		screenPosition.y = (1.0f - clipPos.y) / 2.0f * m_ViewportSize.y;

		return screenPosition;
	}

	void EditorLayer::OnOverlayRender()
	{
		if (m_SceneState == SceneState::Play)
		{
			Entity camera = m_ActiveScene->GetPrimaryCameraEntity();
			if (!camera)
				return;

			Renderer2D::BeginScene(camera.GetComponent<CameraComponent>().Camera, camera.GetComponent<TransformComponent>().Transform.GetMatrix());
		}
		else
		{
			Renderer2D::BeginScene(m_EditorCamera);
		}

		if (m_ActiveScene->GetShowColliders())
		{
			// Box Colliders 2D
			{
				auto view = m_ActiveScene->GetAllEntitiesWith<TransformComponent, BoxCollider2DComponent>();
				for (auto entity : view)
				{
					const auto& [tc, bc2d] = view.get<TransformComponent, BoxCollider2DComponent>(entity);
					const auto& transform = tc.Transform;

					glm::vec3 translation = transform.Translation + glm::vec3(bc2d.Offset, 0.001f);
					glm::vec3 scale = transform.Scale * glm::vec3(bc2d.Size * 2.0f, 1.0f);

					const glm::mat4 matrix = glm::translate(glm::mat4(1.0f), translation)
						* glm::rotate(glm::mat4(1.0f), transform.Rotation.z, glm::vec3(0.0f, 0.0f, 1.0f))
						* glm::scale(glm::mat4(1.0f), scale);

					Renderer2D::DrawRect(matrix, glm::vec4(0, 1, 0, 1));
				}
			}

			// Circle Colliders 2D
			{
				auto view = m_ActiveScene->GetAllEntitiesWith<TransformComponent, CircleCollider2DComponent>();
				for (auto entity : view)
				{
					const auto& [tc, cc2d] = view.get<TransformComponent, CircleCollider2DComponent>(entity);
					const auto& transform = tc.Transform;

					glm::vec3 translation = transform.Translation + glm::vec3(cc2d.Offset, 0.001f);
					glm::vec3 scale = transform.Scale * glm::vec3(cc2d.Radius * 2.0f);

					const glm::mat4 matrix = glm::translate(glm::mat4(1.0f), translation)
						* glm::scale(glm::mat4(1.0f), scale);

					Renderer2D::DrawCircle(matrix, glm::vec4(0, 1, 0, 1), 0.01f);
				}
			}
		}
		
		if (m_SceneState != SceneState::Play)
		{
			// Draw ruler lines
			for (auto& line : m_RulerLines)
			{
				Renderer2D::DrawLine(line.Start, line.End, glm::vec4(0.5f, 0.5f, 0.5f, 1.0));

				const glm::vec3 textPosition = (line.Start + line.End) * 0.5f;
				const float distance = glm::distance(line.Start, line.End);

				glm::mat4 transform = glm::translate(glm::mat4(1.0f), textPosition - m_EditorCamera.GetPosition())
					* glm::inverse(m_EditorCamera.GetViewMatrix())
					* glm::scale(glm::mat4(1.0f), 0.025f * glm::vec3(glm::distance(m_EditorCamera.GetPosition(), textPosition)));

				Renderer2D::DrawText(transform, fmt::format("{:.5f} m", distance), TextAlignment::Center, m_EditorFont, glm::vec4(1.0f));
			}

			// Draw selected 2D entity outline
			auto& selectedEntities = m_SceneHierarchyPanel.GetSelectedEntities();
			for (auto e : selectedEntities)
			{
				Entity selectedEntity = { e, m_ActiveScene.get() };
				if (selectedEntity.HasComponent<SpriteRendererComponent>() || selectedEntity.HasComponent<CircleRendererComponent>())
				{
					TransformComponent& tc = selectedEntity.GetComponent<TransformComponent>();
					Renderer2D::DrawRect(tc.Transform.GetMatrix(), glm::vec4(0.82f, 0.62f, 0.13f, 1.0));
				}
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

			ScriptEngine::SetCoreAssemblyPath(Project::GetCoreModulePath());
			ScriptEngine::SetAppAssemblyPath(Project::GetScriptModulePath());
			ScriptEngine::ReloadAssembly();
			
			auto startScenePath = AssetManager::GetFileSystemPathString(Project::GetActive()->GetConfig().StartScene);
			if (!OpenScene(startScenePath))
				NewScene();

			SourceControl::Connect();
			Notification::Clear();
			m_ContentBrowserPanel.Init();
			m_ProjectLauncher.AddRecentProject(path);
			m_LogPanel.ClearLog();
		}
	}

	void EditorLayer::SaveProject()
	{
	}

	void EditorLayer::NewScene()
	{
		if (m_SceneState != SceneState::Edit)
			OnSceneStop();

		m_EditorScene = AssetManager::CreateMemoryOnlyAsset<Scene>();
		m_SceneHierarchyPanel.SetContext(m_EditorScene);

		m_ActiveScene = m_EditorScene;
		m_EditorScenePath = std::filesystem::path();

		//Reset Scene time values
		m_LastSaveTime = 0;
		m_ProgramTime = 0;
	}

	void EditorLayer::AppendScene()
	{
		std::string filepath = FileDialogs::OpenFile("Dymatic Scene (*.dymatic)\0*.dymatic\0");
		if (!filepath.empty())
			AppendScene(filepath);
	}

	void EditorLayer::AppendScene(const std::filesystem::path& path)
	{
	}

	bool EditorLayer::OpenScene()
	{
		std::string filepath = FileDialogs::OpenFile("Dymatic Scene (*.dymatic)\0*.dymatic\0");
		if (!filepath.empty())
			return OpenScene(filepath);
		return false;
	}

	bool EditorLayer::OpenScene(const std::filesystem::path& path)
	{
		if (m_SceneState != SceneState::Edit)
			OnSceneStop();

		if (path.extension().string() != ".dymatic")
		{
			DY_WARN("Could not load {} - not a scene file", path.filename().string());
			return false;
		}
		
		if (Ref<Scene> newScene = AssetManager::GetAsset<Scene>(std::filesystem::relative(path, Project::GetAssetDirectory())))
		{
			m_EditorScene = newScene;
			m_SceneHierarchyPanel.SetContext(m_EditorScene);

			m_ActiveScene = m_EditorScene;
			m_EditorScenePath = path;

			ProjectSettings::AddRecentScenePath(m_EditorScenePath);

			//Reset Scene time values
			m_LastSaveTime = 0;
			m_ProgramTime = 0;

			return true;
		}

		return false;
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

	void EditorLayer::Undo()
	{
		if (Ref<Transaction> transaction = TransactionManager::Undo())
			Notification::Create("Undo Action", fmt::format("The transaction for '{}'\nhas been undone.", transaction->GetName()), {}, 3.0f);
	}

	void EditorLayer::Redo()
	{
		if (Ref<Transaction> transaction = TransactionManager::Redo())
			Notification::Create("Redo Action", fmt::format("The transaction for '{}'\nhas been redone.", transaction->GetName()), {}, 3.0f);
	}

	void EditorLayer::SerializeScene(Ref<Scene> scene, const std::filesystem::path& path)
	{
		DY_CORE_ASSERT(!path.empty());

		const std::filesystem::path& assetPath = AssetManager::GetMetadata(scene->Handle).FilePath;

		AssetManager::RenameAsset(scene, path);

		m_LastSaveTime = 0;

		// Update project screenshot
		{
			std::filesystem::path projectSavedDirectory = Project::GetProjectDirectory() / "Saved";
			if (!std::filesystem::exists(projectSavedDirectory))
				std::filesystem::create_directory(projectSavedDirectory);

			auto& spec = m_SceneRendererContext->ActiveFramebuffer->GetSpecification();
			uint32_t width = std::min(spec.Width, spec.Height);
			
			uint32_t size = width * width * 4;
			float* raw = new float[size];

			m_SceneRendererContext->ActiveFramebuffer->Bind();
			m_SceneRendererContext->ActiveFramebuffer->ReadPixels(0, (spec.Width - width) / 2 + 1, (spec.Height - width) / 2 + 1, width, width, raw);
			m_SceneRendererContext->ActiveFramebuffer->Unbind();

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
			Popup::Create("Compilation Failure", "Development Environment has not been specified.", { { "Ok", nullptr } }, EditorResources::ErrorIcon);
		else if (!std::filesystem::exists(devenvPath))
			Popup::Create("Compilation Failure", "Cannot access Development Environment", { { "Ok", nullptr } }, EditorResources::ErrorIcon);
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
				Popup::Create("Compilation Failure", compileMessage, { { "Ok", nullptr } }, EditorResources::ErrorIcon);
				Taskbar::FlashIcon();
			}
			else
			{
				// No errors detected, indicate successful compilation
				EditorResources::SoundCompileSuccess->Play();
				return;
			}
		}

		// We failed to compile, so prompt the user
		Taskbar::FlashIcon();
		EditorResources::SoundCompileFailure->Play();
	}

	void EditorLayer::SaveAndExit()
	{
		Popup::Create(FA_TRIANGLE_EXCLAMATION " Unsaved Changes", "Save changes before closing?\nActive Scene: " + m_EditorScenePath.stem().string(),
			{ 
				{ FA_CIRCLE_XMARK " Cancel" }, 
				{ FA_TRASH " Discard", [&]() { CloseProgramWindow(); } }, 
				{ FA_FLOPPY_DISK " Save", [&]() { if (SaveScene()) { CloseProgramWindow(); } } } 
			}, EditorResources::QuestionMarkIcon);
	}

	void EditorLayer::CloseProgramWindow()
	{
		Application::Get().Close();
	}

	void EditorLayer::SetRendererVisualizationMode(SceneRendererContext::RendererVisualizationMode visualizationMode)
	{
		if (visualizationMode == m_SceneRendererContext->VisualizationMode)
			return;

		m_PreviousVisualizationMode = m_SceneRendererContext->VisualizationMode;
		m_SceneRendererContext->VisualizationMode = visualizationMode;
	}

	void EditorLayer::ToggleRendererVisualizationMode()
	{
		SceneRendererContext::RendererVisualizationMode visualizationMode = m_SceneRendererContext->VisualizationMode;
		m_SceneRendererContext->VisualizationMode = m_PreviousVisualizationMode;
		m_PreviousVisualizationMode = visualizationMode;
	}

	void EditorLayer::SetGizmoOperation(const GizmoOperation operation)
	{
		m_GizmoOperation = ImGui::GetIO().KeyShift ? (m_GizmoOperation ^ operation) : operation;
	}

	bool EditorLayer::IsGizmoEnabled(const GizmoOperation operation)
	{
		switch (operation)
		{
		case GizmoOperation::None:		return m_GizmoOperation == GizmoOperation::None;
		case GizmoOperation::Translate:	return (bool)(m_GizmoOperation & GizmoOperation::Translate);
		case GizmoOperation::Rotate:	return (bool)(m_GizmoOperation & GizmoOperation::Rotate);
		case GizmoOperation::Scale:		return (bool)(m_GizmoOperation & GizmoOperation::Scale);
		case GizmoOperation::Universal:	return m_GizmoOperation == GizmoOperation::Universal;
		}
	}

	void EditorLayer::OnScenePlay()
	{
		if (m_SceneState == SceneState::Simulate)
			OnSceneStop();

		if (Preferences::GetData().LogClearOnPlay)
			m_LogPanel.ClearLog();

		m_SceneState = SceneState::Play;
		m_ActiveScene->SetPaused(false);

		m_ActiveScene = Scene::Copy(m_EditorScene);
		m_ActiveScene->OnRuntimeStart();

		m_SceneHierarchyPanel.SetContext(m_ActiveScene);

		EditorResources::SoundPlay->Play();
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

		EditorResources::SoundSimulate->Play();
	}

	void EditorLayer::OnSceneStop()
	{
		DY_CORE_ASSERT(m_SceneState == SceneState::Play || m_SceneState == SceneState::Simulate);

		if (m_SceneState == SceneState::Play)
		{
			m_ActiveScene->OnRuntimeStop();

			// Unlock the cursor if the runtime application locked it
			Application::Get().GetWindow().LockCursor(false);
		}
		else if (m_SceneState == SceneState::Simulate)
			m_ActiveScene->OnSimulationStop();

		m_SceneState = SceneState::Edit;

		m_ActiveScene = m_EditorScene;

		m_SceneHierarchyPanel.SetContext(m_ActiveScene);

		EditorResources::SoundStop->Play();
	}

	void EditorLayer::OnScenePause()
	{
		if (m_SceneState == SceneState::Edit)
			return;

		bool paused = !m_ActiveScene->IsPaused();
		m_ActiveScene->SetPaused(paused);

		if (paused)
			EditorResources::SoundPause->Play();
		else
			EditorResources::SoundPlay->Play();
	}

	static void AddEntityToBounds(Entity entity, AABB& selectionBounds)
	{
		// Consider children
		const std::vector<Entity> children = entity.GetChildren();
		for (const auto& child : children)
			AddEntityToBounds(child, selectionBounds);

		if (!entity.HasComponent<TransformComponent>())
			return;

		const auto& tc = entity.GetComponent<TransformComponent>();

		if (entity.HasComponent<StaticMeshComponent>())
		{
			const auto& smc = entity.GetComponent<StaticMeshComponent>();
			selectionBounds.Extend(smc.m_Model->GetAABB().Transform(entity.GetWorldTransform().GetMatrix()));
		}
		else
		{
			const float defaultHalfSize = 0.5f;
			const Transform transform = entity.GetWorldTransform();
			selectionBounds.Extend(AABB(glm::vec3(-defaultHalfSize) * transform.Scale + transform.Translation, glm::vec3(defaultHalfSize) * transform.Scale + transform.Translation));
		}
	}

	void EditorLayer::OnFocus()
	{
		if (m_SceneState == SceneState::Play)
			return;

		Entity active = m_SceneHierarchyPanel.GetActiveEntity();
		if (!active)
			return;

		AABB selectionBounds;
		AddEntityToBounds(active, selectionBounds);

		if (selectionBounds == AABB())
			return;

		const float focusZoomFactor = 1.2f;
		const float fov = m_EditorCamera.GetFOV();
		EditorCamera::EditorCameraTransform cameraTransform = m_EditorCamera.GetTransform();
		cameraTransform.FocalPoint = selectionBounds.GetCenter();
		cameraTransform.Distance = focusZoomFactor * selectionBounds.GetRadius() / glm::tan(fov * 0.5f);
		m_EditorCamera.SmoothTransform(cameraTransform);
	}

	void EditorLayer::ShowEditorWindow()
	{
		Application::Get().GetWindow().ShowWindow();

		// Resolve all events that can only be handled/executed once the window is visible
		Taskbar::UpdateThumbnailButtons();
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
		
		// File Type specific action
		switch (type)
		{
		case FileType::FileTypeScene: { OpenScene(Project::GetAssetFileSystemPath((path))); return; }
		case FileType::FileTypeScript: VisualStudioInterface::OpenFile(Project::GetAssetFileSystemPath((path))); return;
		}

		// Check if the file has a dedicated asset panel which we can open
		if (TryOpenAssetEditorPanel(path))
			return;

		// Otherwise see if a default application was specified
		auto& defaultApplications = Preferences::GetData().DefaultApplications;
		std::string extension = path.extension().string();
		if (defaultApplications.find(extension) != defaultApplications.end())
			System::Execute("\"\"" + defaultApplications[extension].string() + "\" \"" + Project::GetAssetFileSystemPath((path)).string() + "\"\"");
	}

	bool EditorLayer::TryOpenAssetEditorPanel(const std::filesystem::path& path)
	{
		const AssetMetadata& metadata = AssetManager::GetMetadata(path);

		if (!metadata.Handle)
			return false;
		
		if (m_AssetEditorPanels.find(metadata.Handle) == m_AssetEditorPanels.end())
		{
			Ref<EditorPanel> panel = nullptr;

			switch (metadata.Type)
			{
			case AssetType::Texture:		panel = CreateRef<TextureViewerPanel>(metadata.Handle); break;
			case AssetType::Mesh:			panel = CreateRef<MeshViewerPanel>(AssetManager::GetAsset<Model>(metadata.Handle)); break;
			case AssetType::Skeleton:		panel = CreateRef<SkeletonViewerPanel>(AssetManager::GetAsset<Skeleton>(metadata.Handle)); break;
			case AssetType::VirtualTexture:	panel = CreateRef<VirtualTexturePanel>(AssetManager::GetAsset<Texture2D>(metadata.Handle)); break;
			case AssetType::VideoPlayer:	panel = CreateRef<VideoPlayerPanel>(AssetManager::GetAsset<VideoReader>(metadata.Handle)); break;
			case AssetType::ParticleSystem:	panel = CreateRef<ParticleSystemPanel>(AssetManager::GetAsset<ParticleSystem>(metadata.Handle)); break;
			case AssetType::Font:			panel = CreateRef<FontViewerPanel>(metadata.Handle); break;
			case AssetType::Material:
			{
				Ref<MaterialAsset> material = AssetManager::GetAsset<MaterialAsset>(metadata.Handle);
				if (material && material->IsInstance())
					panel = CreateRef<MaterialInstancePanel>(As<MaterialInstance>(material));
				else
					panel = CreateRef<MaterialPanel>(As<MaterialSource>(material));

				break;
			}
			case AssetType::AnimationGraph: panel = CreateRef<AnimationGraphPanel>(AssetManager::GetAsset<AnimationGraph>(metadata.Handle)); break;
			default:
				return false;
			}

			if (panel)
				m_AssetEditorPanels[metadata.Handle] = panel;
		}
		else
			m_AssetEditorPanels[metadata.Handle]->Focus();

		return true;
	}

}