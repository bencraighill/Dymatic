#pragma once
#include "Dymatic/Core/Timestep.h"
#include "Dymatic/Scene/Components.h"

namespace Dymatic {
	
	class PhysicsEngine
	{
	public:
		static void Init();
		static void Shutdown();
	};

}