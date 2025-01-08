#pragma once

#include <glm/glm.hpp>

#include "Dymatic/Core/Keycodes.h"
#include "Dymatic/Core/MouseCodes.h"
#include "Dymatic/Core/GamepadCodes.h"

namespace Dymatic {

	class Input
	{
	public:
		enum class PowerLevel
		{
			Unknown = -1,
			Empty,
			Low,
			Medium,
			Full,
			Wired,
			Max
		};

		enum class BrightnessLevel
		{
			Off = -1,
			High,
			Medium,
			Low
		};

	public:
		static void Init();
		static void Shutdown();
		static void PreUpdate();
		static void PostUpdate();
		static void OnGamepadConnected(int gamepad);
		static void OnGamepadDisconnected(int gamepad);

	public:

		// Keyboard
		static std::string GetKeyboardName();
		static bool IsKeyPressed(KeyCode key);

		// Mouse
		static std::string GetMouseName();
		static bool IsMouseButtonPressed(MouseCode button);
		static glm::vec2 GetMousePosition();
		static glm::vec2 GetMouseDelta();
		static float GetMouseX();
		static float GetMouseY();

		// Gamepad
		static uint32_t GetGamepadCount();
		static bool IsGamepadConnected(GamepadCode gamepad);
		static std::string GetGamepadName(GamepadCode gamepad);
		static PowerLevel GetGamepadPowerLevel(GamepadCode gamepad);
		
		static bool IsGamepadButtonPressed(GamepadCode gamepad, GamepadButtonCode button);
		static float GetGamepadAxis(GamepadCode gamepad, GamepadAxisCode axis);
		static glm::vec3 GetGamepadSensor(GamepadCode gamepad, GamepadSensorCode sensor);
		static glm::vec2 GetGamepadTouchPad(GamepadCode gamepad);
		static bool SetGamepadRumble(GamepadCode gamepad, float left, float right, float duration);
		
		static void SetGamepadLED(GamepadCode gamepad, glm::vec3 color);
		static void SetGamepadPlayerLED(GamepadCode gamepad, BrightnessLevel brightnessLevel, int count = 0, bool fadeIn = false);
		static void SetGamepadMicrophoneLED(GamepadCode gamepad, bool enabled, bool pulse = false);
		
		static void ClearTriggerEffect(GamepadCode gamepad, GamepadAxisCode trigger);
		static void SetTriggerEffect(GamepadCode gamepad, GamepadAxisCode trigger, float startPosition, bool keepEffect, float beginForce, float middleForce, float endForce, float frequency);
		static void SetTriggerEffectContinuous(GamepadCode gamepad, GamepadAxisCode trigger, float startPosition, float force);
		static void SetTriggerEffectSection(GamepadCode gamepad, GamepadButtonCode trigger, float startPosition, float endPosition);
	};


}
