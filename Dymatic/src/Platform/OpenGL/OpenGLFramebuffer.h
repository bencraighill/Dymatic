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

		virtual uint32_t GetWidth() const override { return m_Specification.Width; };
		virtual uint32_t GetHeight() const override { return m_Specification.Height; }
		virtual glm::uvec2 GetSize() const override { return glm::uvec2(m_Specification.Width, m_Specification.Height); }

		virtual void Resize(uint32_t width, uint32_t height) override;
		virtual void ReadPixel(uint32_t attachmentIndex, int x, int y, void* pixelData) override;
		virtual void ReadPixels(uint32_t attachmentIndex, int x, int y, int width, int height, void* pixelData) override;
		virtual float ReadDepthPixel(int x, int y) override;

		virtual Buffer CopyColorBuffer(uint32_t attachmentIndex) override;

		virtual void Copy(Ref<Framebuffer> target) override;
		// TODO: Add ability to specify an attachment rather than just copying the first index
		virtual void CopyColor(uint32_t target) override;
		virtual void CopyColor(Ref<Framebuffer> target) override;
		virtual void CopyColor(Ref<Texture2D> target) override;
		virtual void CopyDepth(Ref<Framebuffer> target) override;

		virtual void ClearAttachment(uint32_t attachmentIndex, const void* value) override;

		virtual uint32_t GetColorAttachmentRendererID(uint32_t index = 0) const override { DY_CORE_ASSERT(index < m_ColorAttachments.size()); return m_ColorAttachments[index]; }
		virtual uint32_t GetDepthAttachmentRendererID() const override { return m_DepthAttachment; }

		virtual void BindColorSampler(uint32_t slot, uint32_t index = 0) const override;
		virtual void BindDepthSampler(uint32_t slot) const override;

		virtual void BindColorTexture(uint32_t slot, uint32_t index = 0) const override;
		virtual void BindDepthTexture(uint32_t slot) const override;

		virtual uint64_t GetColorHandle(uint32_t index = 0) const override;
		virtual uint64_t GetDepthHandle() const override;

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

		std::vector<uint64_t> m_ColorHandles;
		uint64_t m_DepthHandle = 0;
	};

}
