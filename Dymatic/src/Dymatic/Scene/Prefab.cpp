#include "dypch.h"
#include "Dymatic/Scene/Prefab.h"

#include "Dymatic/Scene/Entity.h"

namespace Dymatic {

	Ref<Prefab> Prefab::Create(Entity root)
	{
		return CreateRef<Prefab>(root);
	}

	Prefab::Prefab(Entity root)
	{
		UpdateRoot(root);
	}

	Entity Prefab::GetRootEntity()
	{
		auto& relationshipView = m_Registry.view<RelationshipComponent>();
		for (const auto& e : relationshipView)
		{
			Entity entity = { e, this };
			if (!entity.HasParent())
				return entity;
		}

		return Entity();
	}

	void Prefab::UpdateRoot(Entity source)
	{
		Reset();
		CopyEntity(source);

		// Reset translation for the root entity
		Entity newRoot = GetRootEntity();

		// Internally in prefabs, root will not have a prefab component
		if (newRoot.HasComponent<TransformComponent>())
			newRoot.GetComponent<TransformComponent>().Transform.Translation = {};

		// Prefab components serve as references to prefab assets so we do not bake them in like in scenes
		auto& prefabView = m_Registry.view<PrefabComponent>();
		for (const auto& e : prefabView)
		{
			Entity entity = { e, this };

			if (entity == newRoot)
				continue;

			// Non root prefabs are treated as references so we don't need their sub-hierarchy
			const auto& children = entity.GetChildren();
			for (const auto& child : children)
				DestroyEntity(child);
		}
	}

	void Prefab::OnComponentAdded(Entity entity, const std::type_info& type)
	{
	}

	void Prefab::OnComponentRemoved(Entity entity, const std::type_info& componentType)
	{
	}

}