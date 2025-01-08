#pragma once

#include <glm/glm.hpp>

#include "Dymatic/Renderer/VertexArray.h"
#include "Dymatic/Renderer/GraphicsVendor.h"

namespace Dymatic {

	enum class BlendFunction
	{
		Zero,
		One,
		SourceColor,
		OneMinusSourceColor,
		DestinationColor,
		OneMinusDestinationColor,
		SourceAlpha,
		OneMinusSourceAlpha,
		DestinationAlpha,
		OneMinusDestinationAlpha,
		ConstantColor,
		OneMinusConstantColor,
		ConstantAlpha,
		OneMinusConstantAlpha,
		SourceAlphaSaturate,
		Source1Color,
		OneMinusSource1Color,
		Source1Alpha,
		OneMinusSource1Alpha,
	};

	class RendererAPI
	{
	public:
		enum class API
		{
			None = 0, OpenGL = 1, Vulkan = 2
		};
	public:
		virtual ~RendererAPI() = default;

		virtual void Init() = 0;
		virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;
		virtual void SetClearColor(const glm::vec4& color) = 0;
		virtual void SetWireframe(const bool wireframe) = 0;
		virtual void SetDepthTest(const bool enabled) = 0;
		virtual void UnbindFramebuffers() = 0;
		virtual void Clear() = 0;

		virtual void DrawIndexed(const Ref<VertexArray>& vertexArray, uint32_t indexCount = 0) = 0;
		virtual void DrawIndexedPatches(const Ref<VertexArray>& vertexArray, uint32_t indexCount = 0) = 0;
		virtual void DrawIndexedInstanced(const Ref<VertexArray>& vertexArray, uint32_t indexCount = 0, uint32_t instanceCount = 0) = 0;
		virtual void DrawLines(const Ref<VertexArray>& vertexArray, uint32_t vertexCount) = 0;
		virtual void DrawPoints(const Ref<VertexArray>& vertexArray, uint32_t vertexCount) = 0;

		virtual void SetLineWidth(float width) = 0;
		virtual void SetPointSize(float size) = 0;

		virtual void SetTextureAlignment(uint32_t alignment) = 0;

		virtual void SetBlendFunction(BlendFunction source, BlendFunction destination) = 0;
		virtual void ResetBlendFunction() = 0;

		// Queries
		virtual bool IsExtensionSupported(const std::string& extenison) const = 0;
		virtual GraphicsVendor::GraphicsVendorType GetGraphicsVendor() const = 0;

		static API GetAPI() { return s_API; }
		static Scope<RendererAPI> Create();
	private:
		static API s_API;
	};

}
