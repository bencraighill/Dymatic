#include "dypch.h"
#include "Dymatic/Renderer/TextureFormat.h"

namespace Dymatic::Utils {

	bool IsDepthFormat(TextureFormat format)
	{
		switch (format)
		{
		case TextureFormat::DEPTH24STENCIL8: return true;
		}

		return false;
	}

	uint32_t GetFormatChannels(const TextureFormat format)
	{
		switch (format)
		{
		case TextureFormat::RGBA8:					return 4;
		case TextureFormat::RGB8:					return 3;
		case TextureFormat::RG8:					return 2;
		case TextureFormat::R8:						return 1;

		case TextureFormat::RGBA16F:				return 4;
		case TextureFormat::RGB16F:					return 3;
		case TextureFormat::RG16F:					return 2;
		case TextureFormat::R16F:					return 1;

		case TextureFormat::RGBA32F:				return 4;
		case TextureFormat::RGB32F:					return 3;
		case TextureFormat::RG32F:					return 2;
		case TextureFormat::R32F:					return 1;

		case TextureFormat::RED_INTEGER:			return 1;
		case TextureFormat::RED_UNSIGNED_INTEGER:	return 1;

		case TextureFormat::DEPTH24STENCIL8:		return 1;
		}

		DY_CORE_ASSERT(false);
		return 0;
	}

	uint8_t GetDymaticTextureFormatBPP(TextureFormat format)
	{
		switch (format)
		{
		case TextureFormat::RGBA8:					return 4;
		case TextureFormat::RGB8:					return 3;
		case TextureFormat::RG8:					return 2;
		case TextureFormat::R8:						return 1;

		case TextureFormat::RGBA16F:				return 16;
		case TextureFormat::RGB16F:					return 12;
		case TextureFormat::RG16F:					return 8;
		case TextureFormat::R16F:					return 4;

		case TextureFormat::RGBA32F:				return 32;
		case TextureFormat::RGB32F:					return 24;
		case TextureFormat::RG32F:					return 16;
		case TextureFormat::R32F:					return 8;

		case TextureFormat::RED_INTEGER:			return 4;
		case TextureFormat::RED_UNSIGNED_INTEGER:	return 4;

		case TextureFormat::DEPTH24STENCIL8:		return 4;
		}

		DY_CORE_ASSERT(false);
		return 0;
	}

	const char* TextureFormatToString(TextureFormat format)
	{
		switch (format)
		{
		case TextureFormat::None:					return "None";
		case TextureFormat::RGBA8:					return "RGBA8";
		case TextureFormat::RGB8:					return "RGB8";
		case TextureFormat::RG8:					return "RG8";
		case TextureFormat::R8:						return "R8";
		case TextureFormat::RGBA16F:				return "RGBA16F";
		case TextureFormat::RGB16F:					return "RGB16F";
		case TextureFormat::RG16F:					return "RG16F";
		case TextureFormat::R16F:					return "R16F";
		case TextureFormat::RGBA32F:				return "RGBA32F";
		case TextureFormat::RGB32F:					return "RGB32F";
		case TextureFormat::RG32F:					return "RG32F";
		case TextureFormat::R32F:					return "R32F";
		case TextureFormat::RED_INTEGER:			return "Red Integer";
		case TextureFormat::RED_UNSIGNED_INTEGER:	return "Red Unsigned Integer";
		case TextureFormat::DEPTH24STENCIL8:		return "Depth/Stencil";
		}

		DY_CORE_ASSERT(false, "Unknown Dymatic texture format");
		return "None";
	}

	TextureFormat TextureFormatFromString(const std::string& formatString)
	{
		if (formatString == "None")						return TextureFormat::None;
		if (formatString == "RGBA8")					return TextureFormat::RGBA8;
		if (formatString == "RGB8")						return TextureFormat::RGB8;
		if (formatString == "RG8")						return TextureFormat::RG8;
		if (formatString == "R8")						return TextureFormat::R8;
		if (formatString == "RGBA16F")					return TextureFormat::RGBA16F;
		if (formatString == "RGB16F")					return TextureFormat::RGB16F;
		if (formatString == "RG16F")					return TextureFormat::RG16F;
		if (formatString == "R16F")						return TextureFormat::R16F;
		if (formatString == "RGBA32F")					return TextureFormat::RGBA32F;
		if (formatString == "RGB32F")					return TextureFormat::RGB32F;
		if (formatString == "RG32F")					return TextureFormat::RG32F;
		if (formatString == "R32F")						return TextureFormat::R32F;
		if (formatString == "Red Integer")				return TextureFormat::RED_INTEGER;
		if (formatString == "Red Unsigned Integer")		return TextureFormat::RED_UNSIGNED_INTEGER;
		if (formatString == "Depth/Stencil")			return TextureFormat::DEPTH24STENCIL8;

		DY_CORE_ASSERT(false, "Unknown Dymatic texture format");
		return TextureFormat::None;
	}

}