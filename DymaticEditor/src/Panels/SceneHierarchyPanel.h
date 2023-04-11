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

		void DuplicateEntities();

		void DeleteEntity(Entity entity);
		void DeleteEntities();

		inline void ShowCreateMenu() { m_ShowCreateMenu = true; }
		inline uint64_t& GetPickingID() { return m_PickingID; }
	private:
		template<typename T>
		void DisplayAddComponentEntry(const std::string& entryName);
		void DisplayCreateEntityPopup();

		void DrawEntityNode(Entity entity);
		void DrawComponents(Entity entity);
		
		bool DrawScriptField(const Entity& entity, const std::string& name, const ScriptField& field, void* data);

		inline bool IsEntitySelectable(Entity entity) { if (m_EntityModifiers.find(entity) == m_EntityModifiers.end()) return true; return m_EntityModifiers[entity].Selectable; }
		inline bool IsEntityLocked(Entity entity) { if (m_EntityModifiers.find(entity) == m_EntityModifiers.end()) return false; return m_EntityModifiers[entity].Locked; }
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

		bool m_ShowCreateMenu = false;
		
		std::string m_PickingField;
		uint64_t m_PickingID = 0;

		std::string m_EntitySearchBuffer;

		Ref<Texture2D> m_CheckerboardTexture = Texture2D::Create("Resources/Textures/Checkerboard.png");

		// Modifier Icons
		Ref<Texture2D> m_VisibleIcon = Texture2D::Create("Resources/Icons/SceneHierarchy/VisibleIcon.png");
		Ref<Texture2D> m_HiddenIcon = Texture2D::Create("Resources/Icons/SceneHierarchy/HiddenIcon.png");
		Ref<Texture2D> m_UnlockedIcon = Texture2D::Create("Resources/Icons/SceneHierarchy/UnlockedIcon.png");
		Ref<Texture2D> m_LockedIcon = Texture2D::Create("Resources/Icons/SceneHierarchy/LockedIcon.png");
		Ref<Texture2D> m_SelectableIcon = Texture2D::Create("Resources/Icons/SceneHierarchy/SelectableIcon.png");
		Ref<Texture2D> m_NonSelectableIcon = Texture2D::Create("Resources/Icons/SceneHierarchy/NonSelectableIcon.png");

		Ref<Texture2D> m_IconAddComponent = Texture2D::Create("Resources/Icons/Properties/PropertiesAddComponent.png");
		Ref<Texture2D> m_IconDuplicate = Texture2D::Create("Resources/Icons/Properties/PropertiesDuplicate.png");
		Ref<Texture2D> m_IconDelete = Texture2D::Create("Resources/Icons/Properties/PropertiesDelete.png");
		Ref<Texture2D> m_IconRevert = Texture2D::Create("Resources/Icons/Properties/PropertiesRevert.png");
		Ref<Texture2D> m_IconPicker = Texture2D::Create("Resources/Icons/Properties/PickerIcon.png");

		Ref<Texture2D> m_IconTransformComponent = Texture2D::Create("Resources/Icons/Properties/ComponentIcons/SceneIconTransformComponent.png");
		Ref<Texture2D> m_IconSpriteRendererComponent = Texture2D::Create("Resources/Icons/Properties/ComponentIcons/SceneIconSpriteRendererComponent.png");
		Ref<Texture2D> m_IconParticleSystemComponent = Texture2D::Create("Resources/Icons/Properties/ComponentIcons/SceneIconParticleComponent.png");
		Ref<Texture2D> m_IconCameraComponent = Texture2D::Create("Resources/Icons/Properties/ComponentIcons/SceneIconCameraComponent.png");
	};

}