#include "dypch.h"
#include "Dymatic/Physics/PhysX.h"

namespace Dymatic {

	PhysicsErrorCallback g_DefaultErrorCallback;
	
	void PhysicsErrorCallback::reportError(physx::PxErrorCode::Enum code, const char* message, const char* file, int line)
	{
		DY_CORE_ERROR("PHYSX ERROR: Code({0}), \"{1}\" in '{2}', at line {3}", code, message, file, line);
	}

}