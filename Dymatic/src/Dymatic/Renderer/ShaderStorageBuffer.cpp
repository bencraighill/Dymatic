#include "dypch.h"
#include "ShaderStorageBuffer.h"

#include "Dymatic/Renderer/Renderer.h"
#include "Platform/OpenGL/OpenGLShaderStorageBuffer.h"

namespace Dymatic {

	Ref<ShaderStorageBuffer> ShaderStorageBuffer::Create(uint32_t size, uint32_t binding, ShaderStorageBufferUsage usage)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    DY_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::OpenGL:  return CreateRef<OpenGLShaderStorageBuffer>(size, binding, usage);
		}

		DY_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}