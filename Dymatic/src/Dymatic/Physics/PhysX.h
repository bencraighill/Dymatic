#ifdef DY_RELEASE
#define NDEBUG
#endif
#include <PxPhysics.h>
#include <PxPhysicsAPI.h>

namespace Dymatic {
	
	class PhysicsErrorCallback : public physx::PxErrorCallback
	{
	public:
		virtual void reportError(physx::PxErrorCode::Enum code, const char* message, const char* file, int line);
	};
}