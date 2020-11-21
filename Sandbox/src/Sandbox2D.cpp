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
	m_CheckerboardTexture = Dymatic::Texture2D::Create("assets/textures/Checkerboard.png");
}

void Sandbox2D::OnDetach()
{

}

void Sandbox2D::OnUpdate(Dymatic::Timestep ts)
{
	DY_PROFILE_FUNCTION();

	//Update
	{
		m_CameraController.OnUpdate(ts);
		DY_PROFILE_SCOPE("CameraController::OnUpdate");
	}

	//Render
	{
		DY_PROFILE_SCOPE("Renderer Prep");
		Dymatic::RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.1f, 1.0f });
		Dymatic::RenderCommand::Clear();
	}

	{
		DY_PROFILE_SCOPE("Renderer Draw");
		Dymatic::Renderer2D::BeginScene(m_CameraController.GetCamera());
		Dymatic::Renderer2D::DrawQuad({ -1.0f, 0.0f }, { 0.8f, 0.8f }, { 0.8f, 0.2f, 0.3f, 1.0f });
		Dymatic::Renderer2D::DrawQuad({ 0.5f, -0.5f }, { 0.5f, 0.75f }, { 0.2f, 0.3f,  0.8f, 1.0f });
		Dymatic::Renderer2D::DrawQuad({ 0.0f, 0.0f, -0.1f }, { 10.0f, 10.0f }, m_CheckerboardTexture);
		Dymatic::Renderer2D::EndScene();
	}
	
	//std::dynamic_pointer_cast<Dymatic::OpenGLShader>(m_FlatColorShader)->Bind();
	//std::dynamic_pointer_cast<Dymatic::OpenGLShader>(m_FlatColorShader)->UploadUniformFloat4("u_Color", m_SquareColor);
}

void Sandbox2D::OnImGuiRender()
{
	DY_PROFILE_FUNCTION();

	ImGui::Begin("Settings");
	ImGui::ColorEdit4("Square Color", glm::value_ptr(m_SquareColor));

	static const char* items[]{ "Default", "Dark", "Light" };
	static int selectedItem = 0;

	ImGui::Combo("Editor Color Scheme", &selectedItem, items, IM_ARRAYSIZE(items));
	ImGui::End();
}

void Sandbox2D::OnEvent(Dymatic::Event& e)
{
	m_CameraController.OnEvent(e);
}
