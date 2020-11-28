#include "dypch.h"
#include "OpenGLContext.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <GL/GL.h>

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

		DY_CORE_INFO("OpenGL Renderer in use: \n		Vendor: {0}\n		Renderer: {1}\n		Version: {2}\n", glGetString(GL_VENDOR), glGetString(GL_RENDERER), glGetString(GL_VERSION));

	#ifdef DY_ENABLE_ASSERTS
		int versionMajor;
		int versionMinor;
		glGetIntegerv(GL_MAJOR_VERSION, &versionMajor);
		glGetIntegerv(GL_MINOR_VERSION, &versionMinor);

		DY_CORE_ASSERT(versionMajor > 4 || (versionMajor == 4 && versionMinor >= 5), "Dymatic requires OpenGL version 4.5 or later");
	#endif
	}

	void OpenGLContext::SwapBuffers()
	{
		DY_PROFILE_FUNCTION();

		glfwSwapBuffers(m_WindowHandle);
	}

}