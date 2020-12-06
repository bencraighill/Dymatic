#include "dypch.h"
#include "Platform/OpenGL/OpenGLContext.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

namespace Dymatic {

	OpenGLContext::OpenGLContext(GLFWwindow* windowHandle)
		: m_WindowHandle(windowHandle)
	{
		DY_CORE_ASSERT(windowHandle, "Window handle is null!")
	}

	void OpenGLContext::Init()
	{
		DY_PROFILE_FUNCTION();

		glfwMakeContextCurrent(m_WindowHandle);
		int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
		DY_CORE_ASSERT(status, "Failed to initialize Glad!");

		DY_CORE_INFO("OpenGL Info:");
		DY_CORE_INFO("  Vendor: {0}", glGetString(GL_VENDOR));
		DY_CORE_INFO("  Renderer: {0}", glGetString(GL_RENDERER));
		DY_CORE_INFO("  Version: {0}", glGetString(GL_VERSION));

		DY_CORE_ASSERT(GLVersion.major > 4 || (GLVersion.major == 4 && GLVersion.minor >= 5), "Dymatic requires at least OpenGL version 4.5");
	}

	void OpenGLContext::SwapBuffers()
	{
		DY_PROFILE_FUNCTION();

		glfwSwapBuffers(m_WindowHandle);
	}

}