#pragma once

#include <string>

namespace Dymatic {

	enum class TextureFormat : uint16_t
	{
		None = 0,

		// Color
		RGBA8,
		RGB8,
		RG8,
		R8,

		RGBA16F,
		RGB16F,
		RG16F,
		R16F,

		RGBA32F,
		RGB32F,
		RG32F,
		R32F,

		RED_INTEGER,
		RED_UNSIGNED_INTEGER,

		// Depth/Stencil
		DEPTH24STENCIL8,

		// Aliases
		Depth = DEPTH24STENCIL8
	};

	namespace Utils {
		bool IsDepthFormat(TextureFormat format);
		const char* TextureFormatToString(TextureFormat format);
		TextureFormat TextureFormatFromString(const std::string& formatString);
		uint32_t GetFormatChannels(const TextureFormat format);
		uint8_t GetDymaticTextureFormatBPP(TextureFormat format);
	}

}