#include "dypch.h"
#include "Dymatic/Physics/PhysicsEngine.h"

#include "Dymatic/Core/Hash.h"

#include <Jolt/Jolt.h>

#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>

#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>
#include <Jolt/Physics/Collision/Shape/ScaledShape.h>

#include <Jolt/Physics/SoftBody/SoftBodyCreationSettings.h>
#include <Jolt/Physics/Body/MotionProperties.h>
#include <Jolt/Physics/SoftBody/SoftBodyMotionProperties.h>

#include <Jolt/Physics/Ragdoll/Ragdoll.h>
#include <Jolt/Skeleton/Skeleton.h>

#include <Jolt/Physics/Constraints/PointConstraint.h>
#include <Jolt/Physics/Constraints/ConeConstraint.h>
#include <Jolt/Physics/Constraints/DistanceConstraint.h>
#include <Jolt/Physics/Constraints/HingeConstraint.h>
#include <Jolt/Physics/Constraints/FixedConstraint.h>
#include <Jolt/Physics/Constraints/GearConstraint.h>
#include <Jolt/Physics/Constraints/PulleyConstraint.h>
#include <Jolt/Physics/Constraints/RackAndPinionConstraint.h>
#include <Jolt/Physics/Constraints/SwingTwistConstraint.h>
#include <Jolt/Physics/Constraints/SliderConstraint.h>
#include <Jolt/Physics/Constraints/SixDOFConstraint.h>
#include <Jolt/Physics/Constraints/PathConstraint.h>
#include <Jolt/Physics/Constraints/PathConstraintPathHermite.h>

#include <Jolt/Physics/Collision/Shape/HeightFieldShape.h>

#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>

#include <Jolt/Physics/Vehicle/VehicleConstraint.h>
#include <Jolt/Physics/Vehicle/WheeledVehicleController.h>
#include <Jolt/Physics/Vehicle/TrackedVehicleController.h>
#include <Jolt/Physics/Vehicle/MotorcycleController.h>
#include <Jolt/Physics/Vehicle/Wheel.h>

#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/ShapeCast.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>

#include <Jolt/Geometry/OrientedBox.h>

#include "Dymatic/Scene/Scene.h"
#include "Dymatic/Scene/Components.h"
#include "Dymatic/Scene/Entity.h"
#include "Dymatic/Scripting/ScriptEngine.h"
#include "Dymatic/Project/Project.h"

// Editor/Debug Only
#ifdef JPH_DEBUG_RENDERER
	#include <Jolt/Renderer/DebugRendererRecorder.h>
	#include <Jolt/Core/StreamWrapper.h>
#endif

namespace Dymatic {

#ifdef JPH_DEBUG_RENDERER
	static const std::filesystem::path JoltRecordingDirectory = "logs/Jolt";
	static bool s_DebugLogging = false;
#endif

	// Constants
	static constexpr float c_FixedTimestep = 1.0f / 60.0f;
	static constexpr int c_MaxFrameSteps = 10;
	static constexpr size_t TempBufferSize = 10 * 1024 * 1024;
	static constexpr JPH::uint MaxBodies = 65536;
	static constexpr JPH::uint NumBodyMutexes = 0; // Use default settings
	static constexpr JPH::uint MaxBodyPairs = 65536;
	static constexpr JPH::uint MaxContactConstraints = 10240;

	// List of all physics components that need handling on scene setup/shutdown
	// Note: Components such as the `FieldComponent` are not here as they only affect the physics update
	using AllPhysicsComponents = ComponentGroup<
		RigidBodyComponent, SoftBodyComponent, RagdollComponent,
		PointConstraintComponent, ConeConstraintComponent, DistanceConstraintComponent, HingeConstraintComponent, FixedConstraintComponent,
		SliderConstraintComponent, SixDOFConstraintComponent, FollowConstraintComponent,
		CharacterMovementComponent, SpringArmComponent, LandscapeComponent
	>;

	namespace Utils {

		static void JoltTrace(const char* inFMT, ...)
		{
			// Format the message
			va_list list;
			va_start(list, inFMT);
			char buffer[1024];
			vsnprintf(buffer, sizeof(buffer), inFMT, list);
			va_end(list);

			// Print to the TTY
			DY_CORE_TRACE("[Jolt Physics] {}", fmt::format(inFMT, buffer));
		}

#ifdef JPH_ENABLE_ASSERTS
		static bool JoltAssert(const char* inExpression, const char* inMessage, const char* inFile, JPH::uint inLine)
		{
			DY_CORE_ERROR("[Jolt Physics] {}:{} ({}) {}", inFile, inLine, inExpression, inMessage ? inMessage : "[No Message]");
			DY_CORE_ASSERT(false);

			// Breakpoint
			return true;
		}
#endif

		static JPH::RVec3 ConvertGLMVec3ToJPH(const glm::vec3& vector)
		{
			return JPH::RVec3(vector.x, vector.y, vector.z);
		}

		static JPH::RVec3 GetJoltAxis(const glm::vec3& axis)
		{
			return ConvertGLMVec3ToJPH(glm::normalize(axis));
		}

		static glm::vec3 ConvertJPHVec3ToGLM(const JPH::RVec3& vector)
		{
			return glm::vec3(vector.GetX(), vector.GetY(), vector.GetZ());
		}

		static JPH::Quat ConvertGLMRotationToJPH(const glm::vec3& rotation)
		{
			const glm::quat glmQuat = glm::quat(rotation);
			return JPH::Quat(glmQuat.x, glmQuat.y, glmQuat.z, glmQuat.w);
		}

		static JPH::Quat ConvertGLMQuatToJPH(const glm::quat& quat)
		{
			return JPH::Quat(quat.x, quat.y, quat.z, quat.w);
		}

		static glm::quat ConvertJPHQuatToGLM(const JPH::Quat& jphQuat)
		{
			return glm::quat(jphQuat.GetX(), jphQuat.GetY(), jphQuat.GetZ(), jphQuat.GetW());
		}

		static glm::mat4 ConvertJPHMat4ToGLM(const JPH::Mat44& jphMatrix)
		{
			glm::mat4 matrix;

			for (int row = 0; row < 4; row++)
				for (int col = 0; col < 4; col++)
					matrix[col][row] = jphMatrix(row, col);

			return matrix;
		}

		static JPH::Mat44 ConvertGLMMat4ToJPH(const glm::mat4& matrix)
		{
			return JPH::Mat44(
				JPH::Vec4(matrix[0][0], matrix[0][1], matrix[0][2], matrix[0][3]),
				JPH::Vec4(matrix[1][0], matrix[1][1], matrix[1][2], matrix[1][3]),
				JPH::Vec4(matrix[2][0], matrix[2][1], matrix[2][2], matrix[2][3]),
				JPH::Vec4(matrix[3][0], matrix[3][1], matrix[3][2], matrix[3][3])
			);
		}

	}

	// Note: Having lots of these is inefficient so should not be one-to-one correspondence with layers if we have lots of them (probably just static and dynamic geometry)
	namespace BroadPhaseLayers
	{
		static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
		static constexpr JPH::BroadPhaseLayer MOVING(1);
		static constexpr JPH::uint NUM_LAYERS(2);
	};

	// Callbacks for Worker Threads to ensure they are thread safe!
	static std::unordered_map<int, void*> s_WorkerThreadScriptContext;

	static void OnWorkerThreadInit(int threadIndex)
	{
		s_WorkerThreadScriptContext[threadIndex] = ScriptEngine::RegisterThread();
	}

	static void OnWorkerThreadShutdown(int threadIndex)
	{
		if (s_WorkerThreadScriptContext.find(threadIndex) == s_WorkerThreadScriptContext.end())
			return;

		ScriptEngine::UnregisterThread(s_WorkerThreadScriptContext.at(threadIndex));
	}

	struct PhysicsSceneData;

	static std::unordered_map<uint64_t, uint16_t> s_LayerIDToIndexMap;
	static std::vector<std::vector<bool>> s_LayerCollisionMatrix;

	static constexpr JPH::uint16 NullPhysicsLayer = 0xFFFF;

	namespace Utils {
	
		static JPH::uint16 GetPhysicsJoltLayerFromIndex(const uint16_t index, const bool moving)
		{
			return index * 2 + (moving ? 1 : 0);
		}

		static JPH::uint16 GetPhysicsJoltLayerFromID(const PhysicsLayerID layerID, const bool moving)
		{
			return GetPhysicsJoltLayerFromIndex(s_LayerIDToIndexMap.find(layerID) != s_LayerIDToIndexMap.end() ? s_LayerIDToIndexMap.at(layerID) : 0, moving);
		}

		static uint16_t GetPhysicsLayerIndexFromJoltLayer(const JPH::uint16 layer)
		{
			return layer / 2;
		}

		static bool IsPhysicsLayerMovable(const JPH::uint16 layer)
		{
			if (layer == NullPhysicsLayer)
				return false;

			return layer % 2 == 1;
		}
	
	}

	class DymaticObjectLayerPairFilter : public JPH::ObjectLayerPairFilter
	{
	public:
		virtual bool ShouldCollide(JPH::ObjectLayer object1, JPH::ObjectLayer object2) const override
		{
			if (object1 == NullPhysicsLayer || object2 == NullPhysicsLayer)
				return false;

			// If neither layer is movable no collision test should occur
			if (!Utils::IsPhysicsLayerMovable(object1) && !Utils::IsPhysicsLayerMovable(object2))
				return false;

			return s_LayerCollisionMatrix[Utils::GetPhysicsLayerIndexFromJoltLayer(object1)][Utils::GetPhysicsLayerIndexFromJoltLayer(object2)];
		}
	};

	class DymaticBroadPhaseLayerInterface final : public JPH::BroadPhaseLayerInterface
	{
	public:
		virtual JPH::uint GetNumBroadPhaseLayers() const override
		{
			return BroadPhaseLayers::NUM_LAYERS;
		}

		virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer layer) const override
		{
			return Utils::IsPhysicsLayerMovable(layer) ? BroadPhaseLayers::MOVING : BroadPhaseLayers::NON_MOVING;
		}

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
		virtual const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override
		{
			switch ((JPH::BroadPhaseLayer::Type)inLayer)
			{
			case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING:	return "NON_MOVING";
			case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::MOVING:		return "MOVING";
			}

			DY_CORE_ASSERT(false);
			return "INVALID";
		}
#endif
	};

	class DymaticObjectVsBroadPhaseLayerFilter : public JPH::ObjectVsBroadPhaseLayerFilter
	{
	public:
		virtual bool ShouldCollide(JPH::ObjectLayer layer1, JPH::BroadPhaseLayer layer2) const override
		{
			if (layer1 == NullPhysicsLayer)
				return false;

			return Utils::IsPhysicsLayerMovable(layer1) || layer2 == BroadPhaseLayers::MOVING;
		}
	};

	class DymaticContactListener : public JPH::ContactListener
	{
	public:
		DymaticContactListener(PhysicsSceneData* data)
			: m_Data(data)
		{}

		virtual JPH::ValidateResult	OnContactValidate(const JPH::Body& body1, const JPH::Body& body2, JPH::RVec3Arg baseOffset, const JPH::CollideShapeResult& collisionResult) override;
		
		virtual void OnContactAdded(const JPH::Body& body1, const JPH::Body& body2, const JPH::ContactManifold& manifold, JPH::ContactSettings& ioSettings) override;
		virtual void OnContactPersisted(const JPH::Body& body1, const JPH::Body& body2, const JPH::ContactManifold& manifold, JPH::ContactSettings& ioSettings) override;
		virtual void OnContactRemoved(const JPH::SubShapeIDPair& subShapePair) override;
	private:
		bool ValidateAndGetEntities(JPH::uint64 id1, JPH::uint64 id2, Entity& entity1, Entity& entity2);

		template<void(*Function)(Entity, Entity, const glm::vec3&, const glm::vec3&)>
		void OnContactMethod(const JPH::Body& body1, const JPH::Body& body2, const JPH::ContactManifold& manifold, JPH::ContactSettings& ioSettings);

	private:
		PhysicsSceneData* m_Data;
	};

	class DymaticBodyActivationListener : public JPH::BodyActivationListener
	{
	public:
		virtual void OnBodyActivated(const JPH::BodyID& bodyID, JPH::uint64 inBodyUserData) override
		{
		}

		virtual void OnBodyDeactivated(const JPH::BodyID& inBodyID, JPH::uint64 inBodyUserData) override
		{
		}
	};

#ifdef JPH_DEBUG_RENDERER
	struct DebugRendererRecorderData
	{
		DebugRendererRecorderData(const std::filesystem::path& filepath)
			: InternalStream(filepath, std::ofstream::out | std::ofstream::binary | std::ofstream::trunc), StreamWrapper(InternalStream), RendererRecorder(StreamWrapper)
		{}

		std::ofstream InternalStream;
		JPH::StreamOutWrapper StreamWrapper;
		JPH::DebugRendererRecorder RendererRecorder;
	};
#endif

	struct PhysicsSceneData
	{
		PhysicsSceneData::PhysicsSceneData(Scene* scene)
			: EntityScene(scene), SceneContactListener(this)
		{
			// Note: This is called from a job so whatever is done here needs to be thread safe.
			// TODO: Testing with Jolt Samples implies this may be a major performance hog. We should further experiment with disabling this
			System.SetContactListener(&SceneContactListener);
		}

		Scene* EntityScene;
		JPH::PhysicsSystem System;
		DymaticContactListener SceneContactListener;
		JPH::CharacterVsCharacterCollisionSimple CharacterVsCharacterCollision;
	};

	JPH::ValidateResult	DymaticContactListener::OnContactValidate(const JPH::Body& body1, const JPH::Body& body2, JPH::RVec3Arg baseOffset, const JPH::CollideShapeResult& collisionResult)
	{
		// Can ignore a contact here before it is created (Note: using layers to not make objects collide is a cheaper solution!)
		return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
	}

	template<void(*Function)(Entity, Entity, const glm::vec3&, const glm::vec3&)>
	void DymaticContactListener::OnContactMethod(const JPH::Body& body1, const JPH::Body& body2, const JPH::ContactManifold& manifold, JPH::ContactSettings& ioSettings)
	{
		Entity entity1, entity2;
		if (!ValidateAndGetEntities(body1.GetUserData(), body2.GetUserData(), entity1, entity2))
			return;

		const glm::vec3 normal = Utils::ConvertJPHVec3ToGLM(manifold.mWorldSpaceNormal);
		if (!body1.IsSensor())
			Function(entity1, entity2, Utils::ConvertJPHVec3ToGLM(manifold.GetWorldSpaceContactPointOn1(0)), -normal);

		if (!body2.IsSensor())
			Function(entity2, entity1, Utils::ConvertJPHVec3ToGLM(manifold.GetWorldSpaceContactPointOn2(0)), normal);
	}

	void DymaticContactListener::OnContactAdded(const JPH::Body& body1, const JPH::Body& body2, const JPH::ContactManifold& manifold, JPH::ContactSettings& ioSettings)
	{
		OnContactMethod<ScriptEngine::OnContactEntity>(body1, body2, manifold, ioSettings);
	}

	void DymaticContactListener::OnContactPersisted(const JPH::Body& body1, const JPH::Body& body2, const JPH::ContactManifold& manifold, JPH::ContactSettings& ioSettings)
	{
		OnContactMethod<ScriptEngine::OnContactPersistedEntity>(body1, body2, manifold, ioSettings);
	}

	void DymaticContactListener::OnContactRemoved(const JPH::SubShapeIDPair& subShapePair)
	{
		const auto& bodyInterface = m_Data->System.GetBodyInterfaceNoLock();

		Entity entity1, entity2;
		if (!ValidateAndGetEntities(bodyInterface.GetUserData(subShapePair.GetBody1ID()), bodyInterface.GetUserData(subShapePair.GetBody2ID()), entity1, entity2))
			return;

		ScriptEngine::OnContactRemovedEntity(entity1, entity2);
		ScriptEngine::OnContactRemovedEntity(entity2, entity1);
	}

	bool DymaticContactListener::ValidateAndGetEntities(JPH::uint64 id1, JPH::uint64 id2, Entity& entity1, Entity& entity2)
	{
		if (id1 == 0 || id2 == 0)
			return false;

		const entt::entity e1 = (entt::entity)id1;
		const entt::entity e2 = (entt::entity)id2;

		const auto& registry = m_Data->EntityScene->GetRegistry();
		if (!registry.valid(e1) || !registry.valid(e2))
			return false;

		entity1 = { e1, m_Data->EntityScene };
		entity2 = { e2, m_Data->EntityScene };

		return true;
	}

	// Jolt Interfaces
	static DymaticBroadPhaseLayerInterface s_BroadPhaseLayerInterface;
	static DymaticObjectVsBroadPhaseLayerFilter s_ObjectVsBroadphaseLayerFilter;
	static DymaticObjectLayerPairFilter s_ObjectVsObjectLayerFilter;
	static DymaticBodyActivationListener s_BodyActivationListener;
	static JPH::CharacterVsCharacterCollisionSimple s_CharacterVsCharacterCollision;

	// Jolt Systems
	static JPH::TempAllocatorImpl* s_TempAllocator;
	static JPH::JobSystemThreadPool* s_JobSystem;

	void PhysicsEngine::Init()
	{
		DY_CORE_INFO("Initializing Jolt Physics - Version: {}.{}.{}",
			JPH_VERSION_MAJOR,
			JPH_VERSION_MINOR,
			JPH_VERSION_PATCH
		);

		JPH::RegisterDefaultAllocator();

		// Setup Logging and Asserts
		JPH::Trace = Utils::JoltTrace;
		JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = Utils::JoltAssert);

		JPH::Factory::sInstance = new JPH::Factory();

		JPH::RegisterTypes();
		s_TempAllocator = new JPH::TempAllocatorImpl(TempBufferSize);

		// Initialize the Job Scheduler and ensure that all threads are registered with Dymatic Script Core
		// Note: If we ever have our own job scheduler we shouldn't use this example implementation and should make it work for us
		s_JobSystem = new JPH::JobSystemThreadPool();
		s_JobSystem->SetThreadInitFunction(OnWorkerThreadInit);
		s_JobSystem->SetThreadExitFunction(OnWorkerThreadShutdown);
		s_JobSystem->Init(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, JPH::thread::hardware_concurrency() - 1);
	}

	void PhysicsEngine::Shutdown()
	{
		delete s_JobSystem;
		delete s_TempAllocator;
		JPH::UnregisterTypes();

		delete JPH::Factory::sInstance;
		JPH::Factory::sInstance = nullptr;
	}

	void PhysicsEngine::SetDebugLogsEnabled(const bool enabled)
	{
#if JPH_DEBUG_RENDERER
		s_DebugLogging = enabled;
#endif
	}

	struct RuntimeSoftBodyData
	{
		JPH::Body* Body;
		std::unordered_map<uint32_t, uint32_t> VertexMap;
	};

	namespace Utils {

		static JPH::uint64 GetEntityUserData(Entity entity)
		{
			return (JPH::uint64)(entt::entity)entity;
		}

		static bool IsUserDataValidEntity(Scene* scene, JPH::uint64 userData)
		{
			if (userData == 0)
				return false;

			return scene->DoesEntityExist((entt::entity)userData);
		}

		static void BuildPhysicsSkeletonFromHierarchy(JPH::Ref<JPH::Skeleton> skeleton, const BoneNodeData& node, int parentIndex)
		{
			int currentJointIndex = skeleton->AddJoint(node.Name, parentIndex);

			for (const auto& child : node.Children)
				BuildPhysicsSkeletonFromHierarchy(skeleton, child, currentJointIndex);
		}

		static JPH::Ref<JPH::SoftBodySharedSettings> CreateSoftBodyFromMesh(const Ref<Mesh> mesh, const glm::vec3& scale, std::unordered_map<uint32_t, uint32_t>& vertexMap, const float vertexMass, bool useVertexColorWeight)
		{
			JPH::SoftBodySharedSettings* settings = new JPH::SoftBodySharedSettings;

			std::unordered_map<glm::vec3, uint32_t> vertexPositionMap;

			const auto& vertices = mesh->GetVerticies();
			for (uint32_t vertexIndex = 0; vertexIndex < vertices.size(); vertexIndex++)
			{
				const auto& vertex = vertices[vertexIndex];

				// Check if we are tracking this vertex already
				if (vertexPositionMap.find(vertex.Position) == vertexPositionMap.end())
				{
					// If not insert it to the physics mesh and position map
					JPH::SoftBodySharedSettings::Vertex v;
					v.mPosition = JPH::Float3(vertex.Position.x * scale.x, vertex.Position.y * scale.y, vertex.Position.z * scale.z);
					v.mInvMass = (1.0f / vertexMass) * (useVertexColorWeight ? vertex.Color.b : 1.0f);
					
					vertexPositionMap[vertex.Position] = settings->mVertices.size();
					settings->mVertices.push_back(v);
				}

				// Regardless update the rendering to physics vertex mapping
				vertexMap[vertexIndex] = vertexPositionMap.at(vertex.Position);
			}

			const auto& indices = mesh->GetIndicies();
			auto vertexIndex = [&](uint32_t index) -> JPH::uint
			{
				return vertexMap.at(indices[index]);
			};

			// Add edges based on triangle mesh
			for (size_t i = 0; i < indices.size(); i += 3)
			{
				const JPH::uint v0 = vertexIndex(i + 0);
				const JPH::uint v1 = vertexIndex(i + 1);
				const JPH::uint v2 = vertexIndex(i + 2);
			
				JPH::SoftBodySharedSettings::Edge e;
			
				e.mVertex[0] = v0;
				e.mVertex[1] = v1;
				settings->mEdgeConstraints.push_back(e);
			
				e.mVertex[0] = v1;
				e.mVertex[1] = v2;
				settings->mEdgeConstraints.push_back(e);
			
				e.mVertex[0] = v2;
				e.mVertex[1] = v0;
				settings->mEdgeConstraints.push_back(e);
			}
			
			settings->CalculateEdgeLengths();

			// TODO: Options to add more structure to the interior of the mesh
			// Use tetrahedrons similar to cube sample and then call `CalculateVolumeConstraintVolumes()`

			// Add faces
			for (size_t i = 0; i < indices.size(); i += 3)
			{
				JPH::SoftBodySharedSettings::Face f;
				f.mVertex[0] = vertexIndex(i + 0);
				f.mVertex[1] = vertexIndex(i + 1);
				f.mVertex[2] = vertexIndex(i + 2);
				settings->AddFace(f);
			}

			//JPH::SoftBodySharedSettings::VertexAttributes vertexAttributes;
			//vertexAttributes.mShearCompliance = vertexAttributes.mCompliance = 1.0e-3f;
			//vertexAttributes.mLRAType = JPH::SoftBodySharedSettings::ELRAType::GeodesicDistance;
			//settings->CreateConstraints(&vertexAttributes, 1, JPH::SoftBodySharedSettings::EBendType::None);

			settings->Optimize();
			return settings;
		}

		template<typename ConstraintComponentType, typename ConstraintSetupFunction>
		static void OnConstraintAdded(PhysicsSceneData* data, ConstraintComponentType& cc, Entity entity, ConstraintSetupFunction function, const bool allowGlobalAttachment = false)
		{
			if (!allowGlobalAttachment && !cc.Target)
				return;

			Entity otherEntity = data->EntityScene->GetEntityByUUID(cc.Target);

			if (!allowGlobalAttachment && (!otherEntity || !otherEntity.HasComponent<TransformComponent>()))
				return;

			JPH::Constraint* constraint = function(cc, entity, otherEntity);

			if (!constraint)
				return;

			if (!cc.Enabled)
				constraint->SetEnabled(false);

			cc.RuntimeConstraint = constraint;
			data->System.AddConstraint(constraint);
		}

		static void OnConstraintRemoved(PhysicsSceneData* data, void*& runtimeConstraint)
		{
			if (!runtimeConstraint)
				return;

			data->System.RemoveConstraint((JPH::Constraint*)runtimeConstraint);
			runtimeConstraint = nullptr;
		}

		template<typename ConstraintComponentType>
		static void OnConstraintRemoved(PhysicsSceneData* data, ConstraintComponentType& cc)
		{
			OnConstraintRemoved(data, cc.RuntimeConstraint);
		}

		static void PopulateEntitySpringConstraint(Entity entity, JPH::SpringSettings& settings)
		{
			if (!entity.HasComponent<SpringConstraintComponent>())
				return;
			
			const SpringConstraintComponent& scc = entity.GetComponent<SpringConstraintComponent>();
			settings.mDamping = scc.Damping;

			if (scc.Type == SpringConstraintComponent::SpringType::FrequencyAndDamping)
			{
				settings.mMode = JPH::ESpringMode::FrequencyAndDamping;
				settings.mFrequency = scc.Frequency;
			}
			else
			{
				settings.mMode = JPH::ESpringMode::StiffnessAndDamping;
				settings.mStiffness = scc.Stiffness;
			}
		}

		static JPH::Ref<JPH::PathConstraintPathHermite> BuildFollowConstraintPath(const FollowConstraintComponent& fcc, const SplineComponent& sc)
		{
			JPH::Ref<JPH::PathConstraintPathHermite> path = new JPH::PathConstraintPathHermite;
			path->SetIsLooping(fcc.Looping);
			const JPH::Vec3 normal = Utils::ConvertGLMVec3ToJPH(fcc.Normal);

			// Last point cannot share position with first point
			if (sc.Points.size() < 2 || glm::all(glm::epsilonEqual(sc.Points.front().Position, sc.Points.back().Position, 1.0e-6f)))
			{
				DY_WARN("Last point cannot share position with first point when looping on Follow Constraint!");
				return nullptr;
			}

			const uint32_t pointCount = sc.Points.size();
			for (uint32_t pointIndex = 0; pointIndex < pointCount; pointIndex++)
			{
				const auto& point = sc.Points[pointIndex];
				DY_CORE_ASSERT(point.Tangent != glm::vec3(0.0f));
				path->AddPoint(Utils::ConvertGLMVec3ToJPH(point.Position), Utils::ConvertGLMVec3ToJPH(point.Tangent), normal);
			}

			return path;
		}

	}

	PhysicsScene::PhysicsScene(Scene* scene)
	{
		// Setup Jolt Physics System for Scene
		m_Data = new PhysicsSceneData(scene);
		m_Data->System.Init(MaxBodies, NumBodyMutexes, MaxBodyPairs, MaxContactConstraints, s_BroadPhaseLayerInterface, s_ObjectVsBroadphaseLayerFilter, s_ObjectVsObjectLayerFilter);
		m_Data->System.SetGravity(Utils::ConvertGLMVec3ToJPH(m_Data->EntityScene->m_Gravity));

		const auto& physicsSettings = Project::GetActiveConfig().PhysicsSettings;

		// Set System Settings
		JPH::PhysicsSettings settings;
		settings.mAllowSleeping = physicsSettings.AllowSleeping;
		settings.mTimeBeforeSleep = physicsSettings.SleepTimer;
		settings.mNumPositionSteps = physicsSettings.PositionSteps;
		settings.mNumVelocitySteps = physicsSettings.VelocitySteps;
		settings.mDeterministicSimulation = physicsSettings.Deterministic;
		m_Data->System.SetPhysicsSettings(settings);

		// Add Activation Listener
		m_Data->System.SetBodyActivationListener(&s_BodyActivationListener);

		// Setup Internal/External Layer Mappings
		s_LayerCollisionMatrix.clear();
		s_LayerIDToIndexMap.clear();

		const size_t layerCount = physicsSettings.Layers.size();
		s_LayerIDToIndexMap.reserve(layerCount);
		s_LayerCollisionMatrix.resize(layerCount);

		// Assign physics layer mappings
		uint64_t layerIndex = 0;
		for (const auto& [id, layer] : physicsSettings.Layers)
		{
			s_LayerIDToIndexMap[id] = layerIndex;
			s_LayerCollisionMatrix[layerIndex].resize(layerCount);

			uint64_t otherLayerIndex = 0;
			for (const auto& [otherID, otherLayer] : physicsSettings.Layers)
			{
				s_LayerCollisionMatrix[layerIndex][otherLayerIndex] = layer.ExclusionMask.find(otherID) == layer.ExclusionMask.end();
				otherLayerIndex++;
			}

			layerIndex++;
		}

#ifdef JPH_DEBUG_RENDERER
		if (s_DebugLogging)
		{
			if (!std::filesystem::exists(JoltRecordingDirectory))
				std::filesystem::create_directories(JoltRecordingDirectory);

			m_Recorder = new DebugRendererRecorderData(JoltRecordingDirectory / fmt::format("{}_capture_{}.jor", m_Data->EntityScene->Handle, UUID()));
		}
#endif

		AddAllComponents(AllPhysicsComponents{});

		// Setup Vehicles
		//auto  vehicleView = m_Data->EntityScene->m_Registry.view<VehicleMovementComponent, TransformComponent, RigidbodyComponent>();
		//for (auto e : ragdollView)
		//{
		//	Entity entity = { e, m_Data->EntityScene };
		//
		//	auto& [vmc, transform, rbc] = vehicleView.get<VehicleMovementComponent, TransformComponent, RigidbodyComponent>(e);
		//
		//	JPH::VehicleConstraintSettings settings;
		//	settings.mMaxPitchRollAngle = JPH::DegreesToRadians(60.0f);
		//
		//	JPH::WheelSettingsWV* wheel;
		//
		//	JPH::WheeledVehicleControllerSettings* controller = new JPH::MotorcycleControllerSettings;
		//
		//	controller->mEngine.mMaxTorque = 150.0f;
		//	controller->mEngine.mMinRPM = 1000.0f;
		//	controller->mEngine.mMaxRPM = 10000.0f;
		//	controller->mTransmission.mShiftDownRPM = 2000.0f;
		//	controller->mTransmission.mShiftUpRPM = 8000.0f;
		//	controller->mTransmission.mGearRatios = { 2.27f, 1.63f, 1.3f, 1.09f, 0.96f, 0.88f }; // From: https://www.blocklayer.com/rpm-gear-bikes
		//	controller->mTransmission.mReverseGearRatios = { -4.0f };
		//	controller->mTransmission.mClutchStrength = 2.0f;
		//	settings.mController = controller;
		//	
		//	settings.mWheels = {};
		//	settings.mWheels.push_back(wheel->);
		//
		//	controller->mDifferentials.resize(1);
		//	controller->mDifferentials[0].mLeftWheel = -1;
		//	controller->mDifferentials[0].mRightWheel = 1;
		//	controller->mDifferentials[0].mDifferentialRatio = 1.93f * 40.0f / 16.0f;
		//
		//	JPH::VehicleConstraint* vehicle = new JPH::VehicleConstraint(, settings);
		//	vmc.RuntimeVehicle = vehicle;
		//
		//	vehicle->SetVehicleCollisionTester(new JPH::VehicleCollisionTesterCastCylinder(Layers::MOVING, 1.0f)); // Use half wheel width as convex radius so we get a rounded cylinder
		//	m_Data->System.AddConstraint(vehicle);
		//	m_Data->System.AddStepListener(vehicle);
		//}

		//Utils::ConstraintEntityIterator<HingeConstraintComponent>(m_Data, [this](HingeConstraintComponent& hcc, Entity entity, Entity otherEntity)
		//{
		//	JPH::HingeConstraintSettings settings;
		//	
		//	if (fcc.Type == FixedConstraintComponent::JointType::Automatic)
		//		settings.mAutoDetectPoint = true;
		//	else
		//	{
		//		settings.mSpace = fcc.Type == FixedConstraintComponent::JointType::Local ? JPH::EConstraintSpace::LocalToBodyCOM : JPH::EConstraintSpace::WorldSpace;
		//		settings.mPoint1 = Utils::ConvertGLMVec3ToJPH(fcc.Point1);
		//		settings.mAxisX1 = Utils::ConvertGLMVec3ToJPH(fcc.Point1AxisX);
		//		settings.mAxisY1 = Utils::ConvertGLMVec3ToJPH(fcc.Point1AxisY);
		//		settings.mPoint2 = Utils::ConvertGLMVec3ToJPH(fcc.Point2);
		//		settings.mAxisX2 = Utils::ConvertGLMVec3ToJPH(fcc.Point2AxisX);
		//		settings.mAxisY2 = Utils::ConvertGLMVec3ToJPH(fcc.Point2AxisY);
		//	}
		//
		//	// TODO: Add motor and spring options
		//	// settings.mMotorSettings
		//	// settings.mLimitsSpringSettings
		//
		//	return settings.Create(
		//		*(JPH::Body*)entity.GetComponent<RigidbodyComponent>().RuntimeBody,
		//		*(JPH::Body*)otherEntity.GetComponent<RigidbodyComponent>().RuntimeBody
		//	);
		//});

		// Force Fields (No Setup Required)

		// Sensors (No Setup Required)
		
		// TODO:
		// - Constraints
		// - Spring Arm
		// - Vehicles

		// Note: This should be called infrequently as it is expensive to compute so only do when starting scene
		m_Data->System.OptimizeBroadPhase();
	}

	PhysicsScene::~PhysicsScene()
	{
		RemoveAllComponents(AllPhysicsComponents{});

#ifdef JPH_DEBUG_RENDERER
		if (m_Recorder)
		{
			delete m_Recorder;
			m_Recorder = nullptr;
		}
#endif

		delete m_Data;
	}

	// TODO: REMOVE! (This is copied from AnimationNode source. Move to shared AnimationCore header)
	static void LocalToComponentSpace(const std::unordered_map<std::string, BoneInfo>& boneInfoMap, const BoneNodeData& node, const Pose& inPose, Pose& outPose, const glm::mat4& parentTransform)
	{
		if (boneInfoMap.find(node.Name) == boneInfoMap.end())
			return;

		const uint32_t index = boneInfoMap.at(node.Name).id;
		const glm::mat4 componentTransformation = parentTransform * inPose.BoneMatrices[index];

		outPose.BoneMatrices[index] = componentTransformation;

		for (const auto& child : node.Children)
			LocalToComponentSpace(boneInfoMap, child, inPose, outPose, componentTransformation);
	}

	static void ComponentToLocalSpace(const std::unordered_map<std::string, BoneInfo>& boneInfoMap, const BoneNodeData& node, const Pose& inPose, Pose& outPose, const glm::mat4& parentTransform)
	{
		if (boneInfoMap.find(node.Name) == boneInfoMap.end())
			return;

		const uint32_t index = boneInfoMap.at(node.Name).id;

		const glm::mat4 componentTransformation = inPose.BoneMatrices[index];
		const glm::mat4 localTransformation = glm::inverse(parentTransform) * componentTransformation;

		outPose.BoneMatrices[index] = localTransformation;

		for (const auto& child : node.Children)
			ComponentToLocalSpace(boneInfoMap, child, inPose, outPose, componentTransformation);
	}

	void PhysicsScene::OnUpdate(Timestep ts)
	{
		const float deltaTime = ts.GetSeconds();

		// Pre-Physics Update
		auto followConstraintView = m_Data->EntityScene->m_Registry.view<FollowConstraintComponent>();
		for (auto e : followConstraintView)
		{
			const auto& fcc = followConstraintView.get<FollowConstraintComponent>(e);

			if (!fcc.RuntimeConstraint)
				continue;

			JPH::PathConstraint* constraint = (JPH::PathConstraint*)fcc.RuntimeConstraint;

#ifdef TODO
			// Update the spline path if changed
			if (Entity target = m_Data->EntityScene->GetEntityByUUID(fcc.Target))
			{
				if (target.HasComponent<SplineComponent>())
				{
					JPH::Ref<JPH::PathConstraintPath> path = Utils::BuildFollowConstraintPath(fcc, target.GetComponent<SplineComponent>());
					if (path)
						constraint->SetPath(path, constraint->GetPathFraction());
				}
			}
#endif

#ifdef JPH_DEBUG_RENDERER
			if (m_Recorder)
				constraint->DrawConstraint(&m_Recorder->RendererRecorder);
#endif

			if (constraint->GetPositionMotorState() != (JPH::EMotorState)fcc.Motor.MotorState)
				constraint->SetPositionMotorState((JPH::EMotorState)fcc.Motor.MotorState);

			if (constraint->GetTargetVelocity() != fcc.TargetVelocity)
				constraint->SetTargetVelocity(fcc.TargetVelocity);

			const float targetFraction = glm::clamp(fcc.TargetPathFraction, 0.0f, 1.0f) * constraint->GetPath()->GetPathMaxFraction();
			if (constraint->GetTargetPathFraction() != targetFraction)
				constraint->SetTargetPathFraction(targetFraction);

			JPH::MotorSettings& motorSettings = constraint->GetPositionMotorSettings();
			motorSettings.SetForceLimit(fcc.Motor.MaxMotorAcceleration / constraint->GetBody2()->GetMotionProperties()->GetInverseMass()); // (F = m * a)
			motorSettings.mSpringSettings.mFrequency = fcc.Motor.Frequency;
			motorSettings.mSpringSettings.mDamping = fcc.Motor.Damping;
			constraint->SetMaxFrictionForce(fcc.MaxFrictionAcceleration / constraint->GetBody2()->GetMotionProperties()->GetInverseMass());
		}

		// Characters
		auto characterView = m_Data->EntityScene->m_Registry.view<TransformComponent, CharacterMovementComponent>();
		for (auto e : characterView)
		{
			auto& [transform, cmc] = characterView.get<TransformComponent, CharacterMovementComponent>(e);

			if (!cmc.RuntimeController)
				continue;

			JPH::CharacterVirtual* character = (JPH::CharacterVirtual*)cmc.RuntimeController;

			// Setup character settings
			JPH::CharacterVirtual::ExtendedUpdateSettings updateSettings;
			updateSettings.mStickToFloorStepDown = cmc.StickToFloor ? (-character->GetUp() * updateSettings.mStickToFloorStepDown.Length()) : JPH::Vec3::sZero();
			updateSettings.mWalkStairsStepUp = character->GetUp() * cmc.MaxStepHeight;
			updateSettings.mWalkStairsMinStepForward = cmc.MinStepForward;

			JPH::ObjectLayer layer = Utils::GetPhysicsJoltLayerFromID(cmc.Layer, true);

			// Prevent increased speed on diagonals
			if (glm::length(cmc.RuntimeMovementDirection) > 1.0f)
				cmc.RuntimeMovementDirection = glm::normalize(cmc.RuntimeMovementDirection);

			// Update velocity
			const float blendWeight = cmc.VelocityBlendWeight * (character->IsSupported() ? 1.0f : cmc.AirControl);
			cmc.PreviousMovementDirection = blendWeight * cmc.RuntimeMovementDirection * cmc.MaxWalkSpeed + (1.0f - blendWeight) * cmc.PreviousMovementDirection;
			JPH::Vec3 newVelocity = Utils::ConvertGLMVec3ToJPH(cmc.PreviousMovementDirection);

			// Allow the player to slide if we intended to move
			const bool moving = Utils::ConvertGLMVec3ToJPH(cmc.RuntimeMovementDirection).IsNearZero();

			character->UpdateGroundVelocity();

			JPH::Vec3 currentVerticalVelocity = character->GetLinearVelocity().Dot(character->GetUp()) * character->GetUp();
			JPH::Vec3 groundVelocity = character->GetGroundVelocity();
			const bool movingTowardsGround = (currentVerticalVelocity.GetY() - groundVelocity.GetY()) < 0.1f;
			if (character->GetGroundState() == JPH::CharacterVirtual::EGroundState::OnGround && movingTowardsGround)
			{
				// Assume velocity of ground when on ground
				newVelocity += groundVelocity;

				// Jump
				if (cmc.RuntimeJump && movingTowardsGround)
					newVelocity += cmc.JumpSpeed * character->GetUp();
			}
			else
				newVelocity += currentVerticalVelocity;

			cmc.IsFalling = (character->GetGroundState() == JPH::CharacterVirtual::EGroundState::InAir);
			cmc.RuntimeJump = false;
			cmc.RuntimeLinearVelocity = Utils::ConvertJPHVec3ToGLM(character->GetLinearVelocity());
			cmc.RuntimeGroundVelocity = Utils::ConvertJPHVec3ToGLM(character->GetGroundVelocity());

			// Gravity
			newVelocity += m_Data->System.GetGravity() * cmc.GravityScale * deltaTime;

			// Actual velocity update
			character->SetLinearVelocity(newVelocity);

			// Update rotation
			JPH::Vec3 velocityDirection = newVelocity.Normalized();
			velocityDirection.SetY(0.0f);
			if (cmc.RotateToMotion && !velocityDirection.IsNearZero())
			{
				const JPH::Vec3 worldForward = JPH::Vec3(0.0f, 0.0f, 1.0f);
				const JPH::Vec3 forward = character->GetRotation() * worldForward;

				const float angleBetween = glm::acos(glm::clamp(forward.Dot(velocityDirection), -1.0f, 1.0f));
				const float maxRotationThisFrame = glm::radians(cmc.RotationRate) * deltaTime;
				const float blendFactor = glm::min(maxRotationThisFrame / angleBetween, 1.0f);

				// Apply the rotation to the character
				if (blendFactor > 0.01f)
				{
					JPH::Quat targetRotation = JPH::Quat::sFromTo(worldForward, velocityDirection);
					character->SetRotation(character->GetRotation().SLERP(targetRotation, blendFactor));
				}
			}

			// Update character position
			character->ExtendedUpdate(
				deltaTime,
				-character->GetUp() * m_Data->System.GetGravity().Length(),
				updateSettings,
				m_Data->System.GetDefaultBroadPhaseLayerFilter(layer),
				m_Data->System.GetDefaultLayerFilter(layer),
				{},
				{},
				*s_TempAllocator
			);
		}

		// Note: Generally doing 1 collision step per 1 / 60th of a second (round up) is good.
		const int collisionSteps = glm::min((int)std::ceil(ts / c_FixedTimestep), c_MaxFrameSteps);
		m_Data->System.Update(ts, collisionSteps, s_TempAllocator, s_JobSystem);

#ifdef JPH_DEBUG_RENDERER
		if (m_Recorder)
		{
			// Draw the state of the world to the Jolt Capture
			JPH::BodyManager::DrawSettings settings;
			m_Data->System.DrawBodies(settings, &m_Recorder->RendererRecorder);

			// Mark end of frame
			m_Recorder->RendererRecorder.EndFrame();
		}
#endif

		JPH::BodyInterface& bodyInterface = m_Data->System.GetBodyInterface();

		// Post Physics Update

		// Rigid Bodies
		auto rigidBodyView = m_Data->EntityScene->m_Registry.view<TransformComponent, RigidBodyComponent>();
		for (auto e : rigidBodyView)
		{
			auto& [tc, rbc] = rigidBodyView.get<TransformComponent, RigidBodyComponent>(e);

			if (!rbc.RuntimeBody)
				continue;

			const JPH::BodyID bodyID = ((JPH::Body*)rbc.RuntimeBody)->GetID();

			JPH::RVec3 position;
			JPH::Quat rotation;
			bodyInterface.GetPositionAndRotation(bodyID, position, rotation);

			tc.Transform.Translation = Utils::ConvertJPHVec3ToGLM(position);
			tc.Transform.Rotation = Utils::ConvertJPHVec3ToGLM(rotation.GetEulerAngles());
		}

		// Soft Bodies
		auto softBodyView = m_Data->EntityScene->m_Registry.view<TransformComponent, SoftBodyComponent>();
		for (auto e : softBodyView)
		{
			auto& [tc, sbc] = softBodyView.get<TransformComponent, SoftBodyComponent>(e);

			if (!sbc.RuntimeBody)
				continue;

			RuntimeSoftBodyData* runtimeBody = (RuntimeSoftBodyData*)sbc.RuntimeBody;
			const JPH::BodyID bodyID = runtimeBody->Body->GetID();

			JPH::RVec3 position;
			JPH::Quat rotation;
			bodyInterface.GetPositionAndRotation(bodyID, position, rotation);

			tc.Transform.Translation = Utils::ConvertJPHVec3ToGLM(position);
			tc.Transform.Rotation = Utils::ConvertJPHVec3ToGLM(rotation.GetEulerAngles());

			JPH::BodyLockRead lock(m_Data->System.GetBodyLockInterface(), bodyID);
			if (lock.Succeeded())
			{
				const JPH::Body& body = lock.GetBody();

				const JPH::SoftBodyMotionProperties* motionProperties = static_cast<const JPH::SoftBodyMotionProperties*>(body.GetMotionProperties());
				const auto& softBodyVertices = motionProperties->GetVertices();

				Ref<Mesh> mesh = sbc.RuntimeModel->GetMeshesEditable()[0];
				std::vector<MeshVertex>& vertices = mesh->GetVerticesEditable();
				const auto& indices = mesh->GetIndicies();

				for (size_t vertexIndex = 0; vertexIndex < vertices.size(); vertexIndex++)
				{
					const uint32_t physicsVertexIndex = runtimeBody->VertexMap.at(vertexIndex);
					auto& vertex = vertices[vertexIndex];
					vertex.Position = Utils::ConvertJPHVec3ToGLM(softBodyVertices[physicsVertexIndex].mPosition) / tc.Transform.Scale;
				}

				// Re-compute normal (TODO: Use SIMD or some other method to speed this up)
				for (size_t indicesIndex = 0; indicesIndex < indices.size(); indicesIndex += 3)
				{
					auto& v0 = vertices[indices[indicesIndex + 0]];
					auto& v1 = vertices[indices[indicesIndex + 1]];
					auto& v2 = vertices[indices[indicesIndex + 2]];

					const glm::vec3 edge1 = v1.Position - v0.Position;
					const glm::vec3 edge2 = v2.Position - v0.Position;

					v0.Normal = v1.Normal = v2.Normal = glm::normalize(glm::cross(edge1, edge2));
				}

				mesh->UpdateVertexData();
			}
		}

		// Ragdolls
		auto ragdollView = m_Data->EntityScene->m_Registry.view<TransformComponent, RagdollComponent, StaticMeshComponent>();
		for (auto e : ragdollView)
		{
			auto& [tc, rdc, smc] = ragdollView.get<TransformComponent, RagdollComponent, StaticMeshComponent>(e);

			if (!rdc.RuntimeBody)
				continue;

			JPH::Ragdoll* ragdoll = (JPH::Ragdoll*)rdc.RuntimeBody;

			const size_t bodyCount = ragdoll->GetBodyCount();
			JPH::RVec3 rootOffset;
			std::vector<JPH::Mat44> boneMatrices(bodyCount);
			ragdoll->GetPose(rootOffset, boneMatrices.data());

			const auto& boneInfoMap = smc.m_Model->GetSkeleton()->GetBoneInfoMap();

			for (const auto& [name, boneInfo] : boneInfoMap)
				rdc.RuntimePose->at(boneInfo.id) = Utils::ConvertJPHMat4ToGLM(boneMatrices[boneInfo.id]);

			const bool correctLocalOffset = true;
			if (correctLocalOffset)
			{
				Pose basePose;
				basePose.BoneMatrices.resize(rdc.RuntimePose->size());
				for (const auto& [name, boneInfo] : boneInfoMap)
					basePose.BoneMatrices[boneInfo.id] = glm::inverse(boneInfo.offset);
				ComponentToLocalSpace(boneInfoMap, smc.m_Model->GetSkeleton()->GetRootNode(), basePose, basePose, glm::mat4(1.0f));

				Pose physicsPose;
				physicsPose.BoneMatrices = *rdc.RuntimePose;
				ComponentToLocalSpace(boneInfoMap, smc.m_Model->GetSkeleton()->GetRootNode(), physicsPose, physicsPose, glm::mat4(1.0f));

				// Keep translation and apply rotation locally
				for (const auto& [name, boneInfo] : boneInfoMap)
				{
					auto& physicsMatrix = physicsPose.BoneMatrices[boneInfo.id];

					// Apply rotation and keep translation
					physicsMatrix = glm::mat4_cast(glm::quat_cast(physicsMatrix));
					physicsMatrix[3] = glm::vec4(glm::vec3(basePose.BoneMatrices[boneInfo.id][3]), 1.0f);
				}

				LocalToComponentSpace(boneInfoMap, smc.m_Model->GetSkeleton()->GetRootNode(), physicsPose, physicsPose, glm::mat4(1.0f));
				*rdc.RuntimePose = physicsPose.BoneMatrices;
			}

			for (const auto& [name, boneInfo] : boneInfoMap)
				rdc.RuntimePose->at(boneInfo.id) *= boneInfo.offset;

			tc.Transform.Translation = Utils::ConvertJPHVec3ToGLM(rootOffset);
		}

		// Characters
		for (auto e : characterView)
		{
			auto& [tc, cmc] = characterView.get<TransformComponent, CharacterMovementComponent>(e);

			if (!cmc.RuntimeController)
				continue;

			JPH::CharacterVirtual* character = (JPH::CharacterVirtual*)cmc.RuntimeController;

			tc.Transform.Translation = Utils::ConvertJPHVec3ToGLM(character->GetPosition());
			tc.Transform.Rotation = Utils::ConvertJPHVec3ToGLM(character->GetRotation().GetEulerAngles());
		}

		// Spring Arms
		auto springArmView = m_Data->EntityScene->m_Registry.view<TransformComponent, SpringArmComponent>();
		for (auto e : springArmView)
		{
			auto& [tc, sac] = springArmView.get<TransformComponent, SpringArmComponent>(e);

			sac.TargetLength;

			// Perform raycast for collision
			Entity entity = { e, m_Data->EntityScene };

			const Transform transform = entity.GetWorldTransform();
			const glm::mat4 rotation = glm::toMat4(transform.Rotation);
			std::vector<RaycastHit> hits;
			SphereShapeCastMultihit(glm::vec3(rotation * glm::vec4(sac.TargetOffset, 1.0f)) + transform.Translation, rotation * glm::vec4(0.0f, 0.0f, 1.0f, 1.0f), sac.TargetLength, sac.ProbeRadius, hits);
			sac.CurrentLength = sac.TargetLength;

			// Check for a valid collision
			for (const auto& hit : hits)
			{
				if (sac.ExclusionMask->find(hit.EntityID) == sac.ExclusionMask->end())
				{
					sac.CurrentLength = hit.Distance;
					break;
				}
			}
		}

		// Fields
		auto fieldView = m_Data->EntityScene->m_Registry.view<TransformComponent, FieldComponent>();
		for (auto e : fieldView)
		{
			auto& [tc, fc] = fieldView.get<TransformComponent, FieldComponent>(e);
		
			class FieldCollector : public JPH::CollideShapeBodyCollector
			{
			public:
				FieldCollector(JPH::PhysicsSystem* system, TransformComponent* tc, FieldComponent* fc, float deltaTime)
					: m_System(system), m_TransformComponent(tc), m_ForceComponent(fc), m_DeltaTime(deltaTime) {}

				virtual void AddHit(const JPH::BodyID& inBodyID) override
				{
					JPH::BodyLockWrite lock(m_System->GetBodyLockInterface(), inBodyID);
					JPH::Body& body = lock.GetBody();
					if (!body.IsActive())
						return;
					
					const FieldComponent::FieldType fieldType = m_ForceComponent->Type;
					if (fieldType == FieldComponent::FieldType::Directional)
					{
						body.AddForce(Utils::ConvertGLMVec3ToJPH(m_ForceComponent->Force));
					}
					else if (fieldType == FieldComponent::FieldType::Radial)
					{
						const glm::vec3 bodyPosition = Utils::ConvertJPHVec3ToGLM(body.GetPosition());
						const float distance = glm::distance(m_TransformComponent->Transform.Translation, bodyPosition);
						const float magnitude = m_ForceComponent->Magnitude * (1.0f / (std::exp(distance * m_ForceComponent->Falloff)));
						const glm::vec3 direction = glm::normalize(bodyPosition - m_TransformComponent->Transform.Translation);

						body.AddForce(Utils::ConvertGLMVec3ToJPH(direction * magnitude));
					}
					else if (fieldType == FieldComponent::FieldType::Buoyancy)
					{
						const glm::mat4 transform = m_TransformComponent->Transform.GetMatrix();
						const JPH::Vec3 surfacePosition = Utils::ConvertGLMVec3ToJPH(glm::vec3(transform * glm::vec4(0.0f, 1.0f, 0.0f, 1.0f)));
						const JPH::Vec3 surfaceNormal = Utils::ConvertGLMVec3ToJPH(glm::vec3(glm::toQuat(transform) * glm::vec4(0.0f, 1.0f, 0.0f, 1.0f)));

						body.ApplyBuoyancyImpulse(
							surfacePosition, surfaceNormal,
							m_ForceComponent->Buoyancy, m_ForceComponent->LinearDrag, m_ForceComponent->AngularDrag, Utils::ConvertGLMVec3ToJPH(m_ForceComponent->FluidVelocity),
							m_System->GetGravity(), m_DeltaTime
						);
					}
					else
					{
						DY_CORE_WARN("Body collected by unknown field type");
					}
				}

			private:
				JPH::PhysicsSystem* m_System;
				TransformComponent* m_TransformComponent;
				FieldComponent* m_ForceComponent;
				float m_DeltaTime;
			};
			
			FieldCollector collector(&m_Data->System, &tc, &fc, ts.GetSeconds());

			// Note: Jolt orientation matrix does not support scaling so we apply this ourselves here
			m_Data->System.GetBroadPhaseQuery().CollideOrientedBox(
				JPH::OrientedBox(Utils::ConvertGLMMat4ToJPH(tc.Transform.GetMatrixNoScale()), Utils::ConvertGLMVec3ToJPH(tc.Transform.Scale * 0.5f)),
				collector, JPH::SpecifiedBroadPhaseLayerFilter(BroadPhaseLayers::MOVING), JPH::SpecifiedObjectLayerFilter(Utils::GetPhysicsJoltLayerFromID(fc.Layer, true))
			);
		}
	}

	namespace Utils {

		static JPH::Body* GetEntityRigidJoltBody(Entity entity)
		{
			if (!entity || !entity.HasComponent<RigidBodyComponent>())
				return nullptr;

			const auto& rbc = entity.GetComponent<RigidBodyComponent>();
			return (JPH::Body*)rbc.RuntimeBody;
		}

		static JPH::Body* GetEntitySoftJoltBody(Entity entity)
		{
			if (!entity || !entity.HasComponent<SoftBodyComponent>())
				return nullptr;

			const auto& sbc = entity.GetComponent<SoftBodyComponent>();
			const RuntimeSoftBodyData* runtimeBody = (RuntimeSoftBodyData*)sbc.RuntimeBody;
			return runtimeBody->Body;
		}
	
		static JPH::Body* GetEntityJoltBody(Entity entity)
		{
			if (!entity)
				return nullptr;

			if (entity.HasComponent<RigidBodyComponent>())
				return GetEntityRigidJoltBody(entity);

			if (entity.HasComponent<SoftBodyComponent>())
				return GetEntitySoftJoltBody(entity);

			return nullptr;
		}

		static JPH::Body& GetOptionalBody(JPH::Body* body)
		{
			return body ? *body : JPH::Body::sFixedToWorld;
		}

		static JPH::EConstraintSpace GetJoltConstraintSpace(const ConstraintSpace space)
		{
			switch (space)
			{
			case ConstraintSpace::LocalSpace:
				return JPH::EConstraintSpace::LocalToBodyCOM;
			case ConstraintSpace::WorldSpace:
			case ConstraintSpace::Automatic:
				return JPH::EConstraintSpace::WorldSpace;
			}

			return JPH::EConstraintSpace::LocalToBodyCOM;
		}

		static JPH::ESwingType GetJoltSwingType(const ConstraintSwingType type)
		{
			return (JPH::ESwingType)type;
		}
	
	}

	void PhysicsScene::SetPositionInternal(Entity entity, const glm::vec3& position)
	{
		if (JPH::Body* body = Utils::GetEntityJoltBody(entity))
			m_Data->System.GetBodyInterface().SetPosition(body->GetID(), Utils::ConvertGLMVec3ToJPH(position), JPH::EActivation::Activate);
	}

	void PhysicsScene::SetRotationInternal(Entity entity, const glm::quat& rotation)
	{
		if (!entity.HasComponent<RigidBodyComponent>())
			return;

		if (JPH::Body* body = Utils::GetEntityJoltBody(entity))
			m_Data->System.GetBodyInterface().SetRotation(body->GetID(), Utils::ConvertGLMQuatToJPH(rotation), JPH::EActivation::Activate);
	}

	void PhysicsScene::UpdateConstraintEnabled(const ConstraintComponentBase* cc)
	{
		if (!cc || !cc->RuntimeConstraint)
			return;

		JPH::Constraint* constraint = (JPH::Constraint*)cc->RuntimeConstraint;
		constraint->SetEnabled(cc->Enabled);
	}

	void PhysicsScene::UpdateDistanceConstraintDistance(const DistanceConstraintComponent& dcc)
	{
		if (!dcc.RuntimeConstraint || dcc.Type == DistanceConstraintComponent::DistanceType::Default)
			return;

		JPH::DistanceConstraint* constraint = (JPH::DistanceConstraint*)dcc.RuntimeConstraint;

		if (dcc.Type == DistanceConstraintComponent::DistanceType::Range)
			constraint->SetDistance(dcc.MinDistance, dcc.MaxDistance);
		else
			constraint->SetDistance(dcc.Distance, dcc.Distance);
	}

	bool PhysicsScene::IsActive(Entity entity)
	{
		if (JPH::Body* body = Utils::GetEntityJoltBody(entity))
			return body->IsActive();

		return false;
	}

	void PhysicsScene::SetAllowSleeping(Entity entity, const bool allowSleeping)
	{
		if (JPH::Body* body = Utils::GetEntityJoltBody(entity))
			body->SetAllowSleeping(allowSleeping);
	}

	void PhysicsScene::ResetSleepTimer(Entity entity)
	{
		if (JPH::Body* body = Utils::GetEntityJoltBody(entity))
			body->ResetSleepTimer();
	}

	void PhysicsScene::SetPositionWithoutActivation(Entity entity, const glm::vec3& position)
	{
		if (JPH::Body* body = Utils::GetEntityJoltBody(entity))
			m_Data->System.GetBodyInterface().SetPosition(body->GetID(), Utils::ConvertGLMVec3ToJPH(position), JPH::EActivation::DontActivate);
	}

	void PhysicsScene::SetRotationWithoutActivation(Entity entity, const glm::vec3& rotation)
	{
		if (JPH::Body* body = Utils::GetEntityJoltBody(entity))
			m_Data->System.GetBodyInterface().SetRotation(body->GetID(), Utils::ConvertGLMQuatToJPH(glm::quat(glm::radians(rotation))), JPH::EActivation::DontActivate);
	}

	void PhysicsScene::Activate(Entity entity)
	{
		if (JPH::Body* body = Utils::GetEntityJoltBody(entity))
			m_Data->System.GetBodyInterfaceNoLock().ActivateBody(body->GetID());
	}

	void PhysicsScene::Deactivate(Entity entity)
	{
		if (JPH::Body* body = Utils::GetEntityJoltBody(entity))
			m_Data->System.GetBodyInterfaceNoLock().DeactivateBody(body->GetID());
	}

	void PhysicsScene::AddForce(Entity entity, const glm::vec3& force)
	{
		if (JPH::Body* body = Utils::GetEntityJoltBody(entity))
			body->AddForce(Utils::ConvertGLMVec3ToJPH(force));
	}

	void PhysicsScene::AddForce(Entity entity, const glm::vec3& force, const glm::vec3& position)
	{
		if (JPH::Body* body = Utils::GetEntityJoltBody(entity))
			body->AddForce(Utils::ConvertGLMVec3ToJPH(force), Utils::ConvertGLMVec3ToJPH(position));
	}

	void PhysicsScene::AddImpulse(Entity entity, const glm::vec3& impulse)
	{
		if (JPH::Body* body = Utils::GetEntityJoltBody(entity))
			body->AddImpulse(Utils::ConvertGLMVec3ToJPH(impulse));
	}

	void PhysicsScene::AddImpulse(Entity entity, const glm::vec3& impulse, const glm::vec3& position)
	{
		if (JPH::Body* body = Utils::GetEntityJoltBody(entity))
			body->AddImpulse(Utils::ConvertGLMVec3ToJPH(impulse), Utils::ConvertGLMVec3ToJPH(position));
	}

	void PhysicsScene::AddAngularImpulse(Entity entity, const glm::vec3& angularImpulse)
	{
		if (JPH::Body* body = Utils::GetEntityJoltBody(entity))
			body->AddAngularImpulse(Utils::ConvertGLMVec3ToJPH(angularImpulse));
	}

	void PhysicsScene::AddTorque(Entity entity, const glm::vec3& torque)
	{
		if (JPH::Body* body = Utils::GetEntityJoltBody(entity))
			body->AddTorque(Utils::ConvertGLMVec3ToJPH(torque));
	}

	void PhysicsScene::AddForceAndTorque(Entity entity, const glm::vec3& force, const glm::vec3& torque)
	{
		if (JPH::Body* body = Utils::GetEntityJoltBody(entity))
			m_Data->System.GetBodyInterfaceNoLock().AddForceAndTorque(body->GetID(), Utils::ConvertGLMVec3ToJPH(force), Utils::ConvertGLMVec3ToJPH(torque));
	}

	void PhysicsScene::ApplyBuoyancyImpulse(Entity entity, const glm::vec3& surfacePosition, const glm::vec3& surfaceNormal, const float buoyancy, const float linearDrag, const float angularDrag, const glm::vec3& fluidVelocity, const float deltaTime)
	{
		if (JPH::Body* body = Utils::GetEntityJoltBody(entity))
		{
			body->ApplyBuoyancyImpulse(
				Utils::ConvertGLMVec3ToJPH(surfacePosition),
				Utils::ConvertGLMVec3ToJPH(surfaceNormal),
				buoyancy, linearDrag, angularDrag,
				Utils::ConvertGLMVec3ToJPH(fluidVelocity),
				m_Data->System.GetGravity(),
				deltaTime
			);
		}
	}

	void PhysicsScene::MoveKinematic(Entity entity, const glm::vec3& position, const glm::vec3& rotation, const float deltaTime)
	{
		if (JPH::Body* body = Utils::GetEntityJoltBody(entity))
			body->MoveKinematic(Utils::ConvertGLMVec3ToJPH(position), Utils::ConvertGLMQuatToJPH(glm::quat(glm::radians(rotation))), deltaTime);
	}

	glm::vec3 PhysicsScene::GetCenterOfMassPosition(Entity entity)
	{
		if (JPH::Body* body = Utils::GetEntityJoltBody(entity))
			return Utils::ConvertJPHVec3ToGLM(body->GetCenterOfMassPosition());

		return {};
	}

	glm::vec3 PhysicsScene::GetAccumulatedForce(Entity entity)
	{
		// Note: This only applies on a per physics-tick basis
		if (JPH::Body* body = Utils::GetEntityJoltBody(entity))
			return Utils::ConvertJPHVec3ToGLM(body->GetAccumulatedForce());

		return {};
	}

	glm::vec3 PhysicsScene::GetAccumulatedTorque(Entity entity)
	{
		// Note: This only applies on a per physics-tick basis
		if (JPH::Body* body = Utils::GetEntityJoltBody(entity))
			return Utils::ConvertJPHVec3ToGLM(body->GetAccumulatedTorque());

		return {};
	}

	void PhysicsScene::ResetForce(Entity entity)
	{
		// Note: This only applies on a per physics-tick basis
		if (JPH::Body* body = Utils::GetEntityJoltBody(entity))
			body->ResetForce();
	}

	void PhysicsScene::ResetTorque(Entity entity)
	{
		// Note: This only applies on a per physics-tick basis
		if (JPH::Body* body = Utils::GetEntityJoltBody(entity))
			body->ResetTorque();
	}

	void PhysicsScene::ResetMotion(Entity entity)
	{
		if (JPH::Body* body = Utils::GetEntityJoltBody(entity))
			body->ResetMotion();
	}

	glm::vec3 PhysicsScene::GetLinearVelocity(Entity entity)
	{
		if (JPH::Body* body = Utils::GetEntityJoltBody(entity))
			return Utils::ConvertJPHVec3ToGLM(body->GetLinearVelocity());

		return {};
	}

	void PhysicsScene::SetLinearVelocity(Entity entity, const glm::vec3& linearVelocity)
	{
		if (JPH::Body* body = Utils::GetEntityJoltBody(entity))
			body->SetLinearVelocityClamped(Utils::ConvertGLMVec3ToJPH(linearVelocity));
	}

	void PhysicsScene::AddLinearVelocity(Entity entity, const glm::vec3& linearVelocity)
	{
		if (JPH::Body* body = Utils::GetEntityJoltBody(entity))
			m_Data->System.GetBodyInterface().AddLinearVelocity(body->GetID(), Utils::ConvertGLMVec3ToJPH(linearVelocity));
	}

	glm::vec3 PhysicsScene::GetAngularVelocity(Entity entity)
	{
		if (JPH::Body* body = Utils::GetEntityJoltBody(entity))
			return Utils::ConvertJPHVec3ToGLM(body->GetAngularVelocity());

		return {};
	}

	void PhysicsScene::SetAngularVelocity(Entity entity, const glm::vec3& angularVelocity)
	{
		if (JPH::Body* body = Utils::GetEntityJoltBody(entity))
			body->SetAngularVelocityClamped(Utils::ConvertGLMVec3ToJPH(angularVelocity));
	}

	void PhysicsScene::GetLinearAndAngularVelocity(Entity entity, glm::vec3& linearVelocity, glm::vec3& angularVelocity)
	{
		if (JPH::Body* body = Utils::GetEntityJoltBody(entity))
		{
			JPH::Vec3 outLinearVelocity, outAngularVelocity;
			m_Data->System.GetBodyInterfaceNoLock().GetLinearAndAngularVelocity(body->GetID(), outLinearVelocity, outAngularVelocity);

			linearVelocity = Utils::ConvertJPHVec3ToGLM(outLinearVelocity);
			angularVelocity = Utils::ConvertJPHVec3ToGLM(outAngularVelocity);
		}
	}

	void PhysicsScene::SetLinearAndAngularVelocity(Entity entity, const glm::vec3& linearVelocity, const glm::vec3& angularVelocity)
	{
		if (JPH::Body* body = Utils::GetEntityJoltBody(entity))
			m_Data->System.GetBodyInterfaceNoLock().SetLinearAndAngularVelocity(body->GetID(), Utils::ConvertGLMVec3ToJPH(linearVelocity), Utils::ConvertGLMVec3ToJPH(angularVelocity));
	}

	void PhysicsScene::AddLinearAndAngularVelocity(Entity entity, const glm::vec3& linearVelocity, const glm::vec3& angularVelocity)
	{
		if (JPH::Body* body = Utils::GetEntityJoltBody(entity))
			m_Data->System.GetBodyInterface().AddLinearAndAngularVelocity(body->GetID(), Utils::ConvertGLMVec3ToJPH(linearVelocity), Utils::ConvertGLMVec3ToJPH(angularVelocity));
	}

	glm::vec3 PhysicsScene::GetPointVelocity(Entity entity, const glm::vec3& point)
	{
		if (JPH::Body* body = Utils::GetEntityJoltBody(entity))
			return Utils::ConvertJPHVec3ToGLM(m_Data->System.GetBodyInterfaceNoLock().GetPointVelocity(body->GetID(), Utils::ConvertGLMVec3ToJPH(point)));

		return {};
	}

	void PhysicsScene::SetVertexPosition(Entity entity, const uint32_t index, const glm::vec3& position)
	{
		if (JPH::Body* body = Utils::GetEntitySoftJoltBody(entity))
		{
			JPH::SoftBodyMotionProperties* motionProperties = (JPH::SoftBodyMotionProperties*) body->GetMotionProperties();

			if (index >= motionProperties->GetVertices().size())
				return;

			motionProperties->GetVertex(index).mPosition = Utils::ConvertGLMVec3ToJPH(position);
		}
	}

	void PhysicsScene::SetVertexVelocity(Entity entity, const uint32_t index, const glm::vec3& velocity)
	{
		if (JPH::Body* body = Utils::GetEntitySoftJoltBody(entity))
		{
			JPH::SoftBodyMotionProperties* motionProperties = (JPH::SoftBodyMotionProperties*)body->GetMotionProperties();

			if (index >= motionProperties->GetVertices().size())
				return;

			motionProperties->GetVertex(index).mVelocity = Utils::ConvertGLMVec3ToJPH(velocity);
		}
	}

	void PhysicsScene::SetPositionWeighted(Entity entity, const glm::vec3& position)
	{
		if (JPH::Body* body = Utils::GetEntitySoftJoltBody(entity))
		{
			JPH::SoftBodyMotionProperties* motionProperties = (JPH::SoftBodyMotionProperties*)body->GetMotionProperties();

			const JPH::Vec3 currentPosition = body->GetPosition();

			// Compute center of fixed vertices
			JPH::Vec3 center = JPH::Vec3::sZero();
			uint32_t total = 0;
			auto& vertices = motionProperties->GetVertices();
			for (auto& vertex : vertices)
			{
				if (vertex.mInvMass == 0.0f)
				{
					center += vertex.mPosition;
					total++;
				}
			}

			center /= (float)total;
			const JPH::Vec3 delta = Utils::ConvertGLMVec3ToJPH(position) - (currentPosition + center);

			for (auto& vertex : vertices)
				if (vertex.mInvMass == 0.0f)
					vertex.mPosition += delta;
		}
	}

	void PhysicsScene::SetFixedPosition(Entity entity, const bool fixedPosition)
	{
		if (JPH::Body* body = Utils::GetEntitySoftJoltBody(entity))
		{
			JPH::SoftBodyMotionProperties* motionProperties = (JPH::SoftBodyMotionProperties*)body->GetMotionProperties();
			motionProperties->SetUpdatePosition(!fixedPosition);
		}
	}

	static void PopulateRaycastHit(RaycastHit& hit, PhysicsSceneData* data, const JPH::RRayCast& ray, const JPH::RayCastResult& result, const float length)
	{
		const JPH::RVec3 position = ray.GetPointOnRay(result.mFraction);

		hit.Position = Utils::ConvertJPHVec3ToGLM(position);
		hit.Distance = length * result.mFraction;

		JPH::BodyLockRead lock(data->System.GetBodyLockInterface(), result.mBodyID);
		if (lock.Succeeded())
		{
			const JPH::Body& hitBody = lock.GetBody();

			const JPH::PhysicsMaterial* material = hitBody.GetShape()->GetMaterial(result.mSubShapeID2);

			Entity entity = { entt::entity(hitBody.GetUserData()), data->EntityScene };

			hit.Normal = Utils::ConvertJPHVec3ToGLM(hitBody.GetWorldSpaceSurfaceNormal(result.mSubShapeID2, position));
			hit.EntityID = entity.GetUUID();
		}
	}

	RaycastHit PhysicsScene::Raycast(const glm::vec3& origin, const glm::vec3& direction, float length)
	{
		const glm::vec3 normalizedDirection = glm::normalize(direction);

		// Cast ray
		JPH::RRayCast ray;
		ray.mOrigin = Utils::ConvertGLMVec3ToJPH(origin);
		ray.mDirection = Utils::ConvertGLMVec3ToJPH(normalizedDirection * length);

		RaycastHit hit;
		JPH::RayCastResult result;
		hit.Hit = m_Data->System.GetNarrowPhaseQuery().CastRay(ray, result);

		if (hit.Hit)
			PopulateRaycastHit(hit, m_Data, ray, result, length);

		return hit;
	}

	void PhysicsScene::RaycastMultihit(const glm::vec3& origin, const glm::vec3& direction, float length, std::vector<RaycastHit>& hitResults)
	{
		const glm::vec3 normalizedDirection = glm::normalize(direction);

		// Cast ray
		JPH::RRayCast ray;
		ray.mOrigin = Utils::ConvertGLMVec3ToJPH(origin);
		ray.mDirection = Utils::ConvertGLMVec3ToJPH(normalizedDirection * length);

		JPH::RayCastSettings settings;
		settings.mBackFaceMode = JPH::EBackFaceMode::CollideWithBackFaces;
		settings.mTreatConvexAsSolid = true;

		// Use collector to gather all hits along the path
		JPH::AllHitCollisionCollector<JPH::CastRayCollector> collector;
		m_Data->System.GetNarrowPhaseQuery().CastRay(ray, settings, collector);
		collector.Sort();

		hitResults.reserve(collector.mHits.size());
		for (const JPH::RayCastResult& result : collector.mHits)
		{
			RaycastHit& hit = hitResults.emplace_back();
			hit.Hit = true;

			PopulateRaycastHit(hit, m_Data, ray, result, length);
		}
	}

	static void PopulateRaycastHit(RaycastHit& hit, PhysicsSceneData* data, const JPH::RShapeCast& cast, const JPH::ShapeCastResult& result, const float length)
	{
		const JPH::RVec3 position = cast.GetPointOnRay(result.mFraction);

		hit.Position = Utils::ConvertJPHVec3ToGLM(position);
		hit.Distance = length * result.mFraction;

		JPH::BodyLockRead lock(data->System.GetBodyLockInterface(), result.mBodyID2);
		if (lock.Succeeded())
		{
			const JPH::Body& hitBody = lock.GetBody();

			const JPH::PhysicsMaterial* material = hitBody.GetShape()->GetMaterial(result.mSubShapeID2);

			Entity entity = { entt::entity(hitBody.GetUserData()), data->EntityScene };

			hit.Normal = Utils::ConvertJPHVec3ToGLM(hitBody.GetWorldSpaceSurfaceNormal(result.mSubShapeID2, position));
			hit.EntityID = entity.GetUUID();
		}
	}

	static void InitializeShapeCastSettings(JPH::ShapeCastSettings& settings)
	{
		settings.mUseShrunkenShapeAndConvexRadius = false;
		settings.mActiveEdgeMode = JPH::EActiveEdgeMode::CollideOnlyWithActive;
		settings.mBackFaceModeTriangles = JPH::EBackFaceMode::CollideWithBackFaces;
		settings.mBackFaceModeConvex = JPH::EBackFaceMode::CollideWithBackFaces;
		settings.mReturnDeepestPoint = true;
		settings.mCollectFacesMode = JPH::ECollectFacesMode::NoFaces;
	}

	static RaycastHit ShapeCast(PhysicsSceneData* data,  const glm::vec3& start, const glm::vec3& direction, const float distance, JPH::RefConst<JPH::Shape> shape, const JPH::Mat44& rotation)
	{
		JPH::RShapeCast cast = JPH::RShapeCast::sFromWorldTransform(shape, JPH::Vec3::sReplicate(1.0f),
			JPH::RMat44::sTranslation(Utils::ConvertGLMVec3ToJPH(start)) * rotation, Utils::ConvertGLMVec3ToJPH(direction * distance));

		JPH::ShapeCastSettings settings;
		InitializeShapeCastSettings(settings);

		JPH::ClosestHitCollisionCollector<JPH::CastShapeCollector> collector;
		data->System.GetNarrowPhaseQuery().CastShape(cast, settings, JPH::Vec3::sZero(), collector);

		RaycastHit hit;
		if (hit.Hit = collector.HadHit())
			PopulateRaycastHit(hit, data, cast, collector.mHit, distance);

		return hit;
	}

	static void ShapeCastMultihit(PhysicsSceneData* data, const glm::vec3& start, const glm::vec3& direction, const float distance, JPH::RefConst<JPH::Shape> shape, const JPH::Mat44& rotation, std::vector<RaycastHit>& hitResults)
	{
		JPH::RShapeCast cast = JPH::RShapeCast::sFromWorldTransform(shape, JPH::Vec3::sReplicate(1.0f),
			JPH::RMat44::sTranslation(Utils::ConvertGLMVec3ToJPH(start)) * rotation, Utils::ConvertGLMVec3ToJPH(direction * distance));

		JPH::ShapeCastSettings settings;
		InitializeShapeCastSettings(settings);

		JPH::AllHitCollisionCollector<JPH::CastShapeCollector> collector;
		data->System.GetNarrowPhaseQuery().CastShape(cast, settings, JPH::Vec3::sZero(), collector);

		hitResults.reserve(collector.mHits.size());
		for (const JPH::ShapeCastResult& result : collector.mHits)
		{
			RaycastHit& hit = hitResults.emplace_back();
			hit.Hit = true;

			PopulateRaycastHit(hit, data, cast, result, distance);
		}
	}

	RaycastHit PhysicsScene::BoxShapeCast(const glm::vec3& start, const glm::vec3& direction, const float distance, const glm::vec3& halfSize, const glm::vec3& orientation)
	{
		JPH::RefConst<JPH::Shape> shape = new JPH::BoxShape(Utils::ConvertGLMVec3ToJPH(halfSize));
		const JPH::Mat44 rotation = Utils::ConvertGLMMat4ToJPH(glm::toMat4(glm::quat(glm::radians(orientation))));
		return ShapeCast(m_Data, start, direction, distance, shape, rotation);
	}

	void PhysicsScene::BoxShapeCastMultihit(const glm::vec3& start, const glm::vec3& direction, const float distance, const glm::vec3& halfSize, const glm::vec3& orientation, std::vector<RaycastHit>& hitResults)
	{
		JPH::RefConst<JPH::Shape> shape = new JPH::BoxShape(Utils::ConvertGLMVec3ToJPH(halfSize));
		const JPH::Mat44 rotation = Utils::ConvertGLMMat4ToJPH(glm::toMat4(glm::quat(glm::radians(orientation))));
		ShapeCastMultihit(m_Data, start, direction, distance, shape, rotation, hitResults);
	}

	RaycastHit PhysicsScene::CapsuleShapeCast(const glm::vec3& start, const glm::vec3& direction, const float distance, const float radius, const float halfHeight)
	{
		JPH::RefConst<JPH::Shape> shape = new JPH::CapsuleShape(halfHeight, radius);
		return ShapeCast(m_Data, start, direction, distance, shape, JPH::Mat44::sIdentity());
	}

	void PhysicsScene::CapsuleShapeCastMultihit(const glm::vec3& start, const glm::vec3& direction, const float distance, const float radius, const float halfHeight, std::vector<RaycastHit>& hitResults)
	{
		JPH::RefConst<JPH::Shape> shape = new JPH::CapsuleShape(halfHeight, radius);
		ShapeCastMultihit(m_Data, start, direction, distance, shape, JPH::Mat44::sIdentity(), hitResults);
	}

	RaycastHit PhysicsScene::SphereShapeCast(const glm::vec3& start, const glm::vec3& direction, const float distance, const float radius)
	{
		JPH::RefConst<JPH::Shape> shape = new JPH::SphereShape(radius);
		return ShapeCast(m_Data, start, direction, distance, shape, JPH::Mat44::sIdentity());
	}

	void PhysicsScene::SphereShapeCastMultihit(const glm::vec3& start, const glm::vec3& direction, const float distance, const float radius, std::vector<RaycastHit>& hitResults)
	{
		JPH::RefConst<JPH::Shape> shape = new JPH::SphereShape(radius);
		ShapeCastMultihit(m_Data, start, direction, distance, shape, JPH::Mat44::sIdentity(), hitResults);
	}

	static void GetEntitiesInBounds(PhysicsSceneData* data, const glm::vec3& min, const glm::vec3& max, std::vector<EntityHandle>& entityResults, const JPH::ObjectLayerFilter& filter)
	{
		class BoundsCollector : public JPH::CollideShapeBodyCollector
		{
		public:
			BoundsCollector(PhysicsSceneData* data, std::vector<EntityHandle>* entityResults)
				: m_Data(data), m_EntityResults(entityResults) {}

			virtual void AddHit(const JPH::BodyID& inBodyID) override
			{
				JPH::Body* body = m_Data->System.GetBodyLockInterfaceNoLock().TryGetBody(inBodyID);

				if (!body)
					return;

				JPH::uint64 userData = body->GetUserData();
				if (Utils::IsUserDataValidEntity(m_Data->EntityScene, userData))
					m_EntityResults->emplace_back(Entity{ (entt::entity)userData, m_Data->EntityScene }.GetUUID());
			}

		private:
			PhysicsSceneData* m_Data;
			std::vector<EntityHandle>* m_EntityResults;
		};

		BoundsCollector collector(data, &entityResults);

		// Note: Here we allow all broad phase layers so we include static and dynamic geometry
		// TODO: This should probably be a function flag
		data->System.GetBroadPhaseQuery().CollideAABox(
			JPH::AABox(Utils::ConvertGLMVec3ToJPH(min), Utils::ConvertGLMVec3ToJPH(max)),
			collector, JPH::BroadPhaseLayerFilter(), filter
		);
	}

	void PhysicsScene::GetEntitiesInBounds(const glm::vec3& min, const glm::vec3& max, std::vector<EntityHandle>& entityResults)
	{
		Dymatic::GetEntitiesInBounds(m_Data, min, max, entityResults, {});
	}

	void PhysicsScene::GetEntitiesInBounds(const glm::vec3& min, const glm::vec3& max, PhysicsLayerID layer, std::vector<EntityHandle>& entityResults)
	{
		class BoundsLayerFilter : public JPH::ObjectLayerFilter
		{
		public:
			BoundsLayerFilter(JPH::ObjectLayer staticLayer, JPH::ObjectLayer movingLayer)
				: m_StaticLayer(staticLayer), m_MovingLayer(movingLayer) {}

			virtual bool ShouldCollide(JPH::ObjectLayer inLayer) const override
			{
				return inLayer == m_StaticLayer || inLayer == m_MovingLayer;
			}

		private:
			JPH::ObjectLayer m_StaticLayer;
			JPH::ObjectLayer m_MovingLayer;
		};

		BoundsLayerFilter filter(Utils::GetPhysicsJoltLayerFromID(layer, false), Utils::GetPhysicsJoltLayerFromID(layer, true));
		Dymatic::GetEntitiesInBounds(m_Data, min, max, entityResults, filter);
	}

	template <typename... Component>
	void PhysicsScene::AddAllComponents(ComponentGroup<Component...>)
	{
		(AddComponents<Component>(), ...);
	}

	template <typename Component>
	void PhysicsScene::AddComponents()
	{
		auto view = m_Data->EntityScene->m_Registry.view<TransformComponent, Component>();
		for (auto e : view)
		{
			auto& [transform, component] = view.get<TransformComponent, Component>(e);
			OnComponentAdded({ e, m_Data->EntityScene }, transform, component);
		}
	}

	template <typename... Component>
	void PhysicsScene::OnComponentAdded(Entity entity, const std::type_info& type, ComponentGroup<Component...>)
	{
		((typeid(Component) == type ? OnComponentAdded<Component>(entity, entity.GetComponent<TransformComponent>(), entity.GetComponent<Component>()) : void()), ...);
	}

	void PhysicsScene::OnComponentAdded(Entity entity, const std::type_info& type)
	{
		OnComponentAdded(entity, type, AllPhysicsComponents{});
	}

	template<>
	void PhysicsScene::OnComponentAdded(Entity entity, TransformComponent& tc, RigidBodyComponent& rbc)
	{
		JPH::Ref<JPH::Shape> shape;
		float massOverride = rbc.Mode == RigidBodyComponent::MassMode::Mass ? rbc.Mass : 0.0f;

		if (entity.HasComponent<BoxColliderComponent>())
		{
			const auto& bcc = entity.GetComponent<BoxColliderComponent>();

			JPH::Shape::ShapeResult result;
			JPH::BoxShapeSettings settings;
			settings.mHalfExtent = Utils::ConvertGLMVec3ToJPH(bcc.Size);
			settings.mDensity = rbc.Density;
			shape = settings.Create().Get();
		}
		else if (entity.HasComponent<SphereColliderComponent>())
		{
			const auto& scc = entity.GetComponent<SphereColliderComponent>();

			JPH::Shape::ShapeResult result;
			JPH::SphereShapeSettings settings;
			settings.mRadius = scc.Radius;
			settings.mDensity = rbc.Density;
			shape = new JPH::SphereShape(settings, result);
		}
		else if (entity.HasComponent<CapsuleColliderComponent>())
		{
			const auto& ccc = entity.GetComponent<CapsuleColliderComponent>();

			JPH::Shape::ShapeResult result;
			JPH::CapsuleShapeSettings settings;
			settings.mRadius = ccc.Radius;
			settings.mHalfHeightOfCylinder = ccc.HalfHeight;
			settings.mDensity = rbc.Density;
			shape = new JPH::CapsuleShape(settings, result);

		}
		else if (entity.HasComponent<MeshColliderComponent>())
		{
			const auto& mcc = entity.GetComponent<MeshColliderComponent>();
			const auto& smc = entity.GetComponent<StaticMeshComponent>();

			// Note: We construct a convex mesh to approximate the volume of the triangle mesh if the geometry is not static
			const bool approximateDensity = rbc.Type != RigidBodyComponent::BodyType::Static && rbc.Mode == RigidBodyComponent::MassMode::Density;
			if (mcc.Type == MeshColliderComponent::MeshType::Convex || approximateDensity)
			{
				JPH::ConvexHullShapeSettings settings;

				for (const Ref<Mesh> mesh : smc.m_Model->GetMeshes())
					for (const auto& vertex : mesh->GetVerticies())
						settings.mPoints.emplace_back(Utils::ConvertGLMVec3ToJPH(vertex.Position));

				settings.mDensity = rbc.Density;
				JPH::ShapeSettings::ShapeResult result = settings.Create();

				if (result.HasError())
					DY_CORE_ERROR("Failed to build Convex Hull Shape: {}", result.GetError());

				shape = result.Get();
			}

			if (mcc.Type == MeshColliderComponent::MeshType::Triangle)
			{
				JPH::StaticCompoundShapeSettings compoundSettings;

				for (const Ref<Mesh> mesh : smc.m_Model->GetMeshes())
				{
					JPH::MeshShapeSettings settings;

					const auto& vertices = mesh->GetVerticies();
					for (const auto& vertex : vertices)
						settings.mTriangleVertices.emplace_back(vertex.Position.x, vertex.Position.y, vertex.Position.z);

					const auto& indices = mesh->GetIndicies();
					for (size_t index = 0; index < indices.size(); index += 3)
						settings.mIndexedTriangles.emplace_back(indices[index + 0], indices[index + 1], indices[index + 2]);

					JPH::Ref<JPH::Shape> innerShape = settings.Create().Get();
					compoundSettings.AddShape(JPH::Vec3Arg::sZero(), JPH::QuatArg::sIdentity(), innerShape);
				}

				if (approximateDensity)
				{
					DY_CORE_WARN("Use of non-static triangle mesh collider is not advised. Density has been approximated. Consider using a Convex mesh collider!");
					massOverride = shape->GetVolume() * rbc.Density;
				}

				shape = compoundSettings.Create().Get();
			}
		}
		else
		{
			DY_CORE_ASSERT(false);
			rbc.RuntimeBody = nullptr;
			return;
		}

		// Scale the shape
		JPH::ScaledShapeSettings scaledSettings;
		scaledSettings.mInnerShapePtr = shape;
		scaledSettings.mScale = Utils::ConvertGLMVec3ToJPH(tc.Transform.Scale);
		JPH::Ref<JPH::Shape> scaledShape = scaledSettings.Create().Get();

		JPH::BodyCreationSettings bodySettings(scaledShape, Utils::ConvertGLMVec3ToJPH(tc.Transform.Translation), Utils::ConvertGLMQuatToJPH(tc.Transform.Rotation),
			rbc.Type == RigidBodyComponent::BodyType::Dynamic ? JPH::EMotionType::Dynamic : rbc.Type == RigidBodyComponent::BodyType::Kinematic ? JPH::EMotionType::Kinematic : JPH::EMotionType::Static,
			Utils::GetPhysicsJoltLayerFromID(rbc.Layer, rbc.Type != RigidBodyComponent::BodyType::Static)
		);

		bodySettings.mFriction = rbc.Friction;
		bodySettings.mRestitution = rbc.Restitution;
		bodySettings.mUserData = Utils::GetEntityUserData(entity);
		bodySettings.mIsSensor = rbc.Sensor;

		if (massOverride != 0.0f)
		{
			bodySettings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
			bodySettings.mMassPropertiesOverride.mMass = massOverride;
		}

		JPH::BodyInterface& bodyInterface = m_Data->System.GetBodyInterface();
		JPH::Body* body = bodyInterface.CreateBody(bodySettings);
		rbc.RuntimeBody = body;

		bodyInterface.AddBody(body->GetID(), rbc.Type == RigidBodyComponent::BodyType::Static ? JPH::EActivation::DontActivate : JPH::EActivation::Activate);
	}

	template<>
	void PhysicsScene::OnComponentAdded(Entity entity, TransformComponent& tc, SoftBodyComponent& sbc)
	{
		if (!entity.HasComponent<StaticMeshComponent>())
			return;

		StaticMeshComponent& smc = entity.GetComponent<StaticMeshComponent>();

		if (!smc.m_Model)
			return;

		RuntimeSoftBodyData* runtimeBody = new RuntimeSoftBodyData;
		sbc.RuntimeBody = runtimeBody;
		sbc.RuntimeModel = smc.m_Model->Copy(false);

		// TODO: Add support for soft bodies with skinning/animation (see skinned Jolt sample)
		JPH::Ref<JPH::SoftBodySharedSettings> sharedSettings = Utils::CreateSoftBodyFromMesh(smc.m_Model->GetMeshes()[0], tc.Transform.Scale, runtimeBody->VertexMap, sbc.VertexMass, sbc.UseVertexColorAsWeight);

		sharedSettings->mVertexRadius = sbc.VertexRadius;

		JPH::SoftBodyCreationSettings settings(sharedSettings, Utils::ConvertGLMVec3ToJPH(tc.Transform.Translation), Utils::ConvertGLMQuatToJPH(tc.Transform.Rotation), Utils::GetPhysicsJoltLayerFromID(sbc.Layer, true));
		settings.mFriction = sbc.Friction;
		settings.mRestitution = sbc.Restitution;
		settings.mPressure = sbc.Pressure;
		settings.mUserData = Utils::GetEntityUserData(entity);

		JPH::BodyInterface& bodyInterface = m_Data->System.GetBodyInterface();
		runtimeBody->Body = bodyInterface.CreateSoftBody(settings);
		bodyInterface.AddBody(runtimeBody->Body->GetID(), JPH::EActivation::Activate);
	}

	template<>
	void PhysicsScene::OnComponentAdded(Entity entity, TransformComponent& transform, RagdollComponent& rdc)
	{
		if (!entity.HasComponent<StaticMeshComponent>())
			return;

		StaticMeshComponent& smc = entity.GetComponent<StaticMeshComponent>();

		if (!smc.m_Model)
			return;

		const Ref<Skeleton> skeleton = smc.m_Model->GetSkeleton();

		if (!skeleton)
			return;

		const float radiusFactor = 0.8f;

		// TODO: Add a 'Physics' asset which each skeletal mesh will have a Ref to. This asset will have a Raw Pointer to the Model.
		// This will allow for custom control over the following constrains/shape generation and location etc in editor and will have it precomputed.

		// Setup skeleton with joints
		JPH::Ref<JPH::Skeleton> physicsSkeleton = new JPH::Skeleton;
		Utils::BuildPhysicsSkeletonFromHierarchy(physicsSkeleton, skeleton->GetRootNode(), -1);

		// Sort vertex positions into sets based on bones
		std::unordered_map<int, std::vector<glm::vec3>> boneVertexMap;

		const auto& meshes = smc.m_Model->GetMeshes();
		for (const auto& mesh : meshes)
		{
			const auto& vertices = mesh->GetVerticies();
			for (const auto& vertex : vertices)
			{
				int maxBoneID = -1;
				float maxWeight = -1.0f;

				for (uint32_t i = 0; i < MAX_BONE_INFLUENCE; i++)
				{
					if (vertex.m_Weights[i] > maxWeight)
					{
						maxWeight = vertex.m_Weights[i];
						maxBoneID = vertex.m_BoneIDs[i];
					}
				}

				if (maxBoneID != -1)
					boneVertexMap[maxBoneID].push_back(vertex.Position);
			}
		}

		const auto& boneInfoMap = skeleton->GetBoneInfoMap();

		// Add parts for each skeleton joint
		JPH::RagdollSettings* settings = new JPH::RagdollSettings;
		settings->mSkeleton = physicsSkeleton;
		settings->mParts.resize(physicsSkeleton->GetJointCount());
		for (int p = 0; p < physicsSkeleton->GetJointCount(); p++)
		{
			const auto& vertices = boneVertexMap.at(p);

			// Compute the bone's start and end position
			const BoneNodeData& node = skeleton->GetBoneNodeData(p);
			const auto& boneInfo = boneInfoMap.at(node.Name);

			const glm::mat4 boneTransform = glm::inverse(boneInfo.offset);
			const glm::vec3 boneStart = boneTransform[3];
			glm::vec3 boneEnd;

			if (node.Children.empty())
			{
				if (const BoneNodeData* parent = skeleton->GetBoneParent(p))
				{
					const glm::vec3 parentPosition = glm::inverse(boneInfoMap.at(parent->Name).offset)[3];
					const float previousBoneLength = glm::distance(parentPosition, boneStart);

					const glm::quat rotation = glm::quat_cast(boneTransform);
					const glm::vec3 direction = glm::normalize(rotation * glm::vec3(0, 1, 0));

					boneEnd = boneStart + direction * previousBoneLength;
				}
				else
					DY_CORE_ASSERT(false);
			}
			else
			{
				boneEnd = glm::vec3(0.0f);
				for (const auto& child : node.Children)
					boneEnd += glm::vec3(glm::inverse(boneInfoMap.at(child.Name).offset)[3]);
				boneEnd /= node.Children.size();
			}

			// Compute center of bone influence region
			const glm::vec3 center = 0.5f * (boneStart + boneEnd);
			const float capsuleLength = glm::distance(boneStart, boneEnd);
			const glm::vec3 boneAxis = glm::normalize(boneEnd - boneStart);

			// Compute furthest distance perpendicular to maximal axis
			float maxPerpendicularDistance = 0.0f;
			for (const auto& vertex : vertices)
			{
				// Project onto maxAxis and compute perpendicular component
				const glm::vec3 projection = glm::dot(vertex - center, boneAxis) * boneAxis;
				const glm::vec3 perpendicularComponent = (vertex - center) - projection;

				const float perpendicularDistance = glm::length(perpendicularComponent);
				if (perpendicularDistance > maxPerpendicularDistance)
					maxPerpendicularDistance = perpendicularDistance;
			}

			JPH::RagdollSettings::Part& part = settings->mParts[p];
			part.SetShape(new JPH::CapsuleShape(capsuleLength * 0.5f, maxPerpendicularDistance * radiusFactor));
			part.mPosition = Utils::ConvertGLMVec3ToJPH(center);
			part.mMotionType = JPH::EMotionType::Dynamic;
			part.mObjectLayer = Utils::GetPhysicsJoltLayerFromID(rdc.Layer, true);

			// Compute direction to align capsure Y-Axis with the boneAxis
			part.mRotation = JPH::Quat::sFromTo(JPH::Vec3::sAxisY(), Utils::ConvertGLMVec3ToJPH(boneAxis));

			// First part is the root, doesn't have a parent and doesn't have a constraint
			if (p > 0)
			{
				JPH::SwingTwistConstraintSettings* constraint = new JPH::SwingTwistConstraintSettings;
				constraint->mDrawConstraintSize = 0.1f;
				//constraint->mPosition1 = constraint->mPosition2 = Utils::ConvertGLMVec3ToJPH(0.5f * (boneStart + boneEnd));
				constraint->mPosition1 = constraint->mPosition2 = Utils::ConvertGLMVec3ToJPH(boneStart);
				constraint->mTwistAxis1 = constraint->mTwistAxis2 = Utils::ConvertGLMVec3ToJPH(boneAxis);
				constraint->mPlaneAxis1 = constraint->mPlaneAxis2 = JPH::Vec3::sAxisZ();
				constraint->mTwistMinAngle = -JPH::DegreesToRadians(45.0f);
				constraint->mTwistMaxAngle = JPH::DegreesToRadians(45.0f);
				constraint->mNormalHalfConeAngle = JPH::DegreesToRadians(30.0f);
				constraint->mPlaneHalfConeAngle = JPH::DegreesToRadians(30.0f);
				part.mToParent = constraint;
			}
		}

		// Finalize Ragdoll Settings Setup
		// Optional: Stabilize the inertia of the limbs
		settings->Stabilize();

		settings->DisableParentChildCollisions();
		settings->CalculateBodyIndexToConstraintIndex();

		// Add the configured ragdoll to the physics scene
		// WARNING: The collision group ID needs to be different for different ragdolls
		JPH::Ragdoll* ragdoll = settings->CreateRagdoll(UUID(), 0, &m_Data->System);
		rdc.RuntimeBody = ragdoll;
		rdc.RuntimePose = CreateRef<BoneMatrixList>(physicsSkeleton->GetJointCount());

		ragdoll->AddToPhysicsSystem(JPH::EActivation::Activate);
	}

	template<>
	void PhysicsScene::OnComponentAdded(Entity entity, TransformComponent& tc, PointConstraintComponent& pcc)
	{
		Utils::OnConstraintAdded(m_Data, pcc, entity, [this](PointConstraintComponent& pcc, Entity entity, Entity otherEntity) -> JPH::Constraint*
		{
			JPH::Body* body = Utils::GetEntityJoltBody(entity);
			JPH::Body* targetBody = Utils::GetEntityJoltBody(otherEntity);

			if (!body)
				return nullptr;

			JPH::PointConstraintSettings settings;
			settings.mSpace = Utils::GetJoltConstraintSpace(pcc.Space);
			settings.mPoint1 = Utils::ConvertGLMVec3ToJPH(pcc.LocalPoint);
			settings.mPoint2 = Utils::ConvertGLMVec3ToJPH(pcc.TargetPoint);

			return settings.Create(*body, targetBody ? *targetBody : JPH::Body::sFixedToWorld);
		}, true);
	}

	template<>
	void PhysicsScene::OnComponentAdded(Entity entity, TransformComponent& tc, ConeConstraintComponent& ccc)
	{
		Utils::OnConstraintAdded(m_Data, ccc, entity, [this](ConeConstraintComponent& ccc, Entity entity, Entity otherEntity) -> JPH::Constraint*
		{
			JPH::Body* body = Utils::GetEntityJoltBody(entity);
			JPH::Body* targetBody = Utils::GetEntityJoltBody(otherEntity);

			if (!body || !targetBody)
				return nullptr;

			JPH::ConeConstraintSettings settings;
			settings.mSpace = JPH::EConstraintSpace::WorldSpace;
			settings.mPoint1 = Utils::ConvertGLMVec3ToJPH(entity.GetComponent<TransformComponent>().Transform.Translation + ccc.LocalReferenceFrame.Offset);
			settings.mPoint2 = Utils::ConvertGLMVec3ToJPH(otherEntity.GetComponent<TransformComponent>().Transform.Translation + ccc.TargetReferenceFrame.Offset);

			settings.mTwistAxis1 = Utils::ConvertGLMVec3ToJPH(glm::normalize(ccc.LocalReferenceFrame.TwistAxis));
			settings.mTwistAxis2 = Utils::ConvertGLMVec3ToJPH(glm::normalize(ccc.TargetReferenceFrame.TwistAxis));
			settings.mHalfConeAngle = glm::radians(glm::clamp(ccc.HalfConeAngle, 0.0f, 180.0f));

			return settings.Create(*body, *targetBody);
		});
	}

	template<>
	void PhysicsScene::OnComponentAdded(Entity entity, TransformComponent& tc, DistanceConstraintComponent& dcc)
	{
		Utils::OnConstraintAdded(m_Data, dcc, entity, [this](DistanceConstraintComponent& dcc, Entity entity, Entity otherEntity)
		{
			JPH::DistanceConstraintSettings settings;
			settings.mPoint1 = Utils::ConvertGLMVec3ToJPH(entity.GetComponent<TransformComponent>().Transform.Translation);
			settings.mPoint2 = Utils::ConvertGLMVec3ToJPH(otherEntity.GetComponent<TransformComponent>().Transform.Translation);

			if (dcc.Type == DistanceConstraintComponent::DistanceType::Fixed)
			{
				settings.mMinDistance = dcc.Distance;
				settings.mMaxDistance = dcc.Distance;
			}
			else if (dcc.Type == DistanceConstraintComponent::DistanceType::Range)
			{
				settings.mMinDistance = dcc.MinDistance;
				settings.mMaxDistance = dcc.MaxDistance;
			}

			Utils::PopulateEntitySpringConstraint(entity, settings.mLimitsSpringSettings);

			return settings.Create(
				*(JPH::Body*)entity.GetComponent<RigidBodyComponent>().RuntimeBody,
				*(JPH::Body*)otherEntity.GetComponent<RigidBodyComponent>().RuntimeBody
			);
		});
	}

	template<>
	void PhysicsScene::OnComponentAdded(Entity entity, TransformComponent& tc, HingeConstraintComponent& hcc)
	{
		Utils::OnConstraintAdded(m_Data, hcc, entity, [this](HingeConstraintComponent& hcc, Entity entity, Entity otherEntity) -> JPH::Constraint*
		{
			JPH::Body* body = Utils::GetEntityJoltBody(entity);
			JPH::Body* targetBody = Utils::GetEntityJoltBody(otherEntity);

			if (!body || !targetBody)
				return nullptr;

			JPH::HingeConstraintSettings settings;
			settings.mSpace = Utils::GetJoltConstraintSpace(hcc.Space);

			settings.mPoint1 = Utils::ConvertGLMVec3ToJPH(hcc.LocalReferenceFrame.Point);
			settings.mHingeAxis1 = Utils::ConvertGLMVec3ToJPH(glm::normalize(hcc.LocalReferenceFrame.HingeAxis));
			settings.mNormalAxis1 = Utils::ConvertGLMVec3ToJPH(glm::normalize(hcc.LocalReferenceFrame.NormalAxis));

			settings.mPoint2 = Utils::ConvertGLMVec3ToJPH(hcc.TargetReferenceFrame.Point);
			settings.mHingeAxis2 = Utils::ConvertGLMVec3ToJPH(glm::normalize(hcc.TargetReferenceFrame.HingeAxis));
			settings.mNormalAxis2 = Utils::ConvertGLMVec3ToJPH(glm::normalize(hcc.TargetReferenceFrame.NormalAxis));

			settings.mLimitsMin = glm::radians(glm::clamp(hcc.MinRotation, -180.0f, 0.0f));
			settings.mLimitsMax = glm::radians(glm::clamp(hcc.MaxRotation, 0.0f, 180.0f));
			settings.mMaxFrictionTorque = hcc.MaxFrictionTorque;

			Utils::PopulateEntitySpringConstraint(entity, settings.mLimitsSpringSettings);

			return settings.Create(*body, *targetBody);
		});
	}

	template<>
	void PhysicsScene::OnComponentAdded(Entity entity, TransformComponent& tc, FixedConstraintComponent& fcc)
	{
		Utils::OnConstraintAdded(m_Data, fcc, entity, [this](FixedConstraintComponent& fcc, Entity entity, Entity otherEntity) -> JPH::Constraint*
		{
			JPH::Body* body = Utils::GetEntityRigidJoltBody(entity);
			JPH::Body* targetBody = Utils::GetEntityRigidJoltBody(otherEntity);

			if (!body || !targetBody)
				return nullptr;

			JPH::FixedConstraintSettings settings;

			settings.mAutoDetectPoint = fcc.Type == ConstraintSpace::Automatic;
			settings.mSpace =  Utils::GetJoltConstraintSpace(fcc.Type);
			settings.mPoint1 = Utils::ConvertGLMVec3ToJPH(fcc.LocalReferenceFrame.Point);
			settings.mAxisX1 = Utils::ConvertGLMVec3ToJPH(glm::normalize(fcc.LocalReferenceFrame.AxisX));
			settings.mAxisY1 = Utils::ConvertGLMVec3ToJPH(glm::normalize(fcc.LocalReferenceFrame.AxisY));
			settings.mPoint2 = Utils::ConvertGLMVec3ToJPH(fcc.TargetReferenceFrame.Point);
			settings.mAxisX2 = Utils::ConvertGLMVec3ToJPH(glm::normalize(fcc.TargetReferenceFrame.AxisX));
			settings.mAxisY2 = Utils::ConvertGLMVec3ToJPH(glm::normalize(fcc.TargetReferenceFrame.AxisY));

			return settings.Create(*body, *targetBody);
		});
	}

	template<>
	void PhysicsScene::OnComponentAdded(Entity entity, TransformComponent& tc, GearConstraintComponent& gcc)
	{
		Utils::OnConstraintAdded(m_Data, gcc, entity, [this](GearConstraintComponent& gcc, Entity entity, Entity otherEntity) -> JPH::Constraint*
		{
			JPH::Body* body = Utils::GetEntityRigidJoltBody(entity);
			JPH::Body* targetBody = Utils::GetEntityRigidJoltBody(otherEntity);

			if (!body)
				return nullptr;

			JPH::GearConstraintSettings settings;
			settings.mSpace = Utils::GetJoltConstraintSpace(gcc.Space);
			settings.mHingeAxis1 = Utils::GetJoltAxis(gcc.LocalHingeAxis);
			settings.mHingeAxis2 = Utils::GetJoltAxis(gcc.TargetHingeAxis);
			settings.SetRatio(gcc.Ratio.Numerator, gcc.Ratio.Denominator);

			return settings.Create(*body, Utils::GetOptionalBody(targetBody));
		}, true);
	}

	template<>
	void PhysicsScene::OnComponentAdded(Entity entity, TransformComponent& tc, PulleyConstraintComponent& pcc)
	{
		Utils::OnConstraintAdded(m_Data, pcc, entity, [this](PulleyConstraintComponent& pcc, Entity entity, Entity otherEntity) -> JPH::Constraint*
		{
			JPH::Body* body = Utils::GetEntityRigidJoltBody(entity);
			JPH::Body* targetBody = Utils::GetEntityRigidJoltBody(otherEntity);

			if (!body || !targetBody)
				return nullptr;

			JPH::PulleyConstraintSettings settings;
			settings.mSpace = Utils::GetJoltConstraintSpace(pcc.Space);
			settings.mBodyPoint1 = Utils::ConvertGLMVec3ToJPH(pcc.LocalReferenceFrame.BodyPoint);
			settings.mFixedPoint1 = Utils::ConvertGLMVec3ToJPH(pcc.LocalReferenceFrame.FixedPoint);
			settings.mBodyPoint2 = Utils::ConvertGLMVec3ToJPH(pcc.TargetReferenceFrame.BodyPoint);
			settings.mFixedPoint2 = Utils::ConvertGLMVec3ToJPH(pcc.TargetReferenceFrame.FixedPoint);
			settings.mRatio = pcc.Ratio.GetFloat();
			settings.mMinLength = pcc.MinLength;
			settings.mMaxLength = pcc.MaxLength;

			return settings.Create(*body, *targetBody);
		});
	}

	template<>
	void PhysicsScene::OnComponentAdded(Entity entity, TransformComponent& tc, RackAndPinionConstraintComponent& rapcc)
	{
		Utils::OnConstraintAdded(m_Data, rapcc, entity, [this](RackAndPinionConstraintComponent& rapcc, Entity entity, Entity otherEntity) -> JPH::Constraint*
		{
			JPH::Body* body = Utils::GetEntityRigidJoltBody(entity);
			JPH::Body* targetBody = Utils::GetEntityRigidJoltBody(otherEntity);

			if (!body || !targetBody)
				return nullptr;

			JPH::RackAndPinionConstraintSettings settings;
			settings.mSpace = Utils::GetJoltConstraintSpace(rapcc.Space);
			settings.mHingeAxis = Utils::GetJoltAxis(rapcc.HingeAxis);
			settings.mSliderAxis = Utils::GetJoltAxis(rapcc.SliderAxis);

			if (rapcc.Mode == RackAndPinionConstraintComponent::RatioMode::Ratio)
				settings.mRatio = rapcc.Ratio;
			else
				settings.SetRatio(rapcc.RackTeethCount, rapcc.RackLength, rapcc.PinionTeethCount);

			return settings.Create(*body, *targetBody);
		});
	}

	template<>
	void PhysicsScene::OnComponentAdded(Entity entity, TransformComponent& tc, SwingTwistConstraintComponent& stcc)
	{
		Utils::OnConstraintAdded(m_Data, stcc, entity, [this](SwingTwistConstraintComponent& stcc, Entity entity, Entity otherEntity) -> JPH::Constraint*
		{
			JPH::Body* body = Utils::GetEntityRigidJoltBody(entity);
			JPH::Body* targetBody = Utils::GetEntityRigidJoltBody(otherEntity);

			if (!body || !targetBody)
				return nullptr;

			JPH::SwingTwistConstraintSettings settings;
			settings.mSpace = Utils::GetJoltConstraintSpace(stcc.Space);
			settings.mPosition1 = Utils::ConvertGLMVec3ToJPH(stcc.LocalReferenceFrame.Position);
			settings.mTwistAxis1 = Utils::GetJoltAxis(stcc.LocalReferenceFrame.TwistAxis);
			settings.mPlaneAxis1 = Utils::GetJoltAxis(stcc.LocalReferenceFrame.PlaneAxis);
			settings.mPosition2 = Utils::ConvertGLMVec3ToJPH(stcc.TargetReferenceFrame.Position);
			settings.mTwistAxis2 = Utils::GetJoltAxis(stcc.TargetReferenceFrame.TwistAxis);
			settings.mPlaneAxis2 = Utils::GetJoltAxis(stcc.TargetReferenceFrame.PlaneAxis);
			settings.mSwingType = Utils::GetJoltSwingType(stcc.SwingType);
			settings.mNormalHalfConeAngle = stcc.NormalHalfConeAngle;
			settings.mPlaneHalfConeAngle = stcc.PlaneHalfConeAngle;
			settings.mTwistMinAngle = stcc.TwistMinAngle;
			settings.mTwistMaxAngle = stcc.TwistMaxAngle;
			settings.mMaxFrictionTorque = stcc.MaxFrictionTorque;

			return settings.Create(*body, *targetBody);
		});
	}

	template<>
	void PhysicsScene::OnComponentAdded(Entity entity, TransformComponent& tc, SliderConstraintComponent& scc)
	{
		Utils::OnConstraintAdded(m_Data, scc, entity, [this](SliderConstraintComponent& scc, Entity entity, Entity otherEntity) -> JPH::Constraint*
		{
			JPH::Body* body = Utils::GetEntityRigidJoltBody(entity);
			JPH::Body* targetBody = Utils::GetEntityRigidJoltBody(otherEntity);

			if (!body)
				return nullptr;

			JPH::SliderConstraintSettings settings;

			settings.mAutoDetectPoint = (scc.Space == ConstraintSpace::Automatic);
			settings.mSpace =  Utils::GetJoltConstraintSpace(scc.Space);
			settings.mPoint1 = Utils::ConvertGLMVec3ToJPH(scc.LocalReferenceFrame.Point);
			settings.mSliderAxis1 = Utils::ConvertGLMVec3ToJPH(glm::normalize(scc.LocalReferenceFrame.SliderAxis));
			settings.mNormalAxis1 = Utils::ConvertGLMVec3ToJPH(glm::normalize(scc.LocalReferenceFrame.NormalAxis));
			settings.mPoint2 = Utils::ConvertGLMVec3ToJPH(scc.TargetReferenceFrame.Point);
			settings.mSliderAxis2 = Utils::ConvertGLMVec3ToJPH(glm::normalize(scc.TargetReferenceFrame.SliderAxis));
			settings.mNormalAxis2 = Utils::ConvertGLMVec3ToJPH(glm::normalize(scc.TargetReferenceFrame.NormalAxis));
			settings.mLimitsMin = glm::clamp(scc.SliderMin, -FLT_MAX, 0.0f);
			settings.mLimitsMax = glm::clamp(scc.SliderMax, 0.0f, FLT_MAX);
			Utils::PopulateEntitySpringConstraint(entity, settings.mLimitsSpringSettings);
			settings.mMaxFrictionForce = scc.MaxFrictionForce;

			return settings.Create(*body, Utils::GetOptionalBody(targetBody));
		}, true);
	}

	template<>
	void PhysicsScene::OnComponentAdded(Entity entity, TransformComponent& tc, SixDOFConstraintComponent& sdcc)
	{
		Utils::OnConstraintAdded(m_Data, sdcc, entity, [this](SixDOFConstraintComponent& sdcc, Entity entity, Entity otherEntity) -> JPH::Constraint*
		{
			JPH::Body* body = Utils::GetEntityRigidJoltBody(entity);
			JPH::Body* targetBody = Utils::GetEntityRigidJoltBody(otherEntity);

			if (!body)
				return nullptr;

			JPH::SixDOFConstraintSettings settings;
			settings.mSpace = Utils::GetJoltConstraintSpace(sdcc.Space);
			settings.mPosition1 = Utils::ConvertGLMVec3ToJPH(sdcc.LocalReferenceFrame.Position);
			settings.mAxisX1 = Utils::GetJoltAxis(sdcc.LocalReferenceFrame.AxisX);
			settings.mAxisY1 = Utils::GetJoltAxis(sdcc.LocalReferenceFrame.AxisY);
			settings.mPosition2 = Utils::ConvertGLMVec3ToJPH(sdcc.TargetReferenceFrame.Position);
			settings.mAxisX2 = Utils::GetJoltAxis(sdcc.TargetReferenceFrame.AxisX);
			settings.mAxisY2 = Utils::GetJoltAxis(sdcc.TargetReferenceFrame.AxisY);
			settings.mSwingType = Utils::GetJoltSwingType(sdcc.SwingType);

			std::copy(sdcc.MaxFriction.begin(), sdcc.MaxFriction.end(), settings.mMaxFriction);
			std::copy(sdcc.LimitMin.begin(), sdcc.LimitMin.end(), settings.mLimitMin);
			std::copy(sdcc.LimitMax.begin(), sdcc.LimitMax.end(), settings.mLimitMax);

			// TODO: Have this separately configurable
			Utils::PopulateEntitySpringConstraint(entity, settings.mLimitsSpringSettings[JPH::SixDOFConstraintSettings::EAxis::TranslationX]);
			Utils::PopulateEntitySpringConstraint(entity, settings.mLimitsSpringSettings[JPH::SixDOFConstraintSettings::EAxis::TranslationY]);
			Utils::PopulateEntitySpringConstraint(entity, settings.mLimitsSpringSettings[JPH::SixDOFConstraintSettings::EAxis::TranslationZ]);

			return settings.Create(*body, Utils::GetOptionalBody(targetBody));
		}, true);
	}

	template<>
	void PhysicsScene::OnComponentAdded(Entity entity, TransformComponent& tc, FollowConstraintComponent& fcc)
	{
		Utils::OnConstraintAdded(m_Data, fcc, entity, [this](FollowConstraintComponent& fcc, Entity entity, Entity otherEntity) -> JPH::Constraint*
		{
			// Target must have spline path to follow
			if (!otherEntity.HasComponent<SplineComponent>())
				return nullptr;

			const SplineComponent& sc = otherEntity.GetComponent<SplineComponent>();

			JPH::Ref<JPH::PathConstraintPath> path = Utils::BuildFollowConstraintPath(fcc, sc);
			if (!path)
				return nullptr;

			JPH::Body* activeBody = Utils::GetEntityJoltBody(entity);
			if (!activeBody)
				return nullptr;

			JPH::Body* baseBody = nullptr;
			fcc.BaseRuntimeBody = nullptr;

			// If specified on component use given entity as base
			if (fcc.BaseTarget && m_Data->EntityScene->DoesEntityExist(fcc.BaseTarget))
			{
				Entity baseEntity = m_Data->EntityScene->GetEntityByUUID(fcc.BaseTarget);
				baseBody = Utils::GetEntityJoltBody(baseEntity);
			}

			// Otherwise see if target spline entity also has a rigid body and use that
			if (!baseBody)
				baseBody = Utils::GetEntityJoltBody(otherEntity);

			// Otherwise we assume no dynamic 'base' is desired and create a static body.
			// Note: This body's lifetime is tied to this constraint
			if (!baseBody)
			{
				const Transform transform = otherEntity.GetWorldTransform();
				JPH::BodyInterface& bodyInterface = m_Data->System.GetBodyInterface();

				JPH::Body* emptyBody = bodyInterface.CreateBody(JPH::BodyCreationSettings(new JPH::SphereShape(1.0f), Utils::ConvertGLMVec3ToJPH(transform.Translation), Utils::ConvertGLMQuatToJPH(transform.Rotation), JPH::EMotionType::Static, NullPhysicsLayer));
				bodyInterface.AddBody(emptyBody->GetID(), JPH::EActivation::DontActivate);
				fcc.BaseRuntimeBody = baseBody = emptyBody;
			}

			JPH::PathConstraintSettings settings;
			settings.mPath = path;
			settings.mPathPosition = sc.Points.empty() ? JPH::Vec3::sZero() : Utils::ConvertGLMVec3ToJPH(-sc.Points.front().Position);
			settings.mPathRotation = JPH::Quat::sIdentity();
			settings.mPathFraction = glm::clamp(fcc.StartFraction, 0.0f, 1.0f) * path->GetPathMaxFraction();
			settings.mMaxFrictionForce = fcc.MaxFrictionForce;
			settings.mRotationConstraintType = (JPH::EPathRotationConstraintType)fcc.RotationConstraint;

			return settings.Create(*baseBody, *activeBody);
		});
	}

	template<>
	void PhysicsScene::OnComponentAdded(Entity entity, TransformComponent& tc, CharacterMovementComponent& cmc)
	{
		JPH::ShapeRefC shape = JPH::RotatedTranslatedShapeSettings(JPH::Vec3(0, 0.5f * cmc.CapsuleHeight + cmc.CapsuleRadius, 0), JPH::Quat::sIdentity(), new JPH::CapsuleShape(0.5f * cmc.CapsuleHeight, cmc.CapsuleRadius)).Create().Get();
		JPH::ShapeRefC innerShape = JPH::RotatedTranslatedShapeSettings(JPH::Vec3(0, 0.5f * cmc.CapsuleHeight + cmc.CapsuleRadius, 0), JPH::Quat::sIdentity(), new JPH::CapsuleShape(0.5f * cmc.InnerShapeFraction * cmc.CapsuleHeight, cmc.InnerShapeFraction * cmc.CapsuleRadius)).Create().Get();

		JPH::Ref<JPH::CharacterVirtualSettings> settings = new JPH::CharacterVirtualSettings();
		settings->mMass = cmc.Mass;
		settings->mMaxStrength = cmc.MaxStrength;
		settings->mMaxSlopeAngle = JPH::DegreesToRadians(cmc.MaxSlopeAngle);
		settings->mShape = shape;
		settings->mBackFaceMode = JPH::EBackFaceMode::CollideWithBackFaces;
		settings->mCharacterPadding = 0.02f;
		settings->mPenetrationRecoverySpeed = 1.0f;
		settings->mPredictiveContactDistance = 0.1f;
		settings->mSupportingVolume = JPH::Plane(JPH::Vec3::sAxisY(), -cmc.CapsuleRadius);
		settings->mEnhancedInternalEdgeRemoval = false;
		settings->mInnerBodyShape = innerShape;
		settings->mInnerBodyLayer = Utils::GetPhysicsJoltLayerFromID(cmc.Layer, true);

		JPH::CharacterVirtual* character = new JPH::CharacterVirtual(settings, Utils::ConvertGLMVec3ToJPH(tc.Transform.Translation), Utils::ConvertGLMQuatToJPH(tc.Transform.Rotation), &m_Data->System);
		cmc.RuntimeController = character;

		// Reset other runtime data
		cmc.RuntimeMovementDirection = glm::vec3(0.0f);
		cmc.PreviousMovementDirection = glm::vec3(0.0f);
		cmc.RuntimeLinearVelocity = glm::vec3(0.0f);
		cmc.RuntimeGroundVelocity = glm::vec3(0.0f);
		cmc.RuntimeJump = false;
		cmc.IsFalling = false;

		m_Data->System.GetBodyLockInterface().TryGetBody(character->GetInnerBodyID())->SetUserData(Utils::GetEntityUserData(entity));

		character->SetCharacterVsCharacterCollision(&m_Data->CharacterVsCharacterCollision);
		m_Data->CharacterVsCharacterCollision.Add(character);

		// Note: We can also use Jolt's `SetListener` method to receive character collision callbacks and to adjust body velocity (e.g. for being on a conveyor belt)
	}

	template<>
	void PhysicsScene::OnComponentAdded(Entity entity, TransformComponent& tc, SpringArmComponent& sac)
	{
		sac.ExclusionMask->clear();
	}

	template<>
	void PhysicsScene::OnComponentAdded(Entity entity, TransformComponent& tc, LandscapeComponent& lc)
	{
		static constexpr int s_BlockSizeShift = 2;
		static constexpr int s_BitsPerSample = 8;

		if (!lc.Physics || !lc.Data || lc.Resolution.x <= 0 || lc.Resolution.y <= 0)
			return;

		JPH::HeightFieldShapeSettings settings(lc.Data->As<float>(), JPH::Vec3::sZero(), Utils::ConvertGLMVec3ToJPH(tc.Transform.Scale / glm::vec3(lc.Resolution.x, 1.0f, lc.Resolution.y)), lc.Resolution.x);
		settings.mBlockSize = 1 << s_BlockSizeShift;
		settings.mBitsPerSample = s_BitsPerSample;

		auto& bodyInterface = m_Data->System.GetBodyInterface();

		JPH::HeightFieldShape* heightfield = JPH::StaticCast<JPH::HeightFieldShape>(settings.Create().Get());
		JPH::Body* terrain = bodyInterface.CreateBody(JPH::BodyCreationSettings(heightfield, Utils::ConvertGLMVec3ToJPH(tc.Transform.Translation), Utils::ConvertGLMQuatToJPH(tc.Transform.Rotation), JPH::EMotionType::Static, Utils::GetPhysicsJoltLayerFromID(lc.Layer, false)));
		terrain->SetUserData(Utils::GetEntityUserData(entity));
		bodyInterface.AddBody(terrain->GetID(), JPH::EActivation::DontActivate);

		lc.RuntimeBody = terrain;
	}

	template <typename... Component>
	void PhysicsScene::RemoveAllComponents(ComponentGroup<Component...>)
	{
		(RemoveComponents<Component>(), ...);
	}

	template <typename Component>
	void PhysicsScene::RemoveComponents()
	{
		auto view = m_Data->EntityScene->m_Registry.view<Component>();
		for (auto e : view)
			OnComponentRemoved({ e, m_Data->EntityScene }, view.get<Component>(e));
	}

	template <typename... Component>
	void PhysicsScene::OnComponentRemoved(Entity entity, const std::type_info& type, ComponentGroup<Component...>)
	{
		((typeid(Component) == type ? OnComponentRemoved<Component>(entity, entity.GetComponent<Component>()) : void()), ...);
	}

	void PhysicsScene::OnComponentRemoved(Entity entity, const std::type_info& type)
	{
		OnComponentRemoved(entity, type, AllPhysicsComponents{});
	}

	template<>
	void PhysicsScene::OnComponentRemoved(Entity entity, RigidBodyComponent& rbc)
	{
		if (!rbc.RuntimeBody)
			return;

		JPH::BodyInterface& bodyInterface = m_Data->System.GetBodyInterface();

		const JPH::BodyID bodyID = ((JPH::Body*)rbc.RuntimeBody)->GetID();
		bodyInterface.RemoveBody(bodyID);
		bodyInterface.DestroyBody(bodyID);

		rbc.RuntimeBody = nullptr;
	}

	template<>
	void PhysicsScene::OnComponentRemoved(Entity entity, SoftBodyComponent& sbc)
	{
		if (!sbc.RuntimeBody)
			return;

		JPH::BodyInterface& bodyInterface = m_Data->System.GetBodyInterface();

		RuntimeSoftBodyData* runtimeBody = (RuntimeSoftBodyData*)sbc.RuntimeBody;
		const JPH::BodyID bodyID = runtimeBody->Body->GetID();
		bodyInterface.RemoveBody(bodyID);
		bodyInterface.DestroyBody(bodyID);

		delete runtimeBody;
		sbc.RuntimeBody = nullptr;
		sbc.RuntimeModel = nullptr;
	}

	template<>
	void PhysicsScene::OnComponentRemoved(Entity entity, RagdollComponent& rdc)
	{
		rdc.RuntimePose = nullptr;

		if (!rdc.RuntimeBody)
			return;

		JPH::Ragdoll* ragdoll = (JPH::Ragdoll*)rdc.RuntimeBody;
		ragdoll->RemoveFromPhysicsSystem();
		rdc.RuntimeBody = nullptr;
	}

	template<>
	void PhysicsScene::OnComponentRemoved(Entity entity, PointConstraintComponent& pcc)
	{
		Utils::OnConstraintRemoved(m_Data, pcc);
	}

	template<>
	void PhysicsScene::OnComponentRemoved(Entity entity, ConeConstraintComponent& ccc)
	{
		Utils::OnConstraintRemoved(m_Data, ccc);
	}

	template<>
	void PhysicsScene::OnComponentRemoved(Entity entity, DistanceConstraintComponent& dcc)
	{
		Utils::OnConstraintRemoved(m_Data, dcc);
	}

	template<>
	void PhysicsScene::OnComponentRemoved(Entity entity, HingeConstraintComponent& hcc)
	{
		Utils::OnConstraintRemoved(m_Data, hcc);
	}

	template<>
	void PhysicsScene::OnComponentRemoved(Entity entity, FixedConstraintComponent& fcc)
	{
		Utils::OnConstraintRemoved(m_Data, fcc);
	}

	template<>
	void PhysicsScene::OnComponentRemoved(Entity entity, SliderConstraintComponent& scc)
	{
		Utils::OnConstraintRemoved(m_Data, scc);
	}

	template<>
	void PhysicsScene::OnComponentRemoved(Entity entity, SixDOFConstraintComponent& sdcc)
	{
		Utils::OnConstraintRemoved(m_Data, sdcc);
	}

	template<>
	void PhysicsScene::OnComponentRemoved(Entity entity, FollowConstraintComponent& fcc)
	{
		if (fcc.BaseRuntimeBody)
		{
			JPH::BodyInterface& bodyInterface = m_Data->System.GetBodyInterface();
			JPH::Body* runtimeBody = (JPH::Body*)fcc.BaseRuntimeBody;
			const JPH::BodyID bodyID = runtimeBody->GetID();
			bodyInterface.RemoveBody(bodyID);
			bodyInterface.DestroyBody(bodyID);
			
			fcc.BaseRuntimeBody = nullptr;
		}

		Utils::OnConstraintRemoved(m_Data, fcc);
	}

	template<>
	void PhysicsScene::OnComponentRemoved(Entity entity, CharacterMovementComponent& cmc)
	{
		if (!cmc.RuntimeController)
			return;

		JPH::CharacterVirtual* character = (JPH::CharacterVirtual*)cmc.RuntimeController;
		m_Data->CharacterVsCharacterCollision.Remove(character);

		delete character;
		cmc.RuntimeController = nullptr;
	}

	template<>
	void PhysicsScene::OnComponentRemoved(Entity entity, SpringArmComponent& sac)
	{
		sac.CurrentLength = -1.0f;
	}

	template<>
	void PhysicsScene::OnComponentRemoved(Entity entity, LandscapeComponent& lc)
	{
		if (!lc.RuntimeBody)
			return;

		JPH::BodyInterface& bodyInterface = m_Data->System.GetBodyInterface();

		const JPH::BodyID bodyID = ((JPH::Body*)lc.RuntimeBody)->GetID();
		bodyInterface.RemoveBody(bodyID);
		bodyInterface.DestroyBody(bodyID);

		lc.RuntimeBody = nullptr;
	}

}