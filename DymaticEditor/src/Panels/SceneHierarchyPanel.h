#pragma once

#include "Dymatic.h"
#include "Dymatic/Core/Base.h"
#include "Dymatic/Scene/Scene.h"
#include "Dymatic/Scene/Entity.h"

namespace Dymatic {

	class SceneHierarchyPanel
	{
	public:
		SceneHierarchyPanel() = default;
		SceneHierarchyPanel(const Ref<Scene>& scene);

		void SetContext(const Ref<Scene>& scene);

		void OnImGuiRender();

		Entity GetSelectedEntity() const { return m_SelectionContext; }
		void SetSelectedEntity(Entity entity);

		bool& GetSceneHierarchyVisible() { return m_SceneHierarchyVisible; }
		bool& GetPropertiesVisible() { return m_PropertiesVisible; }

		void DeleteEntity(Entity entity);

		inline void ShowCreateMenu() { m_ShowCreateMenu = true; }
	private:
		template<typename T>
		void DisplayAddComponentEntry(const std::string& entryName);

		void DrawEntityNode(Entity entity);
		void DrawComponents(Entity entity);

		inline bool IsEntitySelectable(Entity entity) { if (m_EntityModifiers.find(entity) == m_EntityModifiers.end()) return true; return m_EntityModifiers[entity].Selectable; }
		inline bool IsEntityLocked(Entity entity) { if (m_EntityModifiers.find(entity) == m_EntityModifiers.end()) return false; return m_EntityModifiers[entity].Locked; }
		inline void ToggleEntitySelectable(Entity entity) { m_EntityModifiers[entity].Selectable = !m_EntityModifiers[entity].Selectable; }
		inline void ToggleEntityLocked(Entity entity) { m_EntityModifiers[entity].Locked = !m_EntityModifiers[entity].Locked; }
	private:
		bool m_SceneHierarchyVisible = true;
		bool m_PropertiesVisible = true;

		struct Modifiers
		{
			bool Selectable = true;
			bool Locked = false;
		};
		std::map<entt::entity, Modifiers> m_EntityModifiers;

		std::string m_SearchBuffer;

		Ref<Scene> m_Context;
		Entity m_SelectionContext;

		bool m_ShowCreateMenu = false;

		Ref<Texture2D> m_CheckerboardTexture = Texture2D::Create("assets/textures/Checkerboard.png");

		// Search bar
		Ref<Texture2D> m_SearchbarIcon = Texture2D::Create("assets/icons/SceneHierarchy/SearchbarIcon.png");
		Ref<Texture2D> m_ClearIcon = Texture2D::Create("assets/icons/SceneHierarchy/ClearIcon.png");

		Ref<Texture2D> m_EntityIcon = Texture2D::Create("assets/icons/Properties/ComponentIcons/SceneIconEmptyEntity.png");
		Ref<Texture2D> m_FolderIcon = Texture2D::Create("assets/icons/SceneHierarchy/FolderIcon.png");

		// Modifier Icons
		Ref<Texture2D> m_VisibleIcon = Texture2D::Create("assets/icons/SceneHierarchy/VisibleIcon.png");
		Ref<Texture2D> m_HiddenIcon = Texture2D::Create("assets/icons/SceneHierarchy/HiddenIcon.png");
		Ref<Texture2D> m_UnlockedIcon = Texture2D::Create("assets/icons/SceneHierarchy/UnlockedIcon.png");
		Ref<Texture2D> m_LockedIcon = Texture2D::Create("assets/icons/SceneHierarchy/LockedIcon.png");
		Ref<Texture2D> m_SelectableIcon = Texture2D::Create("assets/icons/SceneHierarchy/SelectableIcon.png");
		Ref<Texture2D> m_NonSelectableIcon = Texture2D::Create("assets/icons/SceneHierarchy/NonSelectableIcon.png");

		Ref<Texture2D> m_IconAddComponent = Texture2D::Create("assets/icons/Properties/PropertiesAddComponent.png");
		Ref<Texture2D> m_IconDuplicate = Texture2D::Create("assets/icons/Properties/PropertiesDuplicate.png");
		Ref<Texture2D> m_IconDelete = Texture2D::Create("assets/icons/Properties/PropertiesDelete.png");
		Ref<Texture2D> m_IconRevert = Texture2D::Create("assets/icons/Properties/PropertiesRevert.png");

		Ref<Texture2D> m_IconTransformComponent = Texture2D::Create("assets/icons/Properties/ComponentIcons/SceneIconTransformComponent.png");
		Ref<Texture2D> m_IconSpriteRendererComponent = Texture2D::Create("assets/icons/Properties/ComponentIcons/SceneIconSpriteRendererComponent.png");
		Ref<Texture2D> m_IconParticleSystemComponent = Texture2D::Create("assets/icons/Properties/ComponentIcons/SceneIconParticleComponent.png");
		Ref<Texture2D> m_IconCameraComponent = Texture2D::Create("assets/icons/Properties/ComponentIcons/SceneIconCameraComponent.png");
	};

}