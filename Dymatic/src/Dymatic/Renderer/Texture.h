#pragma once

#include <string>

#include "Dymatic/Core/Base.h"
#include "Dymatic/Renderer/TextureFormat.h"

namespace Dymatic {

	class Texture
	{
	public:
		virtual ~Texture() = default;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;
		virtual uint32_t GetRendererID() const = 0;

		virtual const std::string& GetPath() const = 0;

		virtual void GetData(void* data, uint32_t size) = 0;
		virtual void SetData(void* data, uint32_t size) = 0;

		virtual void Bind(uint32_t slot = 0) const = 0;
		virtual void BindTexture(uint32_t slot = 0) const = 0;

		virtual bool IsLoaded() const = 0;

		virtual bool operator==(const Texture& other) const = 0;
	};

	class Texture2D : public Texture
	{
	public:
		static Ref<Texture2D> Create(uint32_t width, uint32_t height, TextureFormat format = TextureFormat::RGBA8);
		static Ref<Texture2D> Create(const std::string& path, TextureFormat format = TextureFormat::None);
	};

	class TextureCube : public Texture
	{
	public:
		static Ref<TextureCube> Create(uint32_t width, uint32_t height, uint32_t levels, TextureFormat format = TextureFormat::RGBA8);
	};

}