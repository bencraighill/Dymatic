#pragma once
#include "Dymatic/Core/Timestep.h"
#include "Dymatic/Scene/Components.h"

namespace Dymatic {

	struct RaycastHit
	{
		bool Hit = false;
		uint64_t EntityID = 0;
		float Distance = 0.0f;
		glm::vec3 Position = glm::vec3(0.0f);
		glm::vec3 Normal = glm::vec3(0.0f);
	};
	
	class PhysicsEngine
	{
	public:
		static void Init();
		static void Shutdown();
	};

}