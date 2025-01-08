#pragma once

#include "Dymatic/Core/Base.h"
#include "Dymatic/Renderer/TextureFormat.h"

#include <filesystem>
#include <stb_image.h>

namespace Dymatic::Utils {

	static TextureFormat GetImageFileInfo(TextureFormat format, const char* path, int* width, int* height, int* channels, bool useFileChannels)
	{
		DY_PROFILE_FUNCTION();

		if (format == TextureFormat::None)
			format = TextureFormat::RGBA8;

		stbi_info(path, width, height, channels);

		if (!useFileChannels)
			*channels = Utils::GetFormatChannels(format);
		else
		{
			switch (*channels)
			{
			case 1: format = TextureFormat::R8; break;
			case 2: format = TextureFormat::RG8; break;
			case 3: format = TextureFormat::RGB8; break;
			case 4: format = TextureFormat::RGBA8; break;
			}
		}

		return format;
	}

	static void* LoadImageFile(TextureFormat format, const char* path, int* width, int* height, int* channels, bool useFileChannels)
	{
		DY_PROFILE_FUNCTION();

		if (!std::filesystem::exists(path))
			return nullptr;

		stbi_set_flip_vertically_on_load(1);

		if (format == TextureFormat::None)
			format = TextureFormat::RGBA8;

		void* data = nullptr;

		switch (format)
		{
		case TextureFormat::RGBA8:
		case TextureFormat::RGB8:
		case TextureFormat::RG8:
		case TextureFormat::R8:
			data = stbi_load(path, width, height, channels, useFileChannels ? 0 : Utils::GetFormatChannels(format));
			break;

		case TextureFormat::RGBA16F:
		case TextureFormat::RGB16F:
		case TextureFormat::RG16F:
		case TextureFormat::R16F:
			data = stbi_loadf(path, width, height, channels, useFileChannels ? 0 : Utils::GetFormatChannels(format));
			break;
		}

		DY_CORE_ASSERT(data);

		if (!useFileChannels)
			*channels = Utils::GetFormatChannels(format);

		return data;
	}

	static void* LoadImageFileMemory(TextureFormat format, Buffer fileData, int* width, int* height, int* channels, bool useFileChannels)
	{
		DY_PROFILE_FUNCTION();

		stbi_set_flip_vertically_on_load(1);

		if (format == TextureFormat::None)
			format = TextureFormat::RGBA8;

		void* data = nullptr;

		switch (format)
		{
		case TextureFormat::RGBA8:
		case TextureFormat::RGB8:
		case TextureFormat::RG8:
		case TextureFormat::R8:
			data = stbi_load_from_memory((const stbi_uc*)fileData.Data, fileData.Size, width, height, channels, useFileChannels ? 0 : Utils::GetFormatChannels(format));
			break;

		case TextureFormat::RGBA16F:
		case TextureFormat::RGB16F:
		case TextureFormat::RG16F:
		case TextureFormat::R16F:
			data = stbi_loadf_from_memory((const stbi_uc*)fileData.Data, fileData.Size, width, height, channels, useFileChannels ? 0 : Utils::GetFormatChannels(format));
			break;
		}

		DY_CORE_ASSERT(data);

		if (!useFileChannels)
			*channels = Utils::GetFormatChannels(format);

		return data;
	}

}