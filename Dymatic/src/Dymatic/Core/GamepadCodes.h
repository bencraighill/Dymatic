#pragma once

namespace Dymatic
{
	using GamepadButtonCode = uint16_t;
	using GamepadAxisCode = uint16_t;
	using GamepadSensorCode = uint16_t;

	namespace Gamepad
	{
		enum : uint16_t
		{
			Invalid = -1,
		};
		
		enum : GamepadButtonCode
		{
			// From glfw3.h
			
			A = 0,
			B = 1,
			X = 2,
			Y = 3,
			LeftBumper = 4,
			RightBumper = 5,
			Back = 6,
			Start = 7,
			Guide = 8,
			LeftThumb = 9,
			RightThumb = 10,
			DPadUp = 11,
			DPadRight = 12,
			DPadDown = 13,
			DPadLeft = 14,
			
			Cross = A,
			Circle = B,
			Square = X,
			Triangle = Y
		};
		
		enum : GamepadAxisCode
		{
			// From glfw3.h

			LeftX = 0,
			LeftY = 1,
			RightX = 2,
			RightY = 3,
			LeftTrigger = 4,
			RightTrigger = 5
		};

		enum GamepadSensorCode
		{
			// From SDL_gamecontroller.h
			Unknown = 0,
			
			Accelerometer = 1,
			Gyroscope = 2,

			// For Joy-Con controllers
			AccelerometerLeft = 3,
			GyroscopeLeft = 4,
			AccelerometerRight = 5,
			GyroscopeRight = 6
		};
	}
}