#include "dypch.h"
#include "VertexArray.h"

#include "Renderer.h"
#include "Platform/OpenGL/OpenGLVertexArray.h"

namespace Dymatic {

	VertexArray* VertexArray::Create()
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::None:		DY_CORE_ASSERT(false, "RendererAPI::None is currently not supported by Dymatic!");  return nullptr;
			case RendererAPI::API::OpenGL:	return new OpenGLVertexArray();
		}

		DY_CORE_ASSERT(false, "Unknown RendererAPI!")
			return nullptr;
	}

}