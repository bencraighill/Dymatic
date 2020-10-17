#include "dypch.h"
#include "Buffer.h"

#include "Renderer.h"

#include "Platform/OpenGL/OpenGLBuffer.h"

#include "Platform/OpenGL/OpenGLBuffer.h"

namespace Dymatic {

	VertexBuffer* VertexBuffer::Create(float* vertices, uint32_t size)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPI::None:		DY_CORE_ASSERT(false, "RendererAPI::None is currently not supported by Dymatic!");  return nullptr;
			case RendererAPI::OpenGL:	return new OpenGLVertexBuffer(vertices, size);
		}

		DY_CORE_ASSERT(false, "Unknown RendererAPI!")
		return nullptr;
	}

	IndexBuffer* IndexBuffer::Create(uint32_t* indices, uint32_t size)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPI::None:		DY_CORE_ASSERT(false, "RendererAPI::None is currently not supported by Dymatic!");  return nullptr;
			case RendererAPI::OpenGL:	return new OpenGLIndexBuffer(indices, size);
		}

		DY_CORE_ASSERT(false, "Unknown RendererAPI!")
		return nullptr;
	}

}