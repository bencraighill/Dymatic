#pragma once

#include "Dymatic/Core/Base.h"

namespace Dymatic {

	enum class FramebufferTextureFormat
	{
		None = 0,
	 
		// Color
		RGBA8,
		RGBA16F,
		RED_INTEGER,

		// Depth/Stencil
		DEPTH24STENCIL8,

		// Defaults
		Depth = DEPTH24STENCIL8
	};

	struct FramebufferTextureSpecification
	{
		FramebufferTextureSpecification() = default;
		FramebufferTextureSpecification(FramebufferTextureFormat format)
			: TextureFormat(format) {}

		FramebufferTextureFormat TextureFormat = FramebufferTextureFormat::None;
		// TODO: filtering/wrap
	};

	struct FramebufferAttachmentSpecification
	{
		FramebufferAttachmentSpecification() = default;
		FramebufferAttachmentSpecification(std::initializer_list<FramebufferTextureSpecification> attachments)
			: Attachments(attachments) {}

		std::vector<FramebufferTextureSpecification> Attachments;
	};

	struct FramebufferSpecification
	{
		uint32_t Width = 0, Height = 0;
		FramebufferAttachmentSpecification Attachments;
		uint32_t Samples = 1;

		bool SwapChainTarget = false;
	};

	class Framebuffer
	{
	public:
		virtual ~Framebuffer() = default;

		virtual void Bind() = 0;
		virtual void Unbind() = 0;

		virtual void Resize(uint32_t width, uint32_t height) = 0;
		virtual void ReadPixel(uint32_t attachmentIndex, int x, int y, void* pixelData) = 0;
		virtual float ReadDepthPixel(int x, int y) = 0;

		virtual void ClearAttachment(uint32_t attachmentIndex, const void* value) = 0;

		virtual uint32_t GetColorAttachmentRendererID(uint32_t index = 0) const = 0;
		virtual uint32_t GetDepthAttachmentRendererID() const = 0;

		virtual void BindColorSampler(uint32_t slot, uint32_t index = 0) const = 0;
		virtual void BindDepthSampler(uint32_t slot) const = 0;

		virtual void BindColorTexture(uint32_t slot, uint32_t index = 0) const = 0;
		virtual void BindDepthTexture(uint32_t slot) const = 0;

		virtual const FramebufferSpecification& GetSpecification() const = 0;

		static Ref<Framebuffer> Create(const FramebufferSpecification& spec);
	};

}
