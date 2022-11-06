#include "dypch.h"
#include "Scene.h"

#include "Components.h"
#include "ScriptableEntity.h"
#include "ScriptEngine.h"
#include "Dymatic/Renderer/Renderer2D.h"
#include "Dymatic/Renderer/Renderer3D.h"
#include "Dymatic/Audio/AudioEngine.h"

#include <glm/glm.hpp>

#include "Entity.h"

// Box2D
#include "box2d/b2_world.h"
#include "box2d/b2_body.h"
#include "box2d/b2_fixture.h"
#include "box2d/b2_polygon_shape.h"
#include "box2d/b2_circle_shape.h"

// PhysX
#ifdef DY_RELEASE
#define NDEBUG
#endif
#include <PxPhysics.h>
#include <PxPhysicsAPI.h>

#include "Dymatic/Math/Math.h"

namespace Dymatic {

	extern physx::PxPhysics* g_PhysicsEngine;
	extern physx::PxCooking* g_PhysicsCooking;

	static Ref<Model> s_ArrowModel = nullptr;
	static Ref<Model> s_CameraModel = nullptr;

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

		if (s_ArrowModel == nullptr)
			s_ArrowModel = Model::Create("assets/objects/arrow/arrow.fbx");
		if (s_CameraModel == nullptr)
			s_CameraModel = Model::Create("assets/objects/camera/camera.fbx");
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

	Entity Scene::CreateEntityWithUUID(UUID uuid, const std::string& name)
	{
		Entity entity = { m_Registry.create(), this };
		entity.AddComponent<IDComponent>(uuid);
		entity.AddComponent<TransformComponent>();
		auto& tag = entity.AddComponent<TagComponent>();
		tag.Tag = name.empty() ? "Entity" : name;
		return entity;
	}

	void Scene::DestroyEntity(Entity entity)
	{
		m_Registry.destroy(entity);
	}

	void Scene::OnRuntimeStart()
	{
		s_ActiveScenes[m_SceneID] = this;
		OnPhysics2DStart();
		OnPhysicsStart();
	}

	void Scene::OnRuntimeStop()
	{
		s_ActiveScenes.erase(m_SceneID);
		OnPhysics2DStop();
		OnPhysicsStop();
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

	void Scene::OnUpdateRuntime(Timestep ts, bool paused)
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

		if (!paused)
		{
			// Update scripts
			{
				m_Registry.view<NativeScriptComponent>().each([=](auto entity, auto& nsc)
					{
						// TODO: Move to Scene::OnScenePlay
						ScriptEngine::BindScript(nsc);
						if (nsc.InstantiateScript)
						{
							if (!nsc.Instance)
							{
								nsc.Instance = nsc.InstantiateScript();
								ScriptEngine::InstantiateScript(nsc);
								nsc.Instance->m_Entity = Entity{ entity, this };
								nsc.Instance->OnCreate();
							}

							nsc.Instance->OnUpdate(ts);
						}
					});
			}

			// Update physics - Box2D
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

			// Update Physics - PhysX
			{
				m_PhysXScene->simulate(ts);
				m_PhysXScene->fetchResults(true);

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

			// Update Audio
			if (mainCamera)
			{
				auto view = m_Registry.view<TransformComponent, AudioComponent>();
				for (auto entity : view)
				{
					auto [transform, ac] = view.get<TransformComponent, AudioComponent>(entity);
					if (ac.GetSound())
					{
						if (!ac.GetSound()->IsInitalizedInternal())
							ac.GetSound()->OnBeginSceneInternal();

						ac.GetSound()->SetPositionInternal(GetWorldTransform({ entity, this })[3]);
					}
				}
				AudioEngine::Update(cameraTransform[3], cameraTransform * glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
			}
		}

		// Renderer
		if (mainCamera)
		{
			Renderer3D::BeginScene(*mainCamera, cameraTransform);
			Renderer3D::UpdateTimestep(ts);

			// Submit Lights
			{
				auto view = m_Registry.view<TransformComponent, DirectionalLightComponent>();
				for (auto entity : view)
				{
					auto [transform, light] = view.get<TransformComponent, DirectionalLightComponent>(entity);
					//Renderer3D::SubmitDirectionalLight(transform.GetTransform(), light);
					Renderer3D::SubmitDirectionalLight(GetWorldTransform({ entity, this }), light);
				}
			}
			{
				auto view = m_Registry.view<TransformComponent, PointLightComponent>();
				for (auto entity : view)
				{
					auto [transform, light] = view.get<TransformComponent, PointLightComponent>(entity);
					//Renderer3D::SubmitPointLight(transform.GetTransform(), light);
					Renderer3D::SubmitPointLight(GetWorldTransform({ entity, this }), light);
				}
			}
			{
				auto view = m_Registry.view<TransformComponent, SpotLightComponent>();
				for (auto entity : view)
				{
					auto [transform, light] = view.get<TransformComponent, SpotLightComponent>(entity);
					//Renderer3D::SubmitSpotLight(transform.GetTransform(), light);
					Renderer3D::SubmitSpotLight(GetWorldTransform({ entity, this }), light);
				}
			}
			Renderer3D::SubmitLightSetup();

			{
				auto view = m_Registry.view<TransformComponent, SkyLightComponent>();
				for (auto entity : view)
				{
					auto [transform, light] = view.get<TransformComponent, SkyLightComponent>(entity);
					Renderer3D::SubmitSkyLight(light);
				}
			}

			// Draw static meshes
			{
				auto view = m_Registry.view<TransformComponent, StaticMeshComponent>();
				for (auto entity : view)
				{
					auto [transform, mesh] = view.get<TransformComponent, StaticMeshComponent>(entity);
					mesh.Update(ts);
					Renderer3D::SubmitStaticMesh(GetWorldTransform({ entity, this }), mesh, (int)entity);
					//Renderer3D::SubmitStaticMesh(transform.GetTransform(), mesh, (int)entity);
				}
			}

			Renderer3D::RenderScene();

			Renderer3D::EndScene();

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

	void Scene::OnUpdateSimulation(Timestep ts, EditorCamera& camera, Entity selectedEntity, bool paused)
	{
		if (!paused)
		{
			// Update physics - Box2D
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

			// Update Physics - PhysX
			{
				m_PhysXScene->simulate(ts);
				m_PhysXScene->fetchResults(true);

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
		}

		// Render
		RenderScene(ts, camera, selectedEntity);
	}

	void Scene::OnUpdateEditor(Timestep ts, EditorCamera& camera, Entity selectedEntity)
	{
		// Render
		RenderScene(ts, camera, selectedEntity);
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

	void Scene::DuplicateEntity(Entity entity)
	{
		Entity newEntity = CreateEntity(entity.GetName());
		CopyComponentIfExists(AllComponents{}, newEntity, entity);
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

	void Scene::OnPhysics2DStop()
	{
		// Destroy box2d world
		delete m_Box2DWorld;
		m_Box2DWorld = nullptr;
	}

	void Scene::OnPhysicsStart()
	{
		// Setup PhysX
		{
			physx::PxDefaultCpuDispatcher* mDispatcher = nullptr;

			physx::PxSceneDesc sceneDesc(g_PhysicsEngine->getTolerancesScale());
			sceneDesc.gravity = physx::PxVec3(0.0f, -9.81f, 0.0f);
			mDispatcher = physx::PxDefaultCpuDispatcherCreate(0);
			sceneDesc.cpuDispatcher = mDispatcher;
			sceneDesc.filterShader = physx::PxDefaultSimulationFilterShader;
			m_PhysXScene = g_PhysicsEngine->createScene(sceneDesc);

			physx::PxPvdSceneClient* pvdClient = m_PhysXScene->getScenePvdClient();
			if (pvdClient)
			{
				pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
				pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
				pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
			}

			auto mMaterial = g_PhysicsEngine->createMaterial(0.5f, 0.5f, 0.6f);

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
					auto shape = g_PhysicsEngine->createShape(physx::PxBoxGeometry(bcc.Size.x * transform.Scale.x, bcc.Size.y * transform.Scale.y, bcc.Size.z * transform.Scale.z), *mMaterial);
					body->attachShape(*shape);
					shape->release();
				}

				if (entity.HasComponent<SphereColliderComponent>())
				{
					auto& scc = entity.GetComponent<SphereColliderComponent>();
					float scale = std::max({ transform.Scale.x, transform.Scale.y, transform.Scale.z });
					auto shape = g_PhysicsEngine->createShape(physx::PxSphereGeometry(scc.Radius * scale), *mMaterial);
					body->attachShape(*shape);
					shape->release();
				}

				if (entity.HasComponent<CapsuleColliderComponent>())
				{
					auto& ccc = entity.GetComponent<CapsuleColliderComponent>();
					float scale = std::max(transform.Scale.y, transform.Scale.z);
					auto shape = g_PhysicsEngine->createShape(physx::PxCapsuleGeometry(ccc.Radius * transform.Scale.x, ccc.HalfHeight * scale), *mMaterial);
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
							for (auto& vertex : mesh->m_Verticies)
								verts.push_back({ vertex.Position.x, vertex.Position.y, vertex.Position.z });

							meshDesc.points.count = verts.size();
							meshDesc.points.stride = sizeof(physx::PxVec3);
							meshDesc.points.data = verts.data();

							meshDesc.triangles.count = mesh->m_Verticies.size() / 3;
							meshDesc.triangles.stride = 3 * sizeof(uint32_t);
							meshDesc.triangles.data = mesh->m_Indicies.data();

							physx::PxDefaultMemoryOutputStream writeBuffer;
							physx::PxTriangleMeshCookingResult::Enum result;
							bool status = g_PhysicsCooking->cookTriangleMesh(meshDesc, writeBuffer, &result);
							if (!status)
								DY_CORE_ASSERT(false, "Triangle Mesh cooking failure");

							physx::PxDefaultMemoryInputData readBuffer(writeBuffer.getData(), writeBuffer.getSize());
							auto mesh = g_PhysicsEngine->createTriangleMesh(readBuffer);
							auto shape = g_PhysicsEngine->createShape(physx::PxTriangleMeshGeometry(mesh), *mMaterial);

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
							for (auto& vertex : mesh->m_Verticies)
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
							auto shape = g_PhysicsEngine->createShape(physx::PxConvexMeshGeometry(mesh), *mMaterial);

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
				auto material = g_PhysicsEngine->createMaterial(rbc.StaticFriction, rbc.DynamicFriction, rbc.Restitution);

				// Setup Physics Material
				physx::PxShape** shapes = new physx::PxShape*[body->getNbShapes()];
				body->getShapes(shapes, body->getNbShapes());

				for (size_t i = 0; i < body->getNbShapes(); i++)
					shapes[i]->setMaterials(&material, 1);

				// Clean Up
				material->release();
				delete[] shapes;

				m_PhysXScene->addActor(*body);

				rbc.RuntimeBody = body;
			}
		}
	}

	void Scene::OnPhysicsStop()
	{
		// Destroy PhysX World
		m_PhysXScene->release();
		m_PhysXScene = nullptr;
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

	void Scene::RenderScene(Timestep ts, EditorCamera& camera, Entity selectedEntity)
	{
		Renderer3D::BeginScene(camera);
		Renderer3D::UpdateTimestep(ts);

		// TODO: Move to OnSceneStop()
		auto view = m_Registry.view<TransformComponent, AudioComponent>();
		for (auto entity : view)
		{
			auto [transform, ac] = view.get<TransformComponent, AudioComponent>(entity);
			if (ac.GetSound())
			{
				if (ac.GetSound()->IsInitalizedInternal())
					ac.GetSound()->OnEndSceneInternal();
			}
		}

		// Submit Lights
		{
			auto view = m_Registry.view<TransformComponent, DirectionalLightComponent>();
			for (auto entity : view)
			{
				auto [transform, light] = view.get<TransformComponent, DirectionalLightComponent>(entity);
				//Renderer3D::SubmitDirectionalLight(transform.GetTransform(), light);
				Renderer3D::SubmitDirectionalLight(GetWorldTransform({ entity, this }), light);
			}
		}
		{
			auto view = m_Registry.view<TransformComponent, PointLightComponent>();
			for (auto entity : view)
			{
				auto [transform, light] = view.get<TransformComponent, PointLightComponent>(entity);
				//Renderer3D::SubmitPointLight(transform.GetTransform(), light);
				Renderer3D::SubmitPointLight(GetWorldTransform({ entity, this }), light);
			}
		}
		{
			auto view = m_Registry.view<TransformComponent, SpotLightComponent>();
			for (auto entity : view)
			{
				auto [transform, light] = view.get<TransformComponent, SpotLightComponent>(entity);
				//Renderer3D::SubmitSpotLight(transform.GetTransform(), light);
				Renderer3D::SubmitSpotLight(GetWorldTransform({ entity, this }), light);
			}
		}
		Renderer3D::SubmitLightSetup();

		{
			auto view = m_Registry.view<TransformComponent, SkyLightComponent>();
			for (auto entity : view)
			{
				auto [transform, light] = view.get<TransformComponent, SkyLightComponent>(entity);
				Renderer3D::SubmitSkyLight(light);
			}
		}

		// Draw static meshes
		{
			auto view = m_Registry.view<TransformComponent, StaticMeshComponent>();
			for (auto entity : view)
			{
				auto [transform, mesh] = view.get<TransformComponent, StaticMeshComponent>(entity);
				mesh.Update(ts);
				//Renderer3D::SubmitStaticMesh(transform.GetTransform(), mesh, (int)entity);
				Renderer3D::SubmitStaticMesh(GetWorldTransform({ entity, this }), mesh, (int)entity);
			}
		}

		// Draw 3D Icons
		{
			{
				auto view = m_Registry.view<TransformComponent, CameraComponent>();
				for (auto entity : view)
				{
					auto [transform, camera] = view.get<TransformComponent, CameraComponent>(entity);

					Renderer3D::SubmitModel(GetWorldTransform({ entity, this }), s_CameraModel, (int)entity);
				}
			}

			{
				auto view = m_Registry.view<TransformComponent, DirectionalLightComponent>();
				for (auto entity : view)
				{
					auto [dlc, camera] = view.get<TransformComponent, DirectionalLightComponent>(entity);
					Renderer3D::SubmitModel(GetWorldTransform({ entity, this }), s_ArrowModel, (int)entity);
				}
			}
		}

		Renderer3D::RenderScene();

		Renderer3D::EndScene();

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
				if (entity != selectedEntity)
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

		// Draw Audio Debug Sphere
		{
			auto view = m_Registry.view<TransformComponent, AudioComponent>();
			for (auto entity : view)
			{
				auto [transform, ac] = view.get<TransformComponent, AudioComponent>(entity);

				float distance = glm::distance(transform.Translation, camera.GetPosition());
				auto transformation = glm::translate(glm::mat4(1.0f), glm::vec3(GetWorldTransform({ entity, this })[3])) * glm::rotate(glm::mat4(1.0f), camera.GetYaw(), glm::vec3{ 0.0f, -1.0f, 0.0f })
					* glm::rotate(glm::mat4(1.0f), camera.GetPitch(), glm::vec3{ -1.0f, 0.0f, 0.0f }) * glm::scale(glm::mat4(1.0f), glm::vec3(distance < 5.0f ? distance / 5.0f : 1.0f));

				Renderer2D::DrawQuad(transformation, m_SoundIcon, 1.0f, glm::vec4(1.0f), (int)entity);

				if (entity == selectedEntity)
					if (ac.GetSound())
						DrawDebugSphere(transformation[3], ac.GetSound()->GetRadius());
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

				if (entity == selectedEntity)
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
	void Scene::OnComponentAdded<SpriteRendererComponent>(Entity entity, SpriteRendererComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<CircleRendererComponent>(Entity entity, CircleRendererComponent& component)
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
	void Scene::OnComponentAdded<NativeScriptComponent>(Entity entity, NativeScriptComponent& component)
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
	void Scene::OnComponentAdded<AudioComponent>(Entity entity, AudioComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<RigidbodyComponent>(Entity entity, RigidbodyComponent& component)
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