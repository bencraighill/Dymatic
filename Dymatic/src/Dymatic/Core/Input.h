#pragma once

#include <glm/glm.hpp>

#include "Dymatic/Core/Keycodes.h"
#include "Dymatic/Core/MouseCodes.h"
#include "Dymatic/Core/GamepadCodes.h"

namespace Dymatic {

	class Input
	{
	public:
		static bool IsKeyPressed(KeyCode key);

		static bool IsMouseButtonPressed(MouseCode button);
		static glm::vec2 GetMousePosition();
		static float GetMouseX();
		static float GetMouseY();

		static bool IsGamepadConnected(int gamepad);
		static std::string GetGamepadName(int gamepad);
		static bool IsGamepadButtonPressed(int gamepad, GamepadButtonCode button);
		static float GetGamepadAxis(int gamepad, GamepadAxisCode axis);
		static glm::vec3 GetGamepadSensor(int gamepad, GamepadSensorCode sensor);
		static bool SetGamepadRumble(int gamepad, float left, float right, float duration);
		static bool SetGamepadLED(int gamepad, glm::vec3 color);
	};


}
