#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Application.h"

#include "Athena/Input/Input.h"

#include <GLFW/glfw3.h>


namespace Athena
{
	bool Input::IsKeyPressed(Keyboard::Key keycode)
	{
		auto window = reinterpret_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
		int state = glfwGetKey(window, Input::ConvertToNativeKeyCode(keycode));
		return state == GLFW_PRESS;
	}

	bool Input::IsMouseButtonPressed(Mouse::Button button)
	{
		auto window = reinterpret_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
		int state = glfwGetMouseButton(window, Input::ConvertToNativeMouseCode(button));
		return state;
	}

	Vector2 Input::GetMousePosition()
	{
		auto window = reinterpret_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
		double x, y;
		glfwGetCursorPos(window, &x, &y);
		return Vector2((float)x, float(y));
	}

	void Input::SetMousePosition(Vector2 position)
	{
		auto window = reinterpret_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
		glfwSetCursorPos(window, position.x, position.y);
	}

	CursorMode Input::GetCursorMode()
	{
		auto window = reinterpret_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
		int value = glfwGetInputMode(window, GLFW_CURSOR);

		if (value == GLFW_CURSOR_NORMAL)
			return CursorMode::Normal;

		if (value == GLFW_CURSOR_HIDDEN)
			return CursorMode::Hidden;

		if (value == GLFW_CURSOR_DISABLED)
			return CursorMode::Disabled;

		ATN_CORE_ASSERT(false);
		return (CursorMode)0;
	}

	void Input::SetCursorMode(CursorMode mode)
	{
		auto window = reinterpret_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());

		int value = 0;
		switch (mode)
		{
		case CursorMode::Normal:   value = GLFW_CURSOR_NORMAL;   break;
		case CursorMode::Hidden:   value = GLFW_CURSOR_HIDDEN;   break;
		case CursorMode::Disabled: value = GLFW_CURSOR_DISABLED; break;
		}

		glfwSetInputMode(window, GLFW_CURSOR, value);
	}

	int32 Input::ConvertToNativeKeyCode(Keyboard::Key keycode)
	{
		return (int32)keycode;
	}

	Keyboard::Key Input::ConvertFromNativeKeyCode(int32 keycode)
	{
		return (Keyboard::Key)keycode;
	}

	int32 Input::ConvertToNativeMouseCode(Mouse::Button keycode)
	{
		return (int32)keycode;
	}

	Mouse::Button Input::ConvertFromNativeMouseCode(int32 keycode)
	{
		return (Mouse::Button)keycode;
	}
}
