#include "Sandbox2D.h"
#include "imgui/imgui.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Platform/OpenGL/OpenGLShader.h"

Sandbox2D::Sandbox2D()
	: Layer("Sandbox2D"), m_CameraController(1280.0f / 720.0f)
{

}

void Sandbox2D::OnAttach()
{
	DY_PROFILE_FUNCTION();

	m_CheckerboardTexture = Dymatic::Texture2D::Create("assets/textures/Checkerboard.png");

	Dymatic::FramebufferSpecification fbSpec;
	fbSpec.Width = 1280;
	fbSpec.Height = 720;
	m_Framebuffer = Dymatic::Framebuffer::Create(fbSpec);
}

void Sandbox2D::OnDetach()
{
	DY_PROFILE_FUNCTION();
}

void Sandbox2D::OnUpdate(Dymatic::Timestep ts)
{
	DY_PROFILE_FUNCTION();

	//Update
	m_CameraController.OnUpdate(ts);


	//Render
	Dymatic::Renderer2D::ResetStats();
	{
		DY_PROFILE_SCOPE("Renderer Prep");
		m_Framebuffer->Bind();
		Dymatic::RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.1f, 1.0f });
		Dymatic::RenderCommand::Clear();
	}


	{
		static float rotation = 0.0f;
		rotation += ts * 50.0f;

		DY_PROFILE_SCOPE("Renderer Draw");
		Dymatic::Renderer2D::BeginScene(m_CameraController.GetCamera());
		Dymatic::Renderer2D::DrawRotatedQuad({ 1.0f, 0.0f }, { 0.8f, 0.8f }, glm::radians(-45.0f), { 0.8f, 0.2f, 0.3f, 1.0f });
		Dymatic::Renderer2D::DrawQuad({ -1.0f, 0.0f }, { 0.8f, 0.8f }, { 0.8f, 0.2f, 0.3f, 1.0f });
		Dymatic::Renderer2D::DrawQuad({ 0.5f, -0.5f }, { 0.5f, 0.75f }, { 0.2f, 0.3f,  0.8f, 1.0f });
		Dymatic::Renderer2D::DrawQuad({ 0.0f, 0.0f, -0.1f }, { 20.0f, 20.0f }, m_CheckerboardTexture, 10.0f, glm::vec4(1.0f, 0.9f, 0.9, 1.0f));
		Dymatic::Renderer2D::DrawRotatedQuad({ -2.0f, 0.0f, 0.0f }, { 1.0f, 1.0f }, glm::radians(rotation), m_CheckerboardTexture, 10.0f, glm::vec4(1.0f, 0.9f, 0.9, 1.0f));
		Dymatic::Renderer2D::EndScene();

		Dymatic::Renderer2D::BeginScene(m_CameraController.GetCamera());
		for (float y = -5.0f; y < 5.0f; y += 0.5f)
		{
			for (float x = -5.0f; x < 5.0f; x += 0.5f)
			{
				glm::vec4 color = { (x + 5.0f) / 10.0f, 0.4, (y + 5.0f) / 10.0f, 0.7f };
				Dymatic::Renderer2D::DrawQuad({ x, y }, { 0.45f, 0.45f }, color);
			}
		}
		Dymatic::Renderer2D::EndScene();
		m_Framebuffer->Unbind();
	}
}

void Sandbox2D::OnImGuiRender()
{
	DY_PROFILE_FUNCTION();

	static bool dockspaceOpen = true;
	static bool opt_fullscreen = true;
	static bool opt_padding = false;
	static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

	// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
	// because it would be confusing to have two docking targets within each others.
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
	if (opt_fullscreen)
	{
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->GetWorkPos());
		ImGui::SetNextWindowSize(viewport->GetWorkSize());
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
	}
	else
	{
		dockspace_flags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
	}

	// When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
	// and handle the pass-thru hole, so we ask Begin() to not render a background.
	if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
		window_flags |= ImGuiWindowFlags_NoBackground;

	// Important: note that we proceed even if Begin() returns false (aka window is collapsed).
	// This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
	// all active windows docked into it will lose their parent and become undocked.
	// We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
	// any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
	if (!opt_padding)
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("DockSpace Demo", &dockspaceOpen, window_flags);
	if (!opt_padding)
		ImGui::PopStyleVar();

	if (opt_fullscreen)
		ImGui::PopStyleVar(2);

	// DockSpace
	ImGuiIO& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
	{
		ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
	}

	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{

			if (ImGui::MenuItem("Exit")) Dymatic::Application::Get().Close();
			ImGui::EndMenu();
		}

		ImGui::EndMenuBar();
	}

	//Window Code

	ImGui::Begin("Settings");

	auto stats = Dymatic::Renderer2D::GetStats();
	ImGui::Text("Renderer2D Stats:");
	ImGui::Text("Draw Calls: %d", stats.DrawCalls);
	ImGui::Text("Quads: %d", stats.QuadCount);
	ImGui::Text("Vertices: %d", stats.GetTotalVertexCount());
	ImGui::Text("Indices: %d", stats.GetTotalIndexCount());

	ImGui::ColorEdit4("Square Color", glm::value_ptr(m_SquareColor));

	static const char* items[]{ "Default", "Dark", "Light" };
	static int selectedItem = 0;

	ImGui::Combo("Editor Color Scheme", &selectedItem, items, IM_ARRAYSIZE(items));

	uint32_t textureID = m_Framebuffer->GetColorAttachmentRendererID();
	ImGui::Image((void*)textureID, { 1280, 720 });
	ImGui::End();


	//Final End for the dockspace
	ImGui::End();
}

void Sandbox2D::OnEvent(Dymatic::Event& e)
{
	m_CameraController.OnEvent(e);
}
