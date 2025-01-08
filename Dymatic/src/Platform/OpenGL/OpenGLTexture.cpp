#include "dypch.h"
#include "Platform/OpenGL/OpenGLTexture.h"
#include "Platform/OpenGL/OpenGLTextureFormat.h"

#include "Dymatic/Renderer/ImageLoader.h"

namespace Dymatic {

	//-----------------------------------------------------------------------------
	// Texture 2D
	//-----------------------------------------------------------------------------

	OpenGLTexture2D::OpenGLTexture2D(const TextureSpecification& specification)
		: m_Specification(specification), m_Width(specification.Width), m_Height(specification.Height)
	{
		DY_PROFILE_FUNCTION();
		Invalidate();
	}

	OpenGLTexture2D::OpenGLTexture2D(const std::filesystem::path& path, const TextureSpecification& specification)
		: m_Path(path), m_Specification(specification)
	{
		DY_PROFILE_FUNCTION();
		
		int width, height, channels;
		void* data = nullptr;

		data = Utils::LoadImageFile(m_Specification.Format, path.string().c_str(), &width, &height, &channels, specification.UseFileChannels);

		if (data)
		{
			LoadTextureFromData(data, width, height, channels);
			stbi_image_free(data);
		}
	}

	OpenGLTexture2D::OpenGLTexture2D(const TextureSpecification& specification, const Buffer& imageData)
		: OpenGLTexture2D(specification)
	{
		DY_PROFILE_FUNCTION();
		SetData(imageData);
	}

	OpenGLTexture2D::OpenGLTexture2D(const Buffer& fileData, const TextureSpecification& specification)
	{
		DY_PROFILE_FUNCTION();

		int width, height, channels;
		void* data = nullptr;

		data = Utils::LoadImageFileMemory(m_Specification.Format, fileData, &width, &height, &channels, specification.UseFileChannels);

		if (data)
		{
			LoadTextureFromData(data, width, height, channels);
			stbi_image_free(data);
		}
	}

	void OpenGLTexture2D::Invalidate()
	{
		if (m_RendererID)
			Release();

		glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererID);
		glTextureStorage2D(m_RendererID, GetMipLevels(), Utils::DymaticTextureFormatToGLInternalFormat(m_Specification.Format), m_Width, m_Height);

		// Note: These filters cannot be changed without generating a new bindless texture handle
		const GLenum samplerFilter = Utils::DymaticSamplerFilterToGLFormat(m_Specification.SamplerFilter);
		glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, samplerFilter);
		glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, samplerFilter);

		const GLenum samplerWrap = Utils::DymaticSamplerWrapToGLFormat(m_Specification.SamplerWrap);
		glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_S, samplerWrap);
		glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, samplerWrap);

		if (m_Specification.GenerateMips)
			glGenerateTextureMipmap(m_RendererID);

		// Allow for bindless texture usage
		m_Handle = glGetTextureHandleARB(m_RendererID);
		glMakeTextureHandleResidentARB(m_Handle);
	}

	void OpenGLTexture2D::Release()
	{
		// Release the bindless texture
		glMakeTextureHandleNonResidentARB(m_Handle);
		glDeleteTextures(1, &m_RendererID);
	}

	void OpenGLTexture2D::LoadTextureFromData(const void* data, uint32_t width, uint32_t height, uint32_t channels)
	{
		m_IsLoaded = true;

		m_Width = width;
		m_Height = height;

		// If default color, attempt to load correct channel count.
		if (m_Specification.UseFileChannels && m_Specification.Format == TextureFormat::RGBA8)
		{
			// Automatic Detection
			if (channels == 4)
				m_Specification.Format = TextureFormat::RGBA8;
			else if (channels == 3)
				m_Specification.Format = TextureFormat::RGB8;
			else if (channels == 2)
				m_Specification.Format = TextureFormat::RG8;
			else if (channels == 1)
				m_Specification.Format = TextureFormat::R8;
		}

		glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererID);
		glTextureStorage2D(m_RendererID, GetMipLevels(), Utils::DymaticTextureFormatToGLInternalFormat(m_Specification.Format), m_Width, m_Height);

		const GLenum samplerFilter = Utils::DymaticSamplerFilterToGLFormat(m_Specification.SamplerFilter);
		glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, samplerFilter);
		glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, samplerFilter);

		const GLenum samplerWrap = Utils::DymaticSamplerWrapToGLFormat(m_Specification.SamplerWrap);
		glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_S, samplerWrap);
		glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, samplerWrap);

		const GLenum format = Utils::DymaticTextureFormatToGLFormat(m_Specification.Format);
		const GLenum type = Utils::DymaticTextureFormatToGLType(m_Specification.Format);
		glTextureSubImage2D(m_RendererID, 0, 0, 0, m_Width, m_Height, format, type, data);

		if (m_Specification.GenerateMips)
			glGenerateTextureMipmap(m_RendererID);

		// Allow for bindless texture usage
		m_Handle = glGetTextureHandleARB(m_RendererID);
		glMakeTextureHandleResidentARB(m_Handle);
	}

	OpenGLTexture2D::~OpenGLTexture2D()
	{
		DY_PROFILE_FUNCTION();
		Release();
	}

	void OpenGLTexture2D::GetData(void* data, uint32_t size)
	{
		DY_PROFILE_FUNCTION();

		uint32_t bpp = Utils::GetDymaticTextureFormatBPP(m_Specification.Format);
		DY_CORE_ASSERT(size == m_Width * m_Height * bpp, "Data must be entire texture!");
		glGetTextureSubImage(m_RendererID, 0, 0, 0, 0, m_Width, m_Height, 1, Utils::DymaticTextureFormatToGLFormat(m_Specification.Format), Utils::DymaticTextureFormatToGLType(m_Specification.Format), m_Width * m_Height * bpp, data);
	}

	Buffer OpenGLTexture2D::GetData()
	{
		DY_PROFILE_FUNCTION();

		uint32_t bpp = Utils::GetDymaticTextureFormatBPP(m_Specification.Format);
		Buffer buffer(m_Width * m_Height * bpp);
		
		GetData(buffer.Data, buffer.Size);

		return buffer;
	}

	void OpenGLTexture2D::SetData(const void* data, uint32_t size)
	{
		DY_PROFILE_FUNCTION();

		uint32_t bpp = Utils::GetDymaticTextureFormatBPP(m_Specification.Format);
		DY_CORE_ASSERT(size == m_Width * m_Height * bpp, "Data must be entire texture!");
		glTextureSubImage2D(m_RendererID, 0, 0, 0, m_Width, m_Height, Utils::DymaticTextureFormatToGLFormat(m_Specification.Format), Utils::DymaticTextureFormatToGLType(m_Specification.Format), data);
		
		if (m_Specification.GenerateMips)
			glGenerateTextureMipmap(m_RendererID);
	}

	void OpenGLTexture2D::SetData(const Buffer& buffer)
	{
		DY_PROFILE_FUNCTION();
		
		SetData(buffer.Data, buffer.Size);
	}

	void OpenGLTexture2D::StageData(const Buffer& buffer)
	{
		if (m_StagingBuffer)
			m_StagingBuffer.Release();

		m_StagingBuffer = buffer;
	}

	void OpenGLTexture2D::UploadData()
	{
		if (!m_StagingBuffer)
			return;

		SetData(m_StagingBuffer);
		m_StagingBuffer.Release();
	}

	void OpenGLTexture2D::Bind(uint32_t slot) const
	{
		DY_PROFILE_FUNCTION();

		glBindTextureUnit(slot, m_RendererID);
	}

	void OpenGLTexture2D::BindTexture(uint32_t slot, int level, bool layered, int layer) const
	{
		DY_PROFILE_FUNCTION();

		glBindImageTexture(slot, m_RendererID, level, layered, layer, GL_READ_WRITE, Utils::DymaticTextureFormatToGLInternalFormat(m_Specification.Format));
	}

	uint32_t OpenGLTexture2D::GetMipLevels() const
	{
		return m_Specification.GenerateMips ? (std::floor(std::log2(std::max(m_Width, m_Height))) + 1) : 1;
	}

	void OpenGLTexture2D::Copy(Ref<Texture2D> target) const
	{
		glCopyImageSubData(m_RendererID, GL_TEXTURE_2D, 0, 0, 0, 0, target->GetRendererID(), GL_TEXTURE_2D, 0, 0, 0, 0, m_Width, m_Height, 1);
	}

	void OpenGLTexture2D::SetSpecification(const TextureSpecification& specification)
	{
		m_Specification = specification;
		m_Width = specification.Width;
		m_Height = specification.Height;
		Invalidate();

		// Zero-Initialize Texture
		Buffer data = Buffer(m_Specification.Width * m_Specification.Height * Utils::GetDymaticTextureFormatBPP(m_Specification.Format));
		data.ZeroInitialize();
		SetData(data);
		data.Release();
	}

	void OpenGLTexture2D::Clear()
	{
		Buffer data = Buffer(m_Width * m_Height * Utils::GetDymaticTextureFormatBPP(m_Specification.Format));
		data.ZeroInitialize();
		SetData(data);
		data.Release();
	}

	void OpenGLTexture2D::Resize(const uint32_t width, const uint32_t height)
	{
		if (m_Width == width && m_Height == height)
			return;

		m_Width = width;
		m_Height = height;
		Invalidate();
	}

	//-----------------------------------------------------------------------------
	// Texture 3D
	//-----------------------------------------------------------------------------

	OpenGLTexture3D::OpenGLTexture3D(const TextureSpecification& specification)
		: m_Specification(specification)
	{
		DY_PROFILE_FUNCTION();

		glCreateTextures(GL_TEXTURE_3D, 1, &m_RendererID);
		glTextureStorage3D(m_RendererID, GetMipLevels(), Utils::DymaticTextureFormatToGLInternalFormat(m_Specification.Format), m_Specification.Width, m_Specification.Height, m_Specification.Depth);

		const GLenum samplerFilter = Utils::DymaticSamplerFilterToGLFormat(m_Specification.SamplerFilter);
		glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, samplerFilter);
		glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, samplerFilter);

		const GLenum samplerWrap = Utils::DymaticSamplerWrapToGLFormat(m_Specification.SamplerWrap);
		glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_S, samplerWrap);
		glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, samplerWrap);
		glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_R, samplerWrap);

		if (m_Specification.GenerateMips)
			glGenerateTextureMipmap(m_RendererID);

		// Allow for bindless texture usage
		m_Handle = glGetTextureHandleARB(m_RendererID);
		glMakeTextureHandleResidentARB(m_Handle);
	}

	OpenGLTexture3D::OpenGLTexture3D(const TextureSpecification& specification, const Buffer& imageData)
		: OpenGLTexture3D(specification)
	{
		DY_PROFILE_FUNCTION();
		SetData(imageData);
	}

	OpenGLTexture3D::~OpenGLTexture3D()
	{
		DY_PROFILE_FUNCTION();

		// Release the bindless texture
		glMakeTextureHandleNonResidentARB(m_Handle);

		glDeleteTextures(1, &m_RendererID);
	}

	void OpenGLTexture3D::GetData(void* data, uint32_t size)
	{
		DY_PROFILE_FUNCTION();

		const uint32_t bpp = Utils::GetDymaticTextureFormatBPP(m_Specification.Format);
		DY_CORE_ASSERT(size == m_Specification.Width * m_Specification.Height * m_Specification.Depth * bpp, "Data must be entire texture!");
		glGetTextureSubImage(m_RendererID, 0, 0, 0, 0, m_Specification.Width, m_Specification.Height, m_Specification.Depth, Utils::DymaticTextureFormatToGLFormat(m_Specification.Format), Utils::DymaticTextureFormatToGLType(m_Specification.Format), m_Specification.Width * m_Specification.Height * m_Specification.Depth * bpp, data);
	}

	Buffer OpenGLTexture3D::GetData()
	{
		DY_PROFILE_FUNCTION();

		const uint32_t bpp = Utils::GetDymaticTextureFormatBPP(m_Specification.Format);
		Buffer buffer(m_Specification.Width * m_Specification.Height * bpp);

		GetData(buffer.Data, buffer.Size);

		return buffer;
	}

	void OpenGLTexture3D::SetData(const void* data, uint32_t size)
	{
		DY_PROFILE_FUNCTION();

		uint32_t bpp = Utils::GetDymaticTextureFormatBPP(m_Specification.Format);
		DY_CORE_ASSERT(size == m_Specification.Width * m_Specification.Height * m_Specification.Depth * bpp, "Data must be entire texture!");
		glTextureSubImage3D(m_RendererID, 0, 0, 0, 0, m_Specification.Width, m_Specification.Height, m_Specification.Depth, Utils::DymaticTextureFormatToGLFormat(m_Specification.Format), Utils::DymaticTextureFormatToGLType(m_Specification.Format), data);
		
		if (m_Specification.GenerateMips)
			glGenerateTextureMipmap(m_RendererID);
	}

	void OpenGLTexture3D::SetData(const Buffer& buffer)
	{
		SetData(buffer.Data, buffer.Size);
	}

	void OpenGLTexture3D::Bind(uint32_t slot) const
	{
		DY_PROFILE_FUNCTION();

		glBindTextureUnit(slot, m_RendererID);
	}

	void OpenGLTexture3D::BindTexture(uint32_t slot, int level, bool layered, int layer) const
	{
		DY_PROFILE_FUNCTION();

		glBindImageTexture(slot, m_RendererID, level, layered, layer, GL_READ_WRITE, Utils::DymaticTextureFormatToGLInternalFormat(m_Specification.Format));
	}

	uint32_t OpenGLTexture3D::GetMipLevels() const
	{
		return m_Specification.GenerateMips ? (std::floor(std::log2(std::max({ m_Specification.Width, m_Specification.Height, m_Specification.Depth }))) + 1) : 1;
	}

	glm::uvec3 OpenGLTexture3D::GetMipmapLevelSize(uint32_t level) const
	{
		glm::ivec3 size = glm::ivec3(m_Specification.Width, m_Specification.Height, m_Specification.Depth) / (1 << level);
		return glm::max(size, glm::ivec3(1));
	}

	//-----------------------------------------------------------------------------
	// Texture Cube
	//-----------------------------------------------------------------------------

	OpenGLTextureCube::OpenGLTextureCube(const TextureSpecification& specification, const Buffer& imageData)
		: m_Specification(specification)
	{
		DY_PROFILE_FUNCTION();

		DY_CORE_ASSERT(m_Specification.Format != TextureFormat::None, "Format not supported!");
		
		glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_RendererID);
		glTextureStorage2D(m_RendererID, m_Specification.Depth, Utils::DymaticTextureFormatToGLInternalFormat(m_Specification.Format), m_Specification.Width, m_Specification.Height);
		
		glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

		glGenerateTextureMipmap(m_RendererID);

		if (imageData)
			SetData(imageData);

		// Allow for bindless texture usage
		m_Handle = glGetTextureHandleARB(m_RendererID);
		glMakeTextureHandleResidentARB(m_Handle);
	}

	OpenGLTextureCube::~OpenGLTextureCube()
	{
		DY_PROFILE_FUNCTION();

		// Release the bindless texture
		glMakeTextureHandleNonResidentARB(m_Handle);

		glDeleteTextures(1, &m_RendererID);
	}

	void OpenGLTextureCube::GetData(void* data, uint32_t size)
	{
	}

	Buffer OpenGLTextureCube::GetData()
	{
		DY_CORE_ASSERT(false);
		return Buffer();
	}

	void OpenGLTextureCube::SetData(const void* data, uint32_t size)
	{
		//glTextureSubImage3D(m_RendererID, 0, 0, 0, i, m_Width, m_Height, 1, Utils::DymaticTextureFormatToGLFormat(m_Format), Utils::DymaticTextureFormatToGLType(m_Format),  data);
	}

	void OpenGLTextureCube::SetData(const Buffer& buffer)
	{
		DY_PROFILE_FUNCTION();

		SetData(buffer.Data, buffer.Size);
	}

	void OpenGLTextureCube::Bind(uint32_t slot) const
	{
		DY_PROFILE_FUNCTION();

		glBindTextureUnit(slot, m_RendererID);
	}

	void OpenGLTextureCube::BindTexture(uint32_t slot, int level, bool layered, int layer) const
	{
	}

}
