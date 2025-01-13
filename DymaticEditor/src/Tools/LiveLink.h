#pragma once

#include "Dymatic/Core/Timestep.h"
#include "Dymatic/Core/Base.h"
#include <glm/glm.hpp>

namespace Dymatic {

	class Scene;

	class LiveLink
	{
	public:
		struct MotionData
		{
			// Note: Orientation is given in radians
			float Roll = 0.0f;
			float Pitch = 0.0f;
			float Yaw = 0.0f;
			
			glm::vec3 Position;
			glm::vec3 Acceleration;
			glm::vec3 Velocity;
		};

	public:
		static void Init();
		static void Shutdown();
		static const MotionData& GetMotionData();

		static void OnImGuiRender(Timestep ts, Ref<Scene> activeScene);
		static void ToggleVisibility();
	};

}