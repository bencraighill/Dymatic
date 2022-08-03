#include "dypch.h"
#include "Dymatic/Physics/PhysicsEngine.h"

#ifdef DY_RELEASE
#define NDEBUG
#endif
#include <PxPhysics.h>
#include <PxPhysicsAPI.h>

namespace Dymatic {

	physx::PxPhysics* g_PhysicsEngine = nullptr;
	physx::PxCooking* g_PhysicsCooking = nullptr;

	class UserErrorCallback : public physx::PxErrorCallback
	{
	public:
		virtual void reportError(physx::PxErrorCode::Enum code, const char* message, const char* file, int line)
		{
			DY_CORE_ERROR("PHYSX ERROR: Code({0}), \"{1}\" in '{2}', at line {3}", code, message, file, line);
		}
	};

	physx::PxFoundation* g_PhysicsFoundation = nullptr;
	physx::PxDefaultAllocator g_DefaultAllocatorCallback;
	UserErrorCallback g_DefaultErrorCallback;
	physx::PxTolerancesScale g_ToleranceScale;
	physx::PxPvd* g_Pvd = nullptr;

	void PhysicsEngine::Init()
	{
		// Init PhysX
		g_PhysicsFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, g_DefaultAllocatorCallback, g_DefaultErrorCallback);
		if (!g_PhysicsFoundation)
			DY_CORE_ASSERT(false, "PxCreateFoundation failed!");

		// Setup System
		g_Pvd = PxCreatePvd(*g_PhysicsFoundation);
		physx::PxPvdTransport* transport = physx::PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);
		g_Pvd->connect(*transport, physx::PxPvdInstrumentationFlag::eALL);
		g_ToleranceScale.length = 100;        // typical length of an object
		g_ToleranceScale.speed = 981;         // typical speed of an object, gravity*1s is a reasonable choice

		// Init Engine
		g_PhysicsEngine = PxCreatePhysics(PX_PHYSICS_VERSION, *g_PhysicsFoundation, g_ToleranceScale, true, g_Pvd);
		if (!g_PhysicsEngine)
			DY_CORE_ASSERT(false, "PxCreatePhysics failed!");

		// Init Cooking
		g_PhysicsCooking = PxCreateCooking(PX_PHYSICS_VERSION, *g_PhysicsFoundation, g_ToleranceScale);
		if (!g_PhysicsCooking)
			DY_CORE_ASSERT(false, "PxCreateCooking failed!");
	}

	void PhysicsEngine::Shutdown()
	{
		//g_Pvd->release();
		//g_PhysicsCooking->release();
		//g_PhysicsEngine->release();
		//g_PhysicsFoundation->release();
	}

}