#pragma once

#include "Dymatic/Renderer/Framebuffer.h"

namespace Dymatic {

	class OpenGLFramebuffer : public Framebuffer
	{
	public:
		OpenGLFramebuffer(const FramebufferSpecification& spec);
		virtual ~OpenGLFramebuffer();

		void Invalidate();

		virtual void Bind() override;
		virtual void Unbind() override;

		virtual uint32_t GetRendererID() const override { return m_RendererID; }

		virtual void Resize(uint32_t width, uint32_t height) override;
		virtual void ReadPixel(uint32_t attachmentIndex, int x, int y, void* pixelData) override;
		virtual float ReadDepthPixel(int x, int y) override;

		virtual void ClearAttachment(uint32_t attachmentIndex, const void* value) override;

		virtual uint32_t GetColorAttachmentRendererID(uint32_t index = 0) const override { DY_CORE_ASSERT(index < m_ColorAttachments.size()); return m_ColorAttachments[index]; }
		virtual uint32_t GetDepthAttachmentRendererID() const override { return m_DepthAttachment; }

		virtual void BindColorSampler(uint32_t slot, uint32_t index = 0) const override;
		virtual void BindDepthSampler(uint32_t slot) const override;

		virtual void BindColorTexture(uint32_t slot, uint32_t index = 0) const override;
		virtual void BindDepthTexture(uint32_t slot) const override;

		virtual void OpenGLFramebuffer::SetAttachmentTarget(uint32_t index, FramebufferTextureTarget target, uint32_t mip) override;
		virtual void OpenGLFramebuffer::SetTarget(TextureTarget target) override;

		virtual const FramebufferSpecification& GetSpecification() const override { return m_Specification; };
	private:
		uint32_t m_RendererID = 0;
		FramebufferSpecification m_Specification;

		std::vector<FramebufferTextureSpecification> m_ColorAttachmentSpecifications;
		FramebufferTextureSpecification m_DepthAttachmentSpecification = TextureFormat::None;

		std::vector<uint32_t> m_ColorAttachments;
		uint32_t m_DepthAttachment = 0;
	};

}
