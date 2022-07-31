#include "dypch.h"
#include "Platform/OpenGL/OpenGLTexture.h"

#include <stb_image.h>

namespace Dymatic {

	OpenGLTexture2D::OpenGLTexture2D(uint32_t width, uint32_t height)
		: m_Width(width), m_Height(height)
	{
		DY_PROFILE_FUNCTION();

		m_InternalFormat = GL_RGBA8;
		m_DataFormat = GL_RGBA;

		glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererID);
		glTextureStorage2D(m_RendererID, 1, m_InternalFormat, m_Width, m_Height);

		glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}

	OpenGLTexture2D::OpenGLTexture2D(const std::string& path)
		: m_Path(path)
	{
		DY_PROFILE_FUNCTION();

		int width, height, channels;
		stbi_set_flip_vertically_on_load(1);
		stbi_uc* data = nullptr;
		{
			DY_PROFILE_SCOPE("stbi_load - OpenGLTexture2D::OpenGLTexture2D(const std::string&)");
			data = stbi_load(path.c_str(), &width, &height, &channels, 0);
		}

		if (data)
		{
			m_IsLoaded = true;

			m_Width = width;
			m_Height = height;

			GLenum internalFormat = 0, dataFormat = 0;
			if (channels == 4)
			{
				internalFormat = GL_RGBA8;
				dataFormat = GL_RGBA;
			}
			else if (channels == 3)
			{
				internalFormat = GL_RGB8;
				dataFormat = GL_RGB;
			}
			else if (channels == 2)
			{
				internalFormat = GL_RG8;
				dataFormat = GL_RG;
			}
			else if (channels == 1)
			{
				internalFormat = GL_R8;
				dataFormat = GL_RED;
			}

			m_InternalFormat = internalFormat;
			m_DataFormat = dataFormat;

			DY_CORE_ASSERT(internalFormat & dataFormat, "Format not supported!");

			glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererID);
			glTextureStorage2D(m_RendererID, 1, internalFormat, m_Width, m_Height);

			glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, GL_REPEAT);

			glTextureSubImage2D(m_RendererID, 0, 0, 0, m_Width, m_Height, dataFormat, GL_UNSIGNED_BYTE, data);

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
		uint32_t bpp = m_DataFormat == GL_RGBA ? 4 : 3;
		DY_CORE_ASSERT(size == m_Width * m_Height * bpp, "Data must be entire texture!");
		glGetTextureSubImage(m_RendererID, 0, 0, 0, 0, m_Width, m_Height, 1, m_DataFormat, GL_UNSIGNED_BYTE, m_Width * m_Height * bpp, data);
	}

	void OpenGLTexture2D::SetData(void* data, uint32_t size)
	{
		DY_PROFILE_FUNCTION();

		uint32_t bpp = m_DataFormat == GL_RGBA ? 4 : 3;
		DY_CORE_ASSERT(size == m_Width * m_Height * bpp, "Data must be entire texture!");
		glTextureSubImage2D(m_RendererID, 0, 0, 0, m_Width, m_Height, m_DataFormat, GL_UNSIGNED_BYTE, data);
	}

	void OpenGLTexture2D::Bind(uint32_t slot) const
	{
		DY_PROFILE_FUNCTION();

		glBindTextureUnit(slot, m_RendererID);
	}

	void OpenGLTexture2D::BindTexture(uint32_t slot) const
	{
		DY_PROFILE_FUNCTION();

		glBindImageTexture(slot, m_RendererID, 0, GL_FALSE, 0, GL_READ_WRITE, m_InternalFormat);
	}

	OpenGLTextureCube::OpenGLTextureCube(const std::string paths[6])
	{
		DY_PROFILE_FUNCTION();
		
		int width, height, channels;
		stbi_set_flip_vertically_on_load(0);
		stbi_uc* xp = nullptr;
		stbi_uc* xn = nullptr;
		stbi_uc* yp = nullptr;
		stbi_uc* yn = nullptr;
		stbi_uc* zp = nullptr;
		stbi_uc* zn = nullptr;
		{
			DY_PROFILE_SCOPE("stbi_load - OpenGLTextureCube::OpenGLTextureCube(const std::string&)");
			xp = stbi_load(paths[0].c_str(), &width, &height, &channels, 0);
			xn = stbi_load(paths[1].c_str(), &width, &height, &channels, 0);
			yp = stbi_load(paths[2].c_str(), &width, &height, &channels, 0);
			yn = stbi_load(paths[3].c_str(), &width, &height, &channels, 0);
			zp = stbi_load(paths[4].c_str(), &width, &height, &channels, 0);
			zn = stbi_load(paths[5].c_str(), &width, &height, &channels, 0);
		}
		
		if (xp && xn && yp && yn && zp && zn)
		{
			m_IsLoaded = true;
		
			m_Width = width;
			m_Height = height;
		
			GLenum internalFormat = 0, dataFormat = 0;
			if (channels == 4)
			{
				internalFormat = GL_RGBA8;
				dataFormat = GL_RGBA;
			}
			else if (channels == 3)
			{
				internalFormat = GL_RGB8;
				dataFormat = GL_RGB;
			}
		
			m_InternalFormat = internalFormat;
			m_DataFormat = dataFormat;
		
			DY_CORE_ASSERT(internalFormat & dataFormat, "Format not supported!");
		
			glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_RendererID);
			glTextureStorage2D(m_RendererID, 1, internalFormat, m_Width, m_Height);
		
			glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		
			glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		
			glTextureSubImage3D(m_RendererID, 0, 0, 0, 0, m_Width, m_Height, 1, dataFormat, GL_UNSIGNED_BYTE, xp);
			glTextureSubImage3D(m_RendererID, 0, 0, 0, 1, m_Width, m_Height, 1, dataFormat, GL_UNSIGNED_BYTE, xn);
			glTextureSubImage3D(m_RendererID, 0, 0, 0, 2, m_Width, m_Height, 1, dataFormat, GL_UNSIGNED_BYTE, yp);
			glTextureSubImage3D(m_RendererID, 0, 0, 0, 3, m_Width, m_Height, 1, dataFormat, GL_UNSIGNED_BYTE, yn);
			glTextureSubImage3D(m_RendererID, 0, 0, 0, 4, m_Width, m_Height, 1, dataFormat, GL_UNSIGNED_BYTE, zp);
			glTextureSubImage3D(m_RendererID, 0, 0, 0, 5, m_Width, m_Height, 1, dataFormat, GL_UNSIGNED_BYTE, zn);
		
			glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
		
			stbi_image_free(xp);
			stbi_image_free(xn);
			stbi_image_free(yp);
			stbi_image_free(yn);
			stbi_image_free(zp);
			stbi_image_free(zn);
		}
		else
			m_IsLoaded = false;
	}

	OpenGLTextureCube::~OpenGLTextureCube()
	{
		DY_PROFILE_FUNCTION();

		glDeleteTextures(1, &m_RendererID);
	}

	void OpenGLTextureCube::GetData(void* data, uint32_t size)
	{

	}

	void OpenGLTextureCube::SetData(void* data, uint32_t size)
	{

	}

	void OpenGLTextureCube::Bind(uint32_t slot) const
	{
		DY_PROFILE_FUNCTION();

		glBindTextureUnit(slot, m_RendererID);
	}

	void OpenGLTextureCube::BindTexture(uint32_t slot) const
	{
//#error
	}

}
