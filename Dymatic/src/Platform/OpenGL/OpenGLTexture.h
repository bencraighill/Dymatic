#pragma once

#include "Dymatic/Renderer/Texture.h"

#include <glad/glad.h>

namespace Dymatic {

	class OpenGLTexture2D : public Texture2D
	{
	public:
		OpenGLTexture2D(const TextureSpecification& specification);
		OpenGLTexture2D(const std::filesystem::path& filepath, const TextureSpecification& specification);
		OpenGLTexture2D(const TextureSpecification& specification, const Buffer& imageData);
		OpenGLTexture2D(const Buffer& fileData, const TextureSpecification& specification);
		virtual ~OpenGLTexture2D();

		virtual uint32_t GetWidth() const override { return m_Width; }
		virtual uint32_t GetHeight() const override { return m_Height; }

		virtual uint32_t GetRendererID() const override { return m_RendererID; }
		virtual uint64_t GetHandle() const override { return m_Handle; }

		virtual const TextureSpecification& GetSpecification() const { return m_Specification; }

		virtual void GetData(void* data, uint32_t size) override;
		virtual Buffer GetData() override;
		virtual void SetData(const void* data, uint32_t size) override;
		virtual void SetData(const Buffer& buffer) override;

		// TODO: If a render thread is added, no need for these functions, just use SetData and delay GPU upload
		virtual void StageData(const Buffer& buffer) override;
		virtual void UploadData() override;

		virtual void Bind(uint32_t slot = 0) const override;
		virtual void BindTexture(uint32_t slot = 0, int level = 0, bool layered = false, int layer = 0) const override;

		virtual uint32_t GetMipLevels() const override;

		virtual bool IsLoaded() const override { return m_IsLoaded; }

		virtual void Copy(Ref<Texture2D> target) const override;

		// Used by virtual/memory-only textures
		virtual void SetSpecification(const TextureSpecification& specification) override;
		virtual void Clear() override;
		virtual void Resize(const uint32_t width, const uint32_t height) override;

		virtual bool operator == (const Texture& other) const override
		{
			return m_RendererID == other.GetRendererID();
		}

		// Used to determine if this was loaded from a 'file' (even if runtime file) or is memory only
		virtual bool IsMemoryOnly() const override { return !m_IsLoaded; }

	private:
		void Invalidate();
		void Release();

		void LoadTextureFromData(const void* data, uint32_t width, uint32_t height, uint32_t channels);

	private:
		uint32_t m_RendererID = 0;
		uint64_t m_Handle = 0;

		uint32_t m_Width, m_Height;
		TextureSpecification m_Specification;

		Buffer m_StagingBuffer;

		std::filesystem::path m_Path;
		bool m_IsLoaded = false;
	};

	class OpenGLTexture3D : public Texture3D
	{
	public:
		OpenGLTexture3D(const TextureSpecification& specification);
		OpenGLTexture3D(const TextureSpecification& specification, const Buffer& imageData);
		virtual ~OpenGLTexture3D();

		virtual uint32_t GetWidth() const override { return m_Specification.Width; }
		virtual uint32_t GetHeight() const override { return m_Specification.Height; }

		virtual uint32_t GetRendererID() const override { return m_RendererID; }
		virtual uint64_t GetHandle() const override { return m_Handle; }

		virtual const TextureSpecification& GetSpecification() const { return m_Specification; }

		virtual void GetData(void* data, uint32_t size) override;
		virtual Buffer GetData() override;
		virtual void SetData(const void* data, uint32_t size) override;
		virtual void SetData(const Buffer& buffer) override;

		virtual void Bind(uint32_t slot = 0) const override;
		virtual void BindTexture(uint32_t slot = 0, int level = 0, bool layered = false, int layer = 0) const override;

		virtual uint32_t GetMipLevels() const override;

		virtual bool IsLoaded() const override { return m_IsLoaded; }

		virtual bool operator == (const Texture& other) const override
		{
			return m_RendererID == other.GetRendererID();
		}

		virtual glm::uvec3 GetMipmapLevelSize(uint32_t level) const override;

	private:
		uint32_t m_RendererID;
		uint64_t m_Handle;
		TextureSpecification m_Specification;

		bool m_IsLoaded = false;
	};

	class OpenGLTextureCube : public TextureCube
	{
	public:
		OpenGLTextureCube(const TextureSpecification& specification, const Buffer& imageData);
		virtual ~OpenGLTextureCube();

		virtual uint32_t GetWidth() const override { return m_Specification.Width; }
		virtual uint32_t GetHeight() const override { return m_Specification.Height; }

		virtual uint32_t GetRendererID() const override { return m_RendererID; }
		virtual uint64_t GetHandle() const override { return m_Handle; }

		virtual const TextureSpecification& GetSpecification() const { return m_Specification; }

		virtual void GetData(void* data, uint32_t size) override;
		virtual Buffer GetData() override;
		virtual void SetData(const void* data, uint32_t size) override;
		virtual void SetData(const Buffer& buffer) override;

		virtual void Bind(uint32_t slot = 0) const override;
		virtual void BindTexture(uint32_t slot = 0, int level = 0, bool layered = false, int layer = 0) const override;

		virtual uint32_t GetMipLevels() const override { return 1; }

		virtual bool IsLoaded() const override { return m_IsLoaded; }

		virtual bool operator == (const Texture& other) const override
		{
			return m_RendererID == ((OpenGLTextureCube&)other).m_RendererID;
		}

	private:
		uint32_t m_RendererID;
		uint64_t m_Handle;
		TextureSpecification m_Specification;

		bool m_IsLoaded = false;
	};
}