#pragma once

#include "Dymatic/Renderer/RendererAPI.h"

namespace Dymatic {

	class RenderCommand
	{
	public:
		static void Init()
		{
			s_RendererAPI->Init();
		}

		static void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
		{
			s_RendererAPI->SetViewport(x, y, width, height);
		}

		static void SetClearColor(const glm::vec4& color)
		{
			s_RendererAPI->SetClearColor(color);
		}

		static void SetWireframe(const bool wireframe)
		{
			s_RendererAPI->SetWireframe(wireframe);
		}

		static void SetDepthTest(const bool enabled)
		{
			s_RendererAPI->SetDepthTest(enabled);
		}

		static void UnbindFramebuffers()
		{
			s_RendererAPI->UnbindFramebuffers();
		}

		static void Clear()
		{
			s_RendererAPI->Clear();
		}

		static void DrawIndexed(const Ref<VertexArray>& vertexArray, uint32_t indexCount = 0)
		{
			s_RendererAPI->DrawIndexed(vertexArray, indexCount);
		}

		static void DrawIndexedPatches(const Ref<VertexArray>& vertexArray, uint32_t indexCount = 0)
		{
			s_RendererAPI->DrawIndexedPatches(vertexArray, indexCount);
		}

		static void DrawIndexedInstanced(const Ref<VertexArray>& vertexArray, uint32_t indexCount = 0, uint32_t instanceCount = 0)
		{
			s_RendererAPI->DrawIndexedInstanced(vertexArray, indexCount, instanceCount);
		}

		static void DrawLines(const Ref<VertexArray>& vertexArray, uint32_t vertexCount)
		{
			s_RendererAPI->DrawLines(vertexArray, vertexCount);
		}

		static void DrawPoints(const Ref<VertexArray>& vertexArray, uint32_t vertexCount)
		{
			s_RendererAPI->DrawPoints(vertexArray, vertexCount);
		}

		static void SetLineWidth(float width)
		{
			s_RendererAPI->SetLineWidth(width);
		}

		static void SetPointSize(float size)
		{
			s_RendererAPI->SetPointSize(size);
		}

		static void SetTextureAlignment(uint32_t alignment)
		{
			s_RendererAPI->SetTextureAlignment(alignment);
		}

		static void SetBlendFunction(BlendFunction source, BlendFunction destination)
		{
			s_RendererAPI->SetBlendFunction(source, destination);
		}

		static void ResetBlendFunction()
		{
			s_RendererAPI->ResetBlendFunction();
		}

		// Queries
		static GraphicsVendor::GraphicsVendorType GetGraphicsVendor()
		{
			return s_RendererAPI->GetGraphicsVendor();
		}

		static bool IsExtensionSupported(const std::string& extenison)
		{
			return s_RendererAPI->IsExtensionSupported(extenison);
		}
	private:
		static Scope<RendererAPI> s_RendererAPI;
	};

}
