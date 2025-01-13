#pragma once

#include "Dymatic/Core/Base.h"
#include "Dymatic/Renderer/Texture.h"

#include <glad/glad.h>

namespace Dymatic {

	namespace Utils {

		static GLenum DymaticTextureFormatToGLInternalFormat(TextureFormat format)
		{
			switch (format)
			{
			case TextureFormat::RGBA8:					return GL_RGBA8;
			case TextureFormat::RGB8:					return GL_RGB8;
			case TextureFormat::RG8:					return GL_RG8;
			case TextureFormat::R8:						return GL_R8;

			case TextureFormat::RGBA16F:				return GL_RGBA16F;
			case TextureFormat::RGB16F:					return GL_RGB16F;
			case TextureFormat::RG16F:					return GL_RG16F;
			case TextureFormat::R16F:					return GL_R16F;

			case TextureFormat::RGBA32F:				return GL_RGBA32F;
			case TextureFormat::RGB32F:					return GL_RGB32F;
			case TextureFormat::RG32F:					return GL_RG32F;
			case TextureFormat::R32F:					return GL_R32F;

			case TextureFormat::RED_INTEGER:			return GL_R32I;
			case TextureFormat::RED_UNSIGNED_INTEGER:	return GL_R32UI;

			case TextureFormat::DEPTH24STENCIL8:		return GL_DEPTH24_STENCIL8;
			}

			DY_CORE_ASSERT(false);
			return 0;
		}

		static GLenum DymaticTextureFormatToGLFormat(TextureFormat format)
		{
			switch (format)
			{
			case TextureFormat::RGBA8:					return GL_RGBA;
			case TextureFormat::RGB8:					return GL_RGB;
			case TextureFormat::RG8:					return GL_RG;
			case TextureFormat::R8:						return GL_RED;

			case TextureFormat::RGBA16F:				return GL_RGBA;
			case TextureFormat::RGB16F:					return GL_RGB;
			case TextureFormat::RG16F:					return GL_RG;
			case TextureFormat::R16F:					return GL_RED;

			case TextureFormat::RGBA32F:				return GL_RGBA;
			case TextureFormat::RGB32F:					return GL_RGB;
			case TextureFormat::RG32F:					return GL_RG;
			case TextureFormat::R32F:					return GL_RED;

			case TextureFormat::RED_INTEGER:			return GL_RED_INTEGER;
			case TextureFormat::RED_UNSIGNED_INTEGER:	return GL_RED_INTEGER;

			case TextureFormat::DEPTH24STENCIL8:		return GL_DEPTH_STENCIL;
			}

			DY_CORE_ASSERT(false);
			return 0;
		}

		static GLenum DymaticTextureFormatToGLType(TextureFormat format)
		{
			switch (format)
			{
			case TextureFormat::RGBA8:					return GL_UNSIGNED_BYTE;
			case TextureFormat::RGB8:					return GL_UNSIGNED_BYTE;
			case TextureFormat::RG8:					return GL_UNSIGNED_BYTE;
			case TextureFormat::R8:						return GL_UNSIGNED_BYTE;

			case TextureFormat::RGBA16F:				return GL_FLOAT;
			case TextureFormat::RGB16F:					return GL_FLOAT;
			case TextureFormat::RG16F:					return GL_FLOAT;
			case TextureFormat::R16F:					return GL_FLOAT;

			case TextureFormat::RGBA32F:				return GL_FLOAT;
			case TextureFormat::RGB32F:					return GL_FLOAT;
			case TextureFormat::RG32F:					return GL_FLOAT;
			case TextureFormat::R32F:					return GL_FLOAT;

			case TextureFormat::RED_INTEGER:			return GL_INT;
			case TextureFormat::RED_UNSIGNED_INTEGER:	return GL_UNSIGNED_INT;

			case TextureFormat::DEPTH24STENCIL8:		return GL_UNSIGNED_INT_24_8;
			}

			DY_CORE_ASSERT(false);
			return 0;
		}

		static GLenum DymaticSamplerWrapToGLFormat(TextureWrap wrap)
		{
			switch (wrap)
			{
			case TextureWrap::None:
			case TextureWrap::Repeat:				return GL_REPEAT;
			case TextureWrap::MirroredRepeat:		return GL_MIRRORED_REPEAT;
			case TextureWrap::ClampToEdge:			return GL_CLAMP_TO_EDGE;
			case TextureWrap::ClampToBorder:		return GL_CLAMP_TO_BORDER;
			case TextureWrap::MirrorClampToEdge:	return GL_CLAMP_TO_BORDER;
			}

			DY_CORE_ASSERT(false);
			return 0;
		}

		static GLenum DymaticSamplerFilterToGLFormat(TextureFilter filter)
		{
			switch (filter)
			{
			case TextureFilter::None:
			case TextureFilter::Linear: return GL_LINEAR;
			case TextureFilter::Nearest: return GL_NEAREST;
			case TextureFilter::LinearMipmapLinear: return GL_LINEAR_MIPMAP_LINEAR;
			case TextureFilter::LinearMipmapNearest: return GL_LINEAR_MIPMAP_NEAREST;
			case TextureFilter::NearestMipmapLinear: return GL_NEAREST_MIPMAP_LINEAR;
			case TextureFilter::NearestMipmapNearest: return GL_NEAREST_MIPMAP_NEAREST;
			}

			DY_CORE_ASSERT(false);
			return 0;
		}

	}

}