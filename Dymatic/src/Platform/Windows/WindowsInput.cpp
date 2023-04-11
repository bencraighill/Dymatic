#include "dypch.h"
#include "Dymatic/Core/Input.h"

#include "Dymatic/Core/Application.h"
#include <GLFW/glfw3.h>

#include <SDL_gamecontroller.h>

namespace Dymatic {

	bool Input::IsKeyPressed(const KeyCode key)
	{
		auto* window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
		auto state = glfwGetKey(window, static_cast<int32_t>(key));
		return state == GLFW_PRESS;
	}

	bool Input::IsMouseButtonPressed(const MouseCode button)
	{
		auto* window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
		auto state = glfwGetMouseButton(window, static_cast<int32_t>(button));
		return state == GLFW_PRESS;
	}

	glm::vec2 Input::GetMousePosition()
	{
		auto* window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);

		return { (float)xpos, (float)ypos };
	}

	float Input::GetMouseX()
	{
		return GetMousePosition().x;
	}

	float Input::GetMouseY()
	{
		return GetMousePosition().y;
	}

	bool Input::IsGamepadConnected(int gamepad)
	{
		return glfwJoystickPresent(gamepad);
	}

	std::string Input::GetGamepadName(int gamepad)
	{
		return glfwGetGamepadName(gamepad);
	}

	bool Input::IsGamepadButtonPressed(int gamepad, GamepadButtonCode button)
	{
		if (!IsGamepadConnected(gamepad))
			return false;
		
		int count;
		return glfwGetJoystickButtons(gamepad, &count)[button];
	}

	float Input::GetGamepadAxis(int gamepad, GamepadAxisCode axis)
	{
		//SDL_GameController* controller = SDL_GameControllerOpen(gamepad);
		//int16_t axisValue = SDL_GameControllerGetAxis(controller, (SDL_GameControllerAxis)axis);
		//SDL_GameControllerClose(controller);
		//
		//return axisValue / 32767.0f;

		int count;
		return glfwGetJoystickAxes(gamepad, &count)[axis];
	}

	glm::vec3 Input::GetGamepadSensor(int gamepad, GamepadSensorCode sensor)
	{
		SDL_GameController* controller = SDL_GameControllerOpen(gamepad);
		glm::vec3 sensorValue;
		SDL_GameControllerGetSensorData(controller, (SDL_SensorType)sensor, (float*)&sensorValue, 3);
		SDL_GameControllerClose(controller);
		
		return sensorValue;
	}

	bool Input::SetGamepadRumble(int gamepad, float left, float right, float duration)
	{
		SDL_GameController* controller = SDL_GameControllerOpen(gamepad);
		bool rumble = SDL_GameControllerRumble(controller, left, right, duration);
		SDL_GameControllerClose(controller);
		
		return rumble;
	}
	
	bool Input::SetGamepadLED(int gamepad, glm::vec3 color)
	{
		SDL_GameController* controller = SDL_GameControllerOpen(gamepad);
		bool led = SDL_GameControllerSetLED(controller, color.r, color.g, color.b);
		SDL_GameControllerClose(controller);
		
		return led;
	}
	
}