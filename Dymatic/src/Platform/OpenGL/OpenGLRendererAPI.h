#pragma once

#include "Dymatic/Renderer/RendererAPI.h"

namespace Dymatic {

	class OpenGLRendererAPI : public RendererAPI
	{
	public:
		virtual void Init() override;
		virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;

		virtual void SetClearColor(const glm::vec4& color) override;
		virtual void SetWireframe(const bool wireframe) override;
		virtual void SetDepthTest(const bool enabled) override;
		virtual void UnbindFramebuffers() override;
		virtual void Clear() override;

		virtual void DrawIndexed(const Ref<VertexArray>& vertexArray, uint32_t indexCount = 0) override;
		virtual void DrawIndexedPatches(const Ref<VertexArray>& vertexArray, uint32_t indexCount = 0) override;
		virtual void DrawIndexedInstanced(const Ref<VertexArray>& vertexArray, uint32_t indexCount = 0, uint32_t instanceCount = 0) override;
		virtual void DrawLines(const Ref<VertexArray>& vertexArray, uint32_t vertexCount) override;
		virtual void DrawPoints(const Ref<VertexArray>& vertexArray, uint32_t vertexCount) override;

		virtual void SetLineWidth(float width) override;
		virtual void SetPointSize(float size) override;
		
		virtual void SetTextureAlignment(uint32_t alignment) override;

		virtual void SetBlendFunction(BlendFunction source, BlendFunction destination) override;
		virtual void ResetBlendFunction() override;
		
		// Queries
		virtual bool IsExtensionSupported(const std::string& extenison) const override;
		virtual GraphicsVendor::GraphicsVendorType GetGraphicsVendor() const override;
	};

}