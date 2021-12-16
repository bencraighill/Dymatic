#pragma once

#include "Dymatic/Core/Timestep.h"
#include "Dymatic/Core/UUID.h"
#include "Dymatic/Renderer/EditorCamera.h"

#include "Dymatic/Renderer/Texture.h"

#include "entt.hpp"

class b2World;

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

		void OnUpdateRuntime(Timestep ts);
		void OnUpdateEditor(Timestep ts, EditorCamera& camera);
		void OnViewportResize(uint32_t width, uint32_t height);

		void DuplicateEntity(Entity entity);

		Entity GetPrimaryCameraEntity();

		template<typename... Components>
		auto GetAllEntitiesWith()
		{
			return m_Registry.view<Components...>();
		}
	private:
		template<typename T>
		void OnComponentAdded(Entity entity, T& component);
	private:
		UUID m_SceneID;
		entt::registry m_Registry;
		uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;

		entt::entity m_SceneEntity;

		b2World* m_Box2DWorld = nullptr;

		Ref<Texture2D> m_GridTexture = Texture2D::Create("assets/icons/Scene/GridTexture.png");
		
		friend class Entity;
		friend class SceneSerializer;
		friend class SceneHierarchyPanel;
	};

}
