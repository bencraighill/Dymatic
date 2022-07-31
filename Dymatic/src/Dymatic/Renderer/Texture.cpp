#include "dypch.h"
#include "Dymatic/Renderer/Texture.h"

#include "Dymatic/Renderer/Renderer.h"
#include "Platform/OpenGL/OpenGLTexture.h"

namespace Dymatic {

	Ref<Texture2D> Texture2D::Create(uint32_t width, uint32_t height)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    DY_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::OpenGL:  return CreateRef<OpenGLTexture2D>(width, height);
		}

		DY_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	Ref<Texture2D> Texture2D::Create(const std::string& path)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    DY_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::OpenGL:  return CreateRef<OpenGLTexture2D>(path);
		}

		DY_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	Ref<TextureCube> TextureCube::Create(const std::string* files)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    DY_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::OpenGL:  return CreateRef<OpenGLTextureCube>(files);
		}

		DY_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}