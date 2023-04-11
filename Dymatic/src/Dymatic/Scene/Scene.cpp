#include "dypch.h"
#include "Scene.h"

#include "Components.h"
#include "ScriptableEntity.h"
#include "Dymatic/Scripting/ScriptEngine.h"
#include "Dymatic/Renderer/Renderer2D.h"
#include "Dymatic/Renderer/SceneRenderer.h"
#include "Dymatic/Audio/AudioEngine.h"
#include "Dymatic/Physics/PhysicsEngine.h"

#include <glm/glm.hpp>

#include "Entity.h"

// Box2D
#include "box2d/b2_world.h"
#include "box2d/b2_body.h"
#include "box2d/b2_fixture.h"
#include "box2d/b2_polygon_shape.h"
#include "box2d/b2_circle_shape.h"

// PhysX
#include "Dymatic/Physics/PhysX.h"
#include "Dymatic/Physics/Vehicle/VehiclePhysics.h"
#include <PsThread.h>

#include "Dymatic/Math/Math.h"

#include "Dymatic/Core/Input.h"

namespace Dymatic {

	extern physx::PxPhysics* g_PhysicsEngine;
	extern physx::PxCooking* g_PhysicsCooking;
	extern physx::PxMaterial* g_DefaultMaterial;

	static bool s_Init = false;
	static Ref<Model> s_ArrowModel = nullptr;
	static Ref<Model> s_CameraModel = nullptr;
	static Ref<Texture2D> m_LightIcon = nullptr;
	static Ref<Texture2D> m_SoundIcon = nullptr;

	static std::unordered_map<UUID, Scene*> s_ActiveScenes;

	static b2BodyType DymaticRigidbody2DTypeToBox2D(Rigidbody2DComponent::BodyType bodyType)
	{
		switch (bodyType)
		{
		case Rigidbody2DComponent::BodyType::Static:    return b2_staticBody;
		case Rigidbody2DComponent::BodyType::Dynamic:   return b2_dynamicBody;
		case Rigidbody2DComponent::BodyType::Kinematic: return b2_kinematicBody;
		}
	}


	Scene::Scene()
	{
		// Create Scene entity
		m_SceneEntity = m_Registry.create();
		m_Registry.emplace<SceneComponent>(m_SceneEntity, m_SceneID);

		if (!s_Init)
		{
			s_ArrowModel = Model::Create("Resources/Objects/arrow/arrow.fbx");
			s_CameraModel = Model::Create("Resources/Objects/camera/camera.fbx");

			m_LightIcon = Texture2D::Create("Resources/Icons/Scene/LightIcon.png");
			m_SoundIcon = Texture2D::Create("Resources/Icons/Scene/SoundIcon.png");
		}
	}

	Scene::~Scene()
	{
		delete m_Box2DWorld;
	}

	template<typename... Component>
	static void CopyComponent(entt::registry& dst, entt::registry& src, const std::unordered_map<UUID, entt::entity>& enttMap)
	{
		([&]()
		{
			auto view = src.view<Component>();
			for (auto srcEntity : view)
			{
				entt::entity dstEntity = enttMap.at(src.get<IDComponent>(srcEntity).ID);

				auto& srcComponent = src.get<Component>(srcEntity);
				dst.emplace_or_replace<Component>(dstEntity, srcComponent);
			}
		}(), ...);
	}

	template<typename... Component>
	static void CopyComponent(ComponentGroup<Component...>, entt::registry& dst, entt::registry& src, const std::unordered_map<UUID, entt::entity>& enttMap)
	{
		CopyComponent<Component...>(dst, src, enttMap);
	}

	template<typename... Component>
	static void CopyComponentIfExists(Entity dst, Entity src)
	{
		([&]()
			{
				if (src.HasComponent<Component>())
					dst.AddOrReplaceComponent<Component>(src.GetComponent<Component>());
			}(), ...);
	}

	template<typename... Component>
	static void CopyComponentIfExists(ComponentGroup<Component...>, Entity dst, Entity src)
	{
		CopyComponentIfExists<Component...>(dst, src);
	}

	Ref<Scene> Scene::Copy(Ref<Scene> other)
	{
		Ref<Scene> newScene = CreateRef<Scene>();

		newScene->m_ViewportWidth = other->m_ViewportWidth;
		newScene->m_ViewportHeight = other->m_ViewportHeight;

		auto& srcSceneRegistry = other->m_Registry;
		auto& dstSceneRegistry = newScene->m_Registry;
		std::unordered_map<UUID, entt::entity> enttMap;

		// Create entities in new scene
		auto idView = srcSceneRegistry.view<IDComponent>();
		for (auto e : idView)
		{
			UUID uuid = srcSceneRegistry.get<IDComponent>(e).ID;
			const auto& name = srcSceneRegistry.get<TagComponent>(e).Tag;
			Entity newEntity = newScene->CreateEntityWithUUID(uuid, name);
			enttMap[uuid] = (entt::entity)newEntity;
		}

		// Copy components (except IDComponent and TagComponent)
		CopyComponent(AllComponents{}, dstSceneRegistry, srcSceneRegistry, enttMap);

		return newScene;
	}

	Entity Scene::CreateEntity(const std::string& name)
	{
		return CreateEntityWithUUID(UUID(), name);
	}

	Entity Scene::CreateEntityWithUUID(UUID uuid, std::string name)
	{
		Entity entity = { m_Registry.create(), this };
		entity.AddComponent<IDComponent>(uuid);
		entity.AddComponent<TransformComponent>();
		auto& tag = entity.AddComponent<TagComponent>();
		tag.Tag = name.empty() ? "Entity" : name;

		m_EntityMap[uuid] = entity;
		
		return entity;
	}

	void Scene::DestroyEntity(Entity entity)
	{
		m_EntityMap.erase(entity.GetUUID());
		m_Registry.destroy(entity);
	}

	void Scene::OnRuntimeStart()
	{
		m_IsRunning = true;

		s_ActiveScenes[m_SceneID] = this;
		OnPhysics2DStart();
		OnPhysicsStart();

		// Scripting
		{
			ScriptEngine::OnRuntimeStart(this);
			// Instantiate all script entities

			auto view = m_Registry.view<ScriptComponent>();
			for (auto e : view)
			{
				Entity entity = { e, this };
				ScriptEngine::OnCreateEntity(entity);
			}
		}
	}

	void Scene::OnRuntimeStop()
	{
		m_IsRunning = false;
		
		s_ActiveScenes.erase(m_SceneID);
		OnPhysics2DStop();
		OnPhysicsStop();

		// Scripting
		{
			auto view = m_Registry.view<ScriptComponent>();
			for (auto e : view)
			{
				Entity entity = { e, this };
				ScriptEngine::OnDestroyEntity(entity);
			}

			ScriptEngine::OnRuntimeStop();
		}

		auto view = m_Registry.view<AudioComponent>();
		for (auto entity : view)
		{
			auto ac = view.get<AudioComponent>(entity);
			ac.AudioSound->Stop();
			ac.Initialized = false;
		}
	}

	void Scene::OnSimulationStart()
	{
		s_ActiveScenes[m_SceneID] = this;
		OnPhysics2DStart();
		OnPhysicsStart();
	}

	void Scene::OnSimulationStop()
	{
		s_ActiveScenes.erase(m_SceneID);
		OnPhysics2DStop();
		OnPhysicsStop();
	}

	void Scene::OnUpdateRuntime(Timestep ts)
	{
		// Get Main Camera
		SceneCamera* mainCamera = nullptr;
		glm::mat4 cameraTransform;
		{
			auto view = m_Registry.view<TransformComponent, CameraComponent>();
			for (auto entity : view)
			{
				auto [transform, camera] = view.get<TransformComponent, CameraComponent>(entity);

				if (camera.Primary)
				{
					mainCamera = &camera.Camera;
					//cameraTransform = transform.GetTransform();
					cameraTransform = GetWorldTransform({ entity, this });
					break;
				}
			}
		}

		if (!m_IsPaused || m_StepFrames-- > 0)
		{
			// Update scripts
			{
				// C# Entity OnUpdate
				auto view = m_Registry.view<ScriptComponent>();
				for (auto e : view)
				{
					Entity entity = { e, this };
					ScriptEngine::OnUpdateEntity(entity, ts);
				}

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

			// Update Physics 2D
			OnPhysics2DUpdate(ts);

			// Update Physics
			OnPhysicsUpdate(ts);

			// Update Audio
			if (mainCamera)
			{
				auto view = m_Registry.view<TransformComponent, AudioComponent>();
				for (auto entity : view)
				{
					auto [transform, ac] = view.get<TransformComponent, AudioComponent>(entity);
					if (ac.AudioSound)
					{
						if (!ac.Initialized)
						{
							if (ac.StartOnAwake)
								ac.AudioSound->Play(ac.StartPosition);
							ac.Initialized = true;
						}

						ac.AudioSound->SetPosition(GetWorldTransform({ entity, this })[3]);
					}
				}
				AudioEngine::Update(cameraTransform[3], cameraTransform * glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
			}
		}

		// Renderer
		if (mainCamera)
		{
			SceneRenderer::BeginScene(*mainCamera, cameraTransform);
			
			if (!m_IsPaused || m_StepFrames-- > 0)
				SceneRenderer::UpdateTimestep(ts);
			
			// Submit Lights
			{
				auto view = m_Registry.view<TransformComponent, DirectionalLightComponent>();
				for (auto entity : view)
				{
					auto [transform, light] = view.get<TransformComponent, DirectionalLightComponent>(entity);
					//Renderer3D::SubmitDirectionalLight(transform.GetTransform(), light);
					SceneRenderer::SubmitDirectionalLight(GetWorldTransform({ entity, this }), light);
				}
			}
			{
				auto view = m_Registry.view<TransformComponent, PointLightComponent>();
				for (auto entity : view)
				{
					auto [transform, light] = view.get<TransformComponent, PointLightComponent>(entity);
					//Renderer3D::SubmitPointLight(transform.GetTransform(), light);
					SceneRenderer::SubmitPointLight(GetWorldTransform({ entity, this }), light);
				}
			}
			{
				auto view = m_Registry.view<TransformComponent, SpotLightComponent>();
				for (auto entity : view)
				{
					auto [transform, light] = view.get<TransformComponent, SpotLightComponent>(entity);
					//Renderer3D::SubmitSpotLight(transform.GetTransform(), light);
					SceneRenderer::SubmitSpotLight(GetWorldTransform({ entity, this }), light);
				}
			}
			
			SceneRenderer::SubmitLightSetup();
			
			{
				auto view = m_Registry.view<TransformComponent, SkyLightComponent>();
				for (auto entity : view)
				{
					auto [transform, light] = view.get<TransformComponent, SkyLightComponent>(entity);
					SceneRenderer::SubmitSkyLight(light);
				}
			}
			
			// Draw static meshes
			{
				auto view = m_Registry.view<TransformComponent, StaticMeshComponent>();
				for (auto entity : view)
				{
					auto [transform, mesh] = view.get<TransformComponent, StaticMeshComponent>(entity);
					mesh.Update(ts);
					SceneRenderer::SubmitStaticMesh(GetWorldTransform({ entity, this }), mesh, (int)entity);
					//Renderer3D::SubmitStaticMesh(transform.GetTransform(), mesh, (int)entity);
				}
			}
			
			// Submit lighting volumes
			{
				auto view = m_Registry.view<TransformComponent, VolumeComponent>();
				for (auto entity : view)
				{
					auto [transform, volume] = view.get<TransformComponent, VolumeComponent>(entity);
					SceneRenderer::SubmitVolume(transform, volume);
				}
			}
			
			SceneRenderer::RenderScene();
			
			SceneRenderer::EndScene();

			Renderer2D::BeginScene(*mainCamera, cameraTransform);

			// Draw sprites
			{
				auto group = m_Registry.group<TransformComponent>(entt::get<SpriteRendererComponent>);
				for (auto entity : group)
				{
					auto [transform, sprite] = group.get<TransformComponent, SpriteRendererComponent>(entity);
					//Renderer2D::DrawSprite(transform.GetTransform(), sprite, (int)entity);
					Renderer2D::DrawSprite(GetWorldTransform({ entity, this }), sprite, (int)entity);
				}
			}

			// Draw circles
			{
				auto view = m_Registry.view<TransformComponent, CircleRendererComponent>();
				for (auto entity : view)
				{
					auto [transform, circle] = view.get<TransformComponent, CircleRendererComponent>(entity);

					//Renderer2D::DrawCircle(transform.GetTransform(), circle.Color, circle.Thickness, circle.Fade, (int)entity);
					Renderer2D::DrawCircle(GetWorldTransform({ entity, this }), circle.Color, circle.Thickness, circle.Fade, (int)entity);
				}
			}

			// Draw Text
			{
				auto view = m_Registry.view<TransformComponent, TextComponent>();
				for (auto entity : view)
				{
					auto [transform, text] = view.get<TransformComponent, TextComponent>(entity);
					Renderer2D::DrawTextComponent(GetWorldTransform({ entity, this }), text, (int)entity);
				}
			}

			// Render UI
			{
				auto view = m_Registry.view<UIImageComponent>();
				for (auto entity : view)
				{
					if (!IsEntityParented({ entity, this }))
						continue;

					auto image = view.get<UIImageComponent>(entity);

					Entity parent = GetEntityParent({entity, this});

					glm::vec3 pos = glm::inverse(mainCamera->GetProjection()) * glm::vec4((image.Anchor.x * 2.0f - 1.0f) + (image.Position.x / (m_ViewportWidth * 0.5f)), (image.Anchor.y * 2.0f - 1.0f) + (image.Position.y / (m_ViewportHeight * 0.5f)), -1.0f, 1.0f);
					auto transform = cameraTransform * glm::translate(glm::mat4(1.0f), pos) * glm::scale(glm::mat4(1.0f), glm::vec3(image.Size.x / (m_ViewportHeight), image.Size.y / m_ViewportHeight, 1.0f));
					 
					Renderer2D::DrawQuad(transform, image.Image);
				}
			}

			Renderer2D::EndScene();
		}

	}

	void Scene::OnUpdateSimulation(Timestep ts, EditorCamera& camera)
	{
		if (!m_IsPaused || m_StepFrames-- > 0)
		{
			// Update Physics 2D
			OnPhysics2DUpdate(ts);

			// Update Physics
			OnPhysicsUpdate(ts);

			// Update Renderer Time
			SceneRenderer::UpdateTimestep(ts);

			AudioEngine::Update(camera.GetPosition(), camera.GetForwardDirection());
		}
		
		// Render
		RenderSceneEditor(ts, camera);
	}

	void Scene::OnUpdateEditor(Timestep ts, EditorCamera& camera)
	{
		// Update Renderer Time
		SceneRenderer::UpdateTimestep(ts);

		AudioEngine::Update(camera.GetPosition(), camera.GetForwardDirection());
		
		// Render
		RenderSceneEditor(ts, camera);
	}

	void Scene::OnViewportResize(uint32_t width, uint32_t height)
	{
		if (m_ViewportWidth == width && m_ViewportHeight == height)
			return;

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
	
	Entity Scene::DuplicateEntity(Entity entity)
	{
		Entity newEntity = CreateEntity(entity.GetName());
		CopyComponentIfExists(AllComponents{}, newEntity, entity);
		return newEntity;
	}

	void Scene::Step(int frames)
	{
		m_StepFrames = frames;
	}

	Entity Scene::FindEntityByName(std::string_view name)
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

	Entity Scene::GetEntityByUUID(UUID uuid)
	{
		if (m_EntityMap.find(uuid) != m_EntityMap.end())
			return { m_EntityMap.at(uuid), this };

		return {};
	}

	bool Scene::IsEntityParented(Entity entity)
	{
		return m_Relations.find(entity) != m_Relations.end();
	}

	bool Scene::DoesEntityHaveChildren(Entity entity)
	{
		for (auto& relation : m_Relations)
			if (relation.second == entity)
				return true;
		return false;
	}

	bool Scene::IsEntitySelected(entt::entity entity)
	{
		return m_SelectedEntities.find(entity) != m_SelectedEntities.end();
	}

	void Scene::ClearSelectedEntities()
	{
		m_SelectedEntities.clear();
	}

	void Scene::SetSelectedEntity(entt::entity entity)
	{
		if (entity == entt::null)
			m_SelectedEntities.clear();
		else if (m_SelectedEntities.find(entity) == m_SelectedEntities.end())
			m_SelectedEntities = { entity };
	}

	void Scene::AddSelectedEntity(entt::entity entity)
	{
		if (entity != entt::null)
			m_SelectedEntities.insert(entity);
	}

	void Scene::RemoveSelectedEntity(entt::entity entity)
	{
		if (m_SelectedEntities.find(entity) != m_SelectedEntities.end())
			m_SelectedEntities.erase(entity);
	}

	Entity Scene::GetEntityParent(Entity entity)
	{
		return Entity{ m_Relations[entity], this };
	}

	std::vector<Entity> Scene::GetEntityChildren(Entity entity)
	{
		std::vector<Entity> children;
		for (auto& relation : m_Relations)
			if (relation.second == entity)
				children.push_back(Entity{ relation.first, this });
		return children;
	}

	void Scene::SetEntityParent(Entity entity, Entity parent)
	{
		m_Relations[entity] = parent;
	}

	void Scene::RemoveEntityParent(Entity entity)
	{
		m_Relations.erase(entity);
	}

	glm::mat4 Scene::GetWorldTransform(Entity entity)
	{
		glm::mat4 matrix = entity.GetComponent<TransformComponent>().GetTransform();
		Entity currentEntity = entity;
		while (IsEntityParented(currentEntity))
		{
			currentEntity = GetEntityParent(currentEntity);
			if (currentEntity.HasComponent<TransformComponent>())
				matrix = currentEntity.GetComponent<TransformComponent>().GetTransform() * matrix;
		}
		return matrix;
	}

	glm::mat4 Scene::WorldToLocalTransform(Entity entity, glm::mat4 matrix)
	{
		Entity currentEntity = entity;
		while (IsEntityParented(currentEntity))
		{
			currentEntity = GetEntityParent(currentEntity);
			if (currentEntity.HasComponent<TransformComponent>())
				matrix = glm::inverse(currentEntity.GetComponent<TransformComponent>().GetTransform()) * matrix;
		}
		return matrix;
	}

	void Scene::OnPhysics2DStart()
	{
		// create box2d world
		// Create physics world and add bodies
		m_Box2DWorld = new b2World({ 0.0f, -9.8f });
		auto view = m_Registry.view<Rigidbody2DComponent>();
		for (auto e : view)
		{
			Entity entity = { e, this };
			auto& transform = entity.GetComponent<TransformComponent>();
			auto& rb2d = entity.GetComponent<Rigidbody2DComponent>();

			b2BodyDef bodyDef;
			bodyDef.type = DymaticRigidbody2DTypeToBox2D(rb2d.Type);
			bodyDef.position.Set(transform.Translation.x, transform.Translation.y);
			bodyDef.angle = transform.Rotation.z;

			b2Body* body = m_Box2DWorld->CreateBody(&bodyDef);
			body->SetFixedRotation(rb2d.FixedRotation);
			rb2d.RuntimeBody = body;

			if (entity.HasComponent<BoxCollider2DComponent>())
			{
				auto& bc2d = entity.GetComponent<BoxCollider2DComponent>();

				b2PolygonShape boxShape;
				boxShape.SetAsBox(bc2d.Size.x * transform.Scale.x, bc2d.Size.y * transform.Scale.y);

				b2FixtureDef fixtureDef;
				fixtureDef.shape = &boxShape;
				fixtureDef.density = bc2d.Density;
				fixtureDef.friction = bc2d.Friction;
				fixtureDef.restitution = bc2d.Restitution;
				fixtureDef.restitutionThreshold = bc2d.RestitutionThreshold;
				body->CreateFixture(&fixtureDef);
			}

			if (entity.HasComponent<CircleCollider2DComponent>())
			{
				auto& cc2d = entity.GetComponent<CircleCollider2DComponent>();

				b2CircleShape circleShape;
				circleShape.m_p.Set(cc2d.Offset.x, cc2d.Offset.y);
				circleShape.m_radius = transform.Scale.x * cc2d.Radius;

				b2FixtureDef fixtureDef;
				fixtureDef.shape = &circleShape;
				fixtureDef.density = cc2d.Density;
				fixtureDef.friction = cc2d.Friction;
				fixtureDef.restitution = cc2d.Restitution;
				fixtureDef.restitutionThreshold = cc2d.RestitutionThreshold;
				body->CreateFixture(&fixtureDef);
			}
		}
	}

	void Scene::OnPhysics2DUpdate(Timestep ts)
	{
		const int32_t velocityIterations = 6;
		const int32_t positionIterations = 2;
		m_Box2DWorld->Step(ts, velocityIterations, positionIterations);

		// retrieve transform from box2d
		auto view = m_Registry.view<Rigidbody2DComponent>();
		for (auto e : view)
		{
			Entity entity = { e, this };
			auto& transform = entity.GetComponent<TransformComponent>();
			auto& rb2d = entity.GetComponent<Rigidbody2DComponent>();
			b2Body* body = (b2Body*)rb2d.RuntimeBody;
			const auto& position = body->GetPosition();
			transform.Translation.x = position.x;
			transform.Translation.y = position.y;
			transform.Rotation.z = body->GetAngle();
		}
	}

	void Scene::OnPhysics2DStop()
	{
		// Destroy box2d world
		delete m_Box2DWorld;
		m_Box2DWorld = nullptr;
	}

	static PxFilterFlags VehicleFilterShader(
		PxFilterObjectAttributes attributes0, PxFilterData filterData0,
		PxFilterObjectAttributes attributes1, PxFilterData filterData1,
		PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize)
	{
		PX_UNUSED(constantBlock);
		PX_UNUSED(constantBlockSize);

		// let triggers through
		if (PxFilterObjectIsTrigger(attributes0) || PxFilterObjectIsTrigger(attributes1))
		{
			pairFlags = PxPairFlag::eTRIGGER_DEFAULT;
			return PxFilterFlags();
		}



		// use a group-based mechanism for all other pairs:
		// - Objects within the default group (mask 0) always collide
		// - By default, objects of the default group do not collide
		//   with any other group. If they should collide with another
		//   group then this can only be specified through the filter
		//   data of the default group objects (objects of a different
		//   group can not choose to do so)
		// - For objects that are not in the default group, a bitmask
		//   is used to define the groups they should collide with
		if ((filterData0.word0 != 0 || filterData1.word0 != 0) &&
			!(filterData0.word0 & filterData1.word1 || filterData1.word0 & filterData0.word1))
			return PxFilterFlag::eSUPPRESS;

		pairFlags = PxPairFlag::eCONTACT_DEFAULT;

		// The pairFlags for each object are stored in word2 of the filter data. Combine them.
		pairFlags |= PxPairFlags(PxU16(filterData0.word2 | filterData1.word2));
		return PxFilterFlags();
	}

	void Scene::OnPhysicsStart()
	{
		// Setup PhysX
		{
			physx::PxSceneDesc sceneDesc(g_PhysicsEngine->getTolerancesScale());
			sceneDesc.gravity = physx::PxVec3(0.0f, -9.81f, 0.0f);
			sceneDesc.cpuDispatcher = physx::PxDefaultCpuDispatcherCreate(shdfnd::Thread::getNbPhysicalCores() - 1);
			{
				sceneDesc.filterShader = VehicleFilterShader;//physx::PxDefaultSimulationFilterShader;
				sceneDesc.flags |= PxSceneFlag::eENABLE_PCM;
				//sceneDesc.flags |= PxSceneFlag::eENABLE_STABILIZATION;
				sceneDesc.flags |= PxSceneFlag::eENABLE_ACTIVE_ACTORS;
				//sceneDesc.flags |= PxSceneFlag::eREQUIRE_RW_LOCK;
			}
			
			m_PhysXScene = g_PhysicsEngine->createScene(sceneDesc);

			physx::PxPvdSceneClient* pvdClient = m_PhysXScene->getScenePvdClient();
			if (pvdClient)
			{
				pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
				pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
				pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
			}

			// Setup Rigid Body Components
			{
				PxFilterData simFilterData;
				simFilterData.word0 = COLLISION_FLAG_DRIVABLE_OBSTACLE;
				simFilterData.word1 = COLLISION_FLAG_DRIVABLE_OBSTACLE_AGAINST;
				PxFilterData qryFilterData;
				SetupDrivableShapeQueryFilterData(&qryFilterData);
				
				auto view = m_Registry.view<RigidbodyComponent>();
				for (auto e : view)
				{
					Entity entity = { e, this };

					auto& transform = entity.GetComponent<TransformComponent>();
					auto& rbc = entity.GetComponent<RigidbodyComponent>();

					physx::PxRigidActor* body;
					physx::PxVec3 pos{ transform.Translation.x, transform.Translation.y, transform.Translation.z };
					glm::quat q{ transform.Rotation };
					physx::PxQuat rot{ q.x, q.y, q.z, q.w };

					if (rbc.Type == RigidbodyComponent::BodyType::Static)
					{
						body = g_PhysicsEngine->createRigidStatic(physx::PxTransform(pos, rot));
					}
					else
					{
						physx::PxRigidDynamic* dynamic = g_PhysicsEngine->createRigidDynamic(physx::PxTransform(pos, rot));
						physx::PxRigidBodyExt::updateMassAndInertia(*dynamic, rbc.Density);
						body = dynamic;
					}

					if (entity.HasComponent<BoxColliderComponent>())
					{
						auto& bcc = entity.GetComponent<BoxColliderComponent>();
						auto shape = g_PhysicsEngine->createShape(physx::PxBoxGeometry(bcc.Size.x * transform.Scale.x, bcc.Size.y * transform.Scale.y, bcc.Size.z * transform.Scale.z), *g_DefaultMaterial);
						
						shape->setSimulationFilterData(simFilterData);
						shape->setQueryFilterData(qryFilterData);
						
						body->attachShape(*shape);
						shape->release();
					}

					if (entity.HasComponent<SphereColliderComponent>())
					{
						auto& scc = entity.GetComponent<SphereColliderComponent>();
						float scale = std::max({ transform.Scale.x, transform.Scale.y, transform.Scale.z });
						auto shape = g_PhysicsEngine->createShape(physx::PxSphereGeometry(scc.Radius * scale), *g_DefaultMaterial);
						body->attachShape(*shape);
						shape->release();
					}

					if (entity.HasComponent<CapsuleColliderComponent>())
					{
						auto& ccc = entity.GetComponent<CapsuleColliderComponent>();
						float scale = std::max(transform.Scale.y, transform.Scale.z);
						auto shape = g_PhysicsEngine->createShape(physx::PxCapsuleGeometry(ccc.Radius * transform.Scale.x, ccc.HalfHeight * scale), *g_DefaultMaterial);
						body->attachShape(*shape);
						shape->release();
					}

					if (entity.HasComponent<MeshColliderComponent>())
					{
						auto& mcc = entity.GetComponent<MeshColliderComponent>();
						auto& smc = entity.GetComponent<StaticMeshComponent>();
						if (mcc.Type == MeshColliderComponent::MeshType::Triangle)
						{
							for (auto& mesh : smc.GetModel()->GetMeshes())
							{
								physx::PxTriangleMeshDesc meshDesc;

								std::vector<physx::PxVec3> verts;
								auto& verticies = mesh->GetVerticies();
								for (auto& vertex : verticies)
									verts.push_back({ vertex.Position.x, vertex.Position.y, vertex.Position.z });

								meshDesc.points.count = verts.size();
								meshDesc.points.stride = sizeof(physx::PxVec3);
								meshDesc.points.data = verts.data();

								meshDesc.triangles.count = verticies.size() / 3;
								meshDesc.triangles.stride = 3 * sizeof(uint32_t);
								meshDesc.triangles.data = mesh->GetIndicies().data();

								physx::PxDefaultMemoryOutputStream writeBuffer;
								physx::PxTriangleMeshCookingResult::Enum result;
								bool status = g_PhysicsCooking->cookTriangleMesh(meshDesc, writeBuffer, &result);
								if (!status)
									DY_CORE_ASSERT(false, "Triangle Mesh cooking failure");

								physx::PxDefaultMemoryInputData readBuffer(writeBuffer.getData(), writeBuffer.getSize());
								auto mesh = g_PhysicsEngine->createTriangleMesh(readBuffer);
								auto shape = g_PhysicsEngine->createShape(physx::PxTriangleMeshGeometry(mesh), *g_DefaultMaterial);

								// Apply Scaling
								auto holder = shape->getGeometry();
								holder.triangleMesh().triangleMesh->acquireReference();
								holder.triangleMesh().scale.scale = physx::PxVec3(transform.Scale.x, transform.Scale.y, transform.Scale.z);
								shape->setGeometry(holder.any());
								holder.triangleMesh().triangleMesh->release();

								body->attachShape(*shape);
								shape->release();
								mesh->release();
							}
						}
						else
						{
							for (auto& mesh : smc.GetModel()->GetMeshes())
							{
								physx::PxConvexMeshDesc meshDesc;

								std::vector<physx::PxVec3> verts;
								auto& verticies = mesh->GetVerticies();
								for (auto& vertex : verticies)
									verts.push_back({ vertex.Position.x, vertex.Position.y, vertex.Position.z });

								meshDesc.points.count = verts.size();
								meshDesc.points.stride = sizeof(physx::PxVec3);
								meshDesc.points.data = verts.data();
								meshDesc.flags = physx::PxConvexFlag::eCOMPUTE_CONVEX;

								physx::PxDefaultMemoryOutputStream writeBuffer;
								physx::PxConvexMeshCookingResult::Enum result;
								bool status = g_PhysicsCooking->cookConvexMesh(meshDesc, writeBuffer, &result);
								if (!status)
									DY_CORE_ASSERT(false, "Convex Mesh cooking failure");

								physx::PxDefaultMemoryInputData readBuffer(writeBuffer.getData(), writeBuffer.getSize());
								auto mesh = g_PhysicsEngine->createConvexMesh(readBuffer);
								auto shape = g_PhysicsEngine->createShape(physx::PxConvexMeshGeometry(mesh), *g_DefaultMaterial);

								// Apply Scaling
								auto holder = shape->getGeometry();
								holder.convexMesh().convexMesh->acquireReference();
								holder.convexMesh().scale.scale = physx::PxVec3(transform.Scale.x, transform.Scale.y, transform.Scale.z);
								shape->setGeometry(holder.any());
								holder.convexMesh().convexMesh->release();

								body->attachShape(*shape);
								shape->release();
								mesh->release();
							}
						}
					}

					// Create New Physics Material
					{
						auto material = g_PhysicsEngine->createMaterial(rbc.StaticFriction, rbc.DynamicFriction, rbc.Restitution);

						// Setup Physics Material
						physx::PxShape** shapes = new physx::PxShape * [body->getNbShapes()];
						body->getShapes(shapes, body->getNbShapes());

						for (PxU32 i = 0; i < body->getNbShapes(); i++)
							shapes[i]->setMaterials(&material, 1);

						// Clean Up
						material->release();
						delete[] shapes;
					}

					m_PhysXScene->addActor(*body);

					rbc.RuntimeBody = body;
				}
			}

			// Setup Characters
			{
				// Create the controller manager
				m_PhysXControllerManager = PxCreateControllerManager(*m_PhysXScene);
				m_PhysXControllerManager->setOverlapRecoveryModule(true);
				m_PhysXControllerManager->setPreciseSweeps(true);
				
				auto view = m_Registry.view<CharacterMovementComponent>();
				for (auto e : view)
				{
					Entity entity = { e, this };
					
					auto& transform = entity.GetComponent<TransformComponent>();
					auto& cmc = entity.GetComponent<CharacterMovementComponent>();

					PxCapsuleControllerDesc desc;
					desc.position = PxExtendedVec3(transform.Translation.x, transform.Translation.y, transform.Translation.z);
					desc.density = cmc.Density;
					desc.radius = cmc.CapsuleRadius;
					desc.height = std::clamp(cmc.CapsuleHeight, cmc.CapsuleRadius, 2.0f * cmc.CapsuleRadius);
					desc.stepOffset = cmc.StepOffset;
					desc.slopeLimit = cosf(glm::radians(cmc.MaxWalkableSlope));
					desc.material = g_DefaultMaterial;

					PxController* controller = m_PhysXControllerManager->createController(desc);
					cmc.CharacterController = controller;
					cmc.Velocity = glm::vec3(0.0f);
				}
			}

			// Setup Vehicles
			{
				PxMaterial* StandardMaterials[1] = { g_DefaultMaterial };
				PxVehicleDrivableSurfaceType VehicleDrivableSurfaceTypes[1] = {0};
				m_PhysXVehicleManager = new physx::VehicleManager;
				m_PhysXVehicleManager->init(*g_PhysicsEngine, (const PxMaterial**)StandardMaterials, VehicleDrivableSurfaceTypes);
				
				auto view = m_Registry.view<VehicleMovementComponent>();
				for (auto e : view)
				{
					Entity entity = { e, this };

					auto& transform = entity.GetComponent<TransformComponent>();
					auto& vmc = entity.GetComponent<VehicleMovementComponent>();

					PxMaterial* vehicleMaterial = g_PhysicsEngine->createMaterial(0.0f, 0.0f, 0.0f);
					const float chassisMass = 1500.0f;

					const PxF32 x = 2.5f * 0.5f;
					const PxF32 y = 2.0f * 0.5f;
					const PxF32 z = 5.0f * 0.5f;
					PxVec3 verts[8] =
					{
						PxVec3( 1.20966995,1.94801998,-2.36412978),
						PxVec3( 1.20966995,1.94801998,2.36412978),
						PxVec3( 1.20966995,0.149092987,2.36412978),
						PxVec3( 1.20966995,0.149092987,-2.36412978),
						PxVec3(-1.20966995,1.94801998,-2.36412978),
						PxVec3(-1.20966995,1.94801998,2.36412978),
						PxVec3(-1.20966995,0.149092987,2.36412978),
						PxVec3(-1.20966995,0.149092987,-2.36412978)
					};

					physx::PxConvexMesh* chassisMesh = createConvexMesh(verts, 8, *g_PhysicsEngine, *g_PhysicsCooking);

					//createWheelConvexMesh();
					physx::PxConvexMesh* wheelMeshes[4] = {
						createCylinderConvexMesh(0.3f, 0.5f, 16, *g_PhysicsEngine, *g_PhysicsCooking),
						createCylinderConvexMesh(0.3f, 0.5f, 16, *g_PhysicsEngine, *g_PhysicsCooking),
						createCylinderConvexMesh(0.3f, 0.5f, 16, *g_PhysicsEngine, *g_PhysicsCooking),
						createCylinderConvexMesh(0.3f, 0.5f, 16, *g_PhysicsEngine, *g_PhysicsCooking)
					};
					const PxVec3 wheelCentreOffsets[4] = {						
						physx::PxVec3(-0.920349956f, -0.295977980f, 1.38437998f),
						physx::PxVec3(0.919347942f, -0.295977980f, 1.38437998),
						physx::PxVec3(-0.920349956, -0.295978993, -1.38437998f),
						physx::PxVec3(0.919347942, -0.295978993,  -1.63789999)
					};

					auto rotation = glm::quat(transform.Rotation);

					vmc.RuntimeVehicleID = m_PhysXVehicleManager->create4WVehicle(*m_PhysXScene, *g_PhysicsEngine, *g_PhysicsCooking,
						*vehicleMaterial, chassisMass, wheelCentreOffsets, chassisMesh, wheelMeshes,
						physx::PxTransform(
							PxVec3(
								transform.Translation.x,
								transform.Translation.y,
								transform.Translation.z
							),
							PxQuat(
								rotation.x,
								rotation.y,
								rotation.z,
								rotation.w
							)
						),
						true
					);
					
					vmc.VehicleController = new VehicleController;
				}
			}
		}
	}

	void Scene::OnPhysicsUpdate(Timestep ts)
	{
		// Update Vehicle Components
		{
			auto view = m_Registry.view<VehicleMovementComponent>();
			for (auto e : view)
			{
				Entity entity = { e, this };
				auto& transform = entity.GetComponent<TransformComponent>();
				auto& vmc = entity.GetComponent<VehicleMovementComponent>();

				// Update transform
				{
					auto body = m_PhysXVehicleManager->getVehicle(vmc.RuntimeVehicleID)->getRigidDynamicActor();
					body->wakeUp();
					const auto& PXtransform = body->getGlobalPose();
					transform.Translation.x = PXtransform.p.x;
					transform.Translation.y = PXtransform.p.y;
					transform.Translation.z = PXtransform.p.z;

					glm::quat rot;
					rot.x = PXtransform.q.x;
					rot.y = PXtransform.q.y;
					rot.z = PXtransform.q.z;
					rot.w = PXtransform.q.w;

					transform.Rotation = glm::eulerAngles(rot);
				}

				{
					PxSceneReadLock scopedLock(*m_PhysXScene);
					((VehicleController*)vmc.VehicleController)->update(ts, m_PhysXVehicleManager->getVehicleWheelQueryResults(vmc.RuntimeVehicleID), *m_PhysXVehicleManager->getVehicle(vmc.RuntimeVehicleID));

					// NOTE: ensure input header is removed
					((VehicleController*)vmc.VehicleController)->setCarKeyboardInputs(
						Input::IsKeyPressed(Key::W),
						Input::IsKeyPressed(Key::S),
						Input::IsKeyPressed(Key::L),
						Input::IsKeyPressed(Key::A),
						Input::IsKeyPressed(Key::D),
						false,
						false
					);
				}
			}

			m_PhysXVehicleManager->suspensionRaycasts(m_PhysXScene);
			m_PhysXVehicleManager->update(ts, m_PhysXScene->getGravity());
		}

		// Update Physics Scene
		{
			m_PhysXScene->simulate(ts);
			m_PhysXScene->fetchResults(true);
		}

		// Update Character Controllers
		{
			auto view = m_Registry.view<CharacterMovementComponent>();
			for (auto e : view)
			{
				Entity entity = { e, this };
				auto& transform = entity.GetComponent<TransformComponent>();
				auto& cmc = entity.GetComponent<CharacterMovementComponent>();

				if (PxCapsuleController* characterController = ((PxCapsuleController*)cmc.CharacterController))
				{		
					// Check if character is falling
					PxControllerState cctState;
					characterController->getState(cctState);
					bool isFalling = (cctState.collisionFlags & PxControllerCollisionFlag::eCOLLISION_DOWN) == 0;

					// Update stored Y Velocity - Accelerate downwards if falling or reset to 0 if not
					cmc.Velocity.y = isFalling ? cmc.Velocity.y + m_PhysXScene->getGravity().y * cmc.GravityScale * ts : 0.0f;

					if (!isFalling && Input::IsKeyPressed(Key::Space))
						cmc.Velocity.y = 6.0f;
					
					// Poll inputs
					const float xInput = Input::IsKeyPressed(Key::W) ? 1.0f : Input::IsKeyPressed(Key::S) ? -1.0f : 0.0f;
					const float zInput = Input::IsKeyPressed(Key::D) ? 1.0f : Input::IsKeyPressed(Key::A) ? -1.0f : 0.0f;
					
					// Update horizontal velocity components
					cmc.Velocity.x = std::clamp(cmc.Velocity.x + cmc.MaxAcceleration * (isFalling ? cmc.AirControl : 1.0f) * xInput, -std::abs(cmc.MaxWalkSpeed), std::abs(cmc.MaxWalkSpeed));
					cmc.Velocity.z = std::clamp(cmc.Velocity.z + cmc.MaxAcceleration * (isFalling ? cmc.AirControl : 1.0f) * zInput, -std::abs(cmc.MaxWalkSpeed), std::abs(cmc.MaxWalkSpeed));

					// Apply friction force
					if (!isFalling)
						cmc.Velocity += -cmc.Velocity * cmc.GroundFriction * ts.GetSeconds();

					// Move the character controller in PhysX world
					const glm::vec3 displacement = cmc.Velocity * ts.GetSeconds();
					characterController->move(PxVec3(displacement.x, displacement.y, displacement.z), 0.0f, ts, PxControllerFilters());
					
					// Update entity position
					const PxExtendedVec3 position = characterController->getPosition();
					transform.Translation = glm::vec3(position.x, position.y, position.z);

					glm::vec3 horizontalVelocity = glm::vec3(cmc.Velocity.x, 0.0f, cmc.Velocity.z);
					if (glm::length(horizontalVelocity) != 0.0f)
					{
						glm::vec3 horizontalDirection = glm::normalize(horizontalVelocity);
						transform.Rotation.y = glm::degrees(glm::eulerAngles(glm::quatLookAt(horizontalDirection, glm::vec3(0, 1, 0)))).y;
					}
				}
			}
		}

		// Update Rigid Body
		{
			auto view = m_Registry.view<RigidbodyComponent>();
			for (auto e : view)
			{
				Entity entity = { e, this };
				auto& transform = entity.GetComponent<TransformComponent>();
				auto& rbc = entity.GetComponent<RigidbodyComponent>();
				if (rbc.Type == RigidbodyComponent::BodyType::Dynamic)
				{
					auto body = (physx::PxRigidDynamic*)rbc.RuntimeBody;
					const auto& PXtransform = body->getGlobalPose();
					body->wakeUp();
					transform.Translation.x = PXtransform.p.x;
					transform.Translation.y = PXtransform.p.y;
					transform.Translation.z = PXtransform.p.z;

					glm::quat rot;
					rot.x = PXtransform.q.x;
					rot.y = PXtransform.q.y;
					rot.z = PXtransform.q.z;
					rot.w = PXtransform.q.w;

					transform.Rotation = glm::eulerAngles(rot);
				}
			}
		}

		// Update Directional Field
		{
			auto view = m_Registry.view<TransformComponent, DirectionalFieldComponent>();
			for (auto entity : view)
			{
				auto [transform, dfc] = view.get<TransformComponent, DirectionalFieldComponent>(entity);
				auto& translation = transform.Translation;
				auto& scale = transform.Scale;

				PxBounds3 fieldBounds = PxBounds3(
					PxVec3(-1.0f * scale.x + translation.x, -1.0f * scale.y + translation.y, -1.0f * scale.z + translation.z), 
					PxVec3( 1.0f * scale.x + translation.x,  1.0f * scale.y + translation.y,  1.0f * scale.z + translation.z)
				);

				auto view = m_Registry.view<RigidbodyComponent>();
				for (auto entity : view)
				{
					auto rbc = view.get<RigidbodyComponent>(entity);

					if (rbc.Type == RigidbodyComponent::BodyType::Dynamic)
					{
						auto* actor = (PxRigidDynamic*)rbc.RuntimeBody;
						auto& bounds = actor->getWorldBounds();
						if (bounds.intersects(fieldBounds))
						{
							actor->addForce(PxVec3(dfc.Force.x, dfc.Force.y, dfc.Force.z), PxForceMode::eFORCE);
						}
					}
				}
			}
		}

		// Update Radial Field
		{
			auto view = m_Registry.view<TransformComponent, RadialFieldComponent>();
			for (auto entity : view)
			{
				auto [fieldTransform, rfc] = view.get<TransformComponent, RadialFieldComponent>(entity);
				auto translation = fieldTransform.Translation;

				auto view = m_Registry.view<TransformComponent, RigidbodyComponent>();
				for (auto entity : view)
				{
					auto [transform, rbc] = view.get<TransformComponent, RigidbodyComponent>(entity);

					if (rbc.Type == RigidbodyComponent::BodyType::Dynamic)
					{
						const float distance = glm::distance(fieldTransform.Translation, transform.Translation);
						if (distance <= rfc.Radius)
						{	
							const float magnitude = rfc.Magnitude * (1.0f / (std::exp(distance * rfc.Falloff)));
							const auto direction = glm::normalize(transform.Translation - fieldTransform.Translation);

							auto* actor = (PxRigidDynamic*)rbc.RuntimeBody;
							actor->addForce(PxVec3(direction.x * magnitude, direction.y * magnitude, direction.z * magnitude), PxForceMode::eFORCE);
						}
					}
				}
			}
		}
		
		// Update Bouyancy Fields
		{
			auto view = m_Registry.view<TransformComponent, BouyancyFieldComponent>();
			for (auto entity : view)
			{
				auto [transform, bfc] = view.get<TransformComponent, BouyancyFieldComponent>(entity);
				auto& translation = transform.Translation;
				auto& scale = transform.Scale;

				PxBounds3 fieldBounds = PxBounds3(
					PxVec3(-1.0f * scale.x + translation.x, -1.0f * scale.y + translation.y, -1.0f * scale.z + translation.z),
					PxVec3(1.0f * scale.x + translation.x, 1.0f * scale.y + translation.y, 1.0f * scale.z + translation.z)
				);

				auto view = m_Registry.view<RigidbodyComponent>();
				for (auto entity : view)
				{
					auto rbc = view.get<RigidbodyComponent>(entity);

					if (rbc.Type == RigidbodyComponent::BodyType::Dynamic)
					{
						auto* actor = (PxRigidDynamic*)rbc.RuntimeBody;
						auto& bounds = actor->getWorldBounds();
						if (bounds.intersects(fieldBounds))
						{
							PxBounds3 intersection;
							intersection.minimum.x = std::max(bounds.minimum.x, fieldBounds.minimum.x);
							intersection.minimum.y = std::max(bounds.minimum.y, fieldBounds.minimum.y);
							intersection.minimum.z = std::max(bounds.minimum.z, fieldBounds.minimum.z);
							intersection.maximum.x = std::min(bounds.maximum.x, fieldBounds.maximum.x);
							intersection.maximum.y = std::min(bounds.maximum.y, fieldBounds.maximum.y);
							intersection.maximum.z = std::min(bounds.maximum.z, fieldBounds.maximum.z);
							auto& boundsDims = bounds.getDimensions();
							auto& intersectionDims = intersection.getDimensions();
							float percentage = abs(intersectionDims.x * intersectionDims.y * intersectionDims.z) / abs(boundsDims.x * boundsDims.y * boundsDims.z);

							const float& mass = actor->getMass();
							const float& density = rbc.Density;
							const float volume = mass / density;

							float bouyancy = (volume * percentage) * bfc.FluidDensity * -m_PhysXScene->getGravity().y;

							actor->setLinearDamping(bfc.LinearDamping);
							actor->setAngularDamping(bfc.AngularDamping);

							actor->addForce(PxVec3(0.0f, bouyancy, 0.0f), PxForceMode::eFORCE);
						}
					}
				}
			}
		}
	}

	void Scene::OnPhysicsStop()
	{
		// Destroy PhysX World
		m_PhysXScene->release();
		m_PhysXScene = nullptr;

		// Clean up controllers
		m_PhysXControllerManager->purgeControllers();

		// Clean up Vehicles
		{
			m_PhysXVehicleManager->shutdown();
			delete m_PhysXVehicleManager;

			auto view = m_Registry.view<VehicleMovementComponent>();
			for (auto e : view)
			{
				Entity entity = { e, this };

				auto& vmc = entity.GetComponent<VehicleMovementComponent>();
				delete vmc.VehicleController;
			}
		}
	}

	static void DrawDebugSphere(glm::vec3 position, float radius, glm::vec4 color = glm::vec4(1.0f))
	{
		const size_t ittertions = 30;
		const float angle = 360.0f / ittertions;
		glm::vec3 positions[ittertions];

		for (size_t i = 0; i < ittertions; i++)
			positions[i] = (glm::vec3(radius * glm::cos(glm::radians(i * angle)), radius * glm::sin(glm::radians(i * angle)), 0.0f));

		for (size_t j = 0; j < 3; j++)
		{
			for (size_t i = 0; i < ittertions - 1; i++)
				Renderer2D::DrawLine(position + positions[i], position + positions[i + 1], color);
			Renderer2D::DrawLine(position + positions[ittertions - 1], position + positions[0], color);

			if (j == 0)
				for (auto& position : positions)
					std::swap(position.x, position.z);
			if (j == 1)
				for (auto& position : positions)
					std::swap(position.y, position.x);
		}
	}

	void Scene::RenderSceneEditor(Timestep ts, EditorCamera& camera)
	{
		SceneRenderer::BeginScene(camera);

		// Submit Lights
		{
			auto view = m_Registry.view<TransformComponent, DirectionalLightComponent>();
			for (auto entity : view)
			{
				auto [transform, light] = view.get<TransformComponent, DirectionalLightComponent>(entity);
				//Renderer3D::SubmitDirectionalLight(transform.GetTransform(), light);
				SceneRenderer::SubmitDirectionalLight(GetWorldTransform({ entity, this }), light);
			}
		}
		{
			auto view = m_Registry.view<TransformComponent, PointLightComponent>();
			for (auto entity : view)
			{
				auto [transform, light] = view.get<TransformComponent, PointLightComponent>(entity);
				//Renderer3D::SubmitPointLight(transform.GetTransform(), light);
				SceneRenderer::SubmitPointLight(GetWorldTransform({ entity, this }), light);
			}
		}
		{
			auto view = m_Registry.view<TransformComponent, SpotLightComponent>();
			for (auto entity : view)
			{
				auto [transform, light] = view.get<TransformComponent, SpotLightComponent>(entity);
				//Renderer3D::SubmitSpotLight(transform.GetTransform(), light);
				SceneRenderer::SubmitSpotLight(GetWorldTransform({ entity, this }), light);
			}
		}
		SceneRenderer::SubmitLightSetup();

		{
			auto view = m_Registry.view<TransformComponent, SkyLightComponent>();
			for (auto entity : view)
			{
				auto [transform, light] = view.get<TransformComponent, SkyLightComponent>(entity);
				SceneRenderer::SubmitSkyLight(light);
			}
		}

		// Submit static meshes
		{
			auto view = m_Registry.view<TransformComponent, StaticMeshComponent>();
			for (auto entity : view)
			{
				auto [transform, mesh] = view.get<TransformComponent, StaticMeshComponent>(entity);
				mesh.Update(ts);
				//Renderer3D::SubmitStaticMesh(transform.GetTransform(), mesh, (int)entity);
				SceneRenderer::SubmitStaticMesh(GetWorldTransform({ entity, this }), mesh, (int)entity, IsEntitySelected(entity));
			}
		}

		// Submit lighting volumes
		{
			auto view = m_Registry.view<TransformComponent, VolumeComponent>();
			for (auto entity : view)
			{
				auto [transform, volume] = view.get<TransformComponent, VolumeComponent>(entity);
				SceneRenderer::SubmitVolume(transform, volume);
			}
		}

		// Draw 3D Icons
		{
			{
				auto view = m_Registry.view<TransformComponent, CameraComponent>();
				for (auto entity : view)
				{
					auto [transform, camera] = view.get<TransformComponent, CameraComponent>(entity);

					SceneRenderer::SubmitModel(GetWorldTransform({ entity, this }), s_CameraModel, (int)entity, IsEntitySelected(entity));
				}
			}

			{
				auto view = m_Registry.view<TransformComponent, DirectionalLightComponent>();
				for (auto entity : view)
				{
					auto [dlc, camera] = view.get<TransformComponent, DirectionalLightComponent>(entity);
					SceneRenderer::SubmitModel(GetWorldTransform({ entity, this }), s_ArrowModel, (int)entity, IsEntitySelected(entity));
				}
			}
		}

		SceneRenderer::RenderScene();

		SceneRenderer::EndScene();

		Renderer2D::BeginScene(camera);

		// Ground Plane
		Renderer2D::SetLineWidth(3.0f);
		for (int i = -10; i <= 10; i++)
			Renderer2D::DrawLine(glm::vec3(i, 0.0f, -10.0f), glm::vec3(i, 0.0f, 10.0f), glm::vec4(0.75f));
		for (int i = -10; i <= 10; i++)
			Renderer2D::DrawLine(glm::vec3(-10.0f, 0.0f, i), glm::vec3(10.0f, 0.0f, i), glm::vec4(0.75f));

		// Render Frustum For Camera
		{
			auto view = m_Registry.view<TransformComponent, CameraComponent>();
			for (auto entity : view)
			{
				if (!IsEntitySelected(entity))
					continue;

				auto [transform, camera] = view.get<TransformComponent, CameraComponent>(entity);

				std::array<glm::vec3, 8> _cameraFrustumCornerVertices{
					{
						{ -1.0f, -1.0f, 1.0f }, { 1.0f, -1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { -1.0f, 1.0f, 1.0f },
						{ -1.0f, -1.0f, -1.0f }, { 1.0f, -1.0f, -1.0f }, { 1.0f, 1.0f, -1.0f }, { -1.0f, 1.0f, -1.0f },
					}
				};

				const auto proj = glm::inverse(camera.Camera.GetProjection());
				const auto trans = GetWorldTransform({ entity, this });
				std::array<glm::vec3, 8> v;

				std::transform(
					_cameraFrustumCornerVertices.begin(),
					_cameraFrustumCornerVertices.end(),
					v.begin(),
					[&](glm::vec3 p) {
						auto v = trans * proj * glm::vec4(p, 1.0f);
						return glm::vec3(v) / v.w;
					}
				);

				const glm::vec4 color = glm::vec4(0.82f, 0.62f, 0.13f, 1.0f);
				Renderer2D::DrawLine(glm::vec3(v[0]), glm::vec3(v[1]), color);
				Renderer2D::DrawLine(glm::vec3(v[1]), glm::vec3(v[2]), color);
				Renderer2D::DrawLine(glm::vec3(v[2]), glm::vec3(v[3]), color);
				Renderer2D::DrawLine(glm::vec3(v[3]), glm::vec3(v[0]), color);

				Renderer2D::DrawLine(glm::vec3(v[4]), glm::vec3(v[5]), color);
				Renderer2D::DrawLine(glm::vec3(v[5]), glm::vec3(v[6]), color);
				Renderer2D::DrawLine(glm::vec3(v[6]), glm::vec3(v[7]), color);
				Renderer2D::DrawLine(glm::vec3(v[7]), glm::vec3(v[4]), color);

				Renderer2D::DrawLine(glm::vec3(v[0]), glm::vec3(v[4]), color);
				Renderer2D::DrawLine(glm::vec3(v[1]), glm::vec3(v[5]), color);
				Renderer2D::DrawLine(glm::vec3(v[3]), glm::vec3(v[7]), color);
				Renderer2D::DrawLine(glm::vec3(v[2]), glm::vec3(v[6]), color);
			}
		}

		// Draw volume bounding box
		{
			auto view = m_Registry.view<TransformComponent, VolumeComponent>();
			for (auto entity : view)
			{
				if (IsEntitySelected(entity))
				{
					auto [transform, volume] = view.get<TransformComponent, VolumeComponent>(entity);
					Renderer2D::DrawCube(transform.Translation, transform.Scale, glm::vec4(0.2f, 0.1f, 0.9f, 1.0f), int(entity));
				}
			}
		}

		// Draw Audio Debug Sphere
		{
			auto view = m_Registry.view<TransformComponent, AudioComponent>();
			for (auto entity : view)
			{
				if (IsEntitySelected(entity))
				{
					auto [transform, ac] = view.get<TransformComponent, AudioComponent>(entity);
					if (ac.AudioSound)
					{
						float distance = glm::distance(transform.Translation, camera.GetPosition());
						auto transformation = glm::translate(glm::mat4(1.0f), glm::vec3(GetWorldTransform({ entity, this })[3])) * glm::rotate(glm::mat4(1.0f), camera.GetYaw(), glm::vec3{ 0.0f, -1.0f, 0.0f })
							* glm::rotate(glm::mat4(1.0f), camera.GetPitch(), glm::vec3{ -1.0f, 0.0f, 0.0f }) * glm::scale(glm::mat4(1.0f), glm::vec3(distance < 5.0f ? distance / 5.0f : 1.0f));

						Renderer2D::DrawQuad(transformation, m_SoundIcon, 1.0f, glm::vec4(1.0f), (int)entity);
						DrawDebugSphere(transformation[3], ac.AudioSound->GetRadius());
					}
				}
			}
		}

		// Draw Icons
		{
			auto view = m_Registry.view<TransformComponent, PointLightComponent>();
			for (auto entity : view)
			{
				auto [tc, light] = view.get<TransformComponent, PointLightComponent>(entity);

				float distance = glm::distance(tc.Translation, camera.GetPosition());
				auto transformation = glm::translate(glm::mat4(1.0f), glm::vec3(GetWorldTransform({ entity, this })[3])) * glm::rotate(glm::mat4(1.0f), camera.GetYaw(), glm::vec3{ 0.0f, -1.0f, 0.0f })
					* glm::rotate(glm::mat4(1.0f), camera.GetPitch(), glm::vec3{ -1.0f, 0.0f, 0.0f }) * glm::scale(glm::mat4(1.0f), glm::vec3(distance < 5.0f ? distance / 5.0f : 1.0f));

				Renderer2D::DrawQuad(transformation, m_LightIcon, 1.0f, glm::vec4(light.Color, 1.0f), (int)entity);

				if (IsEntitySelected(entity))
					DrawDebugSphere(transformation[3], light.Radius, glm::vec4(light.Color, 1.0f));
			}
		}

		// Draw sprites
		{
			auto group = m_Registry.group<TransformComponent>(entt::get<SpriteRendererComponent>);
			for (auto entity : group)
			{
				auto [transform, sprite] = group.get<TransformComponent, SpriteRendererComponent>(entity);

				//Renderer2D::DrawSprite(transform.GetTransform(), sprite, (int)entity);
				Renderer2D::DrawSprite(GetWorldTransform({ entity, this }), sprite, (int)entity);
			}
		}

		// Draw circles
		{
			auto view = m_Registry.view<TransformComponent, CircleRendererComponent>();
			for (auto entity : view)
			{
				auto [transform, circle] = view.get<TransformComponent, CircleRendererComponent>(entity);
				//Renderer2D::DrawCircle(transform.GetTransform(), circle.Color, circle.Thickness, circle.Fade, (int)entity);
				Renderer2D::DrawCircle(GetWorldTransform({ entity, this }), circle.Color, circle.Thickness, circle.Fade, (int)entity);
			}
		}

		// Draw Text
		{
			auto view = m_Registry.view<TransformComponent, TextComponent>();
			for (auto entity : view)
			{
				auto [transform, text] = view.get<TransformComponent, TextComponent>(entity);
				Renderer2D::DrawTextComponent(GetWorldTransform({ entity, this }), text, (int)entity);
			}
		}

		// Draw Particles
		{
			auto view = m_Registry.view<TransformComponent, ParticleSystemComponent>();
			for (auto entity : view)
			{
				auto [transform, ps] = view.get<TransformComponent, ParticleSystemComponent>(entity);

				//ps.Position = transform.Translation + ps.Offset;
				ps.Position = glm::vec3(GetWorldTransform({ entity, this })[3]) + ps.Offset;

				ps.OnUpdate(ts);
				ps.Emit();
				for (auto& particle : ps.GetParticlePool())
				{
					if (!particle.Active)
						continue;

					glm::vec4 color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

					float life = particle.LifeRemaining / particle.LifeTime;
					if (ps.ColorMethod == 0) {
						// Fade away particles
						color = glm::lerp(particle.ColorEnd, particle.ColorBegin, life);
					}
					else if (ps.ColorMethod == 1)
					{
						color = particle.ColorConstant;
					}
					else if (ps.ColorMethod == 2) {
						float lifePercentage = 1.0f - life;
						if (!particle.ColorPoints.empty())
						{
							if (lifePercentage < particle.ColorPoints[0].point) { color = particle.ColorPoints[0].color; }
							else if (lifePercentage > particle.ColorPoints[particle.ColorPoints.size() - 1].point) { color = particle.ColorPoints[particle.ColorPoints.size() - 1].color; }
							else
							{
								for (int i = 0; i < particle.ColorPoints.size() - 1; i++)
								{
									if (lifePercentage >= particle.ColorPoints[i].point && lifePercentage < particle.ColorPoints[i + 1].point)
									{
										float percentage = glm::abs(glm::abs(lifePercentage - particle.ColorPoints[i].point) / glm::abs(particle.ColorPoints[i].point - particle.ColorPoints[i + 1].point));
										color = glm::lerp(particle.ColorPoints[i].color, particle.ColorPoints[i + 1].color, percentage);
										break;
									}
								}
							}
						}
					}

					float size = glm::lerp(particle.SizeEnd, particle.SizeBegin, life);

					glm::vec3 position = { particle.Position.x, particle.Position.y, particle.Position.z };

					glm::mat4 transformation = glm::translate(glm::mat4(1.0f), position)
						* glm::rotate(glm::mat4(1.0f), glm::radians(particle.Rotation), glm::vec3{ 0.0f, 0.0f, 1.0f })
						* glm::scale(glm::mat4(1.0f), { size, size, size });

					if (ps.FaceCamera) {
						transformation = transformation * glm::rotate(glm::mat4(1.0f), camera.GetYaw(), glm::vec3{ 0.0f, -1.0f, 0.0f })
							* glm::rotate(glm::mat4(1.0f), camera.GetPitch(), glm::vec3{ -1.0f, 0.0f, 0.0f });
					}

					//Renderer2D::DrawQuad(transformation, color, (int)entity);
					Renderer2D::DrawCircle(transformation, color, 1.0f, 0.005f, (int)entity);
				}
			}
		}

		Renderer2D::EndScene();
	}

	template<typename T>
	void Scene::OnComponentAdded(Entity entity, T& component)
	{
		static_assert(sizeof(T) == 0);
	}

	template<>
	void Scene::OnComponentAdded<IDComponent>(Entity entity, IDComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<FolderComponent>(Entity entity, FolderComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<TransformComponent>(Entity entity, TransformComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<CameraComponent>(Entity entity, CameraComponent& component)
	{
		if (m_ViewportWidth > 0 && m_ViewportHeight > 0)
			component.Camera.SetViewportSize(m_ViewportWidth, m_ViewportHeight);
	}

	template<>
	void Scene::OnComponentAdded<ScriptComponent>(Entity entity, ScriptComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<NativeScriptComponent>(Entity entity, NativeScriptComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<SpriteRendererComponent>(Entity entity, SpriteRendererComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<CircleRendererComponent>(Entity entity, CircleRendererComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<TextComponent>(Entity entity, TextComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<ParticleSystemComponent>(Entity entity, ParticleSystemComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<TagComponent>(Entity entity, TagComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<Rigidbody2DComponent>(Entity entity, Rigidbody2DComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<BoxCollider2DComponent>(Entity entity, BoxCollider2DComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<CircleCollider2DComponent>(Entity entity, CircleCollider2DComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<StaticMeshComponent>(Entity entity, StaticMeshComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<DirectionalLightComponent>(Entity entity, DirectionalLightComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<PointLightComponent>(Entity entity, PointLightComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<SpotLightComponent>(Entity entity, SpotLightComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<SkyLightComponent>(Entity entity, SkyLightComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<VolumeComponent>(Entity entity, VolumeComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<AudioComponent>(Entity entity, AudioComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<RigidbodyComponent>(Entity entity, RigidbodyComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<CharacterMovementComponent>(Entity entity, CharacterMovementComponent& component)
	{
	}
	
	template<>
	void Scene::OnComponentAdded<VehicleMovementComponent>(Entity entity, VehicleMovementComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<DirectionalFieldComponent>(Entity entity, DirectionalFieldComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<RadialFieldComponent>(Entity entity, RadialFieldComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<BouyancyFieldComponent>(Entity entity, BouyancyFieldComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<BoxColliderComponent>(Entity entity, BoxColliderComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<SphereColliderComponent>(Entity entity, SphereColliderComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<CapsuleColliderComponent>(Entity entity, CapsuleColliderComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<MeshColliderComponent>(Entity entity, MeshColliderComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<UICanvasComponent>(Entity entity, UICanvasComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<UIImageComponent>(Entity entity, UIImageComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<UIButtonComponent>(Entity entity, UIButtonComponent& component)
	{
	}
}