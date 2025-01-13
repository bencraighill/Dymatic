#pragma once

#include <cstdint>
#include <unordered_set>

namespace Dymatic {

	typedef uint64_t PhysicsLayerID;

	struct PhysicsLayer
	{
		std::string Name;
		std::unordered_set<PhysicsLayerID> ExclusionMask;
	};

	struct PhysicsSettings
	{
		bool AllowSleeping = true;
		float SleepTimer = 0.5f;
		uint32_t PositionSteps = 2;
		uint32_t VelocitySteps = 10;
		bool Deterministic = true;

		std::unordered_map<PhysicsLayerID, PhysicsLayer> Layers;
	};

}