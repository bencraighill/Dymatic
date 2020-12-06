#pragma once

#include "Dymatic.h"

#include "ParticleSystem.h"

class Sandbox2D : public Dymatic::Layer
{
public:
	Sandbox2D();
	virtual ~Sandbox2D() = default;

	virtual void OnAttach() override;
	virtual void OnDetach() override;

	void OnUpdate(Dymatic::Timestep ts) override;
	virtual void OnImGuiRender() override;
	void OnEvent(Dymatic::Event& e) override;
private:
	Dymatic::OrthographicCameraController m_CameraController;

	//Temporary Code
	Dymatic::Ref<Dymatic::VertexArray> m_SquareVA;
	Dymatic::Ref<Dymatic::Shader> m_FlatColorShader;

	Dymatic::Ref<Dymatic::Texture2D> m_CheckerboardTexture;

	glm::vec4 m_SquareColor = { 0.2f, 0.3f, 0.8f, 1.0f };

	ParticleSystem m_ParticleSystem;
	ParticleProps m_Particle;
};
