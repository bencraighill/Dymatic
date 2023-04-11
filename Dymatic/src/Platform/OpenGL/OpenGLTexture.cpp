#include "dypch.h"
#include "Platform/OpenGL/OpenGLTexture.h"
#include "Platform/OpenGL/OpenGLTextureFormat.h"

#include <stb_image.h>

namespace Dymatic {

	namespace Utils {
	
		static void* LoadImageFile(TextureFormat format, const char* path, int* width, int* height, int* channels)
		{
			DY_PROFILE_FUNCTION();
			stbi_set_flip_vertically_on_load(1);
			switch (format)
			{
			case TextureFormat::None:
			case TextureFormat::RGBA8:
			case TextureFormat::RGB8:
			case TextureFormat::RG8:
			case TextureFormat::R8:
				return stbi_load(path, width, height, channels, 0);

			case TextureFormat::RGBA16F:
			case TextureFormat::RGB16F:
			case TextureFormat::RG16F:
			case TextureFormat::R16F:
				return stbi_loadf(path, width, height, channels, 0);
			}

			DY_CORE_ASSERT(false);
			return nullptr;
		}
	
	}

	OpenGLTexture2D::OpenGLTexture2D(uint32_t width, uint32_t height, TextureFormat format)
		: m_Width(width), m_Height(height), m_Format(format)
	{
		DY_PROFILE_FUNCTION();

		glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererID);
		glTextureStorage2D(m_RendererID, 1, Utils::DymaticTextureFormatToGLInternalFormat(m_Format), m_Width, m_Height);

		glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}

	OpenGLTexture2D::OpenGLTexture2D(const std::string& path, TextureFormat format)
		: m_Path(path), m_Format(format)
	{
		DY_PROFILE_FUNCTION();
		
		int width, height, channels;
		void* data = nullptr;
		{
			data = Utils::LoadImageFile(format, path.c_str(), &width, &height, &channels);
		}

		if (data)
		{
			m_IsLoaded = true;

			m_Width = width;
			m_Height = height;

			if (format == TextureFormat::None)
			{
				// Automatic Detection
				if (channels == 4)
					m_Format = TextureFormat::RGBA8;
				else if (channels == 3)
					m_Format = TextureFormat::RGB8;
				else if (channels == 2)
					m_Format = TextureFormat::RG8;
				else if (channels == 1)
					m_Format = TextureFormat::R8;
			}

			glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererID);
			glTextureStorage2D(m_RendererID, 1, Utils::DymaticTextureFormatToGLInternalFormat(m_Format), m_Width, m_Height);

			glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, GL_REPEAT);

			glTextureSubImage2D(m_RendererID, 0, 0, 0, m_Width, m_Height, Utils::DymaticTextureFormatToGLFormat(m_Format), Utils::DymaticTextureFormatToGLType(m_Format), data);

			stbi_image_free(data);
		}
	}

	OpenGLTexture2D::~OpenGLTexture2D()
	{
		DY_PROFILE_FUNCTION();

		glDeleteTextures(1, &m_RendererID);
	}

	void OpenGLTexture2D::GetData(void* data, uint32_t size)
	{
		DY_PROFILE_FUNCTION();

		uint32_t bpp = Utils::GetDymaticTextureFormatBPP(m_Format);
		DY_CORE_ASSERT(size == m_Width * m_Height * bpp, "Data must be entire texture!");
		glGetTextureSubImage(m_RendererID, 0, 0, 0, 0, m_Width, m_Height, 1, Utils::DymaticTextureFormatToGLFormat(m_Format), Utils::DymaticTextureFormatToGLType(m_Format), m_Width * m_Height * bpp, data);
	}

	void OpenGLTexture2D::SetData(const void* data, uint32_t size)
	{
		DY_PROFILE_FUNCTION();

		uint32_t bpp = Utils::GetDymaticTextureFormatBPP(m_Format);
		DY_CORE_ASSERT(size == m_Width * m_Height * bpp, "Data must be entire texture!");
		glTextureSubImage2D(m_RendererID, 0, 0, 0, m_Width, m_Height, Utils::DymaticTextureFormatToGLFormat(m_Format), Utils::DymaticTextureFormatToGLType(m_Format), data);
	}

	void OpenGLTexture2D::Bind(uint32_t slot) const
	{
		DY_PROFILE_FUNCTION();

		glBindTextureUnit(slot, m_RendererID);
	}

	void OpenGLTexture2D::BindTexture(uint32_t slot) const
	{
		DY_PROFILE_FUNCTION();

		glBindImageTexture(slot, m_RendererID, 0, GL_FALSE, 0, GL_READ_WRITE, Utils::DymaticTextureFormatToGLInternalFormat(m_Format));
	}



	OpenGLTexture3D::OpenGLTexture3D(uint32_t width, uint32_t height, uint32_t depth, TextureFormat format)
		: m_Width(width), m_Height(height), m_Depth(depth), m_Format(format)
	{
	}

	OpenGLTexture3D::~OpenGLTexture3D()
	{

	}

	void OpenGLTexture3D::GetData(void* data, uint32_t size)
	{

	}

	void OpenGLTexture3D::SetData(const void* data, uint32_t size)
	{

	}

	void OpenGLTexture3D::Bind(uint32_t slot /*= 0*/) const
	{

	}

	void OpenGLTexture3D::BindTexture(uint32_t slot /*= 0*/) const
	{

	}

	OpenGLTextureCube::OpenGLTextureCube(uint32_t width, uint32_t height, uint32_t levels, TextureFormat format)
		: m_Width(width), m_Height(height), m_Levels(levels), m_Format(format)
	{
		DY_PROFILE_FUNCTION();

		DY_CORE_ASSERT(m_Format != TextureFormat::None, "Format not supported!");
		
		glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_RendererID);
		glTextureStorage2D(m_RendererID, levels, Utils::DymaticTextureFormatToGLInternalFormat(m_Format), m_Width, m_Height);
		
		glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		
		glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

		glGenerateTextureMipmap(m_RendererID);
	}

	OpenGLTextureCube::~OpenGLTextureCube()
	{
		DY_PROFILE_FUNCTION();

		glDeleteTextures(1, &m_RendererID);
	}

	void OpenGLTextureCube::GetData(void* data, uint32_t size)
	{
	}

	void OpenGLTextureCube::SetData(const void* data, uint32_t size)
	{
		//glTextureSubImage3D(m_RendererID, 0, 0, 0, i, m_Width, m_Height, 1, Utils::DymaticTextureFormatToGLFormat(m_Format), Utils::DymaticTextureFormatToGLType(m_Format),  data);
	}

	void OpenGLTextureCube::Bind(uint32_t slot) const
	{
		DY_PROFILE_FUNCTION();

		glBindTextureUnit(slot, m_RendererID);
	}

	void OpenGLTextureCube::BindTexture(uint32_t slot) const
	{
	}

}
