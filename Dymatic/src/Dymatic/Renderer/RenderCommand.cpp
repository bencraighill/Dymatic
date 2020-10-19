#include "dypch.h"
#include "RenderCommand.h"

#include "Platform/OpenGL/OpenGLRendererAPI.h"

namespace Dymatic {

	RendererAPI* RenderCommand::s_RendererAPI = new OpenGLRendererAPI;

}