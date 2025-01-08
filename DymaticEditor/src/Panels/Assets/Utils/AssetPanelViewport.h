#pragma once

#include "Dymatic/Renderer/Framebuffer.h"
#include "Dymatic/Renderer/EditorCamera.h"
#include "Dymatic/Renderer/SceneRendererContext.h"

namespace Dymatic {

	class AssetPanelViewport
	{
	public:
		AssetPanelViewport();

		void BeginViewportRender(Timestep ts);
		void SubmitViewportRender();
		void EndViewportRender();

		void OnPreImGuiRender();
		void OnImGuiRender();

		void OnEvent(Event& e);

		void ClearViewport(const glm::vec4& color);
		glm::vec2 GetHoveredPixel();
		glm::vec2 GetHoveredUV();

		inline const EditorCamera& GetEditorCamera() const { return m_Camera; }
		inline EditorCamera& GetEditorCamera() { return m_Camera; }

		inline bool IsHovered() const { return m_ViewportHovered; }
		inline int GetHoveredID() const { return m_HoveredID; }
		inline int GetPreviousHoveredID() const { return m_PreviousHoveredID; }

		inline void SetSelectedID(int selectedID) { m_SelectedID = selectedID; }
		inline int GetSelectedID() const { return m_SelectedID; }

	private:
		// Camera
		EditorCamera m_Camera;
		int m_CameraSpeedScale = 4;
		float m_CameraBaseSpeed = 5.0f;
		bool m_ViewportHovered = false;
		bool m_LockMouse = false;

		// Renderer resources
		Ref<SceneRendererContext> m_SceneRendererContext;
		glm::vec2 m_ViewportSize = glm::vec2(512.0f, 512.0f);

		// Selection Context
		glm::vec2 m_ViewportPosition = glm::vec2(0.0f);
		int m_PreviousHoveredID = -1;
		int m_HoveredID = -1;
		int m_SelectedID = -1;
	};

}