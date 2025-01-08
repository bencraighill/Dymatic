#include "Panels/Assets/Utils/AssetPanelViewport.h"

#include "EditorResources.h"
#include "Dymatic/Core/Application.h"

#include "Dymatic/Renderer/Renderer.h"
#include "Dymatic/Renderer/SceneRenderer.h"
#include "Dymatic/Renderer/Renderer2D.h"

#include "Panels/UI.h"

namespace Dymatic {

	// TODO: Move to engine resources for static access.
	static const glm::vec3 s_DirectionalLightRotation = glm::vec3(0.0f, 45.0f, -135.0f);

	AssetPanelViewport::AssetPanelViewport()
	{
		m_SceneRendererContext = SceneRendererContext::Create(m_ViewportSize);
		m_Camera = EditorCamera();
	}

	void AssetPanelViewport::BeginViewportRender(Timestep ts)
	{
		// Check if we need to resize
		if ((m_ViewportSize.x != m_SceneRendererContext->ActiveFramebuffer->GetSpecification().Width || m_ViewportSize.y != m_SceneRendererContext->ActiveFramebuffer->GetSpecification().Height)
			&& (m_ViewportSize.x != 0.0f && m_ViewportSize.y != 0.0f))
		{
			m_SceneRendererContext->Resize(m_ViewportSize);
			m_Camera.SetViewportSize(m_ViewportSize.x, m_ViewportSize.y);
		}

		// Update the camera
		m_Camera.SetBlockEvents(!m_ViewportHovered && !m_LockMouse);
		m_Camera.OnUpdate(ts);

		// Render
		m_SceneRendererContext->ActiveFramebuffer->Bind();
		ClearViewport({ 0.0f, 0.0f, 0.0f, 1.0f });
		int defaultID = -1;
		m_SceneRendererContext->ActiveFramebuffer->ClearAttachment(1, &defaultID);
		SceneRenderer::SetActiveContext(m_SceneRendererContext);
		SceneRenderer::UpdateTimestep(ts);
		SceneRenderer::BeginScene();
		SceneRenderer::SubmitCamera(m_Camera);

		DirectionalLightComponent dlc;
		dlc.Intensity = 2.0f;
		SceneRenderer::SubmitDirectionalLight(s_DirectionalLightRotation, dlc);

		SkyLightComponent slc;
		slc.Intensity = 0.25f;
		slc.EnvironmentMap = EditorResources::DefaultEnvironmentMap;
		slc.FlowMap = EditorResources::DefaultSkyFlowMap;
		SceneRenderer::SubmitSkyLight(slc);

		// Client asset panel will now call their own draw commands
	}

	void AssetPanelViewport::SubmitViewportRender()
	{
		// End render commands
		SceneRenderer::RenderScene();
		SceneRenderer::EndScene();
	}

	void AssetPanelViewport::EndViewportRender()
	{
		// Update hovered context
		m_PreviousHoveredID = m_HoveredID;

		glm::vec2 mouse = GetHoveredPixel();
		if (mouse.x >= 0 && mouse.y >= 0 && mouse.x < m_ViewportSize.x && mouse.y < m_ViewportSize.y)
			m_SceneRendererContext->ActiveFramebuffer->ReadPixel(1, mouse.x, mouse.y, &m_HoveredID);
		else
			m_HoveredID = -1;

		// Unbind before returning. This is important!
		m_SceneRendererContext->ActiveFramebuffer->Unbind();
	}

	void AssetPanelViewport::OnPreImGuiRender()
	{
		// Lock and unlock the mouse
		if (m_ViewportHovered && !m_LockMouse && (ImGui::IsMouseClicked(ImGuiMouseButton_Right) || ImGui::IsMouseClicked(ImGuiMouseButton_Left) && (ImGui::IsKeyDown(ImGuiKey_LeftAlt) || !m_Camera.GetOrbitRequireAlt())))
		{
			m_LockMouse = true;
			Utils::SetWindowCursorLocked(ImGui::GetWindowViewport()->PlatformHandle, true);
			ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouse;
		}

		// Check if the mouse button should be unlocked
		if (m_LockMouse && (!ImGui::IsMouseDown(ImGuiMouseButton_Right) && !ImGui::IsMouseDown(ImGuiMouseButton_Left)))
		{
			m_LockMouse = false;
			Utils::SetWindowCursorLocked(ImGui::GetWindowViewport()->PlatformHandle, false);
			ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
		}
	}

	void AssetPanelViewport::OnImGuiRender()
	{
		m_ViewportSize = ImGui::GetContentRegionAvail();
		m_SceneRendererContext->ActiveFramebuffer->Bind();
		ImGui::Image((ImTextureID)m_SceneRendererContext->ActiveFramebuffer->GetColorAttachmentRendererID(0), m_ViewportSize, { 0, 1 }, { 1, 0 });
		m_SceneRendererContext->ActiveFramebuffer->Unbind();

		m_ViewportPosition = ImGui::GetItemRectMin();
		m_ViewportHovered = ImGui::IsWindowHovered();

		if (ImGui::IsItemClicked())
			m_SelectedID = m_HoveredID;

		const ImVec2 viewportMin = ImGui::GetItemRectMin();
		const ImVec2 viewportMax = ImGui::GetItemRectMax();

		// TODO: Use editor UI.h styling variables to make this always clearly consistent with EditorLayer
		ImGui::SetCursorScreenPos(viewportMin + ImVec2(5.0f, 5.0f));
		UI::DrawViewportRenderSettingsButton(m_SceneRendererContext->VisualizationMode);

		ImGui::SetCursorScreenPos(ImVec2(viewportMax.x - 50.0f, viewportMin.y) + ImVec2(-5.0f, 5.0f));
		UI::DrawCameraSpeedButton(m_Camera, m_CameraBaseSpeed, m_CameraSpeedScale);
	}

	void AssetPanelViewport::OnEvent(Event& e)
	{
		m_Camera.OnEvent(e);
	}

	void AssetPanelViewport::ClearViewport(const glm::vec4& color)
	{
		m_SceneRendererContext->ActiveFramebuffer->Bind();
		RenderCommand::SetClearColor(color);
		RenderCommand::Clear();
	}

	glm::vec2 AssetPanelViewport::GetHoveredPixel()
	{
		glm::vec2 mouse = ImGui::GetMousePos() - m_ViewportPosition;
		mouse.y = m_ViewportSize.y - mouse.y;

		return mouse;
	}

	glm::vec2 AssetPanelViewport::GetHoveredUV()
	{
		return GetHoveredPixel() / m_ViewportSize;
	}

}