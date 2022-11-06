#pragma once

#include "Dymatic/Renderer/Texture.h"

#include <glad/glad.h>

namespace Dymatic {

	class OpenGLTexture2D : public Texture2D
	{
	public:
		OpenGLTexture2D(uint32_t width, uint32_t height, TextureFormat format);
		OpenGLTexture2D(const std::string& path, TextureFormat format);
		virtual ~OpenGLTexture2D();

		virtual uint32_t GetWidth() const override { return m_Width; }
		virtual uint32_t GetHeight() const override { return m_Height; }
		virtual uint32_t GetRendererID() const override { return m_RendererID; }

		virtual const std::string& GetPath() const override { return m_Path; }

		virtual void GetData(void* data, uint32_t size) override;
		virtual void SetData(void* data, uint32_t size) override;

		virtual void Bind(uint32_t slot = 0) const override;
		virtual void BindTexture(uint32_t slot = 0) const override;

		virtual bool IsLoaded() const override { return m_IsLoaded; }

		virtual bool operator == (const Texture& other) const override
		{
			return m_RendererID == other.GetRendererID();
		}
	private:
		uint32_t m_RendererID;
		TextureFormat m_Format;

		std::string m_Path;
		bool m_IsLoaded = false;
		uint32_t m_Width, m_Height;
	};

	class OpenGLTextureCube : public TextureCube
	{
	public:
		OpenGLTextureCube(uint32_t width, uint32_t height, uint32_t levels, TextureFormat format);
		virtual ~OpenGLTextureCube();

		virtual uint32_t GetWidth() const override { return m_Width; }
		virtual uint32_t GetHeight() const override { return m_Height; }
		virtual uint32_t GetRendererID() const override { return m_RendererID; }

		virtual const std::string& GetPath() const override { return ""; }

		virtual void GetData(void* data, uint32_t size) override;
		virtual void SetData(void* data, uint32_t size) override;

		virtual void Bind(uint32_t slot = 0) const override;
		virtual void BindTexture(uint32_t slot = 0) const override;

		virtual bool IsLoaded() const override { return m_IsLoaded; }

		virtual bool operator == (const Texture& other) const override
		{
			return m_RendererID == ((OpenGLTextureCube&)other).m_RendererID;
		}
	private:
		uint32_t m_RendererID;
		TextureFormat m_Format;

		bool m_IsLoaded = false;
		uint32_t m_Width, m_Height, m_Levels;
	};
}