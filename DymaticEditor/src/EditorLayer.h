#pragma once

#include "Dymatic.h"
#include "Panels/SceneHierarchyPanel.h"
#include "Panels/ContentBrowserPanel.h"

#include "Panels/PopupsAndNotifications.h"
#include "Panels/PreferencesPanel.h"
#include "Panels/Nodes/ScriptEditor.h"
#include "Panels/Nodes/MaterialEditor.h"
#include "Panels/TextEditor.h"
#include "Panels/CurveEditor.h"
#include "Panels/ImageEditor.h"
#include "Panels/MaterialEditorPanel.h"
#include "Panels/ProfilerPanel.h"
#include "Panels/ConsoleWindow.h"
#include "Panels/AssetManagerPanel.h"
#include "Panels/PerformanceAnalyser.h"
#include "Panels/ProjectBrowser.h"
#include "Panels/SourceControl.h"

#include "Tools/PythonTools.h"
#include "Dymatic/Renderer/EditorCamera.h"

namespace Dymatic {

	class EditorLayer : public Layer
	{
	public:
		EditorLayer();
		virtual ~EditorLayer() = default;

		virtual void OnAttach() override;
		virtual void OnDetach() override;

		virtual void OnUpdate(Timestep ts) override;
		virtual void OnImGuiRender() override;
		virtual void OnEvent(Event& e) override;
		void UpdateKeymapEvents(std::vector<Preferences::Keymap::KeyBindEvent> event);
		
	private:
		bool OnKeyPressed(KeyPressedEvent& e);
		bool OnMouseButtonPressed(MouseButtonPressedEvent& e);
		bool OnDropped(WindowDropEvent& e);
		bool OnDragEnter(WindowDragEnterEvent& e);
		bool OnDragLeave(WindowDragLeaveEvent& e);
		bool OnDragOver(WindowDragOverEvent& e);
		bool OnClosed(WindowCloseEvent& e);
		bool OnGamepadConnected(GamepadConnectedEvent& e);
		bool OnGamepadDisconnected(GamepadDisconnectedEvent& e);
		bool OnGamepadButtonPressed(GamepadButtonPressedEvent& e);
		bool OnGamepadButtonReleased(GamepadButtonReleasedEvent& e);
		bool OnGamepadAxisMoved(GamepadAxisMovedEvent& e);

		void OnOverlayRender();

		bool ViewportKeyAllowed() { return (m_ViewportHovered || m_ViewportFocused) && !m_ViewportActive; }

		void NewProject();
		void OpenProject(const std::filesystem::path& path);
		void SaveProject();

		void NewScene();
		void AppendScene();
		void AppendScene(const std::filesystem::path& path);
		void OpenScene();
		void OpenScene(const std::filesystem::path& path);
		bool SaveScene();
		bool SaveSceneAs();

		void SerializeScene(Ref<Scene> scene, const std::filesystem::path& path);

		void Compile();

		void OnScenePlay();
		void OnSceneSimulate();
		void OnSceneStop();
		void OnScenePause();

		void ShowEditorWindow();

		void SaveAndExit();
		void CloseProgramWindow();

		void SetRendererVisualizationMode(SceneRenderer::RendererVisualizationMode visualizationMode);
		void ToggleRendererVisualizationMode();

		void ReloadAvailableWorkspaces();

		void OnOpenFile(const std::filesystem::path& path);
		
	private:
		float m_DeltaTime;

		bool m_IsDragging = false;
		
		Ref<Framebuffer> m_Framebuffer;

		Ref<Scene> m_ActiveScene;
		Ref<Scene> m_EditorScene;
		std::filesystem::path m_EditorScenePath;

		std::vector<std::filesystem::path> m_AvailableWorkspaces;
		std::filesystem::path m_WorkspaceRenameContext;
		std::filesystem::path m_WorkspaceTarget;

		Entity m_HoveredEntity;

		EditorCamera m_EditorCamera;

		//Icons
		Ref<Texture2D> m_DymaticSplash = Texture2D::Create("Resources/Icons/Branding/DymaticSplash.bmp");
		Ref<Texture2D> m_DymaticLogo = Texture2D::Create("Resources/Icons/Branding/DymaticLogo.png");
		Ref<Texture2D> m_QuestionMarkIcon = Texture2D::Create("Resources/Icons/General/QuestionMark.png");
		Ref<Texture2D> m_ErrorIcon = Texture2D::Create("Resources/Icons/General/Error.png");

		Ref<Texture2D> m_EditIcon;
		Ref<Texture2D> m_LoadingCogAnimation[3];

		Ref<Texture2D> m_SaveIcon = Texture2D::Create("Resources/Icons/Toolbar/SaveIcon.png");
		Ref<Texture2D> m_SaveHoveredIcon = Texture2D::Create("Resources/Icons/Toolbar/SaveHoveredIcon.png");
		Ref<Texture2D> m_CompileIcon = Texture2D::Create("Resources/Icons/Toolbar/CompileIcon.png");
		Ref<Texture2D> m_CompileHoveredIcon = Texture2D::Create("Resources/Icons/Toolbar/CompileHoveredIcon.png");
		Ref<Texture2D> m_SourceControlIcon = Texture2D::Create("Resources/Icons/Toolbar/SourceControlIcon.png");
		Ref<Texture2D> m_IDEIcon = Texture2D::Create("Resources/Icons/Toolbar/IDEIcon.png");
		Ref<Texture2D> m_SettingsIcon = Texture2D::Create("Resources/Icons/Toolbar/SettingsIcon.png");

		bool m_ViewportFocused = false, m_ViewportHovered = false, m_ViewportActive = false;
		glm::vec2 m_ViewportSize = { 0.0f, 0.0f };
		glm::vec2 m_ViewportBounds[2];

		int m_GizmoOperation = -1;
		int m_GizmoMode = 0;
		enum class GizmoPivotPoint
		{
			MedianPoint = 0, IndividualOrigins = 1, ActiveElement = 2
		};
		GizmoPivotPoint m_GizmoPivotPoint = GizmoPivotPoint::MedianPoint;

		//Snap Enabled
		bool m_TranslationSnap = false;
		bool m_RotationSnap = false;
		bool m_ScaleSnap = false;
		//Snap Values
		float m_TranslationSnapValue = 0.5f;
		float m_RotationSnapValue = 45.0f;
		float m_ScaleSnapValue = 0.5f;

		int m_CameraSpeedScale = 4;
		float m_CameraBaseSpeed = 5.0f;

		float m_ProgramTime = 0;
		float m_LastSaveTime = 0;

		SceneRenderer::RendererVisualizationMode m_PreviousVisualizationMode = SceneRenderer::RendererVisualizationMode::Rendered;

		// Scene View Gizmo
		float m_YawUpdate;
		float m_PitchUpdate;
		bool m_UpdateAngles = false;

		bool m_ProjectionToggled = 0;

		bool m_ViewportCommandLineOpen = false;
		int m_ViewportCommandLineExecute = 0;
		std::string m_ViewportCommandLineBuffer;

		// Runtime
		enum class SceneState
		{
			Edit = 0, Play = 1, Simulate = 2
		};
		SceneState m_SceneState = SceneState::Edit;

		// Panels
		SceneHierarchyPanel m_SceneHierarchyPanel;
		ContentBrowserPanel m_ContentBrowserPanel;

		PreferencesPannel m_PreferencesPannel;
		NotificationsPannel m_NotificationsPanel;
		ScriptEditorPannel m_NodeEditorPannel;
		MaterialEditor m_MaterialEditor;
		TextEditorPannel m_TextEditor;
		CurveEditor m_CurveEditor;
		ImageEditor m_ImageEditor;
		MaterialEditorPanel m_MaterialEditorPanel;
		ProfilerPanel m_ProfilerPanel;
		ConsoleWindow m_ConsoleWindow;
		AssetManagerPanel m_AssetManagerPanel;
		PerformanceAnalyser m_PerformanceAnalyser;
		ProjectLauncher m_ProjectLauncher;
		SourceControlPanel m_SourceControlPanel;

		// Editor Resources
		Ref<Texture2D> m_IconPlay, m_IconSimulate, m_IconStop, m_IconPause, m_IconStep;
		Ref<Audio> m_SoundPlay, m_SoundSimulate, m_SoundStop, m_SoundPause, m_SoundStep, m_SoundCompileSuccess, m_SoundCompileFailure;

		bool m_ShowSplash;

		friend class EditorPythonInterface;
	};

}