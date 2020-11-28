#include "dypch.h"
#include "Buffer.h"

#include "Renderer.h"

#include "Platform/OpenGL/OpenGLBuffer.h"

#include "Platform/OpenGL/OpenGLBuffer.h"

namespace Dymatic {

	Ref<VertexBuffer> VertexBuffer::Create(uint32_t size)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:		DY_CORE_ASSERT(false, "RendererAPI::None is currently not supported by Dymatic!");  return nullptr;
		case RendererAPI::API::OpenGL:	return CreateRef<OpenGLVertexBuffer>(size);
		}

		DY_CORE_ASSERT(false, "Unknown RendererAPI!")
		return nullptr;
	}

	Ref<VertexBuffer> VertexBuffer::Create(float* vertices, uint32_t size)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::None:		DY_CORE_ASSERT(false, "RendererAPI::None is currently not supported by Dymatic!");  return nullptr;
			case RendererAPI::API::OpenGL:	return CreateRef<OpenGLVertexBuffer>(vertices, size);
		}

		DY_CORE_ASSERT(false, "Unknown RendererAPI!")
		return nullptr;
	}

	Ref<IndexBuffer> IndexBuffer::Create(uint32_t* indices, uint32_t size)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::None:		DY_CORE_ASSERT(false, "RendererAPI::None is currently not supported by Dymatic!");  return nullptr;
			case RendererAPI::API::OpenGL:	return CreateRef<OpenGLIndexBuffer>(indices, size);
		}

		DY_CORE_ASSERT(false, "Unknown RendererAPI!")
		return nullptr;
	}

}