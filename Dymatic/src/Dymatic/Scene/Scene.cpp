#include "dypch.h"
#include "Scene.h"

#include "Components.h"
#include "ScriptableEntity.h"
#include "Dymatic/Scripting/ScriptEngine.h"
#include "Dymatic/Renderer/Renderer2D.h"
#include "Dymatic/Renderer/SceneRenderer.h"
#include "Dymatic/Audio/AudioEngine.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

#include "Entity.h"
#include "EntityRegistryHelper.h"
#include "Dymatic/Scene/Prefab.h"

#include "Dymatic/Utils/PlatformUtils.h"

#include "Dymatic/Editor/Material/MaterialBuilder.h"

// Box2D
#include "box2d/b2_world.h"
#include "box2d/b2_body.h"
#include "box2d/b2_fixture.h"
#include "box2d/b2_polygon_shape.h"
#include "box2d/b2_circle_shape.h"

// Recast
#include <Recast.h>
#include <DetourNavMesh.h>
#include <DetourNavMeshBuilder.h>
#include <DetourNavMeshQuery.h>
#include "DetourNode.h"
#include "Dymatic/Core/Hash.h"

#include "Dymatic/Math/Math.h"

#include "Dymatic/Core/Input.h"

namespace Dymatic {

	// Editor Only
	static Ref<Model> s_CubeModel;
	static Ref<Model> s_SphereModel;
	static Ref<Model> s_CapsuleModel;
	static Ref<Model> s_ArrowModel;
	static Ref<MaterialAsset> s_ArrowMaterial;
	static Ref<Model> s_CameraModel;
	static Ref<MaterialAsset> s_CameraMaterial;
	static Ref<Texture2D> s_LightIcon;
	static Ref<Texture2D> s_SoundIcon;
	static Ref<Texture2D> s_ParticleSystemIcon;
	static Ref<Texture2D> s_DecalIcon;
	static Ref<Texture2D> s_FieldIcon;
	static Ref<MaterialAsset> s_NavMeshMaterial;

	static std::unordered_map<UUID, Scene*> s_ActiveScenes;

	namespace Utils {

		static b2BodyType DymaticRigidbody2DTypeToBox2D(RigidBody2DComponent::BodyType bodyType)
		{
			switch (bodyType)
			{
			case RigidBody2DComponent::BodyType::Static:    return b2_staticBody;
			case RigidBody2DComponent::BodyType::Dynamic:   return b2_dynamicBody;
			case RigidBody2DComponent::BodyType::Kinematic: return b2_kinematicBody;
			}
		}

	}

	void Scene::InitEditorResources()
	{
		using namespace Editor;

		s_CubeModel = Model::Create("Resources/Objects/Basic/Cube.fbx");
		s_SphereModel = Model::Create("Resources/Objects/Basic/Sphere.fbx");
		s_CapsuleModel = Model::Create("Resources/Objects/Basic/Capsule.fbx");

		s_ArrowModel = Model::Create("Resources/Objects/Arrow/Arrow.fbx");
		s_ArrowMaterial = MaterialSource::Create({ { MaterialAsset::MaterialRenderStage::Default, Shader::Create("Resources/Shaders/Scene_ArrowMaterial.glsl") } });
		s_CameraModel = Model::Create("Resources/Objects/Camera/Camera.fbx");
		s_CameraMaterial = MaterialSource::Create({ { MaterialAsset::MaterialRenderStage::Default, Shader::Create("Resources/Shaders/Scene_CameraMaterial.glsl") } }, { { 0, Texture2D::Create("Resources/Objects/Camera/CameraDiffuse.png") } });

		s_LightIcon = Texture2D::Create("Resources/Icons/Scene/LightIcon.png");
		s_SoundIcon = Texture2D::Create("Resources/Icons/Scene/SoundIcon.png");
		s_ParticleSystemIcon = Texture2D::Create("Resources/Icons/Scene/ParticleSystemIcon.png");
		s_DecalIcon = Texture2D::Create("Resources/Icons/Scene/DecalIcon.png");
		s_FieldIcon = Texture2D::Create("Resources/Icons/Scene/FieldIcon.png");

		MaterialBuilder builder;
		builder.Properties.AlphaBlendMode = MaterialAsset::Translucent;
		builder.Properties.CastShadows = false;
		builder.Properties.Lit = false;
		builder.Properties.TwoSided = true;

		builder.Results[MaterialResultPinType::Albedo] = MaterialBuilderNode::Create(MaterialNodeFunction::Constant, { glm::vec3(0.0f) });
		builder.Results[MaterialResultPinType::Emissive] = MaterialBuilderNode::Create(MaterialNodeFunction::Constant, { glm::vec3(0.514f, 0.824f, 0.220f) });
		builder.Results[MaterialResultPinType::Roughness] = MaterialBuilderNode::Create(MaterialNodeFunction::Constant, { 1.0f });
		builder.Results[MaterialResultPinType::Specular] = MaterialBuilderNode::Create(MaterialNodeFunction::Constant, { 0.0f });
		builder.Results[MaterialResultPinType::AmbientOcclusion] = MaterialBuilderNode::Create(MaterialNodeFunction::Constant, { 0.0f });
		builder.Results[MaterialResultPinType::Alpha] = MaterialBuilderNode::Create(MaterialNodeFunction::Constant, { 0.650f });
		builder.Results[MaterialResultPinType::IOR] = MaterialBuilderNode::Create(MaterialNodeFunction::Constant, { 1.0f });
		builder.Results[MaterialResultPinType::WorldDisplacement] = MaterialBuilderNode::Create(MaterialNodeFunction::Constant, { glm::vec3(0.0f, 0.2f, 0.0f) });
		s_NavMeshMaterial = builder.BuildMaterial();
	}

	Scene::Scene(Ref<Scene> other)
	{
		Copy(other.get(), this);
	}

	Scene::~Scene()
	{
		delete m_Box2DWorld;
	}

	Ref<Scene> Scene::Copy(Ref<Scene> other)
	{
		Ref<Scene> newScene = AssetManager::CreateMemoryOnlyAsset<Scene>();
		Copy(other, newScene);
		return newScene;
	}

	void Scene::Copy(Ref<Scene> source, Ref<Scene> target)
	{
		Copy(source.get(), target.get());
	}

	void Scene::Copy(Scene* source, Scene* target)
	{
		target->m_ViewportWidth = target->m_ViewportWidth;
		target->m_ViewportHeight = target->m_ViewportHeight;

		target->m_ShowColliders = source->m_ShowColliders;

		// Copy scene settings
		target->m_Gravity = source->m_Gravity;

		CopyEntityRegistry(source, target);
	}

	Entity Scene::Instantiate(Ref<Prefab> prefab)
	{
		DY_CORE_TRACE("Instantiating prefab with ID: '{}'", prefab->Handle);
		
		// Copy prefab contents to the target scene (tracking old/new entity IDs)
		EntityIDMap entityIDMap(prefab->GetEntityCount());
		Entity instance = CopyEntity(prefab->GetRootEntity(), &entityIDMap);

		// Setup instance root prefab component
		if (!instance.HasComponent<PrefabComponent>())
			instance.AddComponent<PrefabComponent>();

		instance.GetComponent<PrefabComponent>().PrefabID = prefab->Handle;

		// Revert all non-root prefab references
		const auto& prefabView = prefab->GetAllEntitiesWith<IDComponent, PrefabComponent>();
		for (const auto& e : prefabView)
		{
			const auto& id = prefab->GetRegistry().get<IDComponent>(e);
			const EntityHandle newEntityID = entityIDMap.at((EntityHandle)id.ID);
			Entity newEntity = GetEntityByUUID(newEntityID);

			if (newEntity == instance)
				continue;
			
			RevertPrefab(newEntity);
		}

		return instance;
	}

	void Scene::CopyToPrefab(Entity entity, Ref<Prefab> prefab)
	{
		DY_CORE_TRACE("Copying entity {} to prefab {}", entity.GetUUID(), prefab->Handle);
		prefab->UpdateRoot(entity);
	}

	void Scene::RevertPrefab(Entity entity)
	{
		if (!entity.HasComponent<PrefabComponent>())
			return;

		const AssetHandle prefabID = entity.GetComponent<PrefabComponent>().PrefabID;
		const Ref<Prefab> prefab = AssetManager::GetAsset<Prefab>(prefabID);
		if (!prefab)
			return;

		Entity newEntity = Instantiate(prefab);

		// Ensure transform is maintained for prefab root
		if (entity.HasComponent<TransformComponent>())
			newEntity.GetComponent<TransformComponent>().Transform = entity.GetComponent<TransformComponent>().Transform;

		Entity parent = entity.GetParent();
		DestroyEntity(entity);

		if (parent)
			ParentEntity(newEntity, parent);
	}

	class BuildContext : public rcContext
	{
	protected:
		virtual void doLog(const rcLogCategory category, const char* msg, const int len) override
		{
			const std::string message = fmt::format("[Navigation Build]: {}", fmt::string_view(msg, len));

			switch (category)
			{
			case RC_LOG_PROGRESS:	DY_CORE_TRACE(message);		break;
			case RC_LOG_WARNING:	DY_CORE_WARN(message);		break;
			case RC_LOG_ERROR:		DY_CORE_ERROR(message);		break;
			}
		}
	};

	enum class PartitionType
	{
		Watershed,
		Monotone,
		Layer
	};

	static void BuildNavigationVisualizationMesh(const dtNavMesh& mesh, dtPolyRef ref, std::vector<MeshVertex>& vertices, std::vector<uint32_t>& indices, std::unordered_map<glm::vec3, uint32_t>& vertexMap)
	{
		const dtMeshTile* tile = nullptr;
		const dtPoly* poly = nullptr;

		if (dtStatusFailed(mesh.getTileAndPolyByRef(ref, &tile, &poly)))
			return;

		const unsigned int ip = (unsigned int)(poly - tile->polys);

		if (poly->getType() == DT_POLYTYPE_OFFMESH_CONNECTION)
		{
			dtOffMeshConnection* con = &tile->offMeshCons[ip - tile->header->offMeshBase];

			const glm::vec3 start = glm::vec3(con->pos[0], con->pos[1], con->pos[2]);
			const glm::vec3 end = glm::vec3(con->pos[3], con->pos[4], con->pos[5]);

			// TODO: Draw a connection arc
		}
		else
		{
			const dtPolyDetail* pd = &tile->detailMeshes[ip];

			for (int i = 0; i < pd->triCount; i++)
			{
				const unsigned char* t = &tile->detailTris[(pd->triBase + i) * 4];
				std::array<glm::vec3, 3> triangleVertices;

				for (int j = 0; j < 3; j++)
				{
					glm::vec3 vertexPos;

					if (t[j] < poly->vertCount)
					{
						// Vertex is from the original polygon's vertices
						const float* v = &tile->verts[poly->verts[t[j]] * 3];
						vertexPos = glm::vec3(v[0], v[1], v[2]);
					}
					else
					{
						// Vertex is from the detail mesh's vertices
						const float* v = &tile->detailVerts[(pd->vertBase + t[j] - poly->vertCount) * 3];
						vertexPos = glm::vec3(v[0], v[1], v[2]);
					}

					if (vertexMap.find(vertexPos) == vertexMap.end())
					{
						uint32_t index = vertices.size();
						vertices.emplace_back(index, vertexPos, glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.5f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec4(0.0f));
						vertexMap[vertexPos] = index;
					}

					indices.push_back(vertexMap.at(vertexPos));
				}
			}
		}
	}

	static void BuildNavigationVisualizationMesh(const dtNavMesh& mesh, std::vector<MeshVertex>& vertices, std::vector<uint32_t>& indices)
	{
		std::unordered_map<glm::vec3, uint32_t> vertexMap;

		for (int i = 0; i < mesh.getMaxTiles(); i++)
		{
			const dtMeshTile* tile = mesh.getTile(i);
			
			if (!tile->header)
				continue;
			
			dtPolyRef base = mesh.getPolyRefBase(tile);

			for (int j = 0; j < tile->header->polyCount; j++)
			{
				const dtPoly* p = &tile->polys[j];
				BuildNavigationVisualizationMesh(mesh, base | (dtPolyRef)j, vertices, indices, vertexMap);
			}
		}
	}

	void Scene::OnRuntimeStart()
	{
		// TODO: Remove
		// TODO: Have global navigation context for scene inside `NavigationScene` class (like physics scene)
		if (false)
		{
			AABB bounds;

			// Accumulate all triangle meshes
			// TODO: This should be based purely on physics colliders
			std::vector<float> vertices;
			std::vector<int> triangles;

			auto meshView = m_Registry.view<TransformComponent, StaticMeshComponent>();
			for (auto& entity : meshView)
			{
				const auto& [tc, smc] = meshView.get<TransformComponent, StaticMeshComponent>(entity);

				const Ref<Model> model = smc.GetModel();
				if (!model)
					continue;

				const glm::mat4 transform = GetWorldTransformMatrix({ entity, this });

				for (const auto& mesh : model->GetMeshes())
				{
					triangles.reserve(triangles.size() + mesh->GetIndexCount());

					const auto& meshVertices = mesh->GetVerticies();
					const auto& meshIndices = mesh->GetIndicies();

					int nextIndex = 0;
					std::unordered_map<glm::vec3, int> indexMap;

					for (const auto& vertex : meshVertices)
					{
						if (indexMap.find(vertex.Position) != indexMap.end())
							continue;

						const glm::vec3 position = transform * glm::vec4(vertex.Position, 1.0f);
						vertices.emplace_back(position.x);
						vertices.emplace_back(position.y);
						vertices.emplace_back(position.z);

						bounds.Extend(position);

						indexMap[vertex.Position] = nextIndex++;
					}

					for (const auto index : meshIndices)
						triangles.emplace_back(indexMap.at(meshVertices.at(index).Position));
				}
			}

			const glm::vec2 size = bounds.GetSize();

			const PartitionType partitionType = PartitionType::Watershed;
			const float cellSize = 0.3f;
			const float cellHeight = 0.2f;
			const float agentHeight = 2.0f;
			const float agentRadius = 0.6f;
			const float agentMaxClimb = 0.9f;
			const float agentMaxSlope = 45.0f;
			const float regionMinSize = 8;
			const float edgeMaxLen = 12.0f;
			const float edgeMaxError = 1.3f;
			const float vertsPerPoly = 6.0f;
			const float detailSampleDist = 6.0f;
			const float detailSampleMaxError = 1.0f;
			const int minRegionSize = 8;
			const int regionMergeSize = 20;
			const int tileSize = 0.0f;
			const int borderSize = 0.0f;

			const bool filterLowHangingObstacles = true;
			const bool filterLedgeSpans = true;
			const bool filterWalkableLowHeightSpans = true;

			dtNavMeshQuery* navQuery = dtAllocNavMeshQuery();

			// Create a context
			BuildContext context;

			rcConfig config = { 0 };
			config.tileSize = tileSize;
			config.borderSize = borderSize;
			config.cs = cellSize;
			config.ch = cellHeight;
			config.bmin[0] = bounds.Min.x;
			config.bmin[1] = bounds.Min.y;
			config.bmin[2] = bounds.Min.z;
			config.bmax[0] = bounds.Max.x;
			config.bmax[1] = bounds.Max.y;
			config.bmax[2] = bounds.Max.z;
			config.walkableSlopeAngle = agentMaxSlope;
			config.walkableHeight= (int)ceilf(agentHeight / config.ch);
			config.walkableClimb = (int)floorf(agentMaxClimb / config.ch);
			config.walkableRadius = (int)ceilf(agentRadius / config.cs);
			config.maxEdgeLen = (int)(edgeMaxLen / cellSize);
			config.maxSimplificationError = edgeMaxError;
			config.minRegionArea = minRegionSize * minRegionSize;
			config.mergeRegionArea = regionMergeSize * regionMergeSize;
			config.maxVertsPerPoly = vertsPerPoly;
			config.detailSampleDist = detailSampleDist < 0.9f ? 0 : cellSize * detailSampleDist;
			config.detailSampleMaxError = cellHeight * detailSampleMaxError;

			rcCalcGridSize(config.bmin, config.bmax, config.cs, &config.width, &config.height);

			// Allocate the voxel heightfield serving as the target for input data rasterization
			rcHeightfield* solid = rcAllocHeightfield();
			DY_CORE_ASSERT(solid, "Recast out of memory for heightfield!");

			if (!rcCreateHeightfield(&context, *solid, config.width, config.height, config.bmin, config.bmax, config.cs, config.ch))
				DY_CORE_ASSERT(false, "Could not create solid heightfield!");

			const int numTriangles = triangles.size() / 3;
			std::vector<uint8_t> triangleAreas(numTriangles, 0);

			rcMarkWalkableTriangles(&context, config.walkableSlopeAngle, vertices.data(), vertices.size(), triangles.data(), numTriangles, triangleAreas.data());

			int total = 0;
			for (const auto& area : triangleAreas)
				if (area != 0)
					total++;

			DY_CORE_INFO("{} / {}", total, numTriangles);

			if (!rcRasterizeTriangles(&context, vertices.data(), vertices.size(), triangles.data(), triangleAreas.data(), numTriangles, *solid, config.walkableClimb))
				DY_CORE_ASSERT(false, "Could not rasterize triangles!");

			// Filter walkable surfaces
			if (filterLowHangingObstacles)
				rcFilterLowHangingWalkableObstacles(&context, config.walkableClimb, *solid);

			if (filterLedgeSpans)
				rcFilterLedgeSpans(&context, config.walkableHeight, config.walkableClimb, *solid);

			if (filterWalkableLowHeightSpans)
				rcFilterWalkableLowHeightSpans(&context, config.walkableHeight, *solid);

			// Partition walkable surfaces into simple regions

			// Compact the heightfield so it is faster to handle from now on
			rcCompactHeightfield* compactHeightfield = rcAllocCompactHeightfield();
			DY_CORE_ASSERT(compactHeightfield, "Recast out of memory for compact heightfield!");

			if (!rcBuildCompactHeightfield(&context, config.walkableHeight, config.walkableClimb, *solid, *compactHeightfield))
				DY_CORE_ASSERT(false, "Could not build compact data!");

			rcFreeHeightField(solid);
			solid = nullptr;

			// Erode the walkable area by the agent radius
			if (!rcErodeWalkableArea(&context, config.walkableRadius, *compactHeightfield))
				DY_CORE_ASSERT(false, "Could not erode compact heightfield!");

			// (Optional) Mark areas.
			// const ConvexVolume* vols = m_geom->getConvexVolumes();
			// for (int i = 0; i < m_geom->getConvexVolumeCount(); ++i)
			// 	rcMarkConvexPolyArea(m_ctx, vols[i].verts, vols[i].nverts, vols[i].hmin, vols[i].hmax, (unsigned char)vols[i].area, *compactHeightfield);

			// Partition the heightfield so that we can use simple algorithm later to triangulate the walkable areas.
			// 1) Watershed partitioning
			//   - the classic Recast partitioning
			//   - creates the nicest tessellation
			//   - usually slowest
			//   - partitions the heightfield into nice regions without holes or overlaps
			//   - the are some corner cases where this method creates produces holes and overlaps
			//      - holes may appear when a small obstacles is close to large open area (triangulation can handle this)
			//      - overlaps may occur if you have narrow spiral corridors (i.e stairs), this make triangulation to fail
			//   * generally the best choice if you precompute the navmesh, use this if you have large open areas
			// 2) Monotone partitioning
			//   - fastest
			//   - partitions the heightfield into regions without holes and overlaps (guaranteed)
			//   - creates long thin polygons, which sometimes causes paths with detours
			//   * use this if you want fast navmesh generation
			// 3) Layer partitioning
			//   - quite fast
			//   - partitions the heighfield into non-overlapping regions
			//   - relies on the triangulation code to cope with holes (thus slower than monotone partitioning)
			//   - produces better triangles than monotone partitioning
			//   - does not have the corner cases of watershed partitioning
			//   - can be slow and create a bit ugly tessellation (still better than monotone)
			//     if you have large open areas with small obstacles (not a problem if you use tiles)
			//   * good choice to use for tiled navmesh with medium and small sized tiles

			switch (partitionType)
			{
			case PartitionType::Watershed:
			{
				// Prepare for region partitioning, by calculating distance field along the walkable surface.
				if (!rcBuildDistanceField(&context, *compactHeightfield))
					DY_CORE_ASSERT(false, "Could not build distance field!");

				// Partition the walkable surface into simple regions without holes.
				if (!rcBuildRegions(&context, *compactHeightfield, 0, config.minRegionArea, config.mergeRegionArea))
					DY_CORE_ASSERT(false, "Could not build watershed regions!");

				break;
			}
			case PartitionType::Monotone:
			{
				// Partition the walkable surface into simple regions without holes.
				// Monotone partitioning does not need distance field.
				if (!rcBuildRegionsMonotone(&context, *compactHeightfield, 0, config.minRegionArea, config.mergeRegionArea))
					DY_CORE_ASSERT(false, "Could not build monotone regions!");

				break;
			}
			case PartitionType::Layer:
			{
				// Partition the walkable surface into simple regions without holes.
				if (!rcBuildLayerRegions(&context, *compactHeightfield, 0, config.minRegionArea))
					DY_CORE_ASSERT(false, "Could not build layer regions!");

				break;
			}
			default:
				DY_CORE_ASSERT(false, "Unknown navigation partitioning type!");
			}

			// Trace and simplify region contours
			rcContourSet* contourSet = rcAllocContourSet();
			DY_CORE_ASSERT(contourSet);

			if (!rcBuildContours(&context, *compactHeightfield, config.maxSimplificationError, config.maxEdgeLen, *contourSet))
				DY_CORE_ASSERT(false, "Could not create contour set!");

			// Build polygon navmesh from the contours
			rcPolyMesh* polymesh = rcAllocPolyMesh();
			DY_CORE_ASSERT(polymesh);

			if (!rcBuildPolyMesh(&context, *contourSet, config.maxVertsPerPoly, *polymesh))
				DY_CORE_ASSERT(false, "Could not triangulate contours!");

			// Create detail mesh which allows to access approximate height on each polygon
			rcPolyMeshDetail* polyMeshDetail = rcAllocPolyMeshDetail();
			DY_CORE_ASSERT(polyMeshDetail);

			if (!rcBuildPolyMeshDetail(&context, *polymesh, *compactHeightfield, config.detailSampleDist, config.detailSampleMaxError, *polyMeshDetail))
				DY_CORE_ASSERT(false, "Could not build detail mesh!");

			rcFreeCompactHeightfield(compactHeightfield);
			compactHeightfield = nullptr;
			rcFreeContourSet(contourSet);
			contourSet = nullptr;

			// Create Detour data from Recast poly mesh
			if (config.maxVertsPerPoly <= DT_VERTS_PER_POLYGON)
			{
				unsigned char* navData = 0;
				int navDataSize = 0;

				dtNavMeshCreateParams params = { 0 };
				params.verts = polymesh->verts;
				params.vertCount = polymesh->nverts;
				params.polys = polymesh->polys;
				params.polyAreas = polymesh->areas;
				params.polyFlags = polymesh->flags;
				params.polyCount = polymesh->npolys;
				params.nvp = polymesh->nvp;
				params.detailMeshes = polyMeshDetail->meshes;
				params.detailVerts = polyMeshDetail->verts;
				params.detailVertsCount = polyMeshDetail->nverts;
				params.detailTris = polyMeshDetail->tris;
				params.detailTriCount = polyMeshDetail->ntris;
				
				// TODO: Off-mesh connections
				// params.offMeshConVerts = nullptr;
				// params.offMeshConRad = nullptr;
				// params.offMeshConDir = nullptr;
				// params.offMeshConAreas = nullptr;
				// params.offMeshConFlags = nullptr;
				// params.offMeshConUserID = nullptr;
				// params.offMeshConCount = 0;

				params.walkableHeight = agentHeight;
				params.walkableRadius = agentRadius;
				params.walkableClimb = agentMaxClimb;
				rcVcopy(params.bmin, polymesh->bmin);
				rcVcopy(params.bmax, polymesh->bmax);
				params.cs = config.cs;
				params.ch = config.ch;
				params.buildBvTree = true;

				if (!dtCreateNavMeshData(&params, &navData, &navDataSize))
					DY_CORE_ASSERT(false, "Could not build Detour navmesh!");

				dtNavMesh* navMesh = dtAllocNavMesh();
				if (!navMesh)
				{
					dtFree(navData);
					DY_CORE_ASSERT(false, "Could not create Detour navmesh!");
				}

				dtStatus status;

				status = navMesh->init(navData, navDataSize, DT_TILE_FREE_DATA);
				if (dtStatusFailed(status))
				{
					dtFree(navData);
					DY_CORE_ASSERT(false, "Could not init Detour navmesh!");
				}

				status = navQuery->init(navMesh, 2048);
				if (dtStatusFailed(status))
					DY_CORE_ASSERT(false, "Could not init Detour navmesh query!");

				// Generate debug meshes
				auto view = m_Registry.view<NavigationMeshComponent>();
				for (auto entity : view)
				{
					std::vector<MeshVertex> vertices;
					std::vector<uint32_t> indices;
					BuildNavigationVisualizationMesh(*navMesh, vertices, indices);

					auto& nmc = view.get<NavigationMeshComponent>(entity);
					nmc.DebugMesh = Model::Create({ Mesh::Create(std::string(), vertices, indices, nullptr) });
				}

				dtFreeNavMesh(navMesh);
			}

			// Navigation TODO:
			//	- Navigation Filters
			//	- Dynamic Obsticals (using tile cache)
			//	- Crowd logic
			//	- Offmesh Links
			//	- Weights to colliders (given a physics material)
			//	- Add/subtract from mesh

			// TODO: Carefully check we are freeing resources here!!!

			dtFreeNavMeshQuery(navQuery);
		}

		m_IsRunning = true;

		s_ActiveScenes[m_RegistryID] = this;
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
		
		s_ActiveScenes.erase(m_RegistryID);
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

		// Halt audio playback
		auto audioView = m_Registry.view<AudioComponent>();
		for (auto entity : audioView)
		{
			auto& ac = audioView.get<AudioComponent>(entity);
			ac.AudioSound->Stop();
			ac.Initialized = false;
		}

		// Release renderer contexts for capture components
		auto captureView = m_Registry.view<CaptureComponent>();
		for (auto entity : captureView)
		{
			auto& cc = captureView.get<CaptureComponent>(entity);
			cc.RuntimeRendererContext = nullptr;
		}
	}

	void Scene::OnSimulationStart()
	{
		s_ActiveScenes[m_RegistryID] = this;
		OnPhysics2DStart();
		OnPhysicsStart();
	}

	void Scene::OnSimulationStop()
	{
		s_ActiveScenes.erase(m_RegistryID);
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
					cameraTransform = GetWorldTransformMatrix({ entity, this });
					break;
				}
			}
		}

		const bool updating = !m_IsPaused || m_StepFrames-- > 0;

		if (updating)
		{
			// C# Pre-Physics Update
			auto scriptView = m_Registry.view<ScriptComponent>();
			for (auto e : scriptView)
			{
				Entity entity = { e, this };
				ScriptEngine::OnPrePhysicsUpdateEntity(entity, ts);
			}

			// Update Physics 2D
			OnPhysics2DUpdate(ts);

			// Update Physics
			OnPhysicsUpdate(ts);

			// C# Script Post-Physics (Default) Update
			// Note: We re-generate the registry view as other update systems may have modified/added script entities
			scriptView = m_Registry.view<ScriptComponent>();
			for (auto e : scriptView)
			{
				Entity entity = { e, this };
				ScriptEngine::OnUpdateEntity(entity, ts);
			}

			// Deprecated C++ Native Scripts. TODO: Remove
			m_Registry.view<NativeScriptComponent>().each([=](auto entity, auto& nsc)
			{
				if (!nsc.Instance)
				{
					nsc.Instance = nsc.InstantiateScript();
					nsc.Instance->m_Entity = Entity{ entity, this };
					nsc.Instance->OnCreate();
				}

				nsc.Instance->OnUpdate(ts);
			});

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

						ac.AudioSound->SetPosition(GetWorldTransform({ entity, this }).Translation);
					}
				}
				AudioEngine::Update(cameraTransform[3], cameraTransform * glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
			}
		}

		// Renderer
		RenderSceneRuntime(updating ? ts : 0.0f, mainCamera, cameraTransform);
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
		else
			SceneRenderer::UpdateTimestep(0.0f);

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
			if (Entity{ entity, this }.HasComponent<CaptureComponent>())
				continue;

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

	void Scene::Step(int frames)
	{
		m_StepFrames = frames;
	}

	void Scene::SetEntityTransform(Entity entity, const Transform& transform)
	{
		SetEntityTranslation(entity, transform.Translation);
		SetEntityRotation(entity, transform.Rotation);
		SetEntityScale(entity, transform.Scale);
	}

	void Scene::SetEntityTranslation(Entity entity, const glm::vec3& translation)
	{
		TransformComponent& tc = entity.GetComponent<TransformComponent>();

		if (tc.Transform.Translation == translation)
			return;

		tc.Transform.Translation = translation;
		UpdateEntityTranslation(entity);
	}

	void Scene::SetEntityRotation(Entity entity, const glm::quat& rotation)
	{
		TransformComponent& tc = entity.GetComponent<TransformComponent>();

		if (tc.Transform.Rotation == rotation)
			return;

		tc.Transform.Rotation = rotation;
		UpdateEntityRotation(entity);
	}

	void Scene::SetEntityRotation(Entity entity, const glm::vec3& rotation)
	{
		TransformComponent& tc = entity.GetComponent<TransformComponent>();

		if (tc.Transform.GetRotationRadians() == rotation)
			return;

		tc.Transform.SetRotationRadians(rotation);
		UpdateEntityRotation(entity);
	}

	void Scene::SetEntityScale(Entity entity, const glm::vec3& scale)
	{
		TransformComponent& tc = entity.GetComponent<TransformComponent>();

		if (tc.Transform.Scale == scale)
			return;

		tc.Transform.Scale = scale;
		UpdateEntityScale(entity);
	}

	void Scene::UpdateEntityTranslation(Entity entity)
	{
		if (m_PhysicsScene)
			m_PhysicsScene->SetPositionInternal(entity, entity.GetComponent<TransformComponent>().Transform.Translation);
	}

	void Scene::UpdateEntityRotation(Entity entity)
	{
		if (m_PhysicsScene)
			m_PhysicsScene->SetRotationInternal(entity, entity.GetComponent<TransformComponent>().Transform.Rotation);
	}

	void Scene::UpdateEntityScale(Entity entity)
	{
	}

	void Scene::UpdateEntityScriptName(Entity entity)
	{
		ScriptEngine::OnDestroyEntity(entity);
		ScriptEngine::OnCreateEntity(entity);
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

	RaycastHit Scene::Raycast(const glm::vec3& origin, const glm::vec3& direction, float distance)
	{
		if (m_PhysicsScene)
			return m_PhysicsScene->Raycast(origin, direction, distance);

		return RaycastHit();
	}

	RaycastHit Scene::Raycast(const glm::vec3& start, const glm::vec3& end)
	{
		const glm::vec3 direction = glm::normalize(end - start);
		const float distance = glm::distance(start, end);
		return Raycast(start, direction, distance);
	}

	void Scene::DrawDebugLine(glm::vec3 start, glm::vec3 end, glm::vec4 color, float time)
	{
		m_DebugLines.push_back({ start, end, color, time });
	}

	void Scene::DrawDebugCube(glm::vec3 position, glm::vec3 size, glm::vec4 color, float time)
	{
		glm::mat4 transform = glm::scale(glm::translate(glm::mat4(1.0f), position), size);
		m_DebugCubes.push_back({ transform, color, time });
	}

	void Scene::DrawDebugSphere(glm::vec3 center, float radius, glm::vec4 color, float time)
	{
		glm::mat4 transform = glm::scale(glm::translate(glm::mat4(1.0f), center), glm::vec3(radius));
		m_DebugSpheres.push_back({ transform, color, time });
	}

	void Scene::ClearDebugDrawing()
	{
		m_DebugLines.clear();
		m_DebugCubes.clear();
		m_DebugSpheres.clear();
	}

	void Scene::OnPhysics2DStart()
	{
		// create box2d world
		// Create physics world and add bodies
		m_Box2DWorld = new b2World({ 0.0f, -9.8f });
		auto view = m_Registry.view<RigidBody2DComponent>();
		for (auto e : view)
		{
			Entity entity = { e, this };
			auto& transform = entity.GetComponent<TransformComponent>().Transform;
			auto& rb2d = entity.GetComponent<RigidBody2DComponent>();

			b2BodyDef bodyDef;
			bodyDef.type = Utils::DymaticRigidbody2DTypeToBox2D(rb2d.Type);
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
		auto view = m_Registry.view<RigidBody2DComponent>();
		for (auto e : view)
		{
			Entity entity = { e, this };
			auto& transform = entity.GetComponent<TransformComponent>().Transform;
			auto& rb2d = entity.GetComponent<RigidBody2DComponent>();
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

	void Scene::OnPhysicsStart()
	{
		m_PhysicsScene = PhysicsScene::Create(this);
	}

	void Scene::OnPhysicsUpdate(Timestep ts)
	{
		m_PhysicsScene->OnUpdate(ts);
	}

	void Scene::OnPhysicsStop()
	{
		m_PhysicsScene = nullptr;
	}

	void Scene::RenderSceneRuntime(Timestep ts, SceneCamera* mainCamera, const glm::mat4& cameraTransform)
	{
		if (!mainCamera)
			return;

		SceneRenderer::UpdateTimestep(ts);

		SceneRenderer::BeginScene();
		SceneRenderer::SubmitCamera(*mainCamera, cameraTransform);

		// Submit Lights
		{
			auto view = m_Registry.view<TransformComponent, DirectionalLightComponent>();
			for (auto entity : view)
			{
				auto& [transform, light] = view.get<TransformComponent, DirectionalLightComponent>(entity);
				//Renderer3D::SubmitDirectionalLight(transform.GetTransform(), light);
				SceneRenderer::SubmitDirectionalLight(GetWorldTransform({ entity, this }).GetRotationRadians(), light);
			}
		}
		{
			auto view = m_Registry.view<TransformComponent, PointLightComponent>();
			for (auto entity : view)
			{
				auto& [transform, light] = view.get<TransformComponent, PointLightComponent>(entity);
				//Renderer3D::SubmitPointLight(transform.GetTransform(), light);
				SceneRenderer::SubmitPointLight(GetWorldTransform({ entity, this }).Translation, light);
			}
		}
		{
			auto view = m_Registry.view<TransformComponent, SpotLightComponent>();
			for (auto entity : view)
			{
				auto& [transform, light] = view.get<TransformComponent, SpotLightComponent>(entity);
				//Renderer3D::SubmitSpotLight(transform.GetTransform(), light);
				SceneRenderer::SubmitSpotLight(GetWorldTransform({ entity, this }), light);
			}
		}

		SceneRenderer::SubmitLightSetup();

		{
			auto view = m_Registry.view<TransformComponent, SkyLightComponent>();
			for (auto entity : view)
			{
				auto& [transform, light] = view.get<TransformComponent, SkyLightComponent>(entity);
				SceneRenderer::SubmitSkyLight(light);
			}
		}

		// Draw static meshes
		{
			auto view = m_Registry.view<TransformComponent, StaticMeshComponent>();
			for (auto e : view)
			{
				const auto& [transform, smc] = view.get<TransformComponent, StaticMeshComponent>(e);

				if (!smc.m_Model)
					continue;

				Entity entity = { e, this };

				if (entity.HasComponent<SoftBodyComponent>())
				{
					const auto& sbc = entity.GetComponent<SoftBodyComponent>();

					if (sbc.RuntimeModel)
					{
						SceneRenderer::SubmitModel(GetWorldTransformMatrix(entity), sbc.RuntimeModel, smc.m_Materials, nullptr, nullptr, (int)e);
						continue;
					}
				}

				Ref<BoneMatrixList> pose = smc.m_AnimationGraphPlayer ? smc.m_AnimationGraphPlayer->GetFinalBoneMatrices() : nullptr;
				Ref<BlendShapeWeightList> blendShapeWeights = smc.m_AnimationGraphPlayer ? smc.m_AnimationGraphPlayer->GetBlendShapeWeights() : nullptr;

				if (entity.HasComponent<RagdollComponent>())
				{
					const auto& rdc = entity.GetComponent<RagdollComponent>();

					if (rdc.RuntimePose)
						pose = rdc.RuntimePose;
				}

				smc.Update(ts);
				SceneRenderer::SubmitModel(GetWorldTransformMatrix(entity), smc.m_Model, smc.m_Materials, pose, blendShapeWeights, (int)e);
			}
		}

		// Submit particle systems
		{
			auto view = m_Registry.view<TransformComponent, ParticleSystemComponent>();
			for (auto& entity : view)
			{
				auto [transform, ps] = view.get<TransformComponent, ParticleSystemComponent>(entity);
				SceneRenderer::SubmitParticleSystem(GetWorldTransformMatrix({ entity, this }), ps.Player, ps.Material, (int)entity);
			}
		}

		// Submit Decals
		{
			auto view = m_Registry.view<TransformComponent, DecalComponent>();
			for (auto entity : view)
			{
				auto& [transform, dc] = view.get<TransformComponent, DecalComponent>(entity);
				SceneRenderer::SubmitDecal(GetWorldTransformMatrix({ entity, this }), dc.Texture, dc.ConstrainAngle, (int)entity);
			}
		}

		// Submit lighting volumes
		{
			auto view = m_Registry.view<TransformComponent, VolumeComponent>();
			for (auto entity : view)
			{
				auto& [transform, volume] = view.get<TransformComponent, VolumeComponent>(entity);
				SceneRenderer::SubmitVolume(transform.Transform, volume, (int)entity);
			}
		}

		// Navigation Mesh Debug Visualization
		{
			auto view = m_Registry.view<TransformComponent, NavigationMeshComponent>();
			for (auto entity : view)
			{
				auto& [transform, nmc] = view.get<TransformComponent, NavigationMeshComponent>(entity);
				SceneRenderer::SubmitModel(glm::mat4(1.0f), nmc.DebugMesh, s_NavMeshMaterial, (int)entity);
			}
		}

		// Submit post process volumes
		{
			auto view = m_Registry.view<TransformComponent, PostProcessVolumeComponent>();
			for (auto entity : view)
			{
				auto& [transform, ppvc] = view.get<TransformComponent, PostProcessVolumeComponent>(entity);

				if (!ppvc.Enabled)
					continue;

				SceneRenderer::SubmitPostProcessVolume(GetWorldTransformMatrix({ entity, this }), ppvc, (int)entity);
			}
		}

		SceneRenderer::RenderScene();

		// Draw overlay elements
		if (m_ShowColliders)
		{
			{
				auto view = m_Registry.view<TransformComponent, BoxColliderComponent>();
				for (auto entity : view)
				{
					BoxColliderComponent& bcc = view.get<BoxColliderComponent>(entity);
					SceneRenderer::DrawMeshOutlineOverlay(GetWorldTransformMatrix({ entity, this }) * glm::scale(glm::mat4(1.0f), bcc.Size), s_CubeModel, glm::vec4(0.0f, 1.0f, 0.0f, 1.0f), (int)entity);
				}
			}

			{
				auto view = m_Registry.view<TransformComponent, SphereColliderComponent>();
				for (auto entity : view)
				{
					SphereColliderComponent& scc = view.get<SphereColliderComponent>(entity);
					SceneRenderer::DrawMeshOutlineOverlay(GetWorldTransformMatrix({ entity, this }) * glm::scale(glm::mat4(1.0f), glm::vec3(scc.Radius)), s_SphereModel, glm::vec4(0.0f, 1.0f, 0.0f, 1.0f), (int)entity);
				}
			}

			{
				auto view = m_Registry.view<TransformComponent, CapsuleColliderComponent>();
				for (auto entity : view)
				{
					CapsuleColliderComponent& ccc = view.get<CapsuleColliderComponent>(entity);
					SceneRenderer::DrawMeshOutlineOverlay(GetWorldTransformMatrix({ entity, this }) * glm::scale(glm::mat4(1.0f), glm::vec3(ccc.HalfHeight, ccc.Radius, ccc.Radius)), s_CapsuleModel, glm::vec4(0.0f, 1.0f, 0.0f, 1.0f), (int)entity);
				}
			}

			{
				auto view = m_Registry.view<TransformComponent, MeshColliderComponent, StaticMeshComponent>();
				for (auto entity : view)
				{
					StaticMeshComponent& smc = view.get<StaticMeshComponent>(entity);
					SceneRenderer::DrawMeshOutlineOverlay(GetWorldTransformMatrix({ entity, this }), smc.m_Model, glm::vec4(0.0f, 1.0f, 0.0f, 1.0f), (int)entity);
				}
			}
		}

		// Render Debug Objects
		{
			// Cubes
			for (uint32_t index = 0; index < m_DebugCubes.size(); index++)
			{
				DebugCube& cube = m_DebugCubes[index];

				if (cube.Time != 0.0f)
				{
					cube.Time -= ts;
					if (cube.Time < 0.0f)
					{
						m_DebugCubes.erase(m_DebugCubes.begin() + index);
						index--;
						continue;
					}
				}

				SceneRenderer::DrawMeshOutlineOverlay(cube.Transform, s_CubeModel, cube.Color);
			}

			// Spheres
			for (uint32_t index = 0; index < m_DebugSpheres.size(); index++)
			{
				DebugSphere& sphere = m_DebugSpheres[index];

				if (sphere.Time != 0.0f)
				{
					sphere.Time -= ts;
					if (sphere.Time < 0.0f)
					{
						m_DebugSpheres.erase(m_DebugSpheres.begin() + index);
						index--;
						continue;
					}
				}

				SceneRenderer::DrawMeshOutlineOverlay(sphere.Transform, s_SphereModel, sphere.Color);
			}
		}

		// Store active framebuffer
		Ref<SceneRendererContext> activeContext = SceneRenderer::GetActiveContext();

		// Render Scene Geometry to Capture Components
		auto captureView = m_Registry.view<TransformComponent, CameraComponent, CaptureComponent>();
		for (auto entity : captureView)
		{
			auto& [tc, camera, capture] = captureView.get<TransformComponent, CameraComponent, CaptureComponent>(entity);

			if (!capture.Capture || !capture.Target)
				continue;

			const uint32_t targetWidth = capture.Target->GetWidth();
			const uint32_t targetHeight = capture.Target->GetHeight();

			// Setup the capture renderer context if needed
			if (!capture.RuntimeRendererContext)
				capture.RuntimeRendererContext = SceneRendererContext::Create({ targetWidth, targetHeight }, activeContext->SceneContext);

			// Check for a resize of the virtual texture
			if (targetWidth != capture.RuntimeRendererContext->ActiveWidth || targetHeight != capture.RuntimeRendererContext->ActiveHeight)
				capture.RuntimeRendererContext->Resize(targetWidth, targetHeight);

			capture.RuntimeRendererContext->MaskType = capture.MaskType;

			camera.Camera.SetViewportSize(targetWidth, targetHeight);
			capture.RuntimeRendererContext->VisualizationMode = capture.Type;

			SceneRenderer::SetActiveContext(capture.RuntimeRendererContext);
			capture.RuntimeRendererContext->ActiveFramebuffer->Bind();
			RenderCommand::SetClearColor(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
			RenderCommand::Clear();
			SceneRenderer::SubmitCamera(camera.Camera, GetWorldTransformMatrix({ entity, this }));
			SceneRenderer::RenderScene();

			if (capture.Cumulative)
			{
				// Draw additively over the current framebuffer
				RenderCommand::SetBlendFunction(BlendFunction::One, BlendFunction::One);
				Renderer::DrawFullscreenTexture(capture.Target);
				RenderCommand::ResetBlendFunction();
			}

			capture.RuntimeRendererContext->ActiveFramebuffer->CopyColor(capture.Target);
		}

		// Restore the previous active framebuffer
		activeContext->ActiveFramebuffer->Bind();
		SceneRenderer::SetActiveContext(activeContext);

		SceneRenderer::EndScene();

		Renderer2D::BeginScene(*mainCamera, cameraTransform);

		// Draw sprites
		{
			auto group = m_Registry.group<TransformComponent>(entt::get<SpriteRendererComponent>);
			for (auto entity : group)
			{
				auto& [transform, sprite] = group.get<TransformComponent, SpriteRendererComponent>(entity);
				//Renderer2D::DrawSprite(transform.GetTransform(), sprite, (int)entity);
				Renderer2D::DrawSprite(GetWorldTransformMatrix({ entity, this }), sprite, (int)entity);
			}
		}

		// Draw circles
		{
			auto view = m_Registry.view<TransformComponent, CircleRendererComponent>();
			for (auto entity : view)
			{
				auto& [transform, circle] = view.get<TransformComponent, CircleRendererComponent>(entity);

				//Renderer2D::DrawCircle(transform.GetTransform(), circle.Color, circle.Thickness, circle.Fade, (int)entity);
				Renderer2D::DrawCircle(GetWorldTransformMatrix({ entity, this }), circle.Color, circle.Thickness, circle.Fade, (int)entity);
			}
		}

		// Draw Text
		{
			auto view = m_Registry.view<TransformComponent, TextComponent>();
			for (auto entity : view)
			{
				auto& [transform, text] = view.get<TransformComponent, TextComponent>(entity);
				Renderer2D::DrawTextComponent(GetWorldTransformMatrix({ entity, this }), text, (int)entity);
			}
		}

		// Render UI
		{
			auto view = m_Registry.view<UIImageComponent>();
			for (auto entity : view)
			{
				if (m_Registry.get<RelationshipComponent>(entity).ParentHandle != 0)
					continue;

				UIImageComponent& image = view.get<UIImageComponent>(entity);

				glm::vec3 pos = glm::inverse(mainCamera->GetProjection()) * glm::vec4((image.Anchor.x * 2.0f - 1.0f) + (image.Position.x / (m_ViewportWidth * 0.5f)), (image.Anchor.y * 2.0f - 1.0f) + (image.Position.y / (m_ViewportHeight * 0.5f)), -1.0f, 1.0f);
				glm::mat4 transform = cameraTransform * glm::translate(glm::mat4(1.0f), pos) * glm::scale(glm::mat4(1.0f), glm::vec3(image.Size.x / (m_ViewportHeight), image.Size.y / m_ViewportHeight, 1.0f));

				Renderer2D::DrawQuad(transform, image.Image);
			}
		}

		// Render Debug Lines
		for (uint32_t index = 0; index < m_DebugLines.size(); index++)
		{
			DebugLine& line = m_DebugLines[index];

			if (line.Time != 0.0f)
			{
				line.Time -= ts;
				if (line.Time < 0.0f)
				{
					m_DebugLines.erase(m_DebugLines.begin() + index);
					index--;
					continue;
				}
			}

			Renderer2D::DrawLine(line.Start, line.End, line.Color);
		}

		Renderer2D::EndScene();
	}

	static void Draw2DDebugSphere(glm::vec3 position, float radius, glm::vec4 color = glm::vec4(1.0f))
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

	static void DrawDebugIcon(Ref<Texture2D> icon, const EditorCamera& camera, const glm::vec3& translation, int entityID)
	{
		const float distance = glm::distance(translation, camera.GetPosition());
		const glm::mat4 transformation = glm::translate(glm::mat4(1.0f), glm::vec3(translation)) * glm::rotate(glm::mat4(1.0f), camera.GetYaw(), glm::vec3{ 0.0f, -1.0f, 0.0f })
			* glm::rotate(glm::mat4(1.0f), camera.GetPitch(), glm::vec3{ -1.0f, 0.0f, 0.0f }) * glm::scale(glm::mat4(1.0f), glm::vec3(distance < 5.0f ? distance / 5.0f : 1.0f));

		Renderer2D::DrawQuad(transformation, icon, 1.0f, glm::vec4(1.0f), entityID);
	}

	static glm::mat4 RemoveTransformScale(const glm::mat4& transform)
	{
		// Extract the rotation part
		glm::vec3 xAxis = glm::normalize(glm::vec3(transform[0]));
		glm::vec3 yAxis = glm::normalize(glm::vec3(transform[1]));
		glm::vec3 zAxis = glm::normalize(glm::vec3(transform[2]));

		// Extract the translation
		glm::vec3 translation = glm::vec3(transform[3]);

		// Reconstruct the matrix without scale
		glm::mat4 result = glm::mat4(1.0f); // identity
		result[0] = glm::vec4(xAxis, 0.0f);
		result[1] = glm::vec4(yAxis, 0.0f);
		result[2] = glm::vec4(zAxis, 0.0f);
		result[3] = glm::vec4(translation, 1.0f);

		return result;
	}

	void Scene::RenderSceneEditor(Timestep ts, EditorCamera& camera)
	{
		SceneRenderer::BeginScene();
		SceneRenderer::SubmitCamera(camera);

		// Submit Lights
		{
			auto view = m_Registry.view<TransformComponent, DirectionalLightComponent>();
			for (auto entity : view)
			{
				auto [transform, light] = view.get<TransformComponent, DirectionalLightComponent>(entity);
				//Renderer3D::SubmitDirectionalLight(transform.GetTransform(), light);
				SceneRenderer::SubmitDirectionalLight(GetWorldTransform({ entity, this }).GetRotationRadians(), light);
			}
		}
		{
			auto view = m_Registry.view<TransformComponent, PointLightComponent>();
			for (auto entity : view)
			{
				auto [transform, light] = view.get<TransformComponent, PointLightComponent>(entity);
				//Renderer3D::SubmitPointLight(transform.GetTransform(), light);
				SceneRenderer::SubmitPointLight(GetWorldTransform({ entity, this }).Translation, light);
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
			for (auto e : view)
			{
				auto [transform, smc] = view.get<TransformComponent, StaticMeshComponent>(e);

				if (!smc.m_Model)
					continue;

				Entity entity = { e, this };

				Ref<BoneMatrixList> pose = smc.m_AnimationGraphPlayer ? smc.m_AnimationGraphPlayer->GetFinalBoneMatrices() : nullptr;
				Ref<BlendShapeWeightList> blendShapeWeights = smc.m_AnimationGraphPlayer ? smc.m_AnimationGraphPlayer->GetBlendShapeWeights() : nullptr;

				smc.Update(ts);
				SceneRenderer::SubmitModel(GetWorldTransformMatrix(entity), smc.m_Model, smc.m_Materials, pose, blendShapeWeights, (int)e, IsEntitySelected(e));
			}
		}

		// Submit landscapes
		{
			auto view = m_Registry.view<TransformComponent, LandscapeComponent>();
			for (auto& entity : view)
			{
				auto& [transform, lc] = view.get<TransformComponent, LandscapeComponent>(entity);
				SceneRenderer::SubmitModel(GetWorldTransformMatrix({ entity, this }), lc.LandscapeMesh, lc.Material, (int)entity, IsEntitySelected(entity));
			}
		}

		// Submit particle systems
		{
			auto view = m_Registry.view<TransformComponent, ParticleSystemComponent>();
			for (auto entity : view)
			{
				auto& [transform, ps] = view.get<TransformComponent, ParticleSystemComponent>(entity);
				SceneRenderer::SubmitParticleSystem(GetWorldTransformMatrix({ entity, this }), ps.Player, ps.Material, (int)entity);
			}
		}

		// Submit Decals
		{
			auto view = m_Registry.view<TransformComponent, DecalComponent>();
			for (auto entity : view)
			{
				auto& [transform, dc] = view.get<TransformComponent, DecalComponent>(entity);
				SceneRenderer::SubmitDecal(GetWorldTransformMatrix({ entity, this }), dc.Texture, dc.ConstrainAngle, (int)entity);
			}
		}

		// Submit lighting volumes
		{
			auto view = m_Registry.view<TransformComponent, VolumeComponent>();
			for (auto entity : view)
			{
				auto& [transform, volume] = view.get<TransformComponent, VolumeComponent>(entity);
				SceneRenderer::SubmitVolume(GetWorldTransform({ entity, this }), volume, (int)entity);
			}
		}

		// Submit post process volumes
		{
			auto view = m_Registry.view<TransformComponent, PostProcessVolumeComponent>();
			for (auto entity : view)
			{
				auto& [transform, ppvc] = view.get<TransformComponent, PostProcessVolumeComponent>(entity);

				if (!ppvc.Enabled)
					continue;

				SceneRenderer::SubmitPostProcessVolume(GetWorldTransformMatrix({ entity, this }), ppvc, (int)entity);
			}
		}

		// Navigation Mesh Debug Visualization
		{
			auto view = m_Registry.view<TransformComponent, NavigationMeshComponent>();
			for (auto entity : view)
			{
				auto& [transform, nmc] = view.get<TransformComponent, NavigationMeshComponent>(entity);
				SceneRenderer::SubmitModel(glm::mat4(1.0f), nmc.DebugMesh, s_NavMeshMaterial, (int)entity);
			}
		}

		// Draw 3D Icons
		{
			{
				auto view = m_Registry.view<TransformComponent, CameraComponent>();
				for (auto entity : view)
					SceneRenderer::SubmitModel(GetWorldTransformMatrix({ entity, this }), s_CameraModel, s_CameraMaterial, (int)entity, IsEntitySelected(entity));
			}

			{
				auto view = m_Registry.view<TransformComponent, DirectionalLightComponent>();
				for (auto entity : view)
					SceneRenderer::SubmitModel(GetWorldTransform({ entity, this }).GetMatrixNoScale(), s_ArrowModel, s_ArrowMaterial, (int)entity, IsEntitySelected(entity));
			}

			{
				auto view = m_Registry.view<TransformComponent, DecalComponent>();
				for (auto entity : view)
					SceneRenderer::SubmitModel(GetWorldTransform({ entity, this }).GetMatrixNoScale(), s_ArrowModel, s_ArrowMaterial, (int)entity, IsEntitySelected(entity));
			}
		}

		SceneRenderer::RenderScene();

		// Draw overlay elements
		if (m_ShowColliders)
		{
			{
				auto view = m_Registry.view<TransformComponent, BoxColliderComponent>();
				for (auto entity : view)
				{
					BoxColliderComponent& bcc = view.get<BoxColliderComponent>(entity);
					SceneRenderer::DrawMeshOutlineOverlay(GetWorldTransformMatrix({ entity, this }) * glm::scale(glm::mat4(1.0f), bcc.Size), s_CubeModel, glm::vec4(0.0f, 1.0f, 0.0f, 1.0f), (int)entity);
				}
			}

			{
				auto view = m_Registry.view<TransformComponent, SphereColliderComponent>();
				for (auto entity : view)
				{
					SphereColliderComponent& scc = view.get<SphereColliderComponent>(entity);
					SceneRenderer::DrawMeshOutlineOverlay(GetWorldTransformMatrix({ entity, this }) * glm::scale(glm::mat4(1.0f), glm::vec3(scc.Radius)), s_SphereModel, glm::vec4(0.0f, 1.0f, 0.0f, 1.0f), (int)entity);
				}
			}

			{
				auto view = m_Registry.view<TransformComponent, CapsuleColliderComponent>();
				for (auto entity : view)
				{
					CapsuleColliderComponent& ccc = view.get<CapsuleColliderComponent>(entity);
					SceneRenderer::DrawMeshOutlineOverlay(GetWorldTransformMatrix({ entity, this }) * glm::scale(glm::mat4(1.0f), glm::vec3(ccc.HalfHeight, ccc.Radius, ccc.Radius)), s_CapsuleModel, glm::vec4(0.0f, 1.0f, 0.0f, 1.0f), (int)entity);
				}
			}

			{
				auto view = m_Registry.view<TransformComponent, MeshColliderComponent, StaticMeshComponent>();
				for (auto entity : view)
				{
					StaticMeshComponent& smc = view.get<StaticMeshComponent>(entity);
					SceneRenderer::DrawMeshOutlineOverlay(GetWorldTransformMatrix({ entity, this }), smc.m_Model, glm::vec4(0.0f, 1.0f, 0.0f, 1.0f), (int)entity);
				}
			}
		}

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

				auto& [transform, camera] = view.get<TransformComponent, CameraComponent>(entity);

				std::array<glm::vec3, 8> _cameraFrustumCornerVertices{
					{
						{ -1.0f, -1.0f, 1.0f }, { 1.0f, -1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { -1.0f, 1.0f, 1.0f },
						{ -1.0f, -1.0f, -1.0f }, { 1.0f, -1.0f, -1.0f }, { 1.0f, 1.0f, -1.0f }, { -1.0f, 1.0f, -1.0f },
					}
				};

				const auto proj = glm::inverse(camera.Camera.GetProjection());
				const auto trans = GetWorldTransformMatrix({ entity, this });
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
					auto& [tc, volume] = view.get<TransformComponent, VolumeComponent>(entity);
					Renderer2D::DrawCube(tc.Transform.Translation, tc.Transform.Scale, glm::vec4(0.2f, 0.1f, 0.9f, 1.0f), int(entity));
				}
			}
		}

		// Draw post process volume bounding box
		{
			auto view = m_Registry.view<TransformComponent, PostProcessVolumeComponent>();
			for (auto entity : view)
			{
				if (IsEntitySelected(entity))
				{
					auto& [transform, ppvc] = view.get<TransformComponent, PostProcessVolumeComponent>(entity);

					if (!ppvc.Bounded)
						continue;
					
					Renderer2D::DrawCube(transform.Transform.GetMatrix(), glm::vec4(0.2f, 0.1f, 0.9f, 1.0f), int(entity));
				}
			}
		}

		// Draw Mesh Bounding Boxes
		{
			auto view = m_Registry.view<TransformComponent, StaticMeshComponent>();
			for (auto entity : view)
			{
				if (IsEntitySelected(entity))
				{
					auto& [tc, smc] = view.get<TransformComponent, StaticMeshComponent>(entity);
					
					if (smc.m_Model)
					{
						const auto& aabb = smc.m_Model->GetAABB();
						Renderer2D::DrawCube(aabb.GetCenter() + tc.Transform.Translation, aabb.GetSize() * tc.Transform.Scale, glm::vec4(0.2f, 0.1f, 0.9f, 1.0f), int(entity));
					}
				}
			}
		}

		// Draw point light debug sphere and icon
		{
			auto view = m_Registry.view<TransformComponent, PointLightComponent>();
			for (auto entity : view)
			{
				auto& [tc, light] = view.get<TransformComponent, PointLightComponent>(entity);

				const glm::vec3 worldTranslation = GetWorldTransform({ entity, this }).Translation;
				DrawDebugIcon(s_LightIcon, camera, worldTranslation, (int)entity);

				if (IsEntitySelected(entity))
					Draw2DDebugSphere(worldTranslation, light.Radius, glm::vec4(light.Color, 1.0f));
			}
		}

		// Draw audio debug sphere and icon
		{
			auto view = m_Registry.view<TransformComponent, AudioComponent>();
			for (auto entity : view)
			{
				auto& [transform, ac] = view.get<TransformComponent, AudioComponent>(entity);
				const glm::vec3 worldTranslation = GetWorldTransform({ entity, this }).Translation;
				DrawDebugIcon(s_SoundIcon, camera, worldTranslation, (int)entity);

				if (IsEntitySelected(entity) && ac.AudioSound)
					Draw2DDebugSphere(worldTranslation, ac.AudioSound->GetRadius());
			}
		}

		// Draw spline lines
		{
			auto view = m_Registry.view<TransformComponent, SplineComponent>();
			for (auto entity : view)
			{
				auto& [transform, sc] = view.get<TransformComponent, SplineComponent>(entity);

				if (sc.Points.size() < 2)
					continue;

				const bool selected = IsEntitySelected(entity);
				const glm::vec3 worldTranslation = GetWorldTransform({ entity, this }).Translation;
				const glm::vec4 lineColor = selected ? glm::vec4(0.82f, 0.62f, 0.13f, 1.0f) : glm::vec4(1.0f);

				static constexpr uint32_t c_SegmentSamples = 25;
				const uint32_t sampleCount = c_SegmentSamples * sc.Points.size() - 1;

				// Draw curve
				glm::vec3 previousPoint = worldTranslation + sc.Points.front().Position;
				for (uint32_t sampleIndex = 1; sampleIndex < sampleCount; sampleIndex++)
				{
					const glm::vec3 newPoint = worldTranslation + sc.Sample((float)sampleIndex / (float)sampleCount);
					Renderer2D::DrawLine(previousPoint, newPoint, lineColor, (int)entity);
					previousPoint = newPoint;
				}

				// Draw point dots and tangent lines (only if selected)
				if (selected)
				{
					const glm::vec4 pointColor = glm::vec4(glm::vec3(lineColor) * 0.9f, 1.0f);

					for (const auto& point : sc.Points)
					{
						const glm::vec3 position = worldTranslation + point.Position;
						Renderer2D::DrawPoint(position, pointColor, (int)entity);
						Renderer2D::DrawLineDashed(position - point.Tangent, position + point.Tangent, lineColor, 0.5f, 0.05f);
					}
				}
			}
		}

		// Draw particle system debug icon
		{
			auto view = m_Registry.view<TransformComponent, ParticleSystemComponent>();
			for (auto entity : view)
			{
				TransformComponent& tc = view.get<TransformComponent>(entity);
				DrawDebugIcon(s_ParticleSystemIcon, camera, GetWorldTransform({ entity, this }).Translation, (int)entity);
			}
		}

		// Draw decal debug box and icon
		{
			auto view = m_Registry.view<TransformComponent, DecalComponent>();
			for (auto entity : view)
			{
				TransformComponent& tc = view.get<TransformComponent>(entity);

				const Transform worldTransform = GetWorldTransform({ entity, this });
				DrawDebugIcon(s_DecalIcon, camera, worldTransform.Translation, (int)entity);

				if (IsEntitySelected(entity))
					Renderer2D::DrawCube(worldTransform.GetMatrix(), glm::vec4(1.0f));
			}
		}

		// Draw Field debug icon and bounds
		{
			auto view = m_Registry.view<TransformComponent, FieldComponent>();
			for (auto entity : view)
			{
				auto& [tc, fc] = view.get<TransformComponent, FieldComponent>(entity);

				const Transform worldTransform = GetWorldTransform({ entity, this });
				DrawDebugIcon(s_FieldIcon, camera, worldTransform.Translation, (int)entity);

				if (IsEntitySelected(entity))
					Renderer2D::DrawCube(worldTransform.GetMatrix(), glm::vec4(1.0f));
			}
		}

		// Draw sprites
		{
			auto group = m_Registry.group<TransformComponent>(entt::get<SpriteRendererComponent>);
			for (auto entity : group)
			{
				auto& [transform, sprite] = group.get<TransformComponent, SpriteRendererComponent>(entity);

				//Renderer2D::DrawSprite(transform.GetTransform(), sprite, (int)entity);
				Renderer2D::DrawSprite(GetWorldTransformMatrix({ entity, this }), sprite, (int)entity);
			}
		}

		// Draw circles
		{
			auto view = m_Registry.view<TransformComponent, CircleRendererComponent>();
			for (auto entity : view)
			{
				auto& [transform, circle] = view.get<TransformComponent, CircleRendererComponent>(entity);
				//Renderer2D::DrawCircle(transform.GetTransform(), circle.Color, circle.Thickness, circle.Fade, (int)entity);
				Renderer2D::DrawCircle(GetWorldTransformMatrix({ entity, this }), circle.Color, circle.Thickness, circle.Fade, (int)entity);
			}
		}

		// Draw Text
		{
			auto view = m_Registry.view<TransformComponent, TextComponent>();
			for (auto entity : view)
			{
				auto& [transform, text] = view.get<TransformComponent, TextComponent>(entity);
				Renderer2D::DrawTextComponent(GetWorldTransformMatrix({ entity, this }), text, (int)entity);
			}
		}

		Renderer2D::EndScene();
	}

	// On Component Added function callbacks

	void Scene::OnComponentAdded(Entity entity, const std::type_info& type)
	{
		if (&type == &typeid(CameraComponent))
		{
			if (entity.HasComponent<CaptureComponent>())
				return;

			CameraComponent& component = entity.GetComponent<CameraComponent>();

			if (m_ViewportWidth > 0 && m_ViewportHeight > 0)
				component.Camera.SetViewportSize(m_ViewportWidth, m_ViewportHeight);
		}
		else if (&type == &typeid(ScriptComponent))
		{
			ScriptEngine::OnCreateEntity(entity);
		}
		else if (&type == &typeid(FolderComponent))
		{
			if (entity.HasComponent<TransformComponent>())
				entity.RemoveComponent<TransformComponent>();
		}
		else if (m_PhysicsScene)
			m_PhysicsScene->OnComponentAdded(entity, type);
	}

	void Scene::OnComponentRemoved(Entity entity, const std::type_info& type)
	{
		if (&type == &typeid(ScriptComponent))
			ScriptEngine::OnDestroyEntity(entity);
	}

}