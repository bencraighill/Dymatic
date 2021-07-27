#pragma once

#include "Dymatic.h"
#include "Panels/SceneHierarchyPanel.h"
#include "Panels/PopupsAndNotifications.h"
#include "Panels/ContentBrowser.h"
#include "Panels/PreferencesPanel.h"
#include "Panels/NodeEditor/NodeProgram.h"
#include "Panels/TextEditor.h"
#include "Panels/FilePrompt.h"
#include "Panels/CurveEditor.h"
#include "Panels/ImageEditor.h"
#include "Panels/MemoryEditor.h"
#include "Panels/ConsoleWindow.h"
#include "Panels/PerformanceAnalyser.h"

#include "Panels/ImageEditorNew.h"

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
		void UpdateKeys(BindCatagory bindCatagory);
	private:
		bool OnKeyPressed(KeyPressedEvent& e);
		bool OnMouseButtonPressed(MouseButtonPressedEvent& e);

		bool ViewportKeyAllowed() { return m_ViewportHovered || m_ViewportFocused; }

		void NewScene();

		void AppendScene();
		void AppendSceneByFilepath(const std::string filepath);

		void OpenScene();
		void OpenSceneByFilepath(const std::string filepath);

		void SaveSceneToFilepath(const std::string filepath);
		bool SaveScene();
		bool SaveSceneAs();

		void SaveAndExit();
		void CloseProgramWindow();

		void AddRecentFile(std::string filepath);
		std::vector<std::string> GetRecentFiles();

		void SetShadingIndex(int index);

		bool KeyBindCheck(KeyBindEvent bindEvent, BindCatagory bindCatagory);
		std::string GetBindAsString(KeyBindEvent bindEvent);

		std::string GetCurrentFileName();

	public:
		bool m_ViewportVisible = true;
		bool m_ToolbarVisible = true;
		bool m_InfoVisible = true;
		bool m_StatsVisible = false;
		bool m_MemoryEditorVisible = false;
		bool m_NodeEditorVisible = false;
	private:
		Dymatic::OrthographicCameraController m_CameraController;

		float m_DeltaTime;

		KeyPressedEvent& m_KeyPressed = KeyPressedEvent(Key::A, 0);
		MouseButtonPressedEvent& m_MouseButtonPressed = MouseButtonPressedEvent(Mouse::Button0);

		// Temp
		Ref<VertexArray> m_SquareVA;
		Ref<Shader> m_FlatColorShader;
		Ref<Framebuffer> m_Framebuffer;

		Ref<Scene> m_ActiveScene;
		Entity m_SquareEntity;
		Entity m_CameraEntity;
		Entity m_SecondCamera;

		Entity m_HoveredEntity;
		//TODO: REMOVE
		int m_HoveredPixelID;
		//--------//

		bool m_PrimaryCamera = true;

		EditorCamera m_EditorCamera;

		//Textures
		Ref<Texture2D> m_CheckerboardTexture;

		//Icons

		Ref<Texture2D> m_DymaticLogo = Texture2D::Create("assets/icons/DymaticLogoTransparent.png");
		Ref<Texture2D> m_DymaticSplash = Texture2D::Create("assets/icons/DymaticSplash.bmp");

		//Shader Icons
		Ref<Texture2D> m_IconShaderWireframe = Texture2D::Create("assets/icons/Viewport/ShaderIconWireframe.png");
		Ref<Texture2D> m_IconShaderUnlit = Texture2D::Create("assets/icons/Viewport/ShaderIconUnlit.png");
		Ref<Texture2D> m_IconShaderSolid = Texture2D::Create("assets/icons/Viewport/ShaderIconSolid.png");
		Ref<Texture2D> m_IconShaderRendered = Texture2D::Create("assets/icons/Viewport/ShaderIconRendered.png");

		bool m_ViewportFocused = false, m_ViewportHovered = false;
		glm::vec2 m_ViewportSize = { 0.0f, 0.0f };
		glm::vec2 m_ViewportBounds[2];

		glm::vec4 m_SquareColor = { 0.2f, 0.3f, 0.8f, 1.0f };

		int m_GizmoType = -1;
		int m_GizmoSpace = 0;
		//Snap Enabled
		bool m_TranslationSnap = false;
		bool m_RotationSnap = false;
		bool m_ScaleSnap = false;
		//Snap Values
		float m_TranslationSnapValue = 0.5f;
		float m_RotationSnapValue = 45.0f;
		float m_ScaleSnapValue = 0.5f;

		std::string m_CurrentFilepath = "";
		float m_ProgramTime = 0;
		float m_LastSaveTime = 0;

		int m_ShadingIndex = 3;
		int m_PreviousToggleIndex = 3;

		// Scene View Gizmo
		float m_YawUpdate;
		float m_PitchUpdate;
		bool m_UpdateAngles = false;

		bool m_ProjectionToggled = 0;
		glm::mat4 m_PreviousCameraProjection;

		std::string m_LayoutToLoad = "";

		// Panels
		PreferencesPannel m_PreferencesPannel;
		SceneHierarchyPanel m_SceneHierarchyPanel;
		PopupsAndNotifications m_PopupsAndNotifications = PopupsAndNotifications(&m_PreferencesPannel.GetPreferences());
		ContentBrowser m_ContentBrowser = ContentBrowser(&m_PreferencesPannel.GetPreferences(), &m_TextEditor);
		NodeEditorPannel m_NodeEditorPannel;
		TextEditorPannel m_TextEditor;
		CurveEditor m_CurveEditor;
		ImageEditor m_ImageEditor;
		MemoryEditor m_MemoryEditor;
		ConsoleWindow m_ConsoleWindow;
		FilePrompt m_FilePrompt = FilePrompt(&m_PreferencesPannel.GetPreferences());
		PerformanceAnalyser m_PerformanceAnalyser;

		ImageEditorNew m_ImageEditorNew;

		//Sandbox::AgentSimulation m_AgentSimulation;
		//Sandbox::MandelbrotSet m_MandelbrotSet;
		//Sandbox::SandSimulation m_SandSimulation;


		bool m_ShowSplash = m_PreferencesPannel.GetPreferences().m_PreferenceData.showSplash;
	};

}