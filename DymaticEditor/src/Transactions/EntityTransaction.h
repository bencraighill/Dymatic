#pragma once

#include "Dymatic/Core/TransactionManager.h"

#include "Dymatic/Scene/Entity.h"

namespace Dymatic {

	template<typename ComponentType>
	constexpr const std::string GetComponentName()
	{
		const std::string typeName = typeid(ComponentType).name();
		const size_t position = typeName.find_last_of(":");
		return position == std::string::npos ? typeName : typeName.substr(position + 1);
	}

	class CreateEntityTransaction : public Transaction
	{
	public:
		CreateEntityTransaction(const std::string& name, Ref<Scene> m_Scene)
			: m_Name(name), m_Scene(m_Scene.get())
		{}

		virtual void Execute() override
		{
			m_Entity = m_Scene->CreateEntityWithUUID(m_EntityID, m_Name);
		}

		virtual void Undo() override
		{
			m_Scene->DestroyEntity(m_Entity);
		}

		virtual const std::string GetName() const override { return fmt::format("Create Entity '{}'", m_Name); }

	private:
		std::string m_Name;
		Scene* m_Scene;
		Entity m_Entity;
		
		// Storage to preserve the resultant entity ID after undo/redo.
		UUID m_EntityID;
	};

	class DeleteEntityTransaction : public Transaction
	{
	public:
		DeleteEntityTransaction(Entity entity)
			: m_Scene(entity.GetScene()), m_EntityID(entity.GetUUID()), m_Name(entity.GetName())
		{}

		virtual void Execute() override
		{
			Entity entity = m_Scene->GetEntityByUUID(m_EntityID);

			m_Registry = CreateRef<EntityRegistry>();
			m_Registry->CopyEntity(entity, nullptr, true);

			m_Scene->DestroyEntity(entity);
		}

		virtual void Undo() override
		{
			m_Scene->CopyEntity(m_Registry->GetEntityByUUID(m_EntityID), nullptr, true);
			m_Registry = nullptr;
		}

		virtual const std::string GetName() const override { return fmt::format("Delete Entity '{}'", m_Name); }

	private:
		Scene* m_Scene;
		UUID m_EntityID;
		std::string m_Name;

		Ref<EntityRegistry> m_Registry;
	};

	template<typename ComponentType>
	class AddComponentTransaction : public Transaction
	{
	public:
		AddComponentTransaction(Entity entity)
			: m_Scene(entity.GetScene()), m_EntityID(entity.GetUUID())
		{}

		virtual void Execute() override
		{
			m_Scene->GetEntityByUUID(m_EntityID).AddComponent<ComponentType>();
		}

		virtual void Undo() override
		{
			m_Scene->GetEntityByUUID(m_EntityID).RemoveComponent<ComponentType>();
		}

		virtual const std::string GetName() const override { return fmt::format("Add {}", GetComponentName<ComponentType>()); }

	private:
		// Warning: This is not a Ref<> object as entity.GetScene() returns a pointer which would then be cast to a Ref and
		// destroyed early. If we want to use a Ref<> in the future for safety we have to directly pass the Scene reference
		// from the editor (as we do not want Entity objects to store references that may not allow scene deletion).
		Scene* m_Scene;
		UUID m_EntityID;
	};

	template<typename ComponentType>
	class RemoveComponentTransaction : public Transaction
	{
	public:
		RemoveComponentTransaction(Entity entity)
			: m_Scene(entity.GetScene()), m_EntityID(entity.GetUUID()), m_Component(entity.GetComponent<ComponentType>())
		{}

		virtual void Execute() override
		{
			m_Scene->GetEntityByUUID(m_EntityID).RemoveComponent<ComponentType>();
		}

		virtual void Undo() override
		{
			m_Scene->GetEntityByUUID(m_EntityID).AddComponent<ComponentType>(m_Component);
		}

		virtual const std::string GetName() const override { return fmt::format("Remove {}", GetComponentName<ComponentType>()); }

	private:
		// Warning: This is not a Ref<> object as entity.GetScene() returns a pointer which would then be cast to a Ref and
		// destroyed early. If we want to use a Ref<> in the future for safety we have to directly pass the Scene reference
		// from the editor (as we do not want Entity objects to store references that may not allow scene deletion).
		Scene* m_Scene;
		UUID m_EntityID;
		ComponentType m_Component;
	};

	template<typename ComponentType>
	class ModifyComponentTransaction : public Transaction
	{
	public:
		ModifyComponentTransaction(Entity entity, const ComponentType& modifiedComponent)
			: m_Entity(entity), m_OriginalComponent(entity.GetComponent<ComponentType>()), m_ModifiedComponent(modifiedComponent)
		{}

		virtual void Execute() override
		{
			m_Entity->GetComponent<ComponentType>() = m_ModifiedComponent;
		}

		virtual void Undo() override
		{
			m_Entity->GetComponent<ComponentType>() = m_OriginalComponent;
		}

		virtual const std::string GetName() const override { return fmt::format("Modify {}", GetComponentName<ComponentType>()); }

	private:
		Entity m_Entity;
		ComponentType m_OriginalComponent;
		ComponentType m_ModifiedComponent;
	};

}