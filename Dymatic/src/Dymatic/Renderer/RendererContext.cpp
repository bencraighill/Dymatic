#include "dypch.h"
#include "Dymatic/Renderer/RendererContext.h"

#include "Dymatic/Renderer/Renderer.h"
#include "Platform/OpenGL/OpenGLContext.h"

namespace Dymatic {

	Scope<RendererContext> RendererContext::Create(void* window)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::None:    DY_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPI::API::OpenGL:  return CreateScope<OpenGLContext>(static_cast<GLFWwindow*>(window));
		}

		DY_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}