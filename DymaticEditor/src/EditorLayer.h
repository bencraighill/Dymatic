#pragma once

#include "Dymatic.h"
#include "Panels/SceneHierarchyPanel.h"
#include "Panels/PopupsAndNotifications.h"
#include "Panels/ContentBrowser.h"
#include "Panels/PreferencesPanel.h"
#include "Panels/NodeEditor.h"
#include "Panels/NodeEditor/NodeProgram.h"


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

		void NewScene();
		void OpenScene();

		void OpenSceneByFilepath(const std::string& filepath);
		void SaveSceneToFilepath(const std::string& filepath);

		bool SaveScene();
		bool SaveSceneAs();

		void SaveAndExit();
		void CloseProgramWindow();

		void AddRecentFile(std::string filepath);
		std::vector<std::string> GetRecentFiles();

		bool ImageMenuItem(Ref<Texture2D> texture, std::string item, std::string shortcut, bool menu = false, ImVec2 imageSize = ImVec2{ 15, 15 });
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

		bool m_PrimaryCamera = true;

		EditorCamera m_EditorCamera;

		//Textures
		Ref<Texture2D> m_CheckerboardTexture;

		//Icons

		Ref<Texture2D> m_DymaticLogo = Texture2D::Create("assets/icons/DymaticLogoTransparent.png");
		Ref<Texture2D> m_DymaticSplash = Texture2D::Create("assets/icons/DymaticSplash.bmp");

		//Gizmos
		Ref<Texture2D> m_IconTranslation = Texture2D::Create("assets/icons/ViewportTools/ControlTranslation.png");
		Ref<Texture2D> m_IconTranslationActive = Texture2D::Create("assets/icons/ViewportTools/ControlTranslationActive.png");
		Ref<Texture2D> m_IconRotation = Texture2D::Create("assets/icons/ViewportTools/ControlRotation.png");
		Ref<Texture2D> m_IconRotationActive = Texture2D::Create("assets/icons/ViewportTools/ControlRotationActive.png");
		Ref<Texture2D> m_IconScale = Texture2D::Create("assets/icons/ViewportTools/ControlScale.png");
		Ref<Texture2D> m_IconScaleActive = Texture2D::Create("assets/icons/ViewportTools/ControlScaleActive.png");
		//Snaping
		Ref<Texture2D> m_IconTranslationSnap = Texture2D::Create("assets/icons/ViewportTools/ControlSnapTranslation.png");
		Ref<Texture2D> m_IconTranslationSnapActive = Texture2D::Create("assets/icons/ViewportTools/ControlSnapTranslationActive.png");
		Ref<Texture2D> m_IconRotationSnap = Texture2D::Create("assets/icons/ViewportTools/ControlSnapRotation.png");
		Ref<Texture2D> m_IconRotationSnapActive = Texture2D::Create("assets/icons/ViewportTools/ControlSnapRotationActive.png");
		Ref<Texture2D> m_IconScaleSnap = Texture2D::Create("assets/icons/ViewportTools/ControlSnapScale.png");
		Ref<Texture2D> m_IconScaleSnapActive = Texture2D::Create("assets/icons/ViewportTools/ControlSnapScaleActive.png");
		Ref<Texture2D> m_IconSnapDropdown = Texture2D::Create("assets/icons/ViewportTools/ControlSnapDropdown.png");
		//Space
		Ref<Texture2D> m_IconSpaceLocal = Texture2D::Create("assets/icons/ViewportTools/ControlSpaceLocal.png");
		Ref<Texture2D> m_IconSpaceWorld = Texture2D::Create("assets/icons/ViewportTools/ControlSpaceWorld.png");
		//System Icons
		Ref<Texture2D> m_IconSystemNew = Texture2D::Create("assets/icons/SystemIcons/NewIcon.png");
		Ref<Texture2D> m_IconSystemOpen = Texture2D::Create("assets/icons/SystemIcons/OpenIcon.png");
		Ref<Texture2D> m_IconSystemRecent = Texture2D::Create("assets/icons/SystemIcons/RecentIcon.png");
		Ref<Texture2D> m_IconSystemSave = Texture2D::Create("assets/icons/SystemIcons/SaveIcon.png");
		Ref<Texture2D> m_IconSystemSaveAs = Texture2D::Create("assets/icons/SystemIcons/SaveAsIcon.png");
		Ref<Texture2D> m_IconSystemImport = Texture2D::Create("assets/icons/SystemIcons/ImportIcon.png");
		Ref<Texture2D> m_IconSystemExport = Texture2D::Create("assets/icons/SystemIcons/ExportIcon.png");
		Ref<Texture2D> m_IconSystemQuit = Texture2D::Create("assets/icons/SystemIcons/QuitIcon.png");
		Ref<Texture2D> m_IconSystemPreferences = Texture2D::Create("assets/icons/SystemIcons/SettingsIcon.png");
		Ref<Texture2D> m_IconSystemConsoleWindow = Texture2D::Create("assets/icons/SystemIcons/ConsoleWindowIcon.png");

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

		// Panels
		PreferencesPannel m_PreferencesPannel;
		SceneHierarchyPanel m_SceneHierarchyPanel;
		PopupsAndNotifications m_PopupsAndNotifications = PopupsAndNotifications(&m_PreferencesPannel.GetPreferences());
		ContentBrowser m_ContentBrowser = ContentBrowser(&m_PreferencesPannel.GetPreferences());
		NodeEditorPannel m_NodeEditorPannel;
		NodeEditor m_NodeEditor;

		bool m_ShowSplash = m_PreferencesPannel.GetPreferences().m_PreferenceData.showSplash;
	};

}