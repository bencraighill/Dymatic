#include "dypch.h"
#include "Scene.h"

#include "Components.h"
#include "Dymatic/Renderer/Renderer2D.h"

#include <glm/glm.hpp>
#include <glad/glad.h>

#include "Entity.h"

#include <sys/stat.h>

namespace Dymatic {

	Scene::Scene()
	{
	}

	Scene::~Scene()
	{
	}

	Entity Scene::CreateEntity(const std::string& name)
	{
		Entity entity = { m_Registry.create(), this };
		entity.AddComponent<TransformComponent>();
		auto& tag = entity.AddComponent<TagComponent>();
		tag.Tag = name.empty() ? "Entity" : name;
		return entity;
	}

	Entity Scene::DuplicateEntity(Entity entity)
	{
		//Finding Tag
		auto initialTag = entity.GetComponent<TagComponent>().Tag;
		int lastDot = initialTag.find_last_of(".");
		if (lastDot != -1)
		{			
			std::string s = initialTag.substr(lastDot + 1, initialTag.length() - 1);;
			std::string::const_iterator it = s.begin();
			while (it != s.end() && std::isdigit(*it)) ++it;
			bool isNumber = !s.empty() && it == s.end();
			if (isNumber)
				initialTag = initialTag.substr(0, lastDot);

		}

		auto view = m_Registry.view<TagComponent>();
		std::string nameValue = "Duplicated Entity";
		int currentInt = 1;
		for (bool foundValue = false; foundValue == false;)
		{
			bool success = true;
			for (auto entityView : view)
			{
				const auto& tag = view.get<TagComponent>(entityView);
				if (tag.Tag == initialTag + "." + (std::to_string(currentInt).length() == 1 ? "00" + std::to_string(currentInt) : std::to_string(currentInt).length() == 2 ? "0" + std::to_string(currentInt) : std::to_string(currentInt)))
				{
					success = false;
				}
			}
			if (success == true)
			{
				foundValue = true;
				break;
			}
			currentInt++;
		}
		nameValue = initialTag + "." + (std::to_string(currentInt).length() == 1 ? "00" + std::to_string(currentInt) : std::to_string(currentInt).length() == 2 ? "0" + std::to_string(currentInt) : std::to_string(currentInt));


		Entity createdEntity = CreateEntity(nameValue);
		createdEntity.GetComponent<TransformComponent>() = entity.GetComponent<TransformComponent>();
		if (entity.HasComponent<SpriteRendererComponent>())
		{
			createdEntity.AddComponent<SpriteRendererComponent>();
			createdEntity.GetComponent<SpriteRendererComponent>() = entity.GetComponent<SpriteRendererComponent>();
		}
		if (entity.HasComponent<CameraComponent>())
		{
			createdEntity.AddComponent<CameraComponent>();
			createdEntity.GetComponent<CameraComponent>() = entity.GetComponent<CameraComponent>();
		}
		return createdEntity;
	}

	void Scene::DestroyEntity(Entity entity)
	{
		m_Registry.destroy(entity);
	}

	void Scene::OnUpdateRuntime(Timestep ts)
	{
		// Update scripts
		{
			m_Registry.view<NativeScriptComponent>().each([=](auto entity, auto& nsc)
			{
				// TODO: Move to Scene::OnScenePlay
				if (!nsc.Instance)
				{
					nsc.Instance = nsc.InstantiateScript();
					nsc.Instance->m_Entity = Entity{ entity, this };
					nsc.Instance->OnCreate();
				}

				nsc.Instance->OnUpdate(ts);
			});
		}

		// Render 2D
		Camera* mainCamera = nullptr;
		glm::mat4 cameraTransform;
		{
			auto view = m_Registry.view<TransformComponent, CameraComponent>();
			for (auto entity : view)
			{
				auto [transform, camera] = view.get<TransformComponent, CameraComponent>(entity);

				if (camera.Primary)
				{
					mainCamera = &camera.Camera;
					cameraTransform = transform.GetTransform();
					break;
				}
			}
		}

		if (mainCamera)
		{
			Renderer2D::BeginScene(*mainCamera, cameraTransform);

			auto group = m_Registry.group<TransformComponent>(entt::get<SpriteRendererComponent>);
			for (auto entity : group)
			{
				auto [transform, sprite] = group.get<TransformComponent, SpriteRendererComponent>(entity);

				Renderer2D::DrawSprite(transform.GetTransform(), sprite, (int)entity);
			}

			Renderer2D::EndScene();
		}

	}

	void Scene::OnUpdateEditor(Timestep ts, EditorCamera& camera)
	{
		//Ground Plane, TODO: Remove
		//-------------------------------------------------------------------------------------------------------//
		GLint previous[2];
		glGetIntegerv(GL_POLYGON_MODE, previous);

		Renderer2D::BeginScene(camera);

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		glm::mat4 rotation1 = glm::toMat4(glm::quat(glm::vec3{ -1.5708f, 0.0f, 0.0f }));

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3{ 0.0f, -2.0f, 0.0f })
			* rotation1
			* glm::scale(glm::mat4(1.0f), glm::vec3{ 20.0f, 20.0f, 20.0f });

		Dymatic::Renderer2D::DrawQuad(transform, m_GridTexture, 20.0f, glm::vec4{ 0.318f, 0.318f, 0.318f, 1.0f });

		Renderer2D::EndScene();
		//-------------------------------------------------------------------------------------------------------//

		Renderer2D::BeginScene(camera);
		glPolygonMode(GL_FRONT, previous[0]);
		glPolygonMode(GL_BACK, previous[1]);

		auto group = m_Registry.group<TransformComponent>(entt::get<SpriteRendererComponent>);
		for (auto entity : group)
		{
			auto [transform, sprite] = group.get<TransformComponent, SpriteRendererComponent>(entity);

			Renderer2D::DrawSprite(transform.GetTransform(), sprite, (int)entity);
		}

		Renderer2D::EndScene();
	}

	void Scene::OnViewportResize(uint32_t width, uint32_t height)
	{
		m_ViewportWidth = width;
		m_ViewportHeight = height;

		// Resize our non-FixedAspectRatio cameras
		auto view = m_Registry.view<CameraComponent>();
		for (auto entity : view)
		{
			auto& cameraComponent = view.get<CameraComponent>(entity);
			if (!cameraComponent.FixedAspectRatio)
				cameraComponent.Camera.SetViewportSize(width, height);
		}
	}

	Entity Scene::GetPrimaryCameraEntity()
	{
		auto view = m_Registry.view<CameraComponent>();
		for (auto entity : view)
		{
			const auto& camera = view.get<CameraComponent>(entity);
			if (camera.Primary)
				return Entity{ entity, this };
		}
		return {};
	}

	template<typename T>
	void Scene::OnComponentAdded(Entity entity, T& component)
	{
		static_assert(false);
	}

	template<>
	void Scene::OnComponentAdded<TransformComponent>(Entity entity, TransformComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<CameraComponent>(Entity entity, CameraComponent& component)
	{
		component.Camera.SetViewportSize(m_ViewportWidth, m_ViewportHeight);
	}

	template<>
	void Scene::OnComponentAdded<SpriteRendererComponent>(Entity entity, SpriteRendererComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<TagComponent>(Entity entity, TagComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<NativeScriptComponent>(Entity entity, NativeScriptComponent& component)
	{
	}


}