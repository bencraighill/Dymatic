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
	private:
		void DrawEntityNode(Entity entity);
		void DrawComponents(Entity entity);
	private:
		bool m_SceneHierarchyVisible = true;
		bool m_PropertiesVisible = true;

		Ref<Scene> m_Context;
		Entity m_SelectionContext;

		Ref<Texture2D> m_CheckerboardTexture = Texture2D::Create("assets/textures/Checkerboard.png");

		Ref<Texture2D> m_IconAddComponent = Texture2D::Create("assets/icons/Properties/PropertiesAddComponent.png");
		Ref<Texture2D> m_IconDuplicate = Texture2D::Create("assets/icons/Properties/PropertiesDuplicate.png");
		Ref<Texture2D> m_IconDelete = Texture2D::Create("assets/icons/Properties/PropertiesDelete.png");

		Ref<Texture2D> m_IconEmptyEntity = Texture2D::Create("assets/icons/Properties/ComponentIcons/SceneIconEmptyEntity.png");
		Ref<Texture2D> m_IconTransformComponent = Texture2D::Create("assets/icons/Properties/ComponentIcons/SceneIconTransformComponent.png");
		Ref<Texture2D> m_IconSpriteRendererComponent = Texture2D::Create("assets/icons/Properties/ComponentIcons/SceneIconSpriteRendererComponent.png");
		Ref<Texture2D> m_IconParticleSystemComponent = Texture2D::Create("assets/icons/Properties/ComponentIcons/SceneIconParticleComponent.png");
		Ref<Texture2D> m_IconCameraComponent = Texture2D::Create("assets/icons/Properties/ComponentIcons/SceneIconCameraComponent.png");
	};

}