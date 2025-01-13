#pragma once

#include "Dymatic.h"
#include "Dymatic/Core/Base.h"
#include "Dymatic/Scene/Scene.h"
#include "Dymatic/Scene/Entity.h"

namespace Dymatic {

	class ScriptField;

	class SceneHierarchyPanel
	{
	public:
		SceneHierarchyPanel();
		SceneHierarchyPanel(const Ref<Scene>& scene);

		void SetContext(const Ref<Scene>& scene);

		void OnImGuiRender();

		inline Entity GetActiveEntity() { return m_ActiveEntity; }
		void SelectedEntity(Entity entity);
		inline bool IsEntitySelected(Entity entity) { return m_Context->IsEntitySelected(entity); }
		inline std::unordered_set<entt::entity>& GetSelectedEntities() { return m_Context->GetSelectedEntities(); }

		void ClearSelection();

		void DuplicateEntities();

		void DeleteEntity(Entity entity);
		void DeleteEntities();

		void UpdatePrefab(Entity entity);
		void RevertPrefab(Entity entity);
		void UnlinkPrefab(Entity entity);

		inline void ShowCreateMenu() { m_ShowCreateMenu = true; }

		inline bool IsEntitySelectable(Entity entity) { if (m_EntityModifiers.find(entity) == m_EntityModifiers.end()) return true; return m_EntityModifiers[entity].Selectable; }
		inline bool IsEntityLocked(Entity entity) { if (m_EntityModifiers.find(entity) == m_EntityModifiers.end()) return false; return m_EntityModifiers[entity].Locked; }
	private:
		template<typename T>
		void DisplayAddComponentEntry(const std::string& entryName);
		
		template <typename T>
		void DisplayCreateEntityEntry(const std::string& label, const std::string& entityName);
		void DisplayCreateEntityPopup();

		bool IsNodeSearchable(const std::string& name);
		void DrawEntityNode(Entity entity);
		void DrawBoneNode(Entity entity, const std::vector<Entity>& children, const BoneNodeData& node);

		void DrawComponents(Entity entity);
		
		bool DrawScriptField(const Entity& entity, const std::string& name, const ScriptField& field, void* data);

		inline void ToggleEntitySelectable(Entity entity) { m_EntityModifiers[entity].Selectable = !m_EntityModifiers[entity].Selectable; }
		inline void ToggleEntityLocked(Entity entity) { m_EntityModifiers[entity].Locked = !m_EntityModifiers[entity].Locked; }
	private:
		
		struct Modifiers
		{
			bool Selectable = true;
			bool Locked = false;
		};
		std::map<entt::entity, Modifiers> m_EntityModifiers;

		std::string m_SearchBuffer;

		Ref<Scene> m_Context;
		Entity m_ActiveEntity;

		std::function<void()> m_OnComplete;

		bool m_ShowCreateMenu = false;
	};

}