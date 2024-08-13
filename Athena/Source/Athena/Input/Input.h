#pragma once

#include "Athena/Core/Core.h"

#include "Athena/Input/Keyboard.h"
#include "Athena/Input/Mouse.h"

#include "Athena/Math/Vector.h"


namespace Athena
{
	enum class CursorMode
	{
		Normal = 1, 
		Hidden, 
		Disabled
	};

	class ATHENA_API Input
	{
	public:
		static bool IsKeyPressed(Keyboard::Key keycode);
		static bool IsMouseButtonPressed(Mouse::Button button);

		static Vector2 GetMousePosition();
		static void SetMousePosition(Vector2 position);

		static CursorMode GetCursorMode();
		static void SetCursorMode(CursorMode mode);

		static int32 ConvertToNativeKeyCode(Keyboard::Key keycode);
		static Keyboard::Key ConvertFromNativeKeyCode(int32 keycode);

		static int32 ConvertToNativeMouseCode(Mouse::Button keycode);
		static Mouse::Button ConvertFromNativeMouseCode(int32 keycode);
	};
}
