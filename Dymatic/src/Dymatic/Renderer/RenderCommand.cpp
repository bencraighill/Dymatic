#include "dypch.h"
#include "Dymatic/Renderer/RenderCommand.h"

namespace Dymatic {

	Scope<RendererAPI> RenderCommand::s_RendererAPI = RendererAPI::Create();

}