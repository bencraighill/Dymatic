#pragma once

#include "Dymatic/Core/Timestep.h"
#include "Dymatic/Core/UUID.h"
#include "Dymatic/Renderer/EditorCamera.h"

#include "Dymatic/Renderer/Texture.h"

#include "entt.hpp"

class b2World;
namespace physx
{
	class PxScene;
	class PxControllerManager;
	class VehicleManager;
}

namespace Dymatic {

	class Entity;

	class Scene
	{
	public:
		Scene();
		~Scene();

		static Ref<Scene> Copy(Ref<Scene> other);

		Entity CreateEntity(const std::string& name = std::string());
		Entity CreateEntityWithUUID(UUID uuid, std::string name = std::string());
		void DestroyEntity(Entity entity);

		void OnRuntimeStart();
		void OnRuntimeStop();

		void OnSimulationStart();
		void OnSimulationStop();

		void OnUpdateRuntime(Timestep ts);
		void OnUpdateSimulation(Timestep ts, EditorCamera& camera);
		void OnUpdateEditor(Timestep ts, EditorCamera& camera);
		void OnViewportResize(uint32_t width, uint32_t height);

		Entity DuplicateEntity(Entity entity);

		Entity FindEntityByName(std::string_view name);
		Entity GetEntityByUUID(UUID uuid);

		Entity GetPrimaryCameraEntity();

		bool IsRunning() const { return m_IsRunning; }
		bool IsPaused() const { return m_IsPaused; }

		void SetPaused(bool paused) { m_IsPaused = paused; }

		void Step(int frames = 1);

		template<typename... Components>
		auto GetAllEntitiesWith()
		{
			return m_Registry.view<Components...>();
		}

		bool IsEntityParented(Entity entity);
		bool DoesEntityHaveChildren(Entity entity);

		Entity GetEntityParent(Entity entity);
		std::vector<Entity> GetEntityChildren(Entity entity);

		void SetEntityParent(Entity entity, Entity parent);
		void RemoveEntityParent(Entity entity);

		// Helper Function
		glm::mat4 GetWorldTransform(Entity entity);
		glm::mat4 WorldToLocalTransform(Entity entity, glm::mat4 matrix);

	private:
		template<typename T>
		void OnComponentAdded(Entity entity, T& component);

		void OnPhysics2DStart();
		void OnPhysics2DUpdate(Timestep ts);
		void OnPhysics2DStop();

		void OnPhysicsStart();
		void OnPhysicsUpdate(Timestep ts);
		void OnPhysicsStop();

		void RenderSceneEditor(Timestep ts, EditorCamera& camera);

		bool IsEntitySelected(entt::entity entity);
		inline std::unordered_set<entt::entity>& GetSelectedEntities() { return m_SelectedEntities; }
		void ClearSelectedEntities();
		void SetSelectedEntity(entt::entity entity);
		void AddSelectedEntity(entt::entity entity);
		void RemoveSelectedEntity(entt::entity entity);
		
	private:

		UUID m_SceneID;
		entt::registry m_Registry;
		uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;
		bool m_IsRunning = false;

		std::map<entt::entity, entt::entity> m_Relations;

		entt::entity m_SceneEntity;

		bool m_IsPaused = false;
		int m_StepFrames = 0;
		
		std::unordered_set<entt::entity> m_SelectedEntities;

		b2World* m_Box2DWorld = nullptr;
		physx::PxScene* m_PhysXScene = nullptr;
		physx::PxControllerManager* m_PhysXControllerManager = nullptr;
		physx::VehicleManager* m_PhysXVehicleManager = nullptr;

		std::unordered_map<UUID, entt::entity> m_EntityMap;
		
		friend class Entity;
		friend class SceneSerializer;
		friend class SceneHierarchyPanel;
	};

}
