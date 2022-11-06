#pragma once

#include "Dymatic.h"
#include "Panels/SceneHierarchyPanel.h"
#include "Panels/ContentBrowserPanel.h"

#include "Panels/PopupsAndNotifications.h"
#include "Panels/PreferencesPanel.h"
#include "Panels/NodeEditor/NodeEditor.h"
#include "Panels/TextEditor.h"
#include "Panels/CurveEditor.h"
#include "Panels/ImageEditor.h"
#include "Panels/MemoryEditor.h"
#include "Panels/ConsoleWindow.h"
#include "Panels/PerformanceAnalyser.h"


#include "Panels/SandboxArea.h"

#include "Dymatic/Renderer/EditorCamera.h"

namespace Dymatic {

	class EditorLayer : public Layer
	{
	public:
		EditorLayer();
		virtual ~EditorLayer() = default;

		virtual void OnAttach() override;
		virtual void OnDetach() override;

		void OnUpdate(Timestep ts) override;
		virtual void OnImGuiRender() override;
		void OnEvent(Event& e) override;
		void UpdateKeymapEvents(std::vector<Preferences::Keymap::KeyBindEvent> event);
	private:
		bool OnKeyPressed(KeyPressedEvent& e);
		bool OnMouseButtonPressed(MouseButtonPressedEvent& e);
		bool OnDropped(WindowDropEvent& e);
		bool OnDragEnter(WindowDragEnterEvent& e);
		bool OnDragLeave(WindowDragLeaveEvent& e);
		bool OnDragOver(WindowDragOverEvent& e);
		bool OnClosed(WindowCloseEvent& e);

		void OnOverlayRender();

		inline bool ViewportKeyAllowed() { return m_ViewportHovered || m_ViewportFocused; }

		void NewScene();
		void AppendScene();
		void AppendScene(const std::filesystem::path& path);
		void OpenScene();
		void OpenScene(const std::filesystem::path& path);
		bool SaveScene();
		bool SaveSceneAs();

		void SerializeScene(Ref<Scene> scene, const std::filesystem::path& path);

		void OnScenePlay();
		void OnSceneSimulate();
		void OnSceneStop();

		void OnDuplicateEntity();

		void SaveAndExit();
		void CloseProgramWindow();

		// UI Panels
		void UI_Toolbar();

		void AddRecentFile(const std::filesystem::path& path);
		std::vector<std::filesystem::path> GetRecentFiles();

		void SetShadingIndex(int index);

		std::string GetCurrentFileName();

		void OpenWindowLayout(const std::filesystem::path& path);
		void SaveWindowLayout(const std::filesystem::path& path);
	public:
		bool m_ViewportVisible = true;
		bool m_ToolbarVisible = true;
		bool m_InfoVisible = true;
		bool m_StatsVisible = false;
		bool m_MemoryEditorVisible = false;
	private:
		Dymatic::OrthographicCameraController m_CameraController;

		float m_DeltaTime;

		bool m_IsDragging = false;

		// Temp
		Ref<VertexArray> m_SquareVA;
		Ref<Shader> m_FlatColorShader;
		Ref<Framebuffer> m_Framebuffer;

		Ref<Scene> m_ActiveScene;
		Ref<Scene> m_EditorScene;
		std::filesystem::path m_EditorScenePath;
		Entity m_SquareEntity;
		Entity m_CameraEntity;
		Entity m_SecondCamera;

		Entity m_HoveredEntity;


		EditorCamera m_EditorCamera;

		//Textures
		//Ref<Texture2D> m_CheckerboardTexture;

		//Icons

		//Ref<Texture2D> m_DymaticLogo = Texture2D::Create("assets/icons/DymaticLogoTransparent.png");
		Ref<Texture2D> m_DymaticSplash = Texture2D::Create("assets/icons/DymaticSplash.png");
		Ref<Texture2D> m_QuestionMarkIcon = Texture2D::Create("assets/icons/General/QuestionMark.png");
		Ref<Texture2D> m_ErrorIcon = Texture2D::Create("assets/icons/General/Error.png");

		Ref<Texture2D> m_EditIcon;
		Ref<Texture2D> m_LoadingCogAnimation[3];

		//Shader Icons
		//Ref<Texture2D> m_IconShaderWireframe = Texture2D::Create("assets/icons/Viewport/ShaderIconWireframe.png");
		//Ref<Texture2D> m_IconShaderUnlit = Texture2D::Create("assets/icons/Viewport/ShaderIconUnlit.png");
		//Ref<Texture2D> m_IconShaderSolid = Texture2D::Create("assets/icons/Viewport/ShaderIconSolid.png");
		//Ref<Texture2D> m_IconShaderRendered = Texture2D::Create("assets/icons/Viewport/ShaderIconRendered.png");

		bool m_ViewportFocused = false, m_ViewportHovered = false;
		glm::vec2 m_ViewportSize = { 0.0f, 0.0f };
		glm::vec2 m_ViewportBounds[2];

		glm::vec4 m_SquareColor = { 0.2f, 0.3f, 0.8f, 1.0f };

		int m_GizmoType = -1;
		int m_GizmoSpace = 0;

		bool m_ShowPhysicsColliders = false;

		//Snap Enabled
		bool m_TranslationSnap = false;
		bool m_RotationSnap = false;
		bool m_ScaleSnap = false;
		//Snap Values
		float m_TranslationSnapValue = 0.5f;
		float m_RotationSnapValue = 45.0f;
		float m_ScaleSnapValue = 0.5f;

		float m_ProgramTime = 0;
		float m_LastSaveTime = 0;

		int m_ShadingIndex = 3;
		int m_PreviousToggleIndex = 3;

		// Scene View Gizmo
		float m_YawUpdate;
		float m_PitchUpdate;
		bool m_UpdateAngles = false;

		bool m_ProjectionToggled = 0;

		// Runtime
		enum class SceneState
		{
			Edit = 0, Play = 1, Simulate = 2
		};
		SceneState m_SceneState = SceneState::Edit;
		bool m_ScenePaused = false;
		bool m_NextFrame = false;

		// Panels
		SceneHierarchyPanel m_SceneHierarchyPanel;
		ContentBrowserPanel m_ContentBrowserPanel;

		PreferencesPannel m_PreferencesPannel;
		NotificationsPannel m_NotificationsPannel;
		NodeEditorPannel m_NodeEditorPannel;
		TextEditorPannel m_TextEditor;
		CurveEditor m_CurveEditor;
		ImageEditor m_ImageEditor;
		MemoryEditor m_MemoryEditor;
		ConsoleWindow m_ConsoleWindow;
		PerformanceAnalyser m_PerformanceAnalyser;

		//Sandbox::AgentSimulation m_AgentSimulation;
		//Sandbox::MandelbrotSet m_MandelbrotSet;
		//Sandbox::SandSimulation m_SandSimulation;
		//Sandbox::RopeSimulation m_RopeSimulation;
		//Sandbox::ChessAI m_ChessAI;
		//Sandbox::GPUSIM::GPUSimulation gpusim;

		// Editor Resources
		Ref<Texture2D> m_IconPlay, m_IconSimulate, m_IconStop, m_IconPause, m_IconNextFrame;

		bool m_ShowSplash;
	};

}