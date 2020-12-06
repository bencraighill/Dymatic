#pragma once

#include <glm/glm.hpp>

#include "Dymatic/Core/Keycodes.h"
#include "Dymatic/Core/MouseCodes.h"

namespace Dymatic {

	class Input
	{
	public:
		static bool IsKeyPressed(KeyCode key);

		static bool IsMouseButtonPressed(MouseCode button);
		static glm::vec2 GetMousePosition();
		static float GetMouseX();
		static float GetMouseY();
	};


}
