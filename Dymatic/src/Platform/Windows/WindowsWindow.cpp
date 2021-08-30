#include "dypch.h"
#include "Platform/Windows/WindowsWindow.h"

#include "Dymatic/Core/Input.h"

#include "Dymatic/Events/ApplicationEvent.h"
#include "Dymatic/Events/MouseEvent.h"
#include "Dymatic/Events/KeyEvent.h"

#include "Dymatic/Renderer/Renderer.h"

#include "Platform/OpenGL/OpenGLContext.h"

#include "Dymatic/Renderer/Texture.h"

#include <stb_image.h>

namespace Dymatic {

	static uint8_t s_GLFWWindowCount = 0;

	static void GLFWErrorCallback(int error, const char* description)
	{
		DY_CORE_ERROR("GLFW Error ({0}): {1}", error, description);
	}

	WindowsWindow::WindowsWindow(const WindowProps& props)
	{
		DY_PROFILE_FUNCTION();

		Init(props);
	}

	WindowsWindow::~WindowsWindow()
	{
		DY_PROFILE_FUNCTION();

		Shutdown();
	}

	void WindowsWindow::Init(const WindowProps& props)
	{
		DY_PROFILE_FUNCTION();

		m_Data.Title = props.Title;
		m_Data.Width = props.Width;
		m_Data.Height = props.Height;

		DY_CORE_INFO("Creating window {0} ({1}, {2})", props.Title, props.Width, props.Height);

		if (s_GLFWWindowCount == 0)
		{
			DY_PROFILE_SCOPE("glfwInit");
			int success = glfwInit();
			DY_CORE_ASSERT(success, "Could not initialize GLFW!");
			glfwSetErrorCallback(GLFWErrorCallback);

		}

		{
			DY_PROFILE_SCOPE("glfwCreateWindow");
#if defined(DY_DEBUG)
			if (Renderer::GetAPI() == RendererAPI::API::OpenGL)
				glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif
			glfwWindowHint(GLFW_DECORATED, false);
			m_Window = glfwCreateWindow((int)props.Width, (int)props.Height, m_Data.Title.c_str(), nullptr, nullptr);
			++s_GLFWWindowCount;

			GLFWimage images[1];
			images[0].pixels = stbi_load("assets/icons/DymaticLogo.png", &images[0].width, &images[0].height, 0, 4); //rgba channels 
			glfwSetWindowIcon(m_Window, 2, images);
			stbi_image_free(images[0].pixels);
		}

		m_Context = GraphicsContext::Create(m_Window);
		m_Context->Init();

		glfwSetWindowUserPointer(m_Window, &m_Data);
		SetVSync(true);

		// Set GLFW callbacks
		glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int width, int height)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
			data.Width = width;
			data.Height = height;

			WindowResizeEvent event(width, height);
			data.EventCallback(event);
		});

		glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
			WindowCloseEvent event;
			data.EventCallback(event);
		});

		glfwSetDropCallback(m_Window, [](GLFWwindow* window, int count, const char** paths)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
			std::vector<std::string> pathsList;
			for (int i = 0; i < count; i++)
			{
				pathsList.push_back(paths[i]);
			}
			WindowDropEvent event(pathsList);
			data.EventCallback(event);
		});

		glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			switch (action)
			{
			case GLFW_PRESS:
			{
				KeyPressedEvent event(key, 0);
				data.EventCallback(event);
				break;
			}
			case GLFW_RELEASE:
			{
				KeyReleasedEvent event(key);
				data.EventCallback(event);
				break;
			}
			case GLFW_REPEAT:
			{
				KeyPressedEvent event(key, 1);
				data.EventCallback(event);
				break;
			}
			}
		});

		glfwSetCharCallback(m_Window, [](GLFWwindow* window, unsigned int keycode)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			KeyTypedEvent event(keycode);
			data.EventCallback(event);
		});

		glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int mods)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			switch (action)
			{
			case GLFW_PRESS:
			{
				MouseButtonPressedEvent event(button);
				data.EventCallback(event);
				break;
			}
			case GLFW_RELEASE:
			{
				MouseButtonReleasedEvent event(button);
				data.EventCallback(event);
				break;
			}
			}
		});

		glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double xOffset, double yOffset)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			MouseScrolledEvent event((float)xOffset, (float)yOffset);
			data.EventCallback(event);
		});

		glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double xPos, double yPos)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			MouseMovedEvent event((float)xPos, (float)yPos);
			data.EventCallback(event);
		});
	}

	void WindowsWindow::SetSize(int width, int height) const
	{
		glfwSetWindowSize(reinterpret_cast<GLFWwindow*>(GetNativeWindow()), width, height);
	}

	uint32_t WindowsWindow::GetPositionX() const
	{
		int x;
		glfwGetWindowPos(reinterpret_cast<GLFWwindow*>(GetNativeWindow()), &x, NULL);
		return x;
	}

	uint32_t WindowsWindow::GetPositionY() const
	{
		int y;
		glfwGetWindowPos(reinterpret_cast<GLFWwindow*>(GetNativeWindow()), NULL, &y);
		return y;
	}

	void WindowsWindow::SetPosition(int x, int y) const
	{
		glfwSetWindowPos(reinterpret_cast<GLFWwindow*>(GetNativeWindow()), x, y);
	}

	void WindowsWindow::MinimizeWindow() const
	{
		glfwIconifyWindow(m_Window);
	}

	void WindowsWindow::MaximizeWindow() const
	{
		glfwMaximizeWindow(m_Window);
	}

	void WindowsWindow::ReturnWindow() const
	{
		glfwRestoreWindow(m_Window);
	}

	bool WindowsWindow::IsWindowMaximized() const
	{
		return glfwGetWindowAttrib(m_Window, GLFW_MAXIMIZED);
	}

	void WindowsWindow::SetCursor(int shape) const
	{
		switch (shape)
		{
		case 0: {shape = GLFW_ARROW_CURSOR; break; }
		case 1: {shape = GLFW_IBEAM_CURSOR; break; }
		case 2: {shape = GLFW_CROSSHAIR_CURSOR; break; }
		case 3: {shape = GLFW_POINTING_HAND_CURSOR; break; }
		case 4: {shape = GLFW_RESIZE_EW_CURSOR; break; }
		case 5: {shape = GLFW_RESIZE_NS_CURSOR; break; }
		case 6: {shape = GLFW_RESIZE_NWSE_CURSOR; break; }
		case 7: {shape = GLFW_RESIZE_NESW_CURSOR; break; }
		case 8: {shape = GLFW_RESIZE_ALL_CURSOR; break; }
		case 9: {shape = GLFW_NOT_ALLOWED_CURSOR; break; }
		default: { DY_CORE_ASSERT(false, "Cannot set cursor shape to invalid type"); }
		}

		GLFWcursor* cursor = glfwCreateStandardCursor(shape);
		glfwSetCursor(m_Window, cursor);
	}

	void WindowsWindow::Shutdown()
	{
		DY_PROFILE_FUNCTION();

		glfwDestroyWindow(m_Window);
		--s_GLFWWindowCount;

		if (s_GLFWWindowCount == 0)
		{
			glfwTerminate();
		}
	}

	void WindowsWindow::OnUpdate()
	{
		DY_PROFILE_FUNCTION();

		glfwPollEvents();
		m_Context->SwapBuffers();
	}

	void WindowsWindow::SetVSync(bool enabled)
	{
		DY_PROFILE_FUNCTION();

		if (enabled)
			glfwSwapInterval(1);
		else
			glfwSwapInterval(0);

		m_Data.VSync = enabled;
	}

	bool WindowsWindow::IsVSync() const
	{
		return m_Data.VSync;
	}

}