#pragma once
#include "Dymatic/Physics/RaycastHit.h"

#include "Dymatic/Core/Timestep.h"
#include "Dymatic/Scene/Components.h"

namespace JPH {
	class DebugRendererRecorder;
}

namespace Dymatic {

	class Scene;
	class Entity;
	struct PhysicsSceneData;

#ifndef DY_DIST
	struct DebugRendererRecorderData;
#endif DY_DIST

	class PhysicsEngine
	{
	public:
		static void Init();
		static void Shutdown();

		static void SetDebugLogsEnabled(const bool enabled);
	};

	class PhysicsScene
	{
	public:
		static Ref<PhysicsScene> Create(Scene* scene) { return CreateRef<PhysicsScene>(scene); }
		PhysicsScene(Scene* scene);
		~PhysicsScene();

		void OnUpdate(Timestep ts);

		// Component Updates
		void OnComponentAdded(Entity entity, const std::type_info& type);
		void OnComponentRemoved(Entity entity, const std::type_info& type);
		
		template<typename ComponentType>
		void RebuildComponent(Entity entity)
		{
			OnComponentRemoved<ComponentType>(entity, entity.GetComponent<ComponentType>());
			OnComponentAdded<ComponentType>(entity, entity.GetComponent<TransformComponent>(), entity.GetComponent<ComponentType>());
		}

		// System Synchronization
		void SetPositionInternal(Entity entity, const glm::vec3& position);
		void SetRotationInternal(Entity entity, const glm::quat& rotation);

		void UpdateConstraintEnabled(const ConstraintComponentBase* cc);
		void UpdateDistanceConstraintDistance(const DistanceConstraintComponent& dcc);

		// Body Operations
		bool IsActive(Entity entity);
		void Activate(Entity entity);
		void Deactivate(Entity entity);
		void SetAllowSleeping(Entity entity, const bool allowSleeping);
		void ResetSleepTimer(Entity entity);

		void SetPositionWithoutActivation(Entity entity, const glm::vec3& position);
		void SetRotationWithoutActivation(Entity entity, const glm::vec3& rotation);

		void AddForce(Entity entity, const glm::vec3& force);
		void AddForce(Entity entity, const glm::vec3& force, const glm::vec3& position);

		void AddImpulse(Entity entity, const glm::vec3& impulse);
		void AddImpulse(Entity entity, const glm::vec3& impulse, const glm::vec3& position);

		void AddAngularImpulse(Entity entity, const glm::vec3& impulse);
		void AddTorque(Entity entity, const glm::vec3& torque);
		void AddForceAndTorque(Entity entity, const glm::vec3& force, const glm::vec3& torque);

		void ApplyBuoyancyImpulse(Entity entity, const glm::vec3& surfacePosition, const glm::vec3& surfaceNormal, const float buoyancy, const float linearDrag, const float angularDrag, const glm::vec3& fluidVelocity, const float deltaTime);
		void MoveKinematic(Entity entity, const glm::vec3& position, const glm::vec3& rotation, const float deltaTime);

		glm::vec3 GetCenterOfMassPosition(Entity entity);

		glm::vec3 GetAccumulatedForce(Entity entity);
		glm::vec3 GetAccumulatedTorque(Entity entity);
		void ResetForce(Entity entity);
		void ResetTorque(Entity entity);
		void ResetMotion(Entity entity);

		glm::vec3 GetLinearVelocity(Entity entity);
		void SetLinearVelocity(Entity entity, const glm::vec3& linearVelocity);
		void AddLinearVelocity(Entity entity, const glm::vec3& linearVelocity);

		glm::vec3 GetAngularVelocity(Entity entity);
		void SetAngularVelocity(Entity entity, const glm::vec3& angularVelocity);

		void GetLinearAndAngularVelocity(Entity entity, glm::vec3& linearVelocity, glm::vec3& angularVelocity);
		void SetLinearAndAngularVelocity(Entity entity, const glm::vec3& linearVelocity, const glm::vec3& angularVelocity);
		void AddLinearAndAngularVelocity(Entity entity, const glm::vec3& linearVelocity, const glm::vec3& angularVelocity);

		glm::vec3 GetPointVelocity(Entity entity, const glm::vec3& point);

		// Soft Body Only Operations
		void SetVertexPosition(Entity entity, const uint32_t index, const glm::vec3& position);
		void SetVertexVelocity(Entity entity, const uint32_t index, const glm::vec3& velocity);
		void SetPositionWeighted(Entity entity, const glm::vec3& position);
		void SetFixedPosition(Entity entity, const bool fixedPosition);

		// Queries
		RaycastHit Raycast(const glm::vec3& origin, const glm::vec3& direction, float length);
		void RaycastMultihit(const glm::vec3& origin, const glm::vec3& direction, float length, std::vector<RaycastHit>& hitResults);

		RaycastHit BoxShapeCast(const glm::vec3& start, const glm::vec3& direction, const float distance, const glm::vec3& halfSize, const glm::vec3& orientation);
		void BoxShapeCastMultihit(const glm::vec3& start, const glm::vec3& direction, const float distance, const glm::vec3& halfSize, const glm::vec3& orientation, std::vector<RaycastHit>& hitResults);

		RaycastHit CapsuleShapeCast(const glm::vec3& start, const glm::vec3& direction, const float distance, const float radius, const float halfHeight);
		void CapsuleShapeCastMultihit(const glm::vec3& start, const glm::vec3& direction, const float distance, const float radius, const float halfHeight, std::vector<RaycastHit>& hitResults);

		RaycastHit SphereShapeCast(const glm::vec3& start, const glm::vec3& direction, const float distance, const float radius);
		void SphereShapeCastMultihit(const glm::vec3& start, const glm::vec3& direction, const float distance, const float radius, std::vector<RaycastHit>& hitResults);

		void GetEntitiesInBounds(const glm::vec3& min, const glm::vec3& max, std::vector<EntityHandle>& entityResults);
		void GetEntitiesInBounds(const glm::vec3& min, const glm::vec3& max, PhysicsLayerID layer, std::vector<EntityHandle>& entityResults);

	private:
		template <typename... Component>
		void AddAllComponents(ComponentGroup<Component...>);

		template <typename Component>
		void AddComponents();

		template <typename... Component>
		void OnComponentAdded(Entity entity, const std::type_info&, ComponentGroup<Component...>);

		template <typename Component>
		void OnComponentAdded(Entity entity, TransformComponent& tc, Component& component);

		template <typename... Component>
		void RemoveAllComponents(ComponentGroup<Component...>);

		template <typename Component>
		void RemoveComponents();

		template <typename... Component>
		void OnComponentRemoved(Entity entity, const std::type_info&, ComponentGroup<Component...>);

		template <typename Component>
		void OnComponentRemoved(Entity entity, Component& component);

	private:
		PhysicsSceneData* m_Data;

#ifndef DY_DIST
		DebugRendererRecorderData* m_Recorder = nullptr;
#endif
	};

}