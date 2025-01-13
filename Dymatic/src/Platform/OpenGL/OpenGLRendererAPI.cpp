#include "dypch.h"
#include "Platform/OpenGL/OpenGLRendererAPI.h"

#include <glad/glad.h>

#include <set>
#include <string>

namespace Dymatic {

	static std::set<std::string> s_SupportedExtensions;

	namespace Utils {

		void OpenGLMessageCallback(
			unsigned source,
			unsigned type,
			unsigned id,
			unsigned severity,
			int length,
			const char* message,
			const void* userParam)
		{
			switch (severity)
			{
			case GL_DEBUG_SEVERITY_HIGH:         DY_CORE_CRITICAL(message); return;
			case GL_DEBUG_SEVERITY_MEDIUM:       DY_CORE_ERROR(message); return;
			case GL_DEBUG_SEVERITY_LOW:          DY_CORE_WARN(message); return;
			case GL_DEBUG_SEVERITY_NOTIFICATION: DY_CORE_TRACE(message); return;
			}

			DY_CORE_ASSERT(false, "Unknown severity level!");
		}

		static GLenum DymaticBlendFunctionToGL(BlendFunction function)
		{
			switch (function)
			{
				case BlendFunction::Zero:						return GL_ZERO;
				case BlendFunction::One:						return GL_ONE;
				case BlendFunction::SourceColor:				return GL_SRC_COLOR;
				case BlendFunction::OneMinusSourceColor:		return GL_ONE_MINUS_SRC_COLOR;
				case BlendFunction::DestinationColor:			return GL_DST_COLOR;
				case BlendFunction::OneMinusDestinationColor:	return GL_ONE_MINUS_DST_COLOR;
				case BlendFunction::SourceAlpha:				return GL_SRC_ALPHA;
				case BlendFunction::OneMinusSourceAlpha:		return GL_ONE_MINUS_SRC_ALPHA;
				case BlendFunction::DestinationAlpha:			return GL_DST_ALPHA;
				case BlendFunction::OneMinusDestinationAlpha:	return GL_ONE_MINUS_DST_ALPHA;
				case BlendFunction::ConstantColor:				return GL_CONSTANT_COLOR;
				case BlendFunction::OneMinusConstantColor:		return GL_ONE_MINUS_CONSTANT_COLOR;
				case BlendFunction::ConstantAlpha:				return GL_CONSTANT_ALPHA;
				case BlendFunction::OneMinusConstantAlpha:		return GL_ONE_MINUS_CONSTANT_ALPHA;
				case BlendFunction::SourceAlphaSaturate:		return GL_SRC_ALPHA_SATURATE;
				case BlendFunction::Source1Color:				return GL_SRC1_COLOR;
				case BlendFunction::OneMinusSource1Color:		return GL_ONE_MINUS_SRC1_COLOR;
				case BlendFunction::Source1Alpha:				return GL_SRC1_ALPHA;
				case BlendFunction::OneMinusSource1Alpha:		return GL_ONE_MINUS_SRC1_ALPHA;
			}

			return GL_ZERO;
		}

	}

	void OpenGLRendererAPI::Init()
	{
		DY_PROFILE_FUNCTION();

		DY_CORE_INFO("Initializing OpenGL Renderer API...");

#ifdef DY_DEBUG
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(Utils::OpenGLMessageCallback, nullptr);

		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, GL_FALSE);
#endif

		glEnable(GL_BLEND);
		ResetBlendFunction();

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_LINE_SMOOTH);

		glEnable(GL_CULL_FACE);

		glEnable(GL_STENCIL_TEST);
		glStencilMask(0x00);
		glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
		glStencilOp(GL_KEEP, GL_REPLACE, GL_REPLACE);

		glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

		// Use triangles for tessellation
		glPatchParameteri(GL_PATCH_VERTICES, 3);

		// Get a list of supported extensions
		GLint numExtensions;
		glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);

		for (GLint i = 0; i < numExtensions; i++)
		{
			const char* extension = reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, i));
			s_SupportedExtensions.insert(extension);
		}
	}

	void OpenGLRendererAPI::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
	{
		glViewport(x, y, width, height);
	}

	void OpenGLRendererAPI::SetClearColor(const glm::vec4& color)
	{
		glClearColor(color.r, color.g, color.b, color.a);
	}

	void OpenGLRendererAPI::SetWireframe(const bool wireframe)
	{
		glPolygonMode(GL_FRONT_AND_BACK, (wireframe ? GL_LINE : GL_FILL));
	}

	void OpenGLRendererAPI::SetDepthTest(const bool enabled)
	{
		if (enabled)
			glEnable(GL_DEPTH_TEST);
		else
			glDisable(GL_DEPTH_TEST);
	}

	void OpenGLRendererAPI::UnbindFramebuffers()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void OpenGLRendererAPI::Clear()
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	}

	void OpenGLRendererAPI::DrawIndexed(const Ref<VertexArray>& vertexArray, uint32_t indexCount)
	{
		vertexArray->Bind();
		uint32_t count = indexCount ? indexCount : vertexArray->GetIndexBuffer()->GetCount();
		glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr);
	}

	void OpenGLRendererAPI::DrawIndexedPatches(const Ref<VertexArray>& vertexArray, uint32_t indexCount)
	{
		vertexArray->Bind();
		uint32_t count = indexCount ? indexCount : vertexArray->GetIndexBuffer()->GetCount();
		glDrawElements(GL_PATCHES, count, GL_UNSIGNED_INT, nullptr);
	}

	void OpenGLRendererAPI::DrawIndexedInstanced(const Ref<VertexArray>& vertexArray, uint32_t indexCount, uint32_t instanceCount)
	{
		vertexArray->Bind();
		uint32_t count = indexCount ? indexCount : vertexArray->GetIndexBuffer()->GetCount();
		glDrawElementsInstanced(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr, instanceCount);
	}

	void OpenGLRendererAPI::DrawLines(const Ref<VertexArray>& vertexArray, uint32_t vertexCount)
	{
		vertexArray->Bind();
		glDrawArrays(GL_LINES, 0, vertexCount);
	}

	void OpenGLRendererAPI::DrawPoints(const Ref<VertexArray>& vertexArray, uint32_t vertexCount)
	{
		vertexArray->Bind();
		glDrawArrays(GL_POINTS, 0, vertexCount);
	}

	void OpenGLRendererAPI::SetLineWidth(float width)
	{
		glLineWidth(width);
	}

	void OpenGLRendererAPI::SetPointSize(float size)
	{
		glPointSize(size);
	}

	void OpenGLRendererAPI::SetTextureAlignment(uint32_t alignment)
	{
		glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
	}

	void OpenGLRendererAPI::SetBlendFunction(BlendFunction source, BlendFunction destination)
	{
		glBlendFunc(Utils::DymaticBlendFunctionToGL(source), Utils::DymaticBlendFunctionToGL(destination));
	}

	void OpenGLRendererAPI::ResetBlendFunction()
	{
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	bool OpenGLRendererAPI::IsExtensionSupported(const std::string& extenison) const
	{
		return s_SupportedExtensions.find(extenison) != s_SupportedExtensions.end();
	}

	GraphicsVendor::GraphicsVendorType OpenGLRendererAPI::GetGraphicsVendor() const
	{
		const char* vendor = (const char*)glGetString(GL_VENDOR);
		return GraphicsVendor::GraphicsVendorFromString(vendor);
	}

}
