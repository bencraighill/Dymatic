#include "dypch.h"
#include "Dymatic/Renderer/Renderer.h"
#include "Dymatic/Renderer/Renderer2D.h"
#include "Dymatic/Renderer/SceneRenderer.h"

namespace Dymatic {
	
	void Renderer::Init()
	{
		DY_PROFILE_FUNCTION();

		RenderCommand::Init();
		Renderer2D::Init();
		SceneRenderer::Init();
	}

	void Renderer::Shutdown()
	{
		Renderer2D::Shutdown();
		SceneRenderer::Shutdown();
	}

	void Renderer::OnWindowResize(uint32_t width, uint32_t height)
	{
		RenderCommand::SetViewport(0, 0, width, height);
	}

}
