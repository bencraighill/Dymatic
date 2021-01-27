#pragma once

#include "Dymatic/Core/Timestep.h"
#include "Dymatic/Renderer/EditorCamera.h"
#include "Dymatic/Renderer/Framebuffer.h"

//Included for texture Renderering
#include "Dymatic/Renderer/Texture.h"

#include "entt.hpp"

namespace Dymatic {

	class Entity;

	class Scene
	{
	public:
		Scene();
		~Scene();

		Entity CreateEntity(const std::string& name = std::string());
		Entity DuplicateEntity(Entity entity);
		void DestroyEntity(Entity entity);

		void OnUpdateRuntime(Timestep ts);
		void OnUpdateEditor(Timestep ts, EditorCamera& camera);
		void OnViewportResize(uint32_t width, uint32_t height);

		void DrawIDBuffer(Ref<Framebuffer> target, EditorCamera& camera);
		int Pixel(int x, int y);

		Entity GetPrimaryCameraEntity();
	private:
		template<typename T>
		void OnComponentAdded(Entity entity, T& component);
	private:
		entt::registry m_Registry;
		uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;

		Ref<Texture2D> m_GridTexture = Texture2D::Create("assets/icons/Scene/GridTexture.png");
		Ref<Texture2D> m_CheckerboardTexture = Texture2D::Create("assets/textures/Checkerboard.png");
		
		friend class Entity;
		friend class SceneSerializer;
		friend class SceneHierarchyPanel;
	};

}
