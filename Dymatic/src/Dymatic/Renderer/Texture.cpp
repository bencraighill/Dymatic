#include "dypch.h"
#include "Dymatic/Renderer/Texture.h"

#include "Dymatic/Renderer/Renderer.h"
#include "Platform/OpenGL/OpenGLTexture.h"

namespace Dymatic {

	Ref<Texture2D> Texture2D::Create(uint32_t width, uint32_t height, TextureFormat format)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    DY_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::OpenGL:  return CreateRef<OpenGLTexture2D>(width, height, format);
		}

		DY_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	Ref<Texture2D> Texture2D::Create(const std::string& path, TextureFormat format)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    DY_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::OpenGL:  return CreateRef<OpenGLTexture2D>(path, format);
		}

		DY_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	Ref<Texture3D> Texture3D::Create(uint32_t width, uint32_t height, uint32_t depth, TextureFormat format)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    DY_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::OpenGL:  return CreateRef<OpenGLTexture3D>(width, height, depth, format);
		}

		DY_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	Ref<TextureCube> TextureCube::Create(uint32_t width, uint32_t height, uint32_t levels, TextureFormat format)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    DY_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::OpenGL:  return CreateRef<OpenGLTextureCube>(width, height, levels, format);
		}

		DY_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}
	
}