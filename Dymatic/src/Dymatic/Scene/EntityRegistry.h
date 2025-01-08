#pragma once

#include "Dymatic/Asset/Asset.h"
#include "Dymatic/Scene/Transform.h"

#include "entt.hpp"
#include <unordered_map>

namespace Dymatic {

	class Entity;
	typedef uint64_t EntityHandle;

	typedef std::unordered_map<EntityHandle, EntityHandle> EntityIDMap;

	class EntityRegistry : public Asset
	{
	public:
		EntityRegistry();
		void Reset();

		Entity CreateEntity(const std::string& name = std::string());
		Entity CreateEntityWithUUID(UUID uuid, std::string name = std::string());
		void DestroyEntity(Entity entity);

		Entity CopyEntity(Entity entity, EntityIDMap* newEntities = nullptr, const bool preserveIDs = false);
		Entity DuplicateEntity(Entity entity);

		bool DoesEntityExist(EntityHandle uuid);
		bool DoesEntityExist(entt::entity entity);
		Entity FindEntityByName(std::string_view name);
		Entity GetEntityByUUID(UUID uuid);

		template<typename... Components>
		auto GetAllEntitiesWith()
		{
			return m_Registry.view<Components...>();
		}

		// Helper methods
		void ParentEntity(Entity child, Entity parent);
		void ParentEntity(Entity child, Entity parent, const std::string& boneName);
		void AttachEntity(Entity entity, const std::string& boneName);
		void UnparentEntity(Entity entity);

		Transform GetWorldTransform(Entity entity);
		Transform GetLocalTransform(Entity entity, Transform worldTransform);

		glm::mat4 GetWorldTransformMatrix(Entity entity);
		void SetWorldTransform(Entity, const Transform& transform);

		static void CopyEntityRegistry(EntityRegistry* source, EntityRegistry* target);

		inline const entt::registry& GetRegistry() const { return m_Registry; }
		inline entt::registry& GetRegistry() { return m_Registry; }
		size_t GetEntityCount() const;

	protected:
		template<typename T>
		void OnComponentAdded(Entity entity, T& component)
		{
			OnComponentAdded(entity, typeid(T));
		}

		template<typename T>
		void OnComponentRemoved(Entity entity, T& component)
		{
			OnComponentRemoved(entity, typeid(T));
		}

		virtual void OnComponentAdded(Entity entity, const std::type_info& componentType);
		virtual void OnComponentRemoved(Entity entity, const std::type_info& componentType);

		virtual AssetType GetAssetType() const override { return AssetType::None; }

	private:
		void DoDestroyEntity(Entity entity);
		
	protected:
		UUID m_RegistryID;
		entt::registry m_Registry;
		
		std::unordered_map<UUID, entt::entity> m_EntityMap;
		entt::entity m_SceneEntity;

		friend class Entity;
		friend class EntityRegistrySerializer;
		friend class SceneHierarchyPanel;
	};

}