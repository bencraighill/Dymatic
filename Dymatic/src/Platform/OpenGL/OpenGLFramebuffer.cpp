#include "dypch.h"

#include "Platform/OpenGL/OpenGLFramebuffer.h"
#include "Platform/OpenGL/OpenGLTextureFormat.h"

#include <glad/glad.h>

namespace Dymatic {

	static const uint32_t s_MaxFramebufferSize = 8192;

	namespace Utils {

		static GLenum GetFramebufferTextureTarget(bool multisampled, FramebufferTextureTarget target)
		{
			switch (target)
			{
			case FramebufferTextureTarget::TEXTURE_2D:
				return multisampled ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;

			case FramebufferTextureTarget::CUBE_MAP_POSITIVE_X:		return GL_TEXTURE_CUBE_MAP_POSITIVE_X;
			case FramebufferTextureTarget::CUBE_MAP_NEGATIVE_X:		return GL_TEXTURE_CUBE_MAP_NEGATIVE_X;
			case FramebufferTextureTarget::CUBE_MAP_POSITIVE_Y:		return GL_TEXTURE_CUBE_MAP_POSITIVE_Y;
			case FramebufferTextureTarget::CUBE_MAP_NEGATIVE_Y:		return GL_TEXTURE_CUBE_MAP_NEGATIVE_Y;
			case FramebufferTextureTarget::CUBE_MAP_POSITIVE_Z:		return GL_TEXTURE_CUBE_MAP_POSITIVE_Z;
			case FramebufferTextureTarget::CUBE_MAP_NEGATIVE_Z:		return GL_TEXTURE_CUBE_MAP_NEGATIVE_Z;
			}

			DY_CORE_ASSERT(false);
			return 0;
		}

		static GLenum GetTextureTarget(bool multisampled, TextureTarget target)
		{
			switch (target)
			{
			case TextureTarget::TEXTURE_2D:
				return multisampled ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;

			case TextureTarget::TEXTURE_CUBE_MAP:	return GL_TEXTURE_CUBE_MAP;
			}

			DY_CORE_ASSERT(false);
			return 0;
		}

		static void CreateTextures(bool multisampled, TextureTarget target, uint32_t* outID, uint32_t count)
		{
			glCreateTextures(GetTextureTarget(multisampled, target), count, outID);
		}

		static void BindTexture(bool multisampled, TextureTarget target, uint32_t id)
		{
			glBindTexture(GetTextureTarget(multisampled, target), id);
		}

		static void AttachColorTexture(uint32_t id, const FramebufferSpecification& spec, const FramebufferTextureSpecification& attachment, int index)
		{
			bool multisampled = spec.Samples > 1;

			GLenum target = GetFramebufferTextureTarget(multisampled, attachment.TextureTarget);

			if (multisampled)
			{
				glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, spec.Samples, Utils::DymaticTextureFormatToGLInternalFormat(attachment.TextureFormat), spec.Width, spec.Height, GL_FALSE);
			}
			else
			{
				glTexImage2D(target, 0, Utils::DymaticTextureFormatToGLInternalFormat(attachment.TextureFormat), spec.Width, spec.Height, 0, Utils::DymaticTextureFormatToGLFormat(attachment.TextureFormat), Utils::DymaticTextureFormatToGLType(attachment.TextureFormat), nullptr);

				glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
				glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			}

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, target, id, 0);
		}

		static void AttachDepthTexture(uint32_t id, GLenum attachmentType, const FramebufferSpecification& spec, const FramebufferTextureSpecification& attachment)
		{
			bool multisampled = spec.Samples > 1;

			GLenum target = GetFramebufferTextureTarget(multisampled, attachment.TextureTarget);

			if (multisampled)
			{
				glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, spec.Samples, Utils::DymaticTextureFormatToGLInternalFormat(attachment.TextureFormat), spec.Width, spec.Height, GL_FALSE);
			}
			else
			{
				glTexStorage2D(target, 1, Utils::DymaticTextureFormatToGLInternalFormat(attachment.TextureFormat), spec.Width, spec.Height);

				glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
				glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			}

			glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentType, target, id, 0);
		}
	}

	OpenGLFramebuffer::OpenGLFramebuffer(const FramebufferSpecification& spec)
		: m_Specification(spec)
	{
		for (auto format : m_Specification.Attachments.Attachments)
		{
			if (!Utils::IsDepthFormat(format.TextureFormat))
				m_ColorAttachmentSpecifications.emplace_back(format);
			else
				m_DepthAttachmentSpecification = format;
		}

		Invalidate();
	}

	OpenGLFramebuffer::~OpenGLFramebuffer()
	{
		glDeleteFramebuffers(1, &m_RendererID);
		glDeleteTextures(m_ColorAttachments.size(), m_ColorAttachments.data());
		glDeleteTextures(1, &m_DepthAttachment);
	}

	void OpenGLFramebuffer::Invalidate()
	{
		if (m_RendererID)
		{
			glDeleteFramebuffers(1, &m_RendererID);
			glDeleteTextures(m_ColorAttachments.size(), m_ColorAttachments.data());
			glDeleteTextures(1, &m_DepthAttachment);

			m_ColorAttachments.clear();
			m_DepthAttachment = 0;
		}

		glCreateFramebuffers(1, &m_RendererID);
		glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);

		bool multisample = m_Specification.Samples > 1;

		// Attachments
		if (m_ColorAttachmentSpecifications.size())
		{
			m_ColorAttachments.resize(m_ColorAttachmentSpecifications.size());
			Utils::CreateTextures(multisample, m_Specification.Target, m_ColorAttachments.data(), m_ColorAttachments.size());

			for (size_t i = 0; i < m_ColorAttachments.size(); i++)
			{
				Utils::BindTexture(multisample, m_Specification.Target, m_ColorAttachments[i]);
				Utils::AttachColorTexture(m_ColorAttachments[i], m_Specification, m_ColorAttachmentSpecifications[i], i);
			}
		}

		if (m_DepthAttachmentSpecification.TextureFormat != TextureFormat::None)
		{
			Utils::CreateTextures(multisample, m_Specification.Target, &m_DepthAttachment, 1);
			Utils::BindTexture(multisample, m_Specification.Target, m_DepthAttachment);
			switch (m_DepthAttachmentSpecification.TextureFormat)
			{
				case TextureFormat::DEPTH24STENCIL8:
					Utils::AttachDepthTexture(m_DepthAttachment, GL_DEPTH_STENCIL_ATTACHMENT, m_Specification, m_DepthAttachmentSpecification);
					break;
			}
		}

		if (m_ColorAttachments.size() > 1)
		{
			DY_CORE_ASSERT(m_ColorAttachments.size() <= 7);
			GLenum buffers[7] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4, GL_COLOR_ATTACHMENT5, GL_COLOR_ATTACHMENT6 };
			glDrawBuffers(m_ColorAttachments.size(), buffers);
		}
		else if (m_ColorAttachments.empty())
		{
			// Only depth pass
			glDrawBuffer(GL_NONE);
		}

		DY_CORE_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Framebuffer is incomplete!");

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void OpenGLFramebuffer::Bind()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);
		glViewport(0, 0, m_Specification.Width, m_Specification.Height);
	}

	void OpenGLFramebuffer::Unbind()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void OpenGLFramebuffer::Resize(uint32_t width, uint32_t height)
	{
		if (width == 0 || height == 0 || width > s_MaxFramebufferSize || height > s_MaxFramebufferSize)
		{
			DY_CORE_WARN("Attempted to rezize framebuffer to {0}, {1}", width, height);
			return;
		}
		m_Specification.Width = width;
		m_Specification.Height = height;

		Invalidate();
	}

	void OpenGLFramebuffer::ReadPixel(uint32_t attachmentIndex, int x, int y, void* pixelData)
	{
		DY_CORE_ASSERT(attachmentIndex < m_ColorAttachments.size());

		auto& spec = m_ColorAttachmentSpecifications[attachmentIndex];
		glReadBuffer(GL_COLOR_ATTACHMENT0 + attachmentIndex);
		glReadPixels(x, y, 1, 1, Utils::DymaticTextureFormatToGLFormat(spec.TextureFormat), Utils::DymaticTextureFormatToGLType(spec.TextureFormat), pixelData);
	}

	float OpenGLFramebuffer::ReadDepthPixel(int x, int y)
	{
		auto& spec = m_DepthAttachmentSpecification;
		glReadBuffer(GL_DEPTH_ATTACHMENT);
		float pixelData;
		glReadPixels(x, y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &pixelData);
		return pixelData;
	}

	void OpenGLFramebuffer::ClearAttachment(uint32_t attachmentIndex, const void* value)
	{
		DY_CORE_ASSERT(attachmentIndex < m_ColorAttachments.size());

		auto& spec = m_ColorAttachmentSpecifications[attachmentIndex];
		glClearTexImage(m_ColorAttachments[attachmentIndex], 0,
			Utils::DymaticTextureFormatToGLFormat(spec.TextureFormat), Utils::DymaticTextureFormatToGLType(spec.TextureFormat), value);
	}

	void OpenGLFramebuffer::BindColorSampler(uint32_t slot, uint32_t index) const
	{
		glBindTextureUnit(slot, GetColorAttachmentRendererID(index));
	}

	void OpenGLFramebuffer::BindDepthSampler(uint32_t slot) const
	{
		glBindTextureUnit(slot, GetDepthAttachmentRendererID());
	}

	void OpenGLFramebuffer::BindColorTexture(uint32_t slot, uint32_t index) const
	{
		glBindImageTexture(slot, GetColorAttachmentRendererID(index), 0, GL_FALSE, 0, GL_READ_WRITE, Utils::DymaticTextureFormatToGLInternalFormat(m_ColorAttachmentSpecifications[index].TextureFormat));
	}

	void OpenGLFramebuffer::BindDepthTexture(uint32_t slot) const
	{
		glBindImageTexture(slot, GetDepthAttachmentRendererID(), 0, GL_FALSE, 0, GL_READ_WRITE, Utils::DymaticTextureFormatToGLInternalFormat(m_DepthAttachmentSpecification.TextureFormat));
	}

	void OpenGLFramebuffer::SetAttachmentTarget(uint32_t index, FramebufferTextureTarget target, uint32_t mip)
	{
		m_ColorAttachmentSpecifications[index].TextureTarget = target;
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index,Utils::GetFramebufferTextureTarget(m_Specification.Samples > 1, target), m_ColorAttachments[index], mip);
	}

	void OpenGLFramebuffer::SetTarget(TextureTarget target)
	{
		m_Specification.Target = target;
		Invalidate();
	}

}
