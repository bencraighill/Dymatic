#include "dypch.h"
#include "Texture.h"

#include "Renderer.h"
#include "Platform/OpenGL/OpenGLTexture.h"


namespace Dymatic {

	Ref<Texture2D> Texture2D::Create(const std::string& path)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::None:	DY_CORE_ASSERT(false, "RendererAPI::None is currently not supported by Dymatic!");  return nullptr;
			case RendererAPI::API::OpenGL:	return std::make_shared<OpenGLTexture2D>(path);
		}

		DY_CORE_ASSERT(false, "Unknown RendererAPI!")
		return nullptr;
	}

}