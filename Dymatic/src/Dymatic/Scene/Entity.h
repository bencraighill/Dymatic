#pragma once

#include "Dymatic/Core/UUID.h"
#include "Scene.h"
#include "Components.h"

#include "entt.hpp"

namespace Dymatic {

	typedef uint64_t EntityHandle;

	class Entity
	{
	public:
		Entity() = default;
		Entity(entt::entity handle, EntityRegistry* scene);
		Entity(const Entity& other) = default;

		template<typename T, typename... Args>
		T& AddComponent(Args&&... args)
		{
			DY_CORE_ASSERT(!HasComponent<T>(), "Entity already has component!");
			T& component = m_Scene->m_Registry.emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
			m_Scene->OnComponentAdded<T>(*this, component);
			return component;
		}

		template<typename T, typename... Args>
		T& AddOrReplaceComponent(Args&&... args)
		{
			T& component = m_Scene->m_Registry.emplace_or_replace<T>(m_EntityHandle, std::forward<Args>(args)...);
			m_Scene->OnComponentAdded<T>(*this, component);
			return component;
		}

		template<typename T>
		const T& GetComponent() const
		{
			DY_CORE_ASSERT(HasComponent<T>(), "Entity does not have component!");
			return m_Scene->m_Registry.get<T>(m_EntityHandle);
		}

		template<typename T>
		T& GetComponent()
		{
			DY_CORE_ASSERT(HasComponent<T>(), "Entity does not have component!");
			return m_Scene->m_Registry.get<T>(m_EntityHandle);
		}

		template<typename T>
		bool HasComponent() const
		{
			return m_Scene->m_Registry.has<T>(m_EntityHandle);
		}

		template<typename T>
		void RemoveComponent()
		{
			DY_CORE_ASSERT(HasComponent<T>(), "Entity does not have component!");
			m_Scene->OnComponentRemoved<T>(*this, GetComponent<T>());
			m_Scene->m_Registry.remove<T>(m_EntityHandle);
		}

		operator bool() const { return m_EntityHandle != entt::null; }
		operator entt::entity() const { return m_EntityHandle; }
		operator uint32_t() const { return (uint32_t)m_EntityHandle; }

		UUID GetUUID() const { return GetComponent<IDComponent>().ID; }
		const std::string& GetName() { return GetComponent<TagComponent>().Tag; }

		EntityHandle GetParentHandle() const { return GetComponent<RelationshipComponent>().ParentHandle; };
		const bool HasParent() const { return GetComponent<RelationshipComponent>().ParentHandle != 0; }

		const std::vector<EntityHandle>& GetChildrenUUIDs() const { return GetComponent<RelationshipComponent>().Children; }
		const size_t GetChildCount() const { return GetComponent<RelationshipComponent>().Children.size(); }
		const bool HasChildren() const { return GetChildCount() != 0; }

		Entity GetParent() const
		{
			UUID parentUUID = GetComponent<RelationshipComponent>().ParentHandle;

			if (parentUUID == 0)
				return Entity();
			
			return m_Scene->GetEntityByUUID(GetComponent<RelationshipComponent>().ParentHandle); 
		}

		std::vector<Entity> GetChildren() const
		{
			std::vector<Entity> children;
			auto& childrenUUIDs = GetComponent<RelationshipComponent>().Children;
			children.reserve(childrenUUIDs.size());
			
			for (UUID childUUID : childrenUUIDs)
				children.push_back(m_Scene->GetEntityByUUID(childUUID));
			
			return children;
		}

		Transform GetWorldTransform()
		{
			return m_Scene->GetWorldTransform(*this);
		}

		Transform GetLocalTransform(const Transform& worldTransform)
		{
			return m_Scene->GetLocalTransform(*this, worldTransform);
		}

		inline Scene* GetScene() const { return (Scene*)m_Scene; }
		inline EntityRegistry* GetEntityRegistry() const { return m_Scene; }

		bool operator==(const Entity& other) const
		{
			return m_EntityHandle == other.m_EntityHandle && m_Scene == other.m_Scene;
		}

		bool operator!=(const Entity& other) const
		{
			return !(*this == other);
		}

	private:
		entt::entity m_EntityHandle{ entt::null };
		EntityRegistry* m_Scene = nullptr;
	};

}