#pragma once

#include "Dymatic/Core/Timestep.h"
#include "Dymatic/Core/UUID.h"
#include "Dymatic/Renderer/EditorCamera.h"

#include "Dymatic/Scene/EntityRegistry.h"
#include "Dymatic/Scene/Prefab.h"

#include "Dymatic/Renderer/Texture.h"
#include "Dymatic/Physics/PhysicsEngine.h"

#include "entt.hpp"

class b2World;

namespace Dymatic {

	class Entity;
	class SceneCamera;

	class Scene : public EntityRegistry
	{
	public:
		static Ref<Scene> Create() { return CreateRef<Scene>(); }
		static Ref<Scene> Create(Ref<Scene> other) { return CreateRef<Scene>(other); }

		static AssetType GetStaticType() { return AssetType::Scene; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }

		// Editor Only
		static void InitEditorResources();
		
	public:
		Scene() = default;
		Scene(Ref<Scene> other);
		~Scene();

		static Ref<Scene> Copy(Ref<Scene> other);
		static void Copy(Ref<Scene> source, Ref<Scene> target);

		Entity Instantiate(Ref<Prefab> prefab);
		void CopyToPrefab(Entity entity, Ref<Prefab> prefab);
		void RevertPrefab(Entity entity);

		void OnRuntimeStart();
		void OnRuntimeStop();

		void OnSimulationStart();
		void OnSimulationStop();

		void OnUpdateRuntime(Timestep ts);
		void OnUpdateSimulation(Timestep ts, EditorCamera& camera);
		void OnUpdateEditor(Timestep ts, EditorCamera& camera);
		void OnViewportResize(uint32_t width, uint32_t height);

		void RenderSceneRuntime(Timestep ts, SceneCamera* mainCamera, const glm::mat4& cameraTransform);
		void RenderSceneEditor(Timestep ts, EditorCamera& camera);

		Entity GetPrimaryCameraEntity();

		bool IsRunning() const { return m_IsRunning; }
		bool IsPaused() const { return m_IsPaused; }

		void SetPaused(bool paused) { m_IsPaused = paused; }

		void Step(int frames = 1);

		// Entity Component Methods
		void SetEntityTransform(Entity entity, const Transform& transform);

		void SetEntityTranslation(Entity entity, const glm::vec3& translation);
		void SetEntityRotation(Entity entity, const glm::quat& rotation);
		void SetEntityRotation(Entity entity, const glm::vec3& rotation);
		void SetEntityScale(Entity entity, const glm::vec3& scale);

		// Note: Call the following if you modified transform directly and need to trigger an update
		void UpdateEntityTranslation(Entity entity);
		void UpdateEntityRotation(Entity entity);
		void UpdateEntityScale(Entity entity);

		void UpdateEntityScriptName(Entity entity);

		// Physics methods
		Ref<PhysicsScene> GetPhysicsScene() const { return m_PhysicsScene; }
		RaycastHit Raycast(const glm::vec3& origin, const glm::vec3& direction, float distance);
		RaycastHit Raycast(const glm::vec3& start, const glm::vec3& end);

		// Editor methods
		inline const bool GetShowColliders() const { return m_ShowColliders; }
		inline void SetShowColliders(bool showColliders) { m_ShowColliders = showColliders; }
		void DrawDebugLine(glm::vec3 start, glm::vec3 end, glm::vec4 color, float time = 0.0f);
		void DrawDebugCube(glm::vec3 position, glm::vec3 size, glm::vec4 color, float time = 0.0f);
		void DrawDebugSphere(glm::vec3 center, float radius, glm::vec4 color, float time = 0.0f);
		void ClearDebugDrawing();
		
	private:
		virtual void OnComponentAdded(Entity entity, const std::type_info& type) override;
		virtual void OnComponentRemoved(Entity entity, const std::type_info& type) override;

		static void Copy(Scene* source, Scene* target);

		void OnPhysics2DStart();
		void OnPhysics2DUpdate(Timestep ts);
		void OnPhysics2DStop();

		void OnPhysicsStart();
		void OnPhysicsUpdate(Timestep ts);
		void OnPhysicsStop();

		bool IsEntitySelected(entt::entity entity);
		inline std::unordered_set<entt::entity>& GetSelectedEntities() { return m_SelectedEntities; }
		void ClearSelectedEntities();
		void SetSelectedEntity(entt::entity entity);
		void AddSelectedEntity(entt::entity entity);
		void RemoveSelectedEntity(entt::entity entity);
		
	private:
		uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;
		bool m_IsRunning = false;

		bool m_IsPaused = false;
		int m_StepFrames = 0;
		
		std::unordered_set<entt::entity> m_SelectedEntities;

		b2World* m_Box2DWorld = nullptr;
		Ref<PhysicsScene> m_PhysicsScene = nullptr;

		// Scene Settings
		glm::vec3 m_Gravity = glm::vec3(0.0f, -9.81f, 0.0f);

		// Editor Data
		bool m_ShowColliders = false;
		struct DebugLine
		{
			glm::vec3 Start;
			glm::vec3 End;
			glm::vec4 Color;
			float Time;
		};
		std::vector<DebugLine> m_DebugLines;
		struct DebugCube
		{
			glm::mat4 Transform;
			glm::vec4 Color;
			float Time;
		};
		std::vector<DebugCube> m_DebugCubes;
		struct DebugSphere
		{
			glm::mat4 Transform;
			glm::vec4 Color;
			float Time;
		};
		std::vector<DebugSphere> m_DebugSpheres;
		
		friend class Entity;
		friend class SceneSerializer;
		friend class EntityRegistrySerializer;
		friend class SceneHierarchyPanel;
		friend class PhysicsScene;
	};

}
