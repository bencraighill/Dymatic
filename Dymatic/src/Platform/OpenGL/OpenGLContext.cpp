#include "dypch.h"
#include "Platform/OpenGL/OpenGLContext.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

// Instruct AMD and NVIDIA graphics drivers to use the dedicated GPU if one exists
extern "C" {
	_declspec(dllexport) DWORD NvOptimusEnablement = 1;
	_declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

namespace Dymatic {

	OpenGLContext::OpenGLContext(GLFWwindow* windowHandle)
		: m_WindowHandle(windowHandle)
	{
		DY_CORE_ASSERT(windowHandle, "Window handle is null!")
	}

	void OpenGLContext::Init()
	{
		DY_PROFILE_FUNCTION();

		DY_CORE_INFO("Creating OpenGL Context");

		glfwMakeContextCurrent(m_WindowHandle);
		int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
		DY_CORE_ASSERT(status, "Failed to initialize Glad!");

		DY_CORE_INFO("OpenGL Info:");
		DY_CORE_INFO("  Vendor: {}", (const char*)glGetString(GL_VENDOR));
		DY_CORE_INFO("  Renderer: {}", (const char*)glGetString(GL_RENDERER));
		DY_CORE_INFO("  Version: {}", (const char*)glGetString(GL_VERSION));

		DY_CORE_ASSERT(GLVersion.major > 4 || (GLVersion.major == 4 && GLVersion.minor >= 6), "Dymatic requires at least OpenGL version 4.6");

		// Get Device Limits
		GLint maxTextureImageUnits, maxShaderStorageBufferBindings, maxUniformBufferBindings, maxCombinedTextureImageUnits, maxTessellationGenerationLevel;
		glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxTextureImageUnits);
		glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &maxUniformBufferBindings);
		glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &maxShaderStorageBufferBindings);
		glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxCombinedTextureImageUnits);
		glGetIntegerv(GL_MAX_TESS_GEN_LEVEL, &maxTessellationGenerationLevel);
		
		DY_CORE_INFO("  Device Limits:");
		DY_CORE_INFO("    Max Texture Image Units: {}", maxTextureImageUnits);
		DY_CORE_INFO("    Max Uniform Buffer Bindings: {}", maxUniformBufferBindings);
		DY_CORE_INFO("    Max Shader Storage Buffer Bindings: {}", maxShaderStorageBufferBindings);
		DY_CORE_INFO("    Max Combined Texture Image Units: {}", maxCombinedTextureImageUnits);
		DY_CORE_INFO("    Max Tessellation Generation Subdivisions: {}", maxTessellationGenerationLevel);

		// Get available extensions
		GLint extensionCount = 0;
		glGetIntegerv(GL_NUM_EXTENSIONS, &extensionCount);

		DY_CORE_INFO("  Supported Extensions ({}):", extensionCount);
		for (GLint extension = 0; extension < extensionCount; extension++)
			DY_CORE_INFO("    - {}", (const char*)glGetStringi(GL_EXTENSIONS, extension));
	}

	void OpenGLContext::Shutdown()
	{
		DY_PROFILE_FUNCTION();
	}

	void OpenGLContext::SwapBuffers()
	{
		DY_PROFILE_FUNCTION();

		glfwSwapBuffers(m_WindowHandle);
	}

}