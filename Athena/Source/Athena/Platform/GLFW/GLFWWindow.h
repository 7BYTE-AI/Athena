#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Log.h"
#include "Athena/Core/FileSystem.h"
#include "Athena/Core/Window.h"

#include "Athena/Input/Input.h"
#include "Athena/Input/WindowEvent.h"
#include "Athena/Input/KeyEvent.h"
#include "Athena/Input/MouseEvent.h"

#include "Athena/Renderer/Renderer.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>

#include <stb_image/stb_image.h>


namespace Athena
{
	static void GLFWErrorCallback(int error, const char* description)
	{
		ATN_CORE_ERROR_TAG("GLFW", "Error({0}) : {1}", error, description);
	}

	static Window::WindowData& GetUserPointer(GLFWwindow* window)
	{
		return *reinterpret_cast<Window::WindowData*>(glfwGetWindowUserPointer(window));
	}

	static void SetEventCallbacks(GLFWwindow* windowHandle)
	{
		glfwSetWindowCloseCallback(windowHandle, [](GLFWwindow* window)
			{
				Window::WindowData& data = GetUserPointer(window);

				Ref<WindowCloseEvent> event = Ref<WindowCloseEvent>::Create();
				data.EventCallback(event);
			});

		glfwSetWindowSizeCallback(windowHandle, [](GLFWwindow* window, int width, int height)
			{
				Window::WindowData& data = GetUserPointer(window);
				data.Width = width;
				data.Height = height;

				Ref<WindowResizeEvent> event = Ref<WindowResizeEvent>::Create(width, height);
				data.EventCallback(event);
			});

		glfwSetWindowPosCallback(windowHandle, [](GLFWwindow* window, int xpos, int ypos)
			{
				Window::WindowData& data = GetUserPointer(window);
				
				Ref<WindowMoveEvent> event = Ref<WindowMoveEvent>::Create(xpos, ypos);
				data.EventCallback(event);
			});

		glfwSetWindowFocusCallback(windowHandle, [](GLFWwindow* window, int focused)
			{
				Window::WindowData& data = GetUserPointer(window);

				if (focused)
				{
					Ref<WindowGainedFocusEvent> event = Ref<WindowGainedFocusEvent>::Create();
					data.EventCallback(event);
				}
				else
				{
					Ref<WindowLostFocusEvent> event = Ref<WindowLostFocusEvent>::Create();
					data.EventCallback(event);
				}
			});

		glfwSetWindowMaximizeCallback(windowHandle, [](GLFWwindow* window, int maximized)
			{
				Window::WindowData& data = GetUserPointer(window);
				if (maximized)
				{
					data.Mode = WindowMode::Maximized;

					Ref<WindowMaximizeEvent> event = Ref<WindowMaximizeEvent>::Create();
					data.EventCallback(event);
				}
				else
				{
					data.Mode = WindowMode::Default;

					Ref<WindowRestoreEvent> event = Ref<WindowRestoreEvent>::Create();
					data.EventCallback(event);
				}
			});

		glfwSetWindowIconifyCallback(windowHandle, [](GLFWwindow* window, int iconified)
			{
				Window::WindowData& data = GetUserPointer(window);
				if (iconified)
				{
					data.Mode = WindowMode::Minimized;

					Ref<WindowIconifyEvent> event = Ref<WindowIconifyEvent>::Create();
					data.EventCallback(event);
				}
				else
				{
					bool isMaximized = glfwGetWindowAttrib(window, GLFW_MAXIMIZED);
					data.Mode = isMaximized ? WindowMode::Maximized : WindowMode::Default;

					Ref<WindowRestoreEvent> event = Ref<WindowRestoreEvent>::Create();
					data.EventCallback(event);
				}
			});

		glfwSetKeyCallback(windowHandle, [](GLFWwindow* window, int key, int scancode, int action, int mods)
			{
				Window::WindowData& data = GetUserPointer(window);
				
				bool ctrl = mods & GLFW_MOD_CONTROL;
				bool alt = mods & GLFW_MOD_ALT;
				bool shift = mods & GLFW_MOD_SHIFT;

				Keyboard::Key akey = Input::ConvertFromNativeKeyCode(key);

				switch (action)
				{
				case GLFW_PRESS:
				{
					Ref<KeyPressedEvent> event = Ref<KeyPressedEvent>::Create(akey, false, ctrl, alt, shift);
					data.EventCallback(event);
					break;
				}
				case GLFW_RELEASE:
				{
					Ref<KeyReleasedEvent> event = Ref<KeyReleasedEvent>::Create(akey, ctrl, alt, shift);
					data.EventCallback(event);
					break;
				}
				case GLFW_REPEAT:
				{
					Ref<KeyPressedEvent> event = Ref<KeyPressedEvent>::Create(akey, true, ctrl, alt, shift);
					data.EventCallback(event);
					break;
				}
				}
			});

		glfwSetCharCallback(windowHandle, [](GLFWwindow* window, unsigned int character)
			{
				const auto convertFromUnicode = [](unsigned int code) { return (int)code >= 97 ? (int)code - 32 : (int)code; };

				Window::WindowData& data = GetUserPointer(window);

				Ref<KeyTypedEvent> event = Ref<KeyTypedEvent>::Create(Input::ConvertFromNativeKeyCode(convertFromUnicode(character)));
				data.EventCallback(event);
			});

		glfwSetMouseButtonCallback(windowHandle, [](GLFWwindow* window, int button, int action, int mods)
			{
				Window::WindowData& data = GetUserPointer(window);

				bool ctrl = mods & GLFW_MOD_CONTROL;
				bool alt = mods & GLFW_MOD_ALT;
				bool shift = mods & GLFW_MOD_SHIFT;

				Mouse::Button abutton = Input::ConvertFromNativeMouseCode(button);

				switch (action)
				{
				case GLFW_PRESS:
				{
					Ref<MouseButtonPressedEvent> event = Ref<MouseButtonPressedEvent>::Create(abutton, ctrl, alt, shift);
					data.EventCallback(event);
					break;
				}
				case GLFW_RELEASE:
				{
					Ref<MouseButtonReleasedEvent> event = Ref<MouseButtonReleasedEvent>::Create(abutton, ctrl, alt, shift);
					data.EventCallback(event);
					break;
				}
				}
			});
		
		glfwSetScrollCallback(windowHandle, [](GLFWwindow* window, double xOffset, double yOffset)
			{
				Window::WindowData& data = GetUserPointer(window);

				Ref<MouseScrolledEvent> event = Ref<MouseScrolledEvent>::Create((float)xOffset, (float)yOffset);
				data.EventCallback(event);
			});

		glfwSetCursorPosCallback(windowHandle, [](GLFWwindow* window, double x, double y)
			{
				Window::WindowData& data = GetUserPointer(window);

				Ref<MouseMoveEvent> event = Ref<MouseMoveEvent>::Create((float)x, (float)y);
				data.EventCallback(event);
			});

		glfwSetTitlebarHitTestCallback(windowHandle, [](GLFWwindow* window, int x, int y, int* hit)
			{
				Window::WindowData& data = GetUserPointer(window);
		
				if (data.CustomTitlebar && data.TitlebarHitTest)
				{
					*hit = data.TitlebarHitTest();
				}
			});
	}


	Scope<Window> Window::Create(const WindowCreateInfo& info)
	{
		Scope<Window> window = Scope<Window>::Create();

		WindowData windowData;
		windowData.Width = info.Width;
		windowData.Height = info.Height;
		windowData.VSync = info.VSync;
		windowData.Title = info.Title;
		windowData.CustomTitlebar = info.CustomTitlebar;
		windowData.EventCallback = info.EventCallback;

		window->m_Data = windowData;

		if (m_WindowCount == 0)
		{
			int success = glfwInit();
			ATN_CORE_VERIFY(success, "Could not intialize GLFW");
			ATN_CORE_INFO_TAG("GLFW", "Init GLFW");

			glfwSetErrorCallback(GLFWErrorCallback);
		}

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_TITLEBAR, info.CustomTitlebar ? GLFW_FALSE : GLFW_TRUE);

		GLFWwindow* glfwWindow;
		window->m_WindowHandle = glfwWindow = glfwCreateWindow(
			window->m_Data.Width,
			window->m_Data.Height,
			window->m_Data.Title.c_str(),
			nullptr,
			nullptr);

		m_WindowCount++;

		ATN_CORE_INFO_TAG("GLFW", "Create GLFW Window '{0}' ({1}, {2})", window->m_Data.Title, window->m_Data.Width, window->m_Data.Height);

		glfwSetWindowUserPointer(glfwWindow, &window->m_Data);
		SetEventCallbacks(glfwWindow);

		glfwSetWindowAttrib(glfwWindow, GLFW_RESIZABLE, info.WindowResizeable ? GLFW_TRUE : GLFW_FALSE);

		window->SetWindowMode(info.StartMode);
		window->SetIcon(info.Icon);

		// Raw mouse motion
		if (glfwRawMouseMotionSupported())
		{
			ATN_CORE_INFO_TAG("GLFW", "Raw mouse motion enabled");
			glfwSetInputMode(glfwWindow, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
		}
		else
		{
			ATN_CORE_WARN_TAG("GLFW", "Raw mouse motion not supported on this platform!");
		}

		if (Renderer::GetAPI() == Renderer::API::Vulkan)
		{
			if (!glfwVulkanSupported())
			{
				ATN_CORE_FATAL_TAG("GLFW", "Vulkan is not supported!");
				return window;
			}
		}

		window->m_SwapChain = SwapChain::Create(glfwWindow, window->m_Data.VSync);

		return window;
	}

	Window::~Window()
	{
		glfwDestroyWindow(reinterpret_cast<GLFWwindow*>(m_WindowHandle));
		--m_WindowCount;

		ATN_CORE_INFO_TAG("GLFW", "Destroy Window '{0}'", m_Data.Title);

		if (m_WindowCount <= 0)
		{
			glfwTerminate();
			ATN_CORE_INFO_TAG("GLFW", "Shutdown GLFW");
		}
	}

	void Window::PollEvents()
	{
		ATN_PROFILE_FUNC();
		glfwPollEvents();
	}

	void Window::SwapBuffers()
	{
		m_SwapChain->Present();
	}

	void Window::SetVSync(bool enabled)
	{
		m_Data.VSync = enabled;
		m_SwapChain->SetVSync(enabled);
	}

	void Window::SetIcon(const FilePath& path)
	{
		if (FileSystem::Exists(path))
		{
			GLFWimage image;
			image.pixels = stbi_load(path.string().c_str(), &image.width, &image.height, 0, 4);
			if (image.pixels)
			{
				glfwSetWindowIcon((GLFWwindow*)m_WindowHandle, 1, &image);
				stbi_image_free(image.pixels);
			}
			else
			{
				ATN_CORE_ERROR_TAG("GLFW", "failed to load icon from '{}'!", path);
			}
		}
		else if (!path.empty())
		{
			ATN_CORE_ERROR_TAG("GLFW", "invalid filepath for icon '{}'!", path);
		}
	}

	void Window::SetWindowMode(WindowMode mode)
	{
		WindowMode currentMode = GetWindowMode();
		GLFWwindow* hWnd = reinterpret_cast<GLFWwindow*>(m_WindowHandle);
		GLFWmonitor* monitor = glfwGetPrimaryMonitor();

		if (currentMode == mode)
			return;

		// Remove fullscreen
		if (currentMode == WindowMode::Fullscreen)
		{
			const GLFWvidmode* mode = glfwGetVideoMode(monitor);
			glfwSetWindowMonitor(hWnd, nullptr, 0, 0, m_Data.Width, m_Data.Width, mode->refreshRate);
		}

		switch (mode)
		{
		case WindowMode::Default:
		{
			glfwRestoreWindow(hWnd);
			break;
		}
		case WindowMode::Maximized:
		{
			glfwMaximizeWindow(hWnd);
			break;
		}
		case WindowMode::Minimized:
		{
			glfwIconifyWindow(hWnd);
			break;
		}
		case WindowMode::Fullscreen:
		{
			GLFWmonitor* monitor = glfwGetPrimaryMonitor();
			const GLFWvidmode* mode = glfwGetVideoMode(monitor);
			
			glfwSetWindowMonitor(hWnd, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
			break;
		}
		}

		m_Data.Mode = mode;
	}
}
