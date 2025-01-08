#include "dypch.h"
#include "Dymatic/Core/Input.h"

#include "Dymatic/Core/Application.h"
#include <GLFW/glfw3.h>

#include <SDL_gamecontroller.h>
#include <SDL_video.h>

#define USE_IMGUI_INPUT
#ifdef USE_IMGUI_INPUT
#include <imgui.h>
#endif

#include <codecvt>
#include <setupapi.h>
#include <devguid.h>

#define DS5W_USE_LIB
#include <DualSenseWindows/IO.h>
#include <DualSenseWindows/Device.h>
#include <DualSenseWindows/Helpers.h>

namespace Dymatic {

#ifdef USE_IMGUI_INPUT
	namespace Utils {

		static ImGuiKey KeyCodeToImGuiKey(KeyCode key)
		{
			switch (key)
			{
			case Key::Tab: return ImGuiKey_Tab;
			case Key::Left: return ImGuiKey_LeftArrow;
			case Key::Right: return ImGuiKey_RightArrow;
			case Key::Up: return ImGuiKey_UpArrow;
			case Key::Down: return ImGuiKey_DownArrow;
			case Key::PageUp: return ImGuiKey_PageUp;
			case Key::PageDown: return ImGuiKey_PageDown;
			case Key::Home: return ImGuiKey_Home;
			case Key::End: return ImGuiKey_End;
			case Key::Insert: return ImGuiKey_Insert;
			case Key::Delete: return ImGuiKey_Delete;
			case Key::Backspace: return ImGuiKey_Backspace;
			case Key::Space: return ImGuiKey_Space;
			case Key::Enter: return ImGuiKey_Enter;
			case Key::Escape: return ImGuiKey_Escape;
			case Key::Apostrophe: return ImGuiKey_Apostrophe;
			case Key::Comma: return ImGuiKey_Comma;
			case Key::Minus: return ImGuiKey_Minus;
			case Key::Period: return ImGuiKey_Period;
			case Key::Slash: return ImGuiKey_Slash;
			case Key::Semicolon: return ImGuiKey_Semicolon;
			case Key::Equal: return ImGuiKey_Equal;
			case Key::LeftBracket: return ImGuiKey_LeftBracket;
			case Key::Backslash: return ImGuiKey_Backslash;
			case Key::RightBracket: return ImGuiKey_RightBracket;
			case Key::GraveAccent: return ImGuiKey_GraveAccent;
			case Key::CapsLock: return ImGuiKey_CapsLock;
			case Key::ScrollLock: return ImGuiKey_ScrollLock;
			case Key::NumLock: return ImGuiKey_NumLock;
			case Key::PrintScreen: return ImGuiKey_PrintScreen;
			case Key::Pause: return ImGuiKey_Pause;
			case Key::KP0: return ImGuiKey_Keypad0;
			case Key::KP1: return ImGuiKey_Keypad1;
			case Key::KP2: return ImGuiKey_Keypad2;
			case Key::KP3: return ImGuiKey_Keypad3;
			case Key::KP4: return ImGuiKey_Keypad4;
			case Key::KP5: return ImGuiKey_Keypad5;
			case Key::KP6: return ImGuiKey_Keypad6;
			case Key::KP7: return ImGuiKey_Keypad7;
			case Key::KP8: return ImGuiKey_Keypad8;
			case Key::KP9: return ImGuiKey_Keypad9;
			case Key::KPDecimal: return ImGuiKey_KeypadDecimal;
			case Key::KPDivide: return ImGuiKey_KeypadDivide;
			case Key::KPMultiply: return ImGuiKey_KeypadMultiply;
			case Key::KPSubtract: return ImGuiKey_KeypadSubtract;
			case Key::KPAdd: return ImGuiKey_KeypadAdd;
			case Key::KPEnter: return ImGuiKey_KeypadEnter;
			case Key::KPEqual: return ImGuiKey_KeypadEqual;
			case Key::LeftShift: return ImGuiKey_LeftShift;
			case Key::LeftControl: return ImGuiKey_LeftCtrl;
			case Key::LeftAlt: return ImGuiKey_LeftAlt;
			case Key::LeftSuper: return ImGuiKey_LeftSuper;
			case Key::RightShift: return ImGuiKey_RightShift;
			case Key::RightControl: return ImGuiKey_RightCtrl;
			case Key::RightAlt: return ImGuiKey_RightAlt;
			case Key::RightSuper: return ImGuiKey_RightSuper;
			case Key::Menu: return ImGuiKey_Menu;
			case Key::D0: return ImGuiKey_0;
			case Key::D1: return ImGuiKey_1;
			case Key::D2: return ImGuiKey_2;
			case Key::D3: return ImGuiKey_3;
			case Key::D4: return ImGuiKey_4;
			case Key::D5: return ImGuiKey_5;
			case Key::D6: return ImGuiKey_6;
			case Key::D7: return ImGuiKey_7;
			case Key::D8: return ImGuiKey_8;
			case Key::D9: return ImGuiKey_9;
			case Key::A: return ImGuiKey_A;
			case Key::B: return ImGuiKey_B;
			case Key::C: return ImGuiKey_C;
			case Key::D: return ImGuiKey_D;
			case Key::E: return ImGuiKey_E;
			case Key::F: return ImGuiKey_F;
			case Key::G: return ImGuiKey_G;
			case Key::H: return ImGuiKey_H;
			case Key::I: return ImGuiKey_I;
			case Key::J: return ImGuiKey_J;
			case Key::K: return ImGuiKey_K;
			case Key::L: return ImGuiKey_L;
			case Key::M: return ImGuiKey_M;
			case Key::N: return ImGuiKey_N;
			case Key::O: return ImGuiKey_O;
			case Key::P: return ImGuiKey_P;
			case Key::Q: return ImGuiKey_Q;
			case Key::R: return ImGuiKey_R;
			case Key::S: return ImGuiKey_S;
			case Key::T: return ImGuiKey_T;
			case Key::U: return ImGuiKey_U;
			case Key::V: return ImGuiKey_V;
			case Key::W: return ImGuiKey_W;
			case Key::X: return ImGuiKey_X;
			case Key::Y: return ImGuiKey_Y;
			case Key::Z: return ImGuiKey_Z;
			case Key::F1: return ImGuiKey_F1;
			case Key::F2: return ImGuiKey_F2;
			case Key::F3: return ImGuiKey_F3;
			case Key::F4: return ImGuiKey_F4;
			case Key::F5: return ImGuiKey_F5;
			case Key::F6: return ImGuiKey_F6;
			case Key::F7: return ImGuiKey_F7;
			case Key::F8: return ImGuiKey_F8;
			case Key::F9: return ImGuiKey_F9;
			case Key::F10: return ImGuiKey_F10;
			case Key::F11: return ImGuiKey_F11;
			case Key::F12: return ImGuiKey_F12;
			case Key::F13: return ImGuiKey_F13;
			case Key::F14: return ImGuiKey_F14;
			case Key::F15: return ImGuiKey_F15;
			case Key::F16: return ImGuiKey_F16;
			case Key::F17: return ImGuiKey_F17;
			case Key::F18: return ImGuiKey_F18;
			case Key::F19: return ImGuiKey_F19;
			case Key::F20: return ImGuiKey_F20;
			case Key::F21: return ImGuiKey_F21;
			case Key::F22: return ImGuiKey_F22;
			case Key::F23: return ImGuiKey_F23;
			case Key::F24: return ImGuiKey_F24;
			default: return ImGuiKey_None;
			}
		}

		static ImGuiMouseButton MouseCodeToImGuiMouseButton(MouseCode button)
		{
			switch (button)
			{
			case Mouse::ButtonLeft: return ImGuiMouseButton_Left;
			case Mouse::ButtonRight: return ImGuiMouseButton_Right;
			case Mouse::ButtonMiddle: return ImGuiMouseButton_Middle;
			default:
				// Pray ImGui knows what to do with the button code...
				return button;
			}
		}
	}
#endif

	struct DualSenseData
	{
		DS5W::DeviceContext DeviceContext;
		DS5W::DS5InputState InputState;
		DS5W::DS5OutputState OutputState;
	}* s_DualSenseGamepads[GLFW_JOYSTICK_LAST + 1];

	static glm::vec2 s_PreviousMousePosition;
	
	void Input::Init()
	{
		for (int i = 0; i < GLFW_JOYSTICK_LAST + 1; i++)
			s_DualSenseGamepads[i] = nullptr;

		const uint32_t controllerCount = GetGamepadCount();

		uint32_t dualSenseIndex = 0;
		uint32_t dualSenseCount = 0;
		DS5W::DeviceEnumInfo dualSenseDeviceInfo[GLFW_JOYSTICK_LAST + 1];
		DS5W::enumDevices(dualSenseDeviceInfo, GLFW_JOYSTICK_LAST + 1, &dualSenseCount);
		
		for (uint32_t i = 0; i < controllerCount; i++)
		{
			SDL_GameController* controller = SDL_GameControllerOpen(i);
			if (SDL_GameControllerGetType(controller) == SDL_CONTROLLER_TYPE_PS5)
			{
				s_DualSenseGamepads[i] = new DualSenseData();
				ZeroMemory(&s_DualSenseGamepads[i]->InputState, sizeof(DS5W::DS5InputState));
				ZeroMemory(&s_DualSenseGamepads[i]->OutputState, sizeof(DS5W::DS5OutputState));
				if (!DS5W_SUCCESS(DS5W::initDeviceContext(&dualSenseDeviceInfo[dualSenseIndex], &s_DualSenseGamepads[i]->DeviceContext)))
					DY_CORE_ERROR("Failed to init DualSense device context");
				dualSenseIndex++;
			}
			SDL_GameControllerClose(controller);
		}
	}

	void Input::Shutdown()
	{
		for (uint32_t i = 0; i < GLFW_JOYSTICK_LAST + 1; i++)
			OnGamepadDisconnected(i);
	}

	void Input::PreUpdate()
	{
		for (uint32_t i = 0; i < GLFW_JOYSTICK_LAST + 1; i++)
			if (s_DualSenseGamepads[i])
				if (!DS5W_SUCCESS(DS5W::getDeviceInputState(&s_DualSenseGamepads[i]->DeviceContext, &s_DualSenseGamepads[i]->InputState)))
					DY_CORE_ERROR("Failed to read DualSense device state");
	}

	void Input::PostUpdate()
	{
		for (uint32_t i = 0; i < GLFW_JOYSTICK_LAST + 1; i++)
			if (s_DualSenseGamepads[i])
				if (!DS5W_SUCCESS(DS5W::setDeviceOutputState(&s_DualSenseGamepads[i]->DeviceContext, &s_DualSenseGamepads[i]->OutputState)))
					DY_CORE_ERROR("Failed to set DualSense device output state");

		s_PreviousMousePosition = GetMousePosition();
	}

	void Input::OnGamepadConnected(int gamepad)
	{
		if (SDL_GameController* controller = SDL_GameControllerOpen(gamepad))
		{
			if (SDL_GameControllerGetType(controller) != SDL_CONTROLLER_TYPE_PS5)
			{
				SDL_GameControllerClose(controller);
				return;
			}

			const uint32_t controllerCount = GetGamepadCount();

			uint32_t dualSenseIndex = 0;
			uint32_t dualSenseCount = 0;
			DS5W::DeviceEnumInfo dualSenseDeviceInfo[GLFW_JOYSTICK_LAST + 1];
			DS5W::enumDevices(dualSenseDeviceInfo, GLFW_JOYSTICK_LAST + 1, &dualSenseCount);

			for (uint32_t i = 0; i < gamepad; i++)
			{
				SDL_GameController* controller = SDL_GameControllerOpen(i);
				if (SDL_GameControllerGetType(controller) == SDL_CONTROLLER_TYPE_PS5)
					dualSenseIndex++;
				SDL_GameControllerClose(controller);
			}

			s_DualSenseGamepads[gamepad] = new DualSenseData();
			ZeroMemory(&s_DualSenseGamepads[gamepad]->InputState, sizeof(DS5W::DS5InputState));
			ZeroMemory(&s_DualSenseGamepads[gamepad]->OutputState, sizeof(DS5W::DS5OutputState));
			if (!DS5W_SUCCESS(DS5W::initDeviceContext(&dualSenseDeviceInfo[dualSenseIndex], &s_DualSenseGamepads[gamepad]->DeviceContext)))
				DY_CORE_ERROR("Failed to init DualSense device context");

			SDL_GameControllerClose(controller);
		}
	}

	void Input::OnGamepadDisconnected(int gamepad)
	{
		if (s_DualSenseGamepads[gamepad])
		{
			DS5W::freeDeviceContext(&s_DualSenseGamepads[gamepad]->DeviceContext);
			delete s_DualSenseGamepads[gamepad];
			s_DualSenseGamepads[gamepad] = nullptr;
		}
	}

	std::string Input::GetKeyboardName()
	{
		HDEVINFO deviceInfoSet = SetupDiGetClassDevs(&GUID_DEVCLASS_KEYBOARD, NULL, NULL, DIGCF_PRESENT);
		if (deviceInfoSet == INVALID_HANDLE_VALUE)
			return "Unknown Keyboard";

		SP_DEVINFO_DATA deviceInfoData;
		deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

		for (DWORD i = 0; SetupDiEnumDeviceInfo(deviceInfoSet, i, &deviceInfoData); ++i) 
		{
			DWORD dataType;
			WCHAR buffer[256];
			DWORD bufferSize = sizeof(buffer);

			if (SetupDiGetDeviceRegistryProperty(deviceInfoSet, &deviceInfoData, SPDRP_DEVICEDESC, &dataType, (BYTE*)buffer, bufferSize, &bufferSize)) 
			{
				std::wstring wStr(buffer);
				return std::string(wStr.begin(), wStr.end());
			}
		}

		SetupDiDestroyDeviceInfoList(deviceInfoSet);

		return "Unknown Keyboard";
	}

	bool Input::IsKeyPressed(const KeyCode key)
	{
		auto* window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
		auto state = glfwGetKey(window, static_cast<int32_t>(key));
		bool pressed = state == GLFW_PRESS;

#ifdef USE_IMGUI_INPUT
		pressed = pressed || ImGui::IsKeyDown(Utils::KeyCodeToImGuiKey(key));
#endif 

		return pressed;
	}

	std::string Input::GetMouseName()
	{
		HDEVINFO deviceInfoSet = SetupDiGetClassDevs(&GUID_DEVCLASS_MOUSE, NULL, NULL, DIGCF_PRESENT);
		if (deviceInfoSet == INVALID_HANDLE_VALUE)
			return "Unknown Mouse";

		SP_DEVINFO_DATA deviceInfoData;
		deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

		for (DWORD i = 0; SetupDiEnumDeviceInfo(deviceInfoSet, i, &deviceInfoData); ++i)
		{
			DWORD dataType;
			WCHAR buffer[256];
			DWORD bufferSize = sizeof(buffer);

			if (SetupDiGetDeviceRegistryProperty(deviceInfoSet, &deviceInfoData, SPDRP_DEVICEDESC, &dataType, (BYTE*)buffer, bufferSize, &bufferSize))
			{
				std::wstring wStr(buffer);
				return std::string(wStr.begin(), wStr.end());
			}
		}

		SetupDiDestroyDeviceInfoList(deviceInfoSet);

		return "Unknown Mouse";
	}

	bool Input::IsMouseButtonPressed(const MouseCode button)
	{
		auto* window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
		auto state = glfwGetMouseButton(window, static_cast<int32_t>(button));
		bool pressed = state == GLFW_PRESS;

#ifdef USE_IMGUI_INPUT
		pressed = pressed || ImGui::IsMouseDown(Utils::MouseCodeToImGuiMouseButton(button));
#endif

		return pressed;
	}

	glm::vec2 Input::GetMousePosition()
	{
#ifdef USE_IMGUI_INPUT
		return ImGui::GetMousePos();
#else
		auto* window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);

		return { (float)xpos, (float)ypos };
#endif
	}

	glm::vec2 Input::GetMouseDelta()
	{
#ifdef USE_IMGUI_INPUT
		return ImGui::GetIO().MouseDelta;
#else
		return GetMousePosition() - s_PreviousMousePosition;
#endif
	}

	float Input::GetMouseX()
	{
		return GetMousePosition().x;
	}

	float Input::GetMouseY()
	{
		return GetMousePosition().y;
	}

	uint32_t Input::GetGamepadCount()
	{
		return SDL_NumJoysticks();
	}

	bool Input::IsGamepadConnected(GamepadCode gamepad)
	{
		return glfwJoystickPresent(gamepad);
	}

	std::string Input::GetGamepadName(GamepadCode gamepad)
	{
		return glfwGetGamepadName(gamepad);
	}

	Input::PowerLevel Input::GetGamepadPowerLevel(GamepadCode gamepad)
	{
		if (s_DualSenseGamepads[gamepad])
		{
			auto& inputState = s_DualSenseGamepads[gamepad]->InputState;
			
			if (inputState.battery.fullyCharged)
				return Input::PowerLevel::Max;
			if (inputState.battery.chargin)
				return Input::PowerLevel::Wired;
			return (Input::PowerLevel)inputState.battery.level;
		}
		else
		{
			SDL_GameController* controller = SDL_GameControllerOpen(gamepad);
			PowerLevel powerLevel = (PowerLevel)SDL_JoystickCurrentPowerLevel(SDL_GameControllerGetJoystick(controller));
			SDL_GameControllerClose(controller);

			return powerLevel;
		}
	}

	bool Input::IsGamepadButtonPressed(GamepadCode gamepad, GamepadButtonCode button)
	{
		if (!IsGamepadConnected(gamepad))
			return false;
		
		int count;
		return glfwGetJoystickButtons(gamepad, &count)[button];
	}

	float Input::GetGamepadAxis(GamepadCode gamepad, GamepadAxisCode axis)
	{
		if (s_DualSenseGamepads[gamepad])
		{
			auto& inputState = s_DualSenseGamepads[gamepad]->InputState;
			switch (axis)
			{
			case Gamepad::LeftXAxis: return inputState.leftStick.x / 255.0f;
			case Gamepad::LeftYAxis: return inputState.leftStick.y / 255.0f;
			case Gamepad::RightXAxis: return inputState.rightStick.x / 255.0f;
			case Gamepad::RightYAxis: return inputState.rightStick.y / 255.0f;
			case Gamepad::LeftTriggerAxis: return inputState.leftTrigger / 255.0f;
			case Gamepad::RightTriggerAxis: return inputState.rightTrigger / 255.0f;
			}
		}
		else
		{
			int count;
			return glfwGetJoystickAxes(gamepad, &count)[axis];
		}
	}

	glm::vec3 Input::GetGamepadSensor(GamepadCode gamepad, GamepadSensorCode sensor)
	{
		SDL_GameController* controller = SDL_GameControllerOpen(gamepad);
		glm::vec3 sensorValue;
		SDL_GameControllerGetSensorData(controller, (SDL_SensorType)sensor, (float*)&sensorValue, 3);
		SDL_GameControllerClose(controller);
		
		return sensorValue;
	}

	glm::vec2 Input::GetGamepadTouchPad(GamepadCode gamepad)
	{
		if (s_DualSenseGamepads[gamepad])
		{
			return glm::vec2(s_DualSenseGamepads[gamepad]->InputState.touchPoint1.x, s_DualSenseGamepads[gamepad]->InputState.touchPoint1.y);
		}
		else
		{
			SDL_GameController* controller = SDL_GameControllerOpen(gamepad);
			glm::vec2 touchPad;
			float pressure;
			SDL_GameControllerGetTouchpadFinger(controller, 0, 0, nullptr, &touchPad.x, &touchPad.y, &pressure);
			SDL_GameControllerClose(controller);

			return touchPad;
		}
	}

	bool Input::SetGamepadRumble(GamepadCode gamepad, float left, float right, float duration)
	{
		if (s_DualSenseGamepads[gamepad])
		{
			auto& outputState = s_DualSenseGamepads[gamepad]->OutputState;
			outputState.leftRumble = 255.0f * left;
			outputState.rightRumble = 255.0f * right;
			return true;
		}
		else
		{
			SDL_GameController* controller = SDL_GameControllerOpen(gamepad);
			bool rumble = SDL_GameControllerRumble(controller, left, right, duration);
			SDL_GameControllerClose(controller);

			return rumble;
		}
	}
	
	void Input::SetGamepadLED(GamepadCode gamepad, glm::vec3 color)
	{
		if (s_DualSenseGamepads[gamepad])
		{
			auto& outputState = s_DualSenseGamepads[gamepad]->OutputState;
			outputState.lightbar = DS5W::color_R32G32B32_FLOAT(color.r, color.g, color.b);
		}
		else
		{
			SDL_GameController* controller = SDL_GameControllerOpen(gamepad);
			SDL_GameControllerSetLED(controller, color.r, color.g, color.b);
			SDL_GameControllerClose(controller);
		}
	}

	void Input::SetGamepadPlayerLED(GamepadCode gamepad, BrightnessLevel brightnessLevel, int count, bool fadeIn)
	{
		if (!s_DualSenseGamepads[gamepad])
			return;

		auto& outputState = s_DualSenseGamepads[gamepad]->OutputState;

		outputState.playerLeds.bitmask = 0b00000000;
		outputState.playerLeds.playerLedFade = fadeIn;
		outputState.playerLeds.brightness = (DS5W::LedBrightness)brightnessLevel;
	}

	void Input::SetGamepadMicrophoneLED(GamepadCode gamepad, bool enabled, bool pulse)
	{
		if (!s_DualSenseGamepads[gamepad])
			return;

		auto& outputState = s_DualSenseGamepads[gamepad]->OutputState;

		outputState.microphoneLed = (pulse ? DS5W::MicLed::PULSE : (enabled ? DS5W::MicLed::ON : DS5W::MicLed::OFF));
	}

	void Input::ClearTriggerEffect(GamepadCode gamepad, GamepadButtonCode trigger)
	{
		if (!s_DualSenseGamepads[gamepad])
			return;

		auto& outputState = s_DualSenseGamepads[gamepad]->OutputState;

		if (trigger == Gamepad::LeftTrigger)
			outputState.leftTriggerEffect.effectType = DS5W::TriggerEffectType::NoResitance;
		else if (trigger == Gamepad::RightTrigger)
			outputState.rightTriggerEffect.effectType = DS5W::TriggerEffectType::NoResitance;
	}

	void Input::SetTriggerEffect(GamepadCode gamepad, GamepadButtonCode trigger, float startPosition, bool keepEffect, float beginForce, float middleForce, float endForce, float frequency)
	{
		if (!s_DualSenseGamepads[gamepad])
			return;

		auto& outputState = s_DualSenseGamepads[gamepad]->OutputState;

		if (trigger == Gamepad::LeftTrigger)
		{
			outputState.leftTriggerEffect.effectType = DS5W::TriggerEffectType::EffectEx;
			outputState.leftTriggerEffect.EffectEx.startPosition = 255.0f * startPosition;
			outputState.leftTriggerEffect.EffectEx.keepEffect = keepEffect;
			outputState.leftTriggerEffect.EffectEx.beginForce = 255.0f * beginForce;
			outputState.leftTriggerEffect.EffectEx.middleForce = 255.0f * middleForce;
			outputState.leftTriggerEffect.EffectEx.endForce = 255.0f * endForce;
			outputState.leftTriggerEffect.EffectEx.frequency = 255.0f * frequency;
		}
		else if (trigger == Gamepad::RightTrigger)
		{
			outputState.rightTriggerEffect.effectType = DS5W::TriggerEffectType::EffectEx;
			outputState.rightTriggerEffect.EffectEx.startPosition = 255.0f * startPosition;
			outputState.rightTriggerEffect.EffectEx.keepEffect = keepEffect;
			outputState.rightTriggerEffect.EffectEx.beginForce = 255.0f * beginForce;
			outputState.rightTriggerEffect.EffectEx.middleForce = 255.0f * middleForce;
			outputState.rightTriggerEffect.EffectEx.endForce = 255.0f * endForce;
			outputState.rightTriggerEffect.EffectEx.frequency = 255.0f * frequency;
		}
	}

	void Input::SetTriggerEffectContinuous(GamepadCode gamepad, GamepadButtonCode trigger, float startPosition, float force)
	{
		if (!s_DualSenseGamepads[gamepad])
			return;

		auto& outputState = s_DualSenseGamepads[gamepad]->OutputState;

		if (trigger == Gamepad::LeftTrigger)
		{
			outputState.leftTriggerEffect.effectType = DS5W::TriggerEffectType::ContinuousResitance;
			outputState.leftTriggerEffect.Continuous.startPosition = 255.0f * startPosition;
			outputState.leftTriggerEffect.Continuous.force = 255.0f * force;
		}
		else if (trigger == Gamepad::RightTrigger)
		{
			outputState.rightTriggerEffect.effectType = DS5W::TriggerEffectType::ContinuousResitance;
			outputState.rightTriggerEffect.Continuous.startPosition = 255.0f * startPosition;
			outputState.rightTriggerEffect.Continuous.force = 255.0f * force;
		}
	}

	void Input::SetTriggerEffectSection(GamepadCode gamepad, GamepadButtonCode trigger, float startPosition, float endPosition)
	{
		if (!s_DualSenseGamepads[gamepad])
			return;

		auto& outputState = s_DualSenseGamepads[gamepad]->OutputState;

		if (trigger == Gamepad::LeftTrigger)
		{
			outputState.leftTriggerEffect.effectType = DS5W::TriggerEffectType::SectionResitance;
			outputState.leftTriggerEffect.Section.startPosition = 255.0f * startPosition;
			outputState.leftTriggerEffect.Section.endPosition = 255.0f * endPosition;
		}
		else if (trigger == Gamepad::RightTrigger)
		{
			outputState.rightTriggerEffect.effectType = DS5W::TriggerEffectType::SectionResitance;
			outputState.rightTriggerEffect.Section.startPosition = 255.0f * startPosition;
			outputState.rightTriggerEffect.Section.endPosition = 255.0f * endPosition;
		}
	}
	
}