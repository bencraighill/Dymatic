#include "dypch.h"
#include "Dymatic/Physics/PhysicsEngine.h"

#include "Dymatic/Physics/PhysX.h"

namespace Dymatic {

	extern Dymatic::PhysicsErrorCallback g_DefaultErrorCallback;

	physx::PxPhysics* g_PhysicsEngine = nullptr;
	physx::PxCooking* g_PhysicsCooking = nullptr;

	physx::PxFoundation* g_PhysicsFoundation = nullptr;
	physx::PxDefaultAllocator g_Allocator;
	physx::PxTolerancesScale g_ToleranceScale;
	physx::PxPvd* g_Pvd = nullptr;
	physx::PxMaterial* g_DefaultMaterial = nullptr;

	using namespace physx;

	void PhysicsEngine::Init()
	{
		DY_CORE_INFO("Initialising NVIDIA PhysX - Version: {0}.{1}.{2}", 
			PX_PHYSICS_VERSION_MAJOR, 
			PX_PHYSICS_VERSION_MINOR,
			PX_PHYSICS_VERSION_BUGFIX
		);
		
		// Init PhysX
		g_PhysicsFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, g_Allocator, g_DefaultErrorCallback);
		if (!g_PhysicsFoundation)
			DY_CORE_ASSERT(false, "PxCreateFoundation failed!");

		// Setup Connection to PVD
		g_Pvd = PxCreatePvd(*g_PhysicsFoundation);
		physx::PxPvdTransport* transport = physx::PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);
		g_Pvd->connect(*transport, physx::PxPvdInstrumentationFlag::eALL);
		g_ToleranceScale.length = 100.0f;        // typical length of an object
		g_ToleranceScale.speed = 981.0f;         // typical speed of an object, gravity*1s is a reasonable choice

		// Init Engine
		g_PhysicsEngine = PxCreatePhysics(PX_PHYSICS_VERSION, *g_PhysicsFoundation, g_ToleranceScale, false, g_Pvd);
		if (!g_PhysicsEngine)
			DY_CORE_ASSERT(false, "PxCreatePhysics failed!");

		// Init Cooking
		g_PhysicsCooking = PxCreateCooking(PX_PHYSICS_VERSION, *g_PhysicsFoundation, g_ToleranceScale);
		if (!g_PhysicsCooking)
			DY_CORE_ASSERT(false, "PxCreateCooking failed!");

		// Create default material
		g_DefaultMaterial = g_PhysicsEngine->createMaterial(0.5f, 0.5f, 0.6f);

		// Init Vehicle SDK
		physx::PxInitVehicleSDK(*g_PhysicsEngine, nullptr);
		physx::PxVec3 up(0.0f, 1.0f, 0.0f);
		physx::PxVec3 forward(0.0f, 0.0f, 1.0f);
		physx::PxVehicleSetBasisVectors(up, forward);
		physx::PxVehicleSetUpdateMode(physx::PxVehicleUpdateMode::eVELOCITY_CHANGE);
	}

	void PhysicsEngine::Shutdown()
	{
		//g_Pvd->release();
		//g_PhysicsCooking->release();
		//g_PhysicsEngine->release();
		//g_PhysicsFoundation->release();
	}

}