#pragma once

#include "Dymatic.h"
#include "Panels/SceneHierarchyPanel.h"
#include "Panels/ContentBrowserPanel.h"

#include "Panels/PopupsAndNotifications.h"
#include "Panels/PreferencesPanel.h"
#include "Panels/Nodes/ScriptEditor.h"
#include "Panels/TextEditor.h"
#include "Panels/CurveEditor.h"
#include "Panels/ImageEditor.h"
#include "Panels/ProfilerPanel.h"
#include "Panels/LogPanel.h"
#include "Panels/AssetManagerPanel.h"
#include "Panels/PerformanceAnalyser.h"
#include "Panels/ProjectBrowser.h"
#include "Panels/SourceControl.h"
#include "Panels/ProjectSettingsPanel.h"

#include "Panels/EditorPanel.h"

#include "Tools/PythonTools.h"
#include "Dymatic/Renderer/EditorCamera.h"

#include "Dymatic/Renderer/SceneRendererContext.h"

#include "Dymatic/Video/VideoReader.h"

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

		inline Ref<Scene> GetActiveScene() const { return m_ActiveScene; }
		
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

		glm::vec2 GetViewportSize() { return m_ViewportBounds[1] - m_ViewportBounds[0]; }
		glm::vec2 GetMouseViewportPosition();
		glm::vec3 GetHoveredWorldPositionBounded(const float bound);
		glm::vec3 GetHoveredWorldPositionUnbounded();
		glm::vec3 GetHoveredWorldPosition();
		glm::vec2 WorldToViewportPosition(const glm::vec3& worldPosition);

		void OnOverlayRender();

		bool ViewportKeyAllowed() { return (m_ViewportHovered || m_ViewportFocused) && !m_ViewportActive; }

		void NewProject();
		void OpenProject(const std::filesystem::path& path);
		void SaveProject();

		void NewScene();
		void AppendScene();
		void AppendScene(const std::filesystem::path& path);
		bool OpenScene();
		bool OpenScene(const std::filesystem::path& path);
		bool SaveScene();
		bool SaveSceneAs();

		void Undo();
		void Redo();

		void SerializeScene(Ref<Scene> scene, const std::filesystem::path& path);

		void Compile();

		void OnScenePlay();
		void OnSceneSimulate();
		void OnSceneStop();
		void OnScenePause();

		void OnFocus();

		void ShowEditorWindow();

		void SaveAndExit();
		void CloseProgramWindow();

		void SetRendererVisualizationMode(SceneRendererContext::RendererVisualizationMode visualizationMode);
		void ToggleRendererVisualizationMode();

		void ReloadAvailableWorkspaces();

		void OnOpenFile(const std::filesystem::path& path);
		bool TryOpenAssetEditorPanel(const std::filesystem::path& path);
		
	private:
		float m_DeltaTime;

		bool m_IsDragging = false;
		
		Ref<SceneRendererContext> m_SceneRendererContext;

		bool m_PreviewCamera;
		Ref<SceneRendererContext> m_PreviewSceneRendererContext;

		Ref<Scene> m_ActiveScene;
		Ref<Scene> m_EditorScene;
		std::filesystem::path m_EditorScenePath;

		std::vector<UUID> m_PostUpdateQueue;

		std::vector<std::filesystem::path> m_AvailableWorkspaces;
		std::filesystem::path m_WorkspaceRenameContext;
		std::filesystem::path m_WorkspaceTarget;

		std::filesystem::path m_VideoCaptureOutputPath;
		Ref<VideoWriter> m_VideoWriter;

		uint32_t m_ViewportBookmarkRenameIndex = -1;

		struct DebugMessage
		{
			std::string Text;
			int Level;
		};
		std::vector<DebugMessage> m_DebugMessages;

		int m_RulerMode = 0;
		struct RulerLine
		{
			glm::vec3 Start;
			glm::vec3 End;
		};
		std::vector<RulerLine> m_RulerLines;

		Entity m_HoveredEntity;
		EditorCamera m_EditorCamera;
		Ref<Font> m_EditorFont;

		//Icons
		Ref<Texture2D> m_EditIcon;
		Ref<Texture2D> m_LoadingCogAnimation[3];

		bool m_ViewportFocused = false, m_ViewportHovered = false, m_ViewportActive = false;
		glm::vec2 m_ViewportSize = { 0.0f, 0.0f };
		glm::vec2 m_ViewportBounds[2];
		bool m_LockMouse = false;

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

		SceneRendererContext::RendererVisualizationMode m_PreviousVisualizationMode = SceneRendererContext::RendererVisualizationMode::Rendered;

		// Scene View Gizmo
		float m_YawUpdate;
		float m_PitchUpdate;
		bool m_UpdateAngles = false;

		bool m_ProjectionToggled = 0;

		// Viewport Command Line
		bool m_ViewportCommandLineOpen = false;
		int m_ViewportCommandLineExecute = 0;
		int m_ViewportCommandLineBufferIndex = 0;
		// (Can be modified by the user, resets each time the command is executed)
		std::vector<std::string> m_ViewportCommandLineBuffers;
		// History of all commands executed 
		std::vector<std::string> m_ViewportCommandLineHistory;

		// Runtime
		enum class SceneState
		{
			Edit = 0, Play = 1, Simulate = 2
		};
		SceneState m_SceneState = SceneState::Edit;

		// Panels
		SceneHierarchyPanel m_SceneHierarchyPanel;
		ContentBrowserPanel m_ContentBrowserPanel;
		PreferencesPanel m_PreferencesPannel;
		ProjectSettingsPanel m_ProjectSettingsPanel;
		NotificationsPanel m_NotificationsPanel;
		ScriptEditorPanel m_NodeEditorPannel;
		TextEditorPanel m_TextEditor;
		CurveEditor m_CurveEditor;
		ImageEditor m_ImageEditor;
		ProfilerPanel m_ProfilerPanel;
		LogPanel m_LogPanel;
		AssetManagerPanel m_AssetManagerPanel;
		PerformanceAnalyser m_PerformanceAnalyser;
		ProjectLauncher m_ProjectLauncher;
		SourceControlPanel m_SourceControlPanel;

		std::unordered_map<AssetHandle, Ref<EditorPanel>> m_AssetEditorPanels;

		bool m_ShowSplash;

		friend class EditorPythonInterface;
	};

}