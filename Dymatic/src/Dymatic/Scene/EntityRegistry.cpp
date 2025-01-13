#include "dypch.h"
#include "Dymatic/Scene/EntityRegistry.h"

#include "Components.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

#include "Entity.h"
#include "EntityRegistryHelper.h"

#include "Dymatic/Math/Math.h"

namespace Dymatic {

	EntityRegistry::EntityRegistry()
	{
		Reset();
	}

	void EntityRegistry::Reset()
	{
		// Reset variables
		m_Registry.clear();
		m_EntityMap.clear();
		m_SceneEntity = entt::null;

		// Create Scene entity
		m_SceneEntity = m_Registry.create();
		m_Registry.emplace<SceneComponent>(m_SceneEntity, m_RegistryID);
	}

	Entity EntityRegistry::CreateEntity(const std::string& name)
	{
		return CreateEntityWithUUID(UUID(), name);
	}

	Entity EntityRegistry::CreateEntityWithUUID(UUID uuid, std::string name)
	{
		Entity entity = { m_Registry.create(), this };
		entity.AddComponent<IDComponent>(uuid);
		entity.AddComponent<RelationshipComponent>();
		entity.AddComponent<TransformComponent>();
		auto& tag = entity.AddComponent<TagComponent>();
		tag.Tag = name.empty() ? "Entity" : name;

		m_EntityMap[uuid] = entity;
		
		return entity;
	}

	Entity EntityRegistry::CopyEntity(Entity entity, EntityIDMap* entityIDMap, const bool preserveIDs)
	{
		Entity newEntity = CreateEntityWithUUID(preserveIDs ? entity.GetUUID() : UUID(), entity.GetName());
		CopyComponentIfExists(AllComponents{}, newEntity, entity);

		if (entityIDMap)
			entityIDMap->insert({ (EntityHandle)entity.GetUUID(), (EntityHandle)newEntity.GetUUID() });

		// Duplicate down the hierarchy
		const auto& children = entity.GetChildren();
		for (const auto& child : children)
			ParentEntity(CopyEntity(child, entityIDMap), newEntity);

		return newEntity;
	}

	Entity EntityRegistry::DuplicateEntity(Entity entity)
	{
		Entity newEntity = CopyEntity(entity);

		// Duplication will respect source's parent
		if (entity.HasParent())
			ParentEntity(newEntity, entity.GetParent());

		return newEntity;
	}

	void EntityRegistry::DestroyEntity(Entity entity)
	{
		// Note: Only handle un-parenting logic at root of destruction (as all sub-entities are removed so are of no concern)
		if (entity.HasParent())
			UnparentEntity(entity);

		DoDestroyEntity(entity);
	}

	void EntityRegistry::OnComponentAdded(Entity entity, const std::type_info& componentType)
	{
	}

	void EntityRegistry::OnComponentRemoved(Entity entity, const std::type_info& componentType)
	{
	}

	void EntityRegistry::DoDestroyEntity(Entity entity)
	{
		// Destroy all the children first
		const auto& children = entity.GetChildren();
		for (const auto& child : children)
			DoDestroyEntity(child);

		// Remove this entity
		m_EntityMap.erase(entity.GetUUID());
		m_Registry.destroy(entity);
	}

	bool EntityRegistry::DoesEntityExist(EntityHandle uuid)
	{
		return m_EntityMap.find(uuid) != m_EntityMap.end();
	}

	bool EntityRegistry::DoesEntityExist(entt::entity entity)
	{
		return m_Registry.valid(entity);
	}

	Entity EntityRegistry::FindEntityByName(std::string_view name)
	{
		auto view = m_Registry.view<TagComponent>();
		for (auto entity : view)
		{
			const TagComponent& tc = view.get<TagComponent>(entity);
			if (tc.Tag == name)
				return Entity{ entity, this };
		}
		return {};
	}

	Entity EntityRegistry::GetEntityByUUID(UUID uuid)
	{
		if (m_EntityMap.find(uuid) != m_EntityMap.end())
			return { m_EntityMap.at(uuid), this };

		return {};
	}

	void EntityRegistry::ParentEntity(Entity child, Entity parent)
	{
		if (child == parent)
			return;

		DY_CORE_INFO("{} {}", child.GetComponent<TagComponent>().Tag, child.GetUUID());
		if (child.HasParent())
		{
			DY_CORE_TRACE("{} {}", child.GetComponent<TagComponent>().Tag, child.GetUUID());
			UnparentEntity(child);
		}

		RelationshipComponent& childRelationship = child.GetComponent<RelationshipComponent>();
		childRelationship.ParentHandle = parent.GetUUID();

		RelationshipComponent& parentRelationship = parent.GetComponent<RelationshipComponent>();
		parentRelationship.Children.push_back(child.GetUUID());
	}

	void EntityRegistry::ParentEntity(Entity child, Entity parent, const std::string& boneName)
	{
		ParentEntity(child, parent);
		AttachEntity(child, boneName);
	}

	void EntityRegistry::AttachEntity(Entity entity, const std::string& boneName)
	{
		Entity parent = entity.GetParent();

		if (!parent || !parent.HasComponent<StaticMeshComponent>() || !parent.GetComponent<StaticMeshComponent>().GetSkeleton())
			return;

		if (!entity.HasComponent<AttachmentComponent>())
			entity.AddComponent<AttachmentComponent>();

		entity.GetComponent<AttachmentComponent>().BoneName = boneName;
	}

	void EntityRegistry::UnparentEntity(Entity entity)
	{
		EntityHandle& parentHandle = entity.GetComponent<RelationshipComponent>().ParentHandle;

		if (parentHandle)
		{
			Entity parentEntity = GetEntityByUUID(parentHandle);
			auto& children = parentEntity.GetComponent<RelationshipComponent>().Children;
			children.erase(std::remove(children.begin(), children.end(), entity.GetUUID()), children.end());

			parentHandle = 0;
		}

		if (entity.HasComponent<AttachmentComponent>())
			entity.RemoveComponent<AttachmentComponent>();
	}

	static glm::vec3 GetSpringArmComponentOffset(const SpringArmComponent& sac)
	{
		return sac.TargetOffset + sac.SocketOffset + glm::vec3(0.0f, 0.0f, sac.CurrentLength >= 0.0f ? sac.CurrentLength : sac.TargetLength);
	}

	static bool ModifyTransformForBoneAttachment(Entity entity, Entity parent, Transform& transform)
	{
		if (!entity.HasComponent<AttachmentComponent>())
			return false;

		if (!entity.HasParent())
			return false;

		if (!parent.HasComponent<StaticMeshComponent>())
			return false;

		const auto& smc = parent.GetComponent<StaticMeshComponent>();
		Ref<AnimationGraphPlayer> player = smc.m_AnimationGraphPlayer;
		if (!player)
			return false;

		const auto& attachment = entity.GetComponent<AttachmentComponent>();
		const auto& boneInfoMap = player->GetAnimationGraph()->GetSkeleton()->GetBoneInfoMap();
		if (boneInfoMap.find(attachment.BoneName) == boneInfoMap.end())
			return false;

		const auto& boneMatrices = player->GetGlobalBoneMatrices();
		auto boneID = boneInfoMap.at(attachment.BoneName).id;
		if (boneID >= boneMatrices.size())
			return false;

		transform *= Transform::ConstructTransformFromMatrix(boneMatrices.at(boneID));
		return true;
	}

	Transform EntityRegistry::GetWorldTransform(Entity entity)
	{
		Transform transform;
		Entity currentEntity = entity;

		while (currentEntity)
		{
			Entity parentEntity = currentEntity.GetParent();

			if (currentEntity.HasComponent<TransformComponent>())
			{
				if (parentEntity && parentEntity.HasComponent<SpringArmComponent>())
				{
					const SpringArmComponent& sac = parentEntity.GetComponent<SpringArmComponent>();
					transform *= Transform(GetSpringArmComponentOffset(sac));
				}

				// Otherwise just modify current transform by the parent transform iteratively up the hierarchy
				transform *= currentEntity.GetComponent<TransformComponent>().Transform;

				// Check if we have a bone attachment
				ModifyTransformForBoneAttachment(currentEntity, parentEntity, transform);
			}

			currentEntity = parentEntity;
		}

		return transform;
	}

	Transform EntityRegistry::GetLocalTransform(Entity entity, Transform worldTransform)
	{
		Entity parent = entity.GetParent();

		if (!parent || !parent.HasComponent<TransformComponent>())
			return worldTransform;

		if (parent && parent.HasComponent<SpringArmComponent>())
		{
			const SpringArmComponent& sac = parent.GetComponent<SpringArmComponent>();
			worldTransform *= Transform(-GetSpringArmComponentOffset(sac));
		}

		worldTransform *= GetWorldTransform(parent).Inverse();

		Transform boneTransform;
		if (ModifyTransformForBoneAttachment(entity, parent, boneTransform))
			worldTransform *= boneTransform.Inverse();

		return worldTransform;
	}

	glm::mat4 EntityRegistry::GetWorldTransformMatrix(Entity entity)
	{
		return GetWorldTransform(entity).GetMatrix();
	}
	
	void EntityRegistry::CopyEntityRegistry(EntityRegistry* source, EntityRegistry* target)
	{
		auto& srcSceneRegistry = source->m_Registry;
		auto& dstSceneRegistry = target->m_Registry;
		std::unordered_map<UUID, entt::entity> enttMap;

		// Create entities in new scene
		auto idView = srcSceneRegistry.view<IDComponent>();
		for (auto e : idView)
		{
			UUID uuid = srcSceneRegistry.get<IDComponent>(e).ID;
			const auto& name = srcSceneRegistry.get<TagComponent>(e).Tag;
			Entity newEntity = target->CreateEntityWithUUID(uuid, name);
			newEntity.GetComponent<RelationshipComponent>() = srcSceneRegistry.get<RelationshipComponent>(e);
			enttMap[uuid] = (entt::entity)newEntity;
		}

		// Copy components (except IDComponent and TagComponent)
		CopyComponent(AllComponents{}, dstSceneRegistry, srcSceneRegistry, enttMap);
	}

	size_t EntityRegistry::GetEntityCount() const
	{
		return m_Registry.size();
	}

}