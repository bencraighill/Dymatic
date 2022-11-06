#pragma once

#include "Dymatic/Core/Base.h"
#include "Dymatic/Renderer/TextureFormat.h"

#include <glad/glad.h>

namespace Dymatic {

	namespace Utils {
	
		static bool IsDepthFormat(TextureFormat format)
		{
			switch (format)
			{
			case TextureFormat::DEPTH24STENCIL8: return true;
			}

			return false;
		}

		static uint8_t GetDymaticTextureFormatBPP(TextureFormat format)
		{
			switch (format)
			{
			case TextureFormat::RGBA8:				return 4;
			case TextureFormat::RGB8:				return 3;
			case TextureFormat::RG8:				return 2;
			case TextureFormat::R8:					return 1;

			case TextureFormat::RGBA16F:			return 16;
			case TextureFormat::RGB16F:				return 12;
			case TextureFormat::RG16F:				return 8;
			case TextureFormat::R16F:				return 4;

			case TextureFormat::RED_INTEGER:		return 4;
			case TextureFormat::DEPTH24STENCIL8:	return 4;
			}

			DY_CORE_ASSERT(false);
			return 0;
		}

		static GLenum DymaticTextureFormatToGLInternalFormat(TextureFormat format)
		{
			switch (format)
			{
			case TextureFormat::RGBA8:				return GL_RGBA8;
			case TextureFormat::RGB8:				return GL_RGB8;
			case TextureFormat::RG8:				return GL_RG8;
			case TextureFormat::R8:					return GL_R8;

			case TextureFormat::RGBA16F:			return GL_RGBA16F;
			case TextureFormat::RGB16F:				return GL_RGB16F;
			case TextureFormat::RG16F:				return GL_RG16F;
			case TextureFormat::R16F:				return GL_R16F;

			case TextureFormat::RED_INTEGER:		return GL_R32I;
			case TextureFormat::DEPTH24STENCIL8:	return GL_DEPTH24_STENCIL8;
			}

			DY_CORE_ASSERT(false);
			return 0;
		}

		static GLenum DymaticTextureFormatToGLFormat(TextureFormat format)
		{
			switch (format)
			{
			case TextureFormat::RGBA8:				return GL_RGBA;
			case TextureFormat::RGB8:				return GL_RGB;
			case TextureFormat::RG8:				return GL_RG;
			case TextureFormat::R8:					return GL_RED;

			case TextureFormat::RGBA16F:			return GL_RGBA;
			case TextureFormat::RGB16F:				return GL_RGB;
			case TextureFormat::RG16F:				return GL_RG;
			case TextureFormat::R16F:				return GL_RED;

			case TextureFormat::RED_INTEGER:		return GL_RED_INTEGER;
			case TextureFormat::DEPTH24STENCIL8:	return GL_DEPTH_STENCIL;
			}

			DY_CORE_ASSERT(false);
			return 0;
		}

		static GLenum DymaticTextureFormatToGLType(TextureFormat format)
		{
			switch (format)
			{
			case TextureFormat::RGBA8:				return GL_UNSIGNED_BYTE;
			case TextureFormat::RGB8:				return GL_UNSIGNED_BYTE;
			case TextureFormat::RG8:				return GL_UNSIGNED_BYTE;
			case TextureFormat::R8:					return GL_UNSIGNED_BYTE;

			case TextureFormat::RGBA16F:			return GL_FLOAT;
			case TextureFormat::RGB16F:				return GL_FLOAT;
			case TextureFormat::RG16F:				return GL_FLOAT;
			case TextureFormat::R16F:				return GL_FLOAT;

			case TextureFormat::RED_INTEGER:		return GL_INT;
			case TextureFormat::DEPTH24STENCIL8:	return GL_UNSIGNED_INT_24_8;
			}

			DY_CORE_ASSERT(false);
			return 0;
		}

	}

}