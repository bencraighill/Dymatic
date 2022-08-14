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
		Entity CreateEntityWithUUID(UUID uuid, const std::string& name = std::string());
		void DestroyEntity(Entity entity);

		void OnRuntimeStart();
		void OnRuntimeStop();

		void OnSimulationStart();
		void OnSimulationStop();

		void OnUpdateRuntime(Timestep ts, bool paused = false);
		void OnUpdateSimulation(Timestep ts, EditorCamera& camera, Entity selectedEntity, bool paused = false);
		void OnUpdateEditor(Timestep ts, EditorCamera& camera, Entity selectedEntity);
		void OnViewportResize(uint32_t width, uint32_t height);

		void DuplicateEntity(Entity entity);

		Entity GetPrimaryCameraEntity();

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
		void OnPhysics2DStop();

		void OnPhysicsStart();
		void OnPhysicsStop();

		void RenderScene(Timestep ts, EditorCamera& camera, Entity selectedEntity);
	private:

		// Icons
		Ref<Texture2D> m_LightIcon = Texture2D::Create("assets/icons/Scene/LightIcon.png");
		Ref<Texture2D> m_SoundIcon = Texture2D::Create("assets/icons/Scene/SoundIcon.png");

		UUID m_SceneID;
		entt::registry m_Registry;
		uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;

		std::map<entt::entity, entt::entity> m_Relations;

		entt::entity m_SceneEntity;

		b2World* m_Box2DWorld = nullptr;
		physx::PxScene* m_PhysXScene = nullptr;
		
		friend class Entity;
		friend class SceneSerializer;
		friend class SceneHierarchyPanel;
	};

}
