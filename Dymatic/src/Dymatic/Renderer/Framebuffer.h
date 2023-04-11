#pragma once

#include "Dymatic/Core/Base.h"
#include "Dymatic/Renderer/TextureFormat.h"

namespace Dymatic {

	enum class TextureTarget
	{
		None = 0,

		TEXTURE_2D,
		TEXTURE_CUBE_MAP
	};

	enum class FramebufferTextureTarget
	{
		None = 0,

		TEXTURE_2D,

		CUBE_MAP_POSITIVE_X,
		CUBE_MAP_NEGATIVE_X,
		CUBE_MAP_POSITIVE_Y,
		CUBE_MAP_NEGATIVE_Y,
		CUBE_MAP_POSITIVE_Z,
		CUBE_MAP_NEGATIVE_Z
	};

	struct FramebufferTextureSpecification
	{
		FramebufferTextureSpecification() = default;
		FramebufferTextureSpecification(TextureFormat format)
			: TextureFormat(format) {}
		FramebufferTextureSpecification(TextureFormat format, FramebufferTextureTarget target)
			: TextureFormat(format), TextureTarget(target) {}

		TextureFormat TextureFormat = TextureFormat::None;
		FramebufferTextureTarget TextureTarget = FramebufferTextureTarget::TEXTURE_2D;
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

		TextureTarget Target = TextureTarget::TEXTURE_2D;

		bool SwapChainTarget = false;
	};

	class Framebuffer
	{
	public:
		virtual ~Framebuffer() = default;

		virtual void Bind() = 0;
		virtual void Unbind() = 0;

		virtual uint32_t GetRendererID() const = 0;

		virtual void Resize(uint32_t width, uint32_t height) = 0;
		virtual void ReadPixel(uint32_t attachmentIndex, int x, int y, void* pixelData) = 0;
		virtual void ReadPixels(uint32_t attachmentIndex, int x, int y, int width, int height, void* pixelData) = 0;
		virtual float ReadDepthPixel(int x, int y) = 0;

		virtual void Copy(uint32_t target) = 0;
		virtual void Copy(Ref<Framebuffer> target) = 0;

		virtual void ClearAttachment(uint32_t attachmentIndex, const void* value) = 0;

		virtual uint32_t GetColorAttachmentRendererID(uint32_t index = 0) const = 0;
		virtual uint32_t GetDepthAttachmentRendererID() const = 0;

		virtual void BindColorSampler(uint32_t slot, uint32_t index = 0) const = 0;
		virtual void BindDepthSampler(uint32_t slot) const = 0;

		virtual void BindColorTexture(uint32_t slot, uint32_t index = 0) const = 0;
		virtual void BindDepthTexture(uint32_t slot) const = 0;

		virtual void SetTarget(TextureTarget target) = 0;
		virtual void SetAttachmentTarget(uint32_t index, FramebufferTextureTarget target, uint32_t mip = 0) = 0;

		virtual const FramebufferSpecification& GetSpecification() const = 0;

		static Ref<Framebuffer> Create(const FramebufferSpecification& spec);
	};

}
