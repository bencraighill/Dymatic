#include "dypch.h"
#include "ScriptGlue.h"
#include "ScriptEngine.h"

#include "Dymatic/Core/UUID.h"
#include "Dymatic/Core/KeyCodes.h"
#include "Dymatic/Core/Input.h"
#include "Dymatic/Project/Project.h"
#include "Dymatic/Core/Application.h"

#include "Dymatic/Scene/Scene.h"
#include "Dymatic/Scene/Entity.h"
#include "Dymatic/Scene/Transform.h"

#include "Dymatic/Video/VideoReader.h"

#include "Dymatic/Core/BufferStream.h"
#include "Dymatic/Networking/NetworkManager.h"

#include "mono/metadata/object.h"
#include "mono/metadata/reflection.h"
#include "mono/metadata/appdomain.h"

#include "box2d/b2_body.h"

namespace Dymatic {

	struct ScriptAssetReference
	{
		Ref<Asset> Asset;
		uint32_t References = 1;

		ScriptAssetReference() = default;
		ScriptAssetReference(AssetHandle handle)
			: Asset(AssetManager::GetAsset(handle)) {}
	};

	// A Transform structure sharing the same memory layout and units as the C# transform class
	struct ScriptTransform
	{
		glm::vec3 Translation;
		glm::vec3 Rotation;
		glm::vec3 Scale;

		ScriptTransform(const Transform& transform)
			: Translation(transform.Translation), Rotation(transform.GetRotationDegrees()), Scale(transform.Scale)
		{}

		operator Transform() const
		{
			return Transform(Translation, glm::radians(Rotation), Scale);
		}
	};

	static std::unordered_map<AssetHandle, Ref<MaterialInstance>> s_InstancedMaterials;
	static std::unordered_map<AssetHandle, ScriptAssetReference> s_RegisteredScriptAssets;

	static std::function<void(UUID)> s_OpenSceneCallback = nullptr;

	static std::unordered_map<MonoType*, std::function<void(Entity)>> s_EntityAddComponentFuncs;
	static std::unordered_map<MonoType*, std::function<bool(Entity)>> s_EntityHasComponentFuncs;
	static std::unordered_map<MonoType*, std::function<void(Entity)>> s_EntityRemoveComponentFuncs;

#define DY_ADD_INTERNAL_CALL(Name) mono_add_internal_call("Dymatic.InternalCalls::" #Name, Name)

	// Helper Functions

	static Entity GetEntityByUUID(EntityHandle handle)
	{
		return ScriptEngine::GetSceneContext()->GetEntityByUUID(handle);
	}

	static Ref<PhysicsScene> GetPhysicsScene()
	{
		return ScriptEngine::GetSceneContext()->GetPhysicsScene();
	}

	// C# Script Glue Interface
	static MonoObject* GetScriptInstance(UUID entityID)
	{
		return ScriptEngine::GetManagedInstance(entityID);
	}

	namespace Utils {
	
		static MonoArray* GetEntityMonoArrayFromIDs(const std::vector<EntityHandle>& entityIDs)
		{
			MonoClass* monoClass = mono_class_from_name(ScriptEngine::GetCoreAssemblyImage(), "Dymatic", "Entity");
			MonoArray* monoArray = mono_array_new(ScriptEngine::GetApplicationDomain(), monoClass, entityIDs.size());

			for (size_t i = 0; i < entityIDs.size(); i++)
				mono_array_set(monoArray, MonoObject*, i, ScriptEngine::GetManagedInstanceOrDefaultEntity(entityIDs[i]));

			return monoArray;
		}
	
	}

	static void Scene_OpenScene(UUID assetID)
	{
		if (s_OpenSceneCallback)
			s_OpenSceneCallback(assetID);
	}

	static void Asset_RegisterScriptReference(AssetHandle handle)
	{
		if (handle == 0)
			return;

		if (s_RegisteredScriptAssets.find(handle) == s_RegisteredScriptAssets.end())
		{
			s_RegisteredScriptAssets[handle] = ScriptAssetReference(handle);
			return;
		}

		s_RegisteredScriptAssets[handle].References++;
	}

	static void Asset_UnregisterScriptReference(AssetHandle handle)
	{
		if (handle == 0)
			return;

		if (s_RegisteredScriptAssets.find(handle) == s_RegisteredScriptAssets.end())
			return;

		const uint32_t references = s_RegisteredScriptAssets[handle].References--;

		if (references <= 0)
			s_RegisteredScriptAssets.erase(handle);
	}

	static bool Asset_DoesAssetExist(uint64_t handle)
	{
		return AssetManager::DoesAssetExist(handle);
	}

	static uint64_t Asset_GetAssetHandle(MonoString* filepath)
	{
		char* filepathCStr = mono_string_to_utf8(filepath);
		std::filesystem::path path = std::filesystem::path(filepathCStr);
		mono_free(filepathCStr);

		return AssetManager::GetAssetHandleFromFilePath(path);
	}

	static MonoObject* Prefab_Instantiate(AssetHandle handle)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		Entity instance = scene->Instantiate(AssetManager::GetAsset<Prefab>(handle));
		return ScriptEngine::GetManagedInstanceOrDefaultEntity(instance.GetUUID());
	}

	static MonoObject* Prefab_InstantiateAtTransform(AssetHandle handle, ScriptTransform* transform)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		Entity instance = scene->Instantiate(AssetManager::GetAsset<Prefab>(handle));

		scene->SetEntityTransform(instance, *transform);

		return ScriptEngine::GetManagedInstanceOrDefaultEntity(instance.GetUUID());
	}

	static AssetHandle Material_CreateInstance(AssetHandle handle)
	{
		Ref<MaterialAsset> source = AssetManager::GetAsset<MaterialAsset>(handle);

		if (!source)
			return 0;

		Ref<MaterialInstance> instance = AssetManager::CreateMemoryOnlyAsset<MaterialInstance>(source);
		s_InstancedMaterials[instance->Handle] = instance;
		return instance->Handle;
	}

	static bool MaterialInstance_SetParameter(uint64_t handle, MonoString* name, const MaterialAsset::MaterialParameterType type, MaterialAsset::MaterialParameterData::Data data)
	{
		// Validate the asset exists and is a material instanced during the runtime (so we can override editor assets)
		if (s_InstancedMaterials.find(handle) == s_InstancedMaterials.end())
		{
			DY_WARN("Handle {} does not refer to a valid material instance generated during runtime", handle);
			return false;
		}

		Ref<MaterialInstance> instance = s_InstancedMaterials.at(handle);

		char* nameCStr = mono_string_to_utf8(name);
		const std::string parameterName = nameCStr;
		mono_free(nameCStr);

		// Validate the specified parameter name
		const auto& parameters = instance->GetParameters();
		if (parameters.find(parameterName) == parameters.end())
		{
			DY_WARN("Material instance {} does not have parameter '{}'", handle, parameterName);
			return false;
		}

		// Ensure parameter override matches parameter type on material
		if (parameters.at(parameterName).Type != type)
		{
			DY_WARN("Parameter '{}' of material instance {} is not of type {}", parameterName, handle, MaterialAsset::MaterialParameterData::MaterialParameterTypeToString(type));
			return false;
		}

		// Add the specified parameter override
		instance->SetParameterOverride(parameterName, data);
		return true;
	}

	static bool MaterialInstance_SetParameterFloat(uint64_t handle, MonoString* name, float value)
	{
		return MaterialInstance_SetParameter(handle, name, MaterialAsset::MaterialParameterType::Float, value);
	}

	static bool MaterialInstance_SetParameterVector2(uint64_t handle, MonoString* name, glm::vec2* value)
	{
		return MaterialInstance_SetParameter(handle, name, MaterialAsset::MaterialParameterType::Vector2, *value);
	}

	static bool MaterialInstance_SetParameterVector3(uint64_t handle, MonoString* name, glm::vec3* value)
	{
		return MaterialInstance_SetParameter(handle, name, MaterialAsset::MaterialParameterType::Vector3, *value);
	}

	static bool MaterialInstance_SetParameterVector4(uint64_t handle, MonoString* name, glm::vec4* value)
	{
		return MaterialInstance_SetParameter(handle, name, MaterialAsset::MaterialParameterType::Vector4, *value);
	}

	static void VirtualTexture_Resize(AssetHandle handle, glm::vec2* size)
	{
		const Ref<Texture2D> virtualTexture = AssetManager::GetAsset<Texture2D>(handle);
		virtualTexture->Resize(*size);
	}

	static void VirtualTexture_Clear(AssetHandle handle)
	{
		const Ref<Texture2D> virtualTexture = AssetManager::GetAsset<Texture2D>(handle);
		virtualTexture->Clear();
	}

	static void VirtualTexture_GetSize(AssetHandle handle, glm::vec2* outSize)
	{
		const Ref<Texture2D> virtualTexture = AssetManager::GetAsset<Texture2D>(handle);
		*outSize = virtualTexture->GetSize();
	}

	static uint32_t VirtualTexture_GetPixelCount(AssetHandle handle)
	{
		const Ref<Texture2D> virtualTexture = AssetManager::GetAsset<Texture2D>(handle);
		return virtualTexture->GetWidth() * virtualTexture->GetHeight();
	}

	static uint32_t VirtualTexture_GetDataSize(AssetHandle handle)
	{
		const Ref<Texture2D> virtualTexture = AssetManager::GetAsset<Texture2D>(handle);
		return virtualTexture->GetDataSize();
	}

	static MonoArray* VirtualTexture_GetData(AssetHandle handle)
	{
		const Ref<Texture2D> virtualTexture = AssetManager::GetAsset<Texture2D>(handle);

		// Create a byte array of the desired size
		const size_t dataSize = virtualTexture->GetDataSize();
		MonoClass* byteClass = mono_get_byte_class();
		MonoArray* byteArray = mono_array_new(ScriptEngine::GetApplicationDomain(), byteClass, dataSize);

		// Copy VRAM texture data directly into the mono array memory
		void* data = mono_array_addr(byteArray, uint8_t, 0);
		virtualTexture->GetData(data, dataSize);

		return byteArray;
	}

	static bool VirtualTexture_SetData(AssetHandle handle, MonoArray* dataArray)
	{
		const Ref<Texture2D> virtualTexture = AssetManager::GetAsset<Texture2D>(handle);

		// Extract properties of the mono array and element sizes to compute overall data size
		MonoClass* arrayClass = mono_object_get_class((MonoObject*)dataArray);
		MonoClass* elementClass = mono_class_get_element_class(arrayClass);

		size_t elementSize = mono_class_value_size(elementClass, nullptr);
		size_t arrayLength = mono_array_length(dataArray);

		const size_t dataSize = elementSize * arrayLength;

		// Verify mono array size matches the virtual texture
		if (dataSize != virtualTexture->GetDataSize())
			return false;

		// Access the data and upload to VRAM
		void* data = mono_array_addr(dataArray, uint8_t, 0);
		virtualTexture->SetData(data, dataSize);

		return true;
	}

	static void VideoPlayer_SetTime(AssetHandle handle, float time)
	{
		AssetManager::GetAsset<VideoReader>(handle)->SetTime(time);
	}

	static void VideoPlayer_Update(AssetHandle handle, float ts)
	{
		AssetManager::GetAsset<VideoReader>(handle)->GetNextFrame(ts);
	}

	static void VideoPlayer_GetSubtitle(AssetHandle handle, MonoString** outSubtitle)
	{
		*outSubtitle = mono_string_new(mono_domain_get(), AssetManager::GetAsset<VideoReader>(handle)->GetCurrentSubtitle().c_str());
	}

	static void Entity_AddComponent(UUID entityID, MonoReflectionType* componentType)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		if (!entity)
			return;

		MonoType* managedType = mono_reflection_type_get_type(componentType);
		DY_CORE_ASSERT(s_EntityAddComponentFuncs.find(managedType) != s_EntityAddComponentFuncs.end());
		s_EntityAddComponentFuncs.at(managedType)(entity);
	}

	static bool Entity_HasComponent(EntityHandle entityID, MonoReflectionType* componentType)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		if (!entity)
			return false;

		MonoType* managedType = mono_reflection_type_get_type(componentType);
		DY_CORE_ASSERT(s_EntityHasComponentFuncs.find(managedType) != s_EntityHasComponentFuncs.end());
		return s_EntityHasComponentFuncs.at(managedType)(entity);
	}

	static void Entity_RemoveComponent(EntityHandle entityID, MonoReflectionType* componentType)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		if (!entity)
			return;

		MonoType* managedType = mono_reflection_type_get_type(componentType);
		DY_CORE_ASSERT(s_EntityRemoveComponentFuncs.find(managedType) != s_EntityRemoveComponentFuncs.end());
		s_EntityRemoveComponentFuncs.at(managedType)(entity);
	}

	static EntityHandle Entity_Duplicate(EntityHandle entityID)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		return scene->DuplicateEntity(GetEntityByUUID(entityID)).GetUUID();
	}

	static void Entity_Destroy(EntityHandle entityID)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		scene->DestroyEntity(GetEntityByUUID(entityID));
	}

	static void Entity_Parent(EntityHandle entityID, EntityHandle parentID)
	{
		Scene* scene = ScriptEngine::GetSceneContext();

		if (!scene->DoesEntityExist(parentID))
			return;

		scene->ParentEntity(GetEntityByUUID(entityID), GetEntityByUUID(parentID));
	}

	static void Entity_Unparent(EntityHandle entityID)
	{
		ScriptEngine::GetSceneContext()->UnparentEntity(GetEntityByUUID(entityID));
	}

	static bool Entity_HasParent(EntityHandle entityID)
	{
		return GetEntityByUUID(entityID).HasParent();
	}

	static MonoObject* Entity_GetParent(EntityHandle entityID)
	{
		Entity entity = GetEntityByUUID(entityID);

		if (!entity || !entity.HasParent())
			return nullptr;

		return ScriptEngine::GetManagedInstanceOrDefaultEntity(entity.GetParentHandle());
	}

	static uint32_t Entity_GetChildCount(EntityHandle entityID)
	{
		return GetEntityByUUID(entityID).GetChildCount();
	}

	static MonoArray* Entity_GetChildren(EntityHandle entityID)
	{
		return Utils::GetEntityMonoArrayFromIDs(GetEntityByUUID(entityID).GetChildrenUUIDs());
	}

	static void Entity_Attach(EntityHandle entityID, MonoString* boneName)
	{
		Scene* scene = ScriptEngine::GetSceneContext();

		char* boneNameCStr = mono_string_to_utf8(boneName);
		scene->AttachEntity(GetEntityByUUID(entityID), boneNameCStr);
		mono_free(boneNameCStr);
	}

	static void Entity_GetAttachment(EntityHandle entityID, MonoString** outBoneName)
	{
		Entity entity = GetEntityByUUID(entityID);
		*outBoneName = mono_string_new(mono_domain_get(), (entity && entity.HasComponent<AttachmentComponent>()) ? entity.GetComponent<AttachmentComponent>().BoneName.c_str() : "");
	}

	static MonoObject* Entity_GetEntityByID(EntityHandle entityID)
	{
		return ScriptEngine::GetManagedInstanceOrDefaultEntity(entityID);
	}

	static MonoObject* Entity_FindEntityByName(MonoString* name)
	{
		char* nameCStr = mono_string_to_utf8(name);

		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->FindEntityByName(nameCStr);
		mono_free(nameCStr);

		if (!entity)
			return nullptr;

		return ScriptEngine::GetManagedInstanceOrDefaultEntity(entity.GetUUID());
	}

	static MonoObject* Entity_Create(MonoString* name)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		char* nameCStr = mono_string_to_utf8(name);
		Entity entity = scene->CreateEntity(nameCStr);
		mono_free(nameCStr);

		return ScriptEngine::GetManagedInstanceOrDefaultEntity(entity.GetUUID());
	}

	static MonoObject* Entity_CreateWithScript(MonoString* name, MonoReflectionType* scriptType)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		char* nameCStr = mono_string_to_utf8(name);
		Entity entity = scene->CreateEntity(nameCStr);
		mono_free(nameCStr);

		MonoType* classType = mono_reflection_type_get_type(scriptType);
		MonoClass* klass = mono_type_get_class(classType);
		const char* className = mono_class_get_name(klass);
		const char* namespaceName = mono_class_get_namespace(klass);
		entity.AddComponent<ScriptComponent>(fmt::format("{}.{}", namespaceName, className));

		return ScriptEngine::GetManagedInstanceOrDefaultEntity(entity.GetUUID());
	}

	// Custom Component Constructors
	static void Entity_AddScriptComponent(EntityHandle entityID, MonoString* name)
	{
		char* nameCStr = mono_string_to_utf8(name);
		GetEntityByUUID(entityID).AddComponent<ScriptComponent>(std::string(nameCStr));
		mono_free(nameCStr);
	}

	static void Entity_AddDistanceConstraintComponentDefault(EntityHandle entityID, EntityHandle targetID)
	{
		GetEntityByUUID(entityID).AddComponent<DistanceConstraintComponent>(targetID);
	}

	static void Entity_AddDistanceConstraintComponentFixed(EntityHandle entityID, EntityHandle targetID, float distance)
	{
		GetEntityByUUID(entityID).AddComponent<DistanceConstraintComponent>(targetID, distance);
	}

	static void Entity_AddDistanceConstraintComponentRange(EntityHandle entityID, EntityHandle targetID, float minDistance, float maxDistance)
	{
		GetEntityByUUID(entityID).AddComponent<DistanceConstraintComponent>(targetID, minDistance, maxDistance);
	}

	static void Entity_AddFixedConstraintComponent(EntityHandle entityID, EntityHandle targetID)
	{
		GetEntityByUUID(entityID).AddComponent<FixedConstraintComponent>(targetID);
	}

	static void TagComponent_GetTag(UUID entityID, MonoString** outTag)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		if (!entity)
			return;

		*outTag = mono_string_new(mono_domain_get(), entity.GetComponent<TagComponent>().Tag.c_str());
	}

	static void TagComponent_SetTag(UUID entityID, MonoString* tag)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		char* tagCStr = mono_string_to_utf8(tag);
		entity.GetComponent<TagComponent>().Tag = tagCStr;
		mono_free(tagCStr);
	}

	static void TransformComponent_GetTranslation(UUID entityID, glm::vec3* outTranslation)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		*outTranslation = entity.GetComponent<TransformComponent>().Transform.Translation;
	}

	static void TransformComponent_SetTranslation(UUID entityID, glm::vec3* translation)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		scene->SetEntityTranslation(entity, *translation);
	}

	static void TransformComponent_GetRotation(UUID entityID, glm::vec3* outRotation)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		*outRotation = entity.GetComponent<TransformComponent>().Transform.GetRotationDegrees();
	}

	static void TransformComponent_SetRotation(UUID entityID, glm::vec3* rotation)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		scene->SetEntityRotation(entity, glm::radians(*rotation));
	}

	static void TransformComponent_GetScale(UUID entityID, glm::vec3* outScale)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		*outScale = entity.GetComponent<TransformComponent>().Transform.Scale;
	} 

	static void TransformComponent_SetScale(UUID entityID, glm::vec3* scale)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		scene->SetEntityScale(entity, *scale);
	}

	static void TransformComponent_GetLocalTransform(EntityHandle entityID, glm::vec3* outTranslation, glm::vec3* outRotation, glm::vec3* outScale)
	{
		const Transform& transform = GetEntityByUUID(entityID).GetComponent<TransformComponent>().Transform;
		*outTranslation = transform.Translation;
		*outRotation = transform.GetRotationDegrees();
		*outScale = transform.Scale;
	}

	static void TransformComponent_SetLocalTransform(EntityHandle entityID, glm::vec3* translation, glm::vec3* rotation, glm::vec3* scale)
	{
		Transform& transform = GetEntityByUUID(entityID).GetComponent<TransformComponent>().Transform;
		transform.Translation = *translation;
		transform.SetRotationDegrees(*rotation);
		transform.Scale = *scale;
	}

	static void TransformComponent_GetWorldTranslation(EntityHandle entityID, glm::vec3* outTranslation)
	{
		*outTranslation = GetEntityByUUID(entityID).GetWorldTransform().Translation;
	}

	static void TransformComponent_SetWorldTranslation(EntityHandle entityID, glm::vec3* translation)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		scene->SetEntityTranslation(entity, entity.GetLocalTransform(Transform(*translation)).Translation);
	}

	static void TransformComponent_GetWorldRotation(EntityHandle entityID, glm::vec3* outRotation)
	{
		*outRotation = GetEntityByUUID(entityID).GetWorldTransform().GetRotationDegrees();
	}

	static void TransformComponent_SetWorldRotation(EntityHandle entityID, glm::vec3* rotation)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		Transform worldTransform;
		worldTransform.SetRotationDegrees(*rotation);

		scene->SetEntityRotation(entity, entity.GetLocalTransform(worldTransform).Rotation);
	}

	static void TransformComponent_GetWorldScale(EntityHandle entityID, glm::vec3* outScale)
	{
		*outScale = GetEntityByUUID(entityID).GetWorldTransform().Scale;
	}

	static void TransformComponent_SetWorldScale(EntityHandle entityID, glm::vec3* scale)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		Transform worldTransform;
		worldTransform.Scale = *scale;

		scene->SetEntityScale(entity, entity.GetLocalTransform(worldTransform).Scale);
	}

	static void TransformComponent_GetWorldTransform(EntityHandle entityID, glm::vec3* outTranslation, glm::vec3* outRotation, glm::vec3* outScale)
	{
		Transform transform = GetEntityByUUID(entityID).GetWorldTransform();
		*outTranslation = transform.Translation;
		*outRotation = transform.GetRotationDegrees();
		*outScale = transform.Scale;
	}

	static void TransformComponent_SetWorldTransform(EntityHandle entityID, glm::vec3* translation, glm::vec3* rotation, glm::vec3* scale)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		Entity entity = scene->GetEntityByUUID(entityID);

		Transform worldTransform;
		worldTransform.Translation = *translation;
		worldTransform.SetRotationDegrees(*rotation);
		worldTransform.Scale = *scale;

		Transform localTransform = entity.GetLocalTransform(worldTransform);
		scene->SetEntityTransform(entity, localTransform);
	}

	static uint8_t CameraComponent_GetProjectionType(UUID entityID)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		return (uint8_t)entity.GetComponent<CameraComponent>().Camera.GetProjectionType();
	}

	static void CameraComponent_SetProjectionType(UUID entityID, uint8_t projectionType)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		entity.GetComponent<CameraComponent>().Camera.SetProjectionType((SceneCamera::ProjectionType)projectionType);
	}

	static float CameraComponent_GetPerspectiveFOV(UUID entityID)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		return entity.GetComponent<CameraComponent>().Camera.GetPerspectiveVerticalFOV();
	}

	static void CameraComponent_SetPerspectiveFOV(UUID entityID, float perspectiveFOV)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		entity.GetComponent<CameraComponent>().Camera.SetPerspectiveVerticalFOV(perspectiveFOV);
	}

	static float CameraComponent_GetPerspectiveNear(UUID entityID)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		return entity.GetComponent<CameraComponent>().Camera.GetPerspectiveNearClip();
	}

	static void CameraComponent_SetPerspectiveNear(UUID entityID, float perspectiveNear)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		entity.GetComponent<CameraComponent>().Camera.SetPerspectiveNearClip(perspectiveNear);
	}

	static float CameraComponent_GetPerspectiveFar(UUID entityID)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		return entity.GetComponent<CameraComponent>().Camera.GetPerspectiveFarClip();
	}

	static void CameraComponent_SetPerspectiveFar(UUID entityID, float perspectiveFar)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		entity.GetComponent<CameraComponent>().Camera.SetPerspectiveFarClip(perspectiveFar);
	}

	static float CameraComponent_GetOrthographicSize(UUID entityID)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		return entity.GetComponent<CameraComponent>().Camera.GetOrthographicSize();
	}

	static void CameraComponent_SetOrthographicSize(UUID entityID, float orthographicSize)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		entity.GetComponent<CameraComponent>().Camera.SetOrthographicSize(orthographicSize);
	}

	static float CameraComponent_GetOrthographicNear(UUID entityID)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		return entity.GetComponent<CameraComponent>().Camera.GetOrthographicNearClip();
	}

	static void CameraComponent_SetOrthographicNear(UUID entityID, float orthographicNear)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		entity.GetComponent<CameraComponent>().Camera.SetOrthographicNearClip(orthographicNear);
	}

	static float CameraComponent_GetOrthographicFar(UUID entityID)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		return entity.GetComponent<CameraComponent>().Camera.GetOrthographicFarClip();
	}

	static void CameraComponent_SetOrthographicFar(UUID entityID, float orthographicFar)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		entity.GetComponent<CameraComponent>().Camera.SetOrthographicFarClip(orthographicFar);
	}

	static bool CameraComponent_GetPrimary(UUID entityID)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		return entity.GetComponent<CameraComponent>().Primary;
	}

	static void CameraComponent_SetPrimary(UUID entityID, bool primary)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		entity.GetComponent<CameraComponent>().Primary = primary;
	}

	static bool CameraComponent_GetFixedAspectRatio(UUID entityID)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		return entity.GetComponent<CameraComponent>().FixedAspectRatio;
	}

	static void CameraComponent_SetFixedAspectRatio(UUID entityID, bool fixedAspectRatio)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		entity.GetComponent<CameraComponent>().FixedAspectRatio = fixedAspectRatio;
	}

	static AssetHandle CaptureComponent_GetTarget(EntityHandle entityID)
	{
		const Ref<Texture2D> target = GetEntityByUUID(entityID).GetComponent<CaptureComponent>().Target;
		return target ? target->Handle : 0;
	}

	static void CaptureComponent_SetTarget(EntityHandle entityID, AssetHandle targetID)
	{
		GetEntityByUUID(entityID).GetComponent<CaptureComponent>().Target = AssetManager::GetAsset<Texture2D>(targetID);
	}

	static uint8_t CaptureComponent_GetRenderType(EntityHandle entityID)
	{
		return (uint8_t)GetEntityByUUID(entityID).GetComponent<CaptureComponent>().Type;
	}

	static void CaptureComponent_SetRenderType(EntityHandle entityID, uint8_t renderType)
	{
		GetEntityByUUID(entityID).GetComponent<CaptureComponent>().Type = (SceneRendererContext::RendererVisualizationMode)renderType;
	}

	static uint8_t CaptureComponent_GetMaskType(EntityHandle entityID)
	{
		return (uint8_t)GetEntityByUUID(entityID).GetComponent<CaptureComponent>().MaskType;
	}

	static void CaptureComponent_SetMaskType(EntityHandle entityID, uint8_t maskType)
	{
		GetEntityByUUID(entityID).GetComponent<CaptureComponent>().MaskType = (SceneRendererContext::RenderMaskType)maskType;
	}

	static bool CaptureComponent_GetCapture(EntityHandle entityID)
	{
		return GetEntityByUUID(entityID).GetComponent<CaptureComponent>().Capture;
	}

	static void CaptureComponent_SetCapture(EntityHandle entityID, bool capture)
	{
		GetEntityByUUID(entityID).GetComponent<CaptureComponent>().Capture = capture;
	}

	static bool CaptureComponent_GetCumulative(EntityHandle entityID)
	{
		return GetEntityByUUID(entityID).GetComponent<CaptureComponent>().Cumulative;
	}

	static void CaptureComponent_SetCumulative(EntityHandle entityID, bool cumulative)
	{
		GetEntityByUUID(entityID).GetComponent<CaptureComponent>().Cumulative = cumulative;
	}

	static void CaptureComponent_MaskAdd(EntityHandle entityID, EntityHandle targetID)
	{
		auto& cc = GetEntityByUUID(entityID).GetComponent<CaptureComponent>();

		if (!cc.RuntimeRendererContext)
			return;

		Entity target = GetEntityByUUID(targetID);

		if (!target)
			return;

		cc.RuntimeRendererContext->RenderMask.insert((int)(entt::entity)target);
	}

	static void CaptureComponent_MaskRemove(EntityHandle entityID, EntityHandle targetID)
	{
		auto& cc = GetEntityByUUID(entityID).GetComponent<CaptureComponent>();

		if (!cc.RuntimeRendererContext)
			return;

		Entity target = GetEntityByUUID(targetID);

		if (!target)
			return;

		cc.RuntimeRendererContext->RenderMask.erase((int)(entt::entity)target);
	}

	static void CaptureComponent_MaskClear(EntityHandle entityID)
	{
		auto& cc = GetEntityByUUID(entityID).GetComponent<CaptureComponent>();

		if (!cc.RuntimeRendererContext)
			return;

		cc.RuntimeRendererContext->RenderMask.clear();
	}

	static void ScriptComponent_GetScriptName(EntityHandle entityID, MonoString** outName)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		*outName = mono_string_new(mono_domain_get(), entity.GetComponent<ScriptComponent>().ClassName.c_str());
	}

	static void ScriptComponent_SetScriptName(EntityHandle entityID, MonoString* scriptName)
	{
		char* scriptNameCStr = mono_string_to_utf8(scriptName);
		Entity entity = GetEntityByUUID(entityID);
		entity.GetComponent<ScriptComponent>().ClassName = scriptNameCStr;
		mono_free(scriptNameCStr);

		ScriptEngine::GetSceneContext()->UpdateEntityScriptName(entity);
	}

	static uint64_t SpriteRendererComponent_GetTexture(UUID entityID)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		auto& texture = entity.GetComponent<SpriteRendererComponent>().Texture;
		return texture == nullptr ? 0 : texture->Handle;
	}

	static void SpriteRendererComponent_SetTexture(UUID entityID, uint64_t handle)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);
		
		entity.GetComponent<SpriteRendererComponent>().Texture = handle == 0 ? nullptr : AssetManager::GetAsset<Texture2D>(handle);
	}

	static void SpriteRendererComponent_GetColor(UUID entityID, glm::vec4* outColor)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		*outColor = entity.GetComponent<SpriteRendererComponent>().Color;
	}

	static void SpriteRendererComponent_SetColor(UUID entityID, glm::vec4* color)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		entity.GetComponent<SpriteRendererComponent>().Color = *color;
	}

	static float SpriteRendererComponent_GetTilingFactor(UUID entityID)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		return entity.GetComponent<SpriteRendererComponent>().TilingFactor;
	}

	static void SpriteRendererComponent_SetTilingFactor(UUID entityID, float tilingFactor)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		entity.GetComponent<SpriteRendererComponent>().TilingFactor = tilingFactor;
	}

	static void CircleRendererComponent_GetColor(UUID entityID, glm::vec4* outColor)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		*outColor = entity.GetComponent<CircleRendererComponent>().Color;
	}

	static void CircleRendererComponent_SetColor(UUID entityID, glm::vec4* color)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		entity.GetComponent<CircleRendererComponent>().Color = *color;
	}

	static float CircleRendererComponent_GetThickness(UUID entityID)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		return entity.GetComponent<CircleRendererComponent>().Thickness;
	}

	static void CircleRendererComponent_SetThickness(UUID entityID, float thickness)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		entity.GetComponent<CircleRendererComponent>().Thickness = thickness;
	}

	static float CircleRendererComponent_GetFade(UUID entityID)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		return entity.GetComponent<CircleRendererComponent>().Fade;
	}

	static void CircleRendererComponent_SetFade(UUID entityID, float fade)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		entity.GetComponent<CircleRendererComponent>().Fade = fade;
	}
	
	static void TextComponent_GetText(UUID entityID, MonoString** outText)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		*outText = mono_string_new(mono_domain_get(), entity.GetComponent<TextComponent>().TextString.c_str());
	}

	static void TextComponent_SetText(UUID entityID, MonoString* text)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		char* textCStr = mono_string_to_utf8(text);
		entity.GetComponent<TextComponent>().TextString = textCStr;
		mono_free(textCStr);
	}

	static void TextComponent_GetColor(UUID entityID, glm::vec3* outColor)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		*outColor = entity.GetComponent<TextComponent>().Color;
	}

	static void TextComponent_SetColor(UUID entityID, glm::vec3* color)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		entity.GetComponent<TextComponent>().Color = glm::vec4(*color, 1.0f);
	}

	static void RigidBody2DComponent_ApplyLinearImpulse(UUID entityID, glm::vec2* impulse, glm::vec2* point, bool wake)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		auto& rb2d = entity.GetComponent<RigidBody2DComponent>();
		b2Body* body = (b2Body*)rb2d.RuntimeBody;
		body->ApplyLinearImpulse(b2Vec2(impulse->x, impulse->y), b2Vec2(point->x, point->y), wake);
	}

	static void RigidBody2DComponent_ApplyLinearImpulseToCenter(UUID entityID, glm::vec2* impulse, bool wake)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		auto& rb2d = entity.GetComponent<RigidBody2DComponent>();
		b2Body* body = (b2Body*)rb2d.RuntimeBody;
		body->ApplyLinearImpulseToCenter(b2Vec2(impulse->x, impulse->y), wake);
	}

	static AssetHandle StaticMeshComponent_GetMesh(UUID entityID)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		auto& model = entity.GetComponent<StaticMeshComponent>().m_Model;
		return model == nullptr ? 0 : model->Handle;
	}

	static void StaticMeshComponent_SetMesh(UUID entityID, AssetHandle handle)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		entity.GetComponent<StaticMeshComponent>().SetModel(handle == 0 ? nullptr : AssetManager::GetAsset<Model>(handle));
	}

	static AssetHandle StaticMeshComponent_GetAnimationGraph(UUID entityID)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		auto& player = entity.GetComponent<StaticMeshComponent>().m_AnimationGraphPlayer;
		return player == nullptr ? 0 : player->GetAnimationGraph()->Handle;

		return 0;
	}

	static void StaticMeshComponent_SetAnimationGraph(UUID entityID, AssetHandle handle)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		entity.GetComponent<StaticMeshComponent>().SetAnimationGraph(handle == 0 ? nullptr : AssetManager::GetAsset<AnimationGraph>(handle));
	}

	// Helper Function
	static bool StaticMeshComponent_SetAnimationGraphParameter(UUID entityID, MonoString* name, const AnimationGraphData& value)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		auto& player = entity.GetComponent<StaticMeshComponent>().m_AnimationGraphPlayer;
		if (!player)
		{
			DY_ERROR("Entity {} does not have a set animation graph player!");
			return false;
		}

		char* nameCStr = mono_string_to_utf8(name);
		bool result = player->SetParameter(nameCStr, value);
		mono_free(nameCStr);

		return result;
	}

	static bool StaticMeshComponent_SetAnimationGraphParameterBool(UUID entityID, MonoString* name, bool value)
	{
		return StaticMeshComponent_SetAnimationGraphParameter(entityID, name, AnimationGraphData(value));
	}

	static bool StaticMeshComponent_SetAnimationGraphParameterInt(UUID entityID, MonoString* name, int value)
	{
		return StaticMeshComponent_SetAnimationGraphParameter(entityID, name, AnimationGraphData(value));
	}

	static bool StaticMeshComponent_SetAnimationGraphParameterFloat(UUID entityID, MonoString* name, float value)
	{
		return StaticMeshComponent_SetAnimationGraphParameter(entityID, name, AnimationGraphData(value));
	}

	static bool StaticMeshComponent_SetAnimationGraphParameterVector2(UUID entityID, MonoString* name, glm::vec2* value)
	{
		return StaticMeshComponent_SetAnimationGraphParameter(entityID, name, AnimationGraphData(*value));
	}

	static bool StaticMeshComponent_SetAnimationGraphParameterVector3(UUID entityID, MonoString* name, glm::vec3* value)
	{
		return StaticMeshComponent_SetAnimationGraphParameter(entityID, name, AnimationGraphData(*value));
	}

	static bool StaticMeshComponent_SetAnimationGraphParameterVector4(UUID entityID, MonoString* name, glm::vec4* value)
	{
		return StaticMeshComponent_SetAnimationGraphParameter(entityID, name, AnimationGraphData(*value));
	}

	static bool StaticMeshComponent_SetAnimationGraphParameterTransform(UUID entityID, MonoString* name, ScriptTransform* value)
	{
		return StaticMeshComponent_SetAnimationGraphParameter(entityID, name, AnimationGraphData((Transform)*value));
	}

	static AssetHandle StaticMeshComponent_GetMaterial(UUID entityID, uint32_t slot)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		const auto& materials = entity.GetComponent<StaticMeshComponent>().m_Materials;
		if (slot >= materials.size())
			return 0;

		const auto& material = materials[slot];
		return material == nullptr ? 0 : material->Handle;
	}

	static void StaticMeshComponent_SetMaterial(UUID entityID, AssetHandle handle, uint32_t slot)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		auto& materials = entity.GetComponent<StaticMeshComponent>().m_Materials;
		if (slot >= materials.size())
			return;

		auto& material = materials[slot];
		material = handle == 0 ? nullptr : AssetManager::GetAsset<MaterialAsset>(handle);
	}

	static void StaticMeshComponent_GetBoneTransform(EntityHandle entityID, MonoString* name, ScriptTransform* outTransform)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		Entity entity = GetEntityByUUID(entityID);

		if (!entity.HasComponent<StaticMeshComponent>())
			return;

		const auto& smc = entity.GetComponent<StaticMeshComponent>();
		const Ref<AnimationGraphPlayer> player = smc.GetAnimationPlayer();
		const Ref<Skeleton> skeleton = smc.GetSkeleton();

		if (!skeleton || !player)
			return;

		char* nameCStr = mono_string_to_utf8(name);
		std::string boneName = nameCStr;
		mono_free(nameCStr);

		if (!skeleton->IsValidBoneName(boneName))
			return;

		const auto& boneTransforms = player->GetGlobalBoneMatrices();
		const auto boneID = skeleton->GetBoneID(boneName);
		if (boneID >= boneTransforms.size())
			return;

		const Transform boneTransform = Transform::ConstructTransformFromMatrix(boneTransforms[boneID]);
		*outTransform = (boneTransform * entity.GetWorldTransform());
	}

	static void DirectionalLightComponent_GetColor(UUID entityID, glm::vec3* outColor)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		*outColor = entity.GetComponent<DirectionalLightComponent>().Color;
	}

	static void DirectionalLightComponent_SetColor(UUID entityID, glm::vec3* color)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		entity.GetComponent<DirectionalLightComponent>().Color = glm::vec4(*color, 1.0f);
	}

	static float DirectionalLightComponent_GetIntensity(UUID entityID)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		return entity.GetComponent<DirectionalLightComponent>().Intensity;
	}

	static void DirectionalLightComponent_SetIntensity(UUID entityID, float intensity)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		entity.GetComponent<DirectionalLightComponent>().Intensity = intensity;
	}

	static void PointLightComponent_GetColor(UUID entityID, glm::vec3* outColor)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		*outColor = entity.GetComponent<PointLightComponent>().Color;
	}

	static void PointLightComponent_SetColor(UUID entityID, glm::vec3* color)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		entity.GetComponent<PointLightComponent>().Color = glm::vec4(*color, 1.0f);
	}

	static float PointLightComponent_GetIntensity(UUID entityID)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		return entity.GetComponent<PointLightComponent>().Intensity;
	}

	static void PointLightComponent_SetIntensity(UUID entityID, float intensity)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		entity.GetComponent<PointLightComponent>().Intensity = intensity;
	}

	static float PointLightComponent_GetRadius(UUID entityID)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		return entity.GetComponent<PointLightComponent>().Radius;
	}

	static void PointLightComponent_SetRadius(UUID entityID, float radius)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		entity.GetComponent<PointLightComponent>().Radius = radius;
	}

	static bool PointLightComponent_GetCastsShadows(UUID entityID)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		return entity.GetComponent<PointLightComponent>().CastsShadows;
	}

	static void PointLightComponent_SetCastsShadows(UUID entityID, bool castsShadows)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		entity.GetComponent<PointLightComponent>().CastsShadows = castsShadows;
	}

	static float SkyLightComponent_GetIntensity(UUID entityID)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		return entity.GetComponent<SkyLightComponent>().Intensity;
	}

	static void SkyLightComponent_SetIntensity(UUID entityID, float intensity)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		entity.GetComponent<SkyLightComponent>().Intensity = intensity;
	}

	static AssetHandle AudioComponent_GetAudio(UUID entityID)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		const auto& audio = entity.GetComponent<AudioComponent>().AudioSound;
		return audio == nullptr ? 0 : audio->Handle;
	}

	static void AudioComponent_SetAudio(UUID entityID, AssetHandle handle)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		entity.GetComponent<AudioComponent>().AudioSound = handle == 0 ? nullptr : AssetManager::GetAsset<Audio>(handle);
	}

	static uint32_t AudioComponent_GetPosition(UUID entityID)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		return entity.GetComponent<AudioComponent>().AudioSound->GetPlayPosition();
	}

	static void AudioComponent_SetPosition(UUID entityID, uint32_t position)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		entity.GetComponent<AudioComponent>().AudioSound->SetPlayPosition(position);
	}

	static float AudioComponent_GetVolume(UUID entityID)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		return entity.GetComponent<AudioComponent>().AudioSound->GetVolume();
	}

	static void AudioComponent_SetVolume(UUID entityID, float volume)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		entity.GetComponent<AudioComponent>().AudioSound->SetVolume(volume);
	}

	static bool AudioComponent_Get3D(UUID entityID)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		return entity.GetComponent<AudioComponent>().AudioSound->Is3D();
	}

	static void AudioComponent_Set3D(UUID entityID, bool is3D)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		entity.GetComponent<AudioComponent>().AudioSound->SetIs3D(is3D);
	}

	static bool AudioComponent_GetIsLooping(UUID entityID)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		return entity.GetComponent<AudioComponent>().AudioSound->IsLooping();
	}

	static void AudioComponent_SetIsLooping(UUID entityID, bool isLooping)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		entity.GetComponent<AudioComponent>().AudioSound->SetLooping(isLooping);
	}

	static bool AudioComponent_GetStartOnAwake(UUID entityID)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		return entity.GetComponent<AudioComponent>().StartOnAwake;
	}

	static void AudioComponent_SetStartOnAwake(UUID entityID, bool startOnAwake)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		entity.GetComponent<AudioComponent>().StartOnAwake = startOnAwake;
	}

	static float AudioComponent_GetPan(UUID entityID)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		return entity.GetComponent<AudioComponent>().AudioSound->GetPan();
	}

	static void AudioComponent_SetPan(UUID entityID, float pan)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		entity.GetComponent<AudioComponent>().AudioSound->SetPan(pan);
	}

	static float AudioComponent_GetSpeed(UUID entityID)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		return entity.GetComponent<AudioComponent>().AudioSound->GetSpeed();
	}

	static void AudioComponent_SetSpeed(UUID entityID, float speed)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		entity.GetComponent<AudioComponent>().AudioSound->SetSpeed(speed);
	}

	static float AudioComponent_GetEcho(UUID entityID)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		return entity.GetComponent<AudioComponent>().AudioSound->GetEcho();
	}

	static void AudioComponent_SetEcho(UUID entityID, float echo)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		DY_CORE_ASSERT(entity);

		entity.GetComponent<AudioComponent>().AudioSound->SetEcho(echo);
	}

	static void SplineComponent_GetPosition(EntityHandle entityID, uint32_t index, glm::vec3* outPosition)
	{
		*outPosition = GetEntityByUUID(entityID).GetComponent<SplineComponent>().Points.at(index).Position;
	}

	static void SplineComponent_SetPosition(EntityHandle entityID, uint32_t index, glm::vec3* position)
	{
		GetEntityByUUID(entityID).GetComponent<SplineComponent>().Points.at(index).Position = *position;
	}

	static void SplineComponent_GetTangent(EntityHandle entityID, uint32_t index, glm::vec3* outTangent)
	{
		*outTangent = GetEntityByUUID(entityID).GetComponent<SplineComponent>().Points.at(index).Tangent;
	}

	static void SplineComponent_SetTangent(EntityHandle entityID, uint32_t index, glm::vec3* tangent)
	{
		GetEntityByUUID(entityID).GetComponent<SplineComponent>().Points.at(index).Tangent = *tangent;
	}

	static uint8_t SplineComponent_GetType(EntityHandle entityID, uint32_t index)
	{
		return (uint8_t)GetEntityByUUID(entityID).GetComponent<SplineComponent>().Points.at(index).Type;
	}

	static void SplineComponent_SetType(EntityHandle entityID, uint32_t index, uint8_t type)
	{
		GetEntityByUUID(entityID).GetComponent<SplineComponent>().Points.at(index).Type = (SplineComponent::SplineType)type;
	}

	static uint32_t SplineComponent_GetPointCount(EntityHandle entityID)
	{
		return (uint8_t)GetEntityByUUID(entityID).GetComponent<SplineComponent>().Points.size();
	}

	static void SplineComponent_GetPositionWeighted(EntityHandle entityID, float weight, glm::vec3* outPosition)
	{
		*outPosition = GetEntityByUUID(entityID).GetComponent<SplineComponent>().Sample(weight);
	}

	static void SplineComponent_GetPositionDistance(EntityHandle entityID, float distance, glm::vec3* outPosition)
	{
		*outPosition = GetEntityByUUID(entityID).GetComponent<SplineComponent>().SampleDistance(distance);
	}

	namespace Utils {

		static void GetMonoStringFromPhysicsLayer(const PhysicsLayerID layerID, MonoString** outLayer)
		{
			const auto& layerName = Project::GetActiveConfig().PhysicsSettings.Layers.at(layerID).Name;
			*outLayer = mono_string_new(mono_domain_get(), layerName.c_str());
		}
	
		static PhysicsLayerID GetPhysicsLayerFromMonoString(MonoString* layerString)
		{
			PhysicsLayerID layerResult = 0;
			char* layerCStr = mono_string_to_utf8(layerString);

			const auto& layers = Project::GetActiveConfig().PhysicsSettings.Layers;

			for (const auto& [layerID, layer] : layers)
			{
				if (layer.Name == layerCStr)
				{
					layerResult = layerID;
					break;
				}
			}

			mono_free(layerCStr);
			return layerResult;
		}

	}

	static void RigidBodyComponent_GetLayer(EntityHandle entityID, MonoString** outLayer)
	{
		Utils::GetMonoStringFromPhysicsLayer(GetEntityByUUID(entityID).GetComponent<RigidBodyComponent>().Layer, outLayer);
	}

	static void RigidBodyComponent_SetLayer(EntityHandle entityID, MonoString* layer)
	{
		GetEntityByUUID(entityID).GetComponent<RigidBodyComponent>().Layer = Utils::GetPhysicsLayerFromMonoString(layer);
	}

	static bool RigidBodyComponent_IsActive(EntityHandle handle)
	{
		return GetPhysicsScene()->IsActive(GetEntityByUUID(handle));
	}

	static void RigidBodyComponent_Activate(EntityHandle handle)
	{
		GetPhysicsScene()->Activate(GetEntityByUUID(handle));
	}

	static void RigidBodyComponent_Deactivate(EntityHandle handle)
	{
		GetPhysicsScene()->Deactivate(GetEntityByUUID(handle));
	}

	static void RigidBodyComponent_SetAllowSleeping(EntityHandle handle, const bool allowSleeping)
	{
		GetPhysicsScene()->SetAllowSleeping(GetEntityByUUID(handle), allowSleeping);
	}

	static void RigidBodyComponent_ResetSleepTimer(EntityHandle handle)
	{
		GetPhysicsScene()->ResetSleepTimer(GetEntityByUUID(handle));
	}

	static void RigidBodyComponent_SetPositionWithoutActivation(EntityHandle handle, glm::vec3* position)
	{
		GetPhysicsScene()->SetPositionWithoutActivation(GetEntityByUUID(handle), *position);
	}

	static void RigidBodyComponent_SetRotationWithoutActivation(EntityHandle handle, glm::vec3* rotation)
	{
		GetPhysicsScene()->SetRotationWithoutActivation(GetEntityByUUID(handle), *rotation);
	}

	static void RigidBodyComponent_AddForce(EntityHandle handle, glm::vec3* force)
	{
		GetPhysicsScene()->AddForce(GetEntityByUUID(handle), *force);
	}

	static void RigidBodyComponent_AddForceAtPosition(EntityHandle handle, glm::vec3* force, glm::vec3* position)
	{
		GetPhysicsScene()->AddForce(GetEntityByUUID(handle), *force, *position);
	}

	static void RigidBodyComponent_AddImpulse(EntityHandle handle, glm::vec3* impulse)
	{
		GetPhysicsScene()->AddImpulse(GetEntityByUUID(handle), *impulse);
	}

	static void RigidBodyComponent_AddImpulseAtPosition(EntityHandle handle, glm::vec3* impulse, glm::vec3* position)
	{
		GetPhysicsScene()->AddImpulse(GetEntityByUUID(handle), *impulse, *position);
	}

	static void RigidBodyComponent_AddAngularImpulse(EntityHandle handle, glm::vec3* angularImpulse)
	{
		GetPhysicsScene()->AddAngularImpulse(GetEntityByUUID(handle), *angularImpulse);
	}

	static void RigidBodyComponent_AddTorque(EntityHandle handle, glm::vec3* torque)
	{
		GetPhysicsScene()->AddTorque(GetEntityByUUID(handle), *torque);
	}

	static void RigidBodyComponent_AddForceAndTorque(EntityHandle handle, glm::vec3* force, glm::vec3* torque)
	{
		GetPhysicsScene()->AddForceAndTorque(GetEntityByUUID(handle), *force, *torque);
	}

	static void RigidBodyComponent_ApplyBuoyancyImpulse(EntityHandle handle, glm::vec3* surfacePosition, glm::vec3* surfaceNormal, float buoyancy, float linearDrag, float angularDrag, glm::vec3* fluidVelocity, float deltaTime)
	{
		GetPhysicsScene()->ApplyBuoyancyImpulse(GetEntityByUUID(handle), *surfacePosition, *surfaceNormal, buoyancy, linearDrag, angularDrag, *fluidVelocity, deltaTime);
	}

	static void RigidBodyComponent_MoveKinematic(EntityHandle handle, glm::vec3* position, glm::vec3* rotation, float deltaTime)
	{
		GetPhysicsScene()->MoveKinematic(GetEntityByUUID(handle), *position, *rotation, deltaTime);
	}

	static void RigidBodyComponent_GetCenterOfMassPosition(EntityHandle handle, glm::vec3* outPosition)
	{
		*outPosition = GetPhysicsScene()->GetCenterOfMassPosition(GetEntityByUUID(handle));
	}

	static void RigidBodyComponent_GetAccumulatedForce(EntityHandle handle, glm::vec3* outForce)
	{
		*outForce = GetPhysicsScene()->GetAccumulatedForce(GetEntityByUUID(handle));
	}

	static void RigidBodyComponent_GetAccumulatedTorque(EntityHandle handle, glm::vec3* outTorque)
	{
		*outTorque = GetPhysicsScene()->GetAccumulatedTorque(GetEntityByUUID(handle));
	}

	static void RigidBodyComponent_ResetForce(EntityHandle handle)
	{
		GetPhysicsScene()->ResetForce(GetEntityByUUID(handle));
	}

	static void RigidBodyComponent_ResetTorque(EntityHandle handle)
	{
		GetPhysicsScene()->ResetTorque(GetEntityByUUID(handle));
	}

	static void RigidBodyComponent_ResetMotion(EntityHandle handle)
	{
		GetPhysicsScene()->ResetMotion(GetEntityByUUID(handle));
	}

	static void RigidBodyComponent_GetLinearVelocity(EntityHandle handle, glm::vec3* outLinearVelocity)
	{
		*outLinearVelocity = GetPhysicsScene()->GetLinearVelocity(GetEntityByUUID(handle));
	}

	static void RigidBodyComponent_SetLinearVelocity(EntityHandle handle, glm::vec3* linearVelocity)
	{
		GetPhysicsScene()->SetLinearVelocity(GetEntityByUUID(handle), *linearVelocity);
	}

	static void RigidBodyComponent_AddLinearVelocity(EntityHandle handle, glm::vec3* linearVelocity)
	{
		GetPhysicsScene()->AddLinearVelocity(GetEntityByUUID(handle), *linearVelocity);
	}

	static void RigidBodyComponent_GetAngularVelocity(EntityHandle handle, glm::vec3* outAngularVelocity)
	{
		*outAngularVelocity = GetPhysicsScene()->GetAngularVelocity(GetEntityByUUID(handle));
	}

	static void RigidBodyComponent_SetAngularVelocity(EntityHandle handle, glm::vec3* angularVelocity)
	{
		GetPhysicsScene()->SetAngularVelocity(GetEntityByUUID(handle), *angularVelocity);
	}

	static void RigidBodyComponent_GetLinearAndAngularVelocity(EntityHandle handle, glm::vec3* outLinearVelocity, glm::vec3* outAngularVelocity)
	{
		GetPhysicsScene()->GetLinearAndAngularVelocity(GetEntityByUUID(handle), *outLinearVelocity, *outAngularVelocity);
	}

	static void RigidBodyComponent_SetLinearAndAngularVelocity(EntityHandle handle, glm::vec3* linearVelocity, glm::vec3* angularVelocity)
	{
		GetPhysicsScene()->SetLinearAndAngularVelocity(GetEntityByUUID(handle), *linearVelocity, *angularVelocity);
	}

	static void RigidBodyComponent_AddLinearAndAngularVelocity(EntityHandle handle, glm::vec3* linearVelocity, glm::vec3* angularVelocity)
	{
		GetPhysicsScene()->AddLinearAndAngularVelocity(GetEntityByUUID(handle), *linearVelocity, *angularVelocity);
	}

	static void RigidBodyComponent_GetPointVelocity(EntityHandle handle, glm::vec3* point, glm::vec3* outVelocity)
	{
		*outVelocity = GetPhysicsScene()->GetPointVelocity(GetEntityByUUID(handle), *point);
	}

	static void SoftBodyComponent_GetLayer(EntityHandle entityID, MonoString** outLayer)
	{
		Utils::GetMonoStringFromPhysicsLayer(GetEntityByUUID(entityID).GetComponent<SoftBodyComponent>().Layer, outLayer);
	}

	static void SoftBodyComponent_SetLayer(EntityHandle entityID, MonoString* layer)
	{
		GetEntityByUUID(entityID).GetComponent<SoftBodyComponent>().Layer = Utils::GetPhysicsLayerFromMonoString(layer);
	}

	static void SoftBodyComponent_SetVertexPosition(EntityHandle entityID, const uint32_t index, glm::vec3* position)
	{
		GetPhysicsScene()->SetVertexPosition(GetEntityByUUID(entityID), index, *position);
	}

	static void SoftBodyComponent_SetVertexVelocity(EntityHandle entityID, const uint32_t index, glm::vec3* velocity)
	{
		GetPhysicsScene()->SetVertexVelocity(GetEntityByUUID(entityID), index, *velocity);
	}

	static void SoftBodyComponent_SetPositionWeighted(EntityHandle entityID, glm::vec3* position)
	{
		GetPhysicsScene()->SetPositionWeighted(GetEntityByUUID(entityID), *position);
	}

	static void SoftBodyComponent_GetCenterOfMassPosition(EntityHandle entityID, glm::vec3* outPosition)
	{
		// Note: This is identical to RigidBody call so we can definetly unify this interface in the future
		*outPosition = GetPhysicsScene()->GetCenterOfMassPosition(GetEntityByUUID(entityID));
	}

	static void SoftBodyComponent_SetFixedPosition(EntityHandle entityID, bool fixedPosition)
	{
		GetPhysicsScene()->SetFixedPosition(GetEntityByUUID(entityID), fixedPosition);
	}

	static void CharacterMovementComponent_GetLayer(EntityHandle entityID, MonoString** outLayer)
	{
		Utils::GetMonoStringFromPhysicsLayer(GetEntityByUUID(entityID).GetComponent<CharacterMovementComponent>().Layer, outLayer);
	}

	static void CharacterMovementComponent_SetLayer(EntityHandle entityID, MonoString* layer)
	{
		GetEntityByUUID(entityID).GetComponent<CharacterMovementComponent>().Layer = Utils::GetPhysicsLayerFromMonoString(layer);
	}

	static float CharacterMovementComponent_GetMaxWalkSpeed(EntityHandle entityID)
	{
		return GetEntityByUUID(entityID).GetComponent<CharacterMovementComponent>().MaxWalkSpeed;
	}

	static void CharacterMovementComponent_SetMaxWalkSpeed(EntityHandle entityID, float maxWalkSpeed)
	{
		GetEntityByUUID(entityID).GetComponent<CharacterMovementComponent>().MaxWalkSpeed = maxWalkSpeed;
	}

	static float CharacterMovementComponent_GetJumpSpeed(EntityHandle entityID)
	{
		return GetEntityByUUID(entityID).GetComponent<CharacterMovementComponent>().JumpSpeed;
	}

	static void CharacterMovementComponent_SetJumpSpeed(EntityHandle entityID, float jumpSpeed)
	{
		GetEntityByUUID(entityID).GetComponent<CharacterMovementComponent>().JumpSpeed = jumpSpeed;
	}

	static float CharacterMovementComponent_GetGravityScale(EntityHandle entityID)
	{
		return GetEntityByUUID(entityID).GetComponent<CharacterMovementComponent>().GravityScale;
	}

	static void CharacterMovementComponent_SetGravityScale(EntityHandle entityID, float gravityScale)
	{
		GetEntityByUUID(entityID).GetComponent<CharacterMovementComponent>().GravityScale = gravityScale;
	}

	static float CharacterMovementComponent_GetMaxSlopeAngle(EntityHandle entityID)
	{
		return GetEntityByUUID(entityID).GetComponent<CharacterMovementComponent>().MaxSlopeAngle;
	}

	static void CharacterMovementComponent_SetMaxSlopeAngle(EntityHandle entityID, float maxSlopeAngle)
	{
		GetEntityByUUID(entityID).GetComponent<CharacterMovementComponent>().MaxSlopeAngle = maxSlopeAngle;
	}

	static float CharacterMovementComponent_GetAirControl(EntityHandle entityID)
	{
		return GetEntityByUUID(entityID).GetComponent<CharacterMovementComponent>().AirControl;
	}

	static void CharacterMovementComponent_SetAirControl(EntityHandle entityID, float airControl)
	{
		GetEntityByUUID(entityID).GetComponent<CharacterMovementComponent>().AirControl = airControl;
	}

	static float CharacterMovementComponent_GetVelocityBlendWeight(EntityHandle entityID)
	{
		return GetEntityByUUID(entityID).GetComponent<CharacterMovementComponent>().VelocityBlendWeight;
	}

	static void CharacterMovementComponent_SetVelocityBlendWeight(EntityHandle entityID, float velocityBlendWeight)
	{
		GetEntityByUUID(entityID).GetComponent<CharacterMovementComponent>().VelocityBlendWeight = velocityBlendWeight;
	}

	static bool CharacterMovementComponent_GetRotateToMotion(EntityHandle entityID)
	{
		return GetEntityByUUID(entityID).GetComponent<CharacterMovementComponent>().RotateToMotion;
	}

	static void CharacterMovementComponent_SetRotateToMotion(EntityHandle entityID, bool rotateToMotion)
	{
		GetEntityByUUID(entityID).GetComponent<CharacterMovementComponent>().RotateToMotion = rotateToMotion;
	}

	static float CharacterMovementComponent_GetRotationRate(EntityHandle entityID)
	{
		return GetEntityByUUID(entityID).GetComponent<CharacterMovementComponent>().RotationRate;
	}

	static void CharacterMovementComponent_SetRotationRate(EntityHandle entityID, float rotationRate)
	{
		GetEntityByUUID(entityID).GetComponent<CharacterMovementComponent>().RotationRate = rotationRate;
	}

	static void CharacterMovementComponent_SetMovementInput(EntityHandle entityID, glm::vec3* movement)
	{
		GetEntityByUUID(entityID).GetComponent<CharacterMovementComponent>().RuntimeMovementDirection = *movement;
	}

	static void CharacterMovementComponent_Jump(EntityHandle entityID)
	{
		GetEntityByUUID(entityID).GetComponent<CharacterMovementComponent>().RuntimeJump = true;
	}

	static bool CharacterMovementComponent_IsFalling(EntityHandle entityID)
	{
		return GetEntityByUUID(entityID).GetComponent<CharacterMovementComponent>().IsFalling;
	}

	static void CharacterMovementComponent_GetLinearVelocity(EntityHandle entityID, glm::vec3* outLinearVelocity)
	{
		*outLinearVelocity = GetEntityByUUID(entityID).GetComponent<CharacterMovementComponent>().RuntimeLinearVelocity;
	}

	static void CharacterMovementComponent_GetGroundVelocity(EntityHandle entityID, glm::vec3* outGroundVelocity)
	{
		*outGroundVelocity = GetEntityByUUID(entityID).GetComponent<CharacterMovementComponent>().RuntimeGroundVelocity;
	}

	static float SpringArmComponent_GetTargetLength(EntityHandle entityID)
	{
		return GetEntityByUUID(entityID).GetComponent<SpringArmComponent>().TargetLength;
	}

	static void SpringArmComponent_SetTargetLength(EntityHandle entityID, float targetLength)
	{
		GetEntityByUUID(entityID).GetComponent<SpringArmComponent>().TargetLength = targetLength;
	}

	static float SpringArmComponent_GetProbeRadius(EntityHandle entityID)
	{
		return GetEntityByUUID(entityID).GetComponent<SpringArmComponent>().ProbeRadius;
	}

	static void SpringArmComponent_SetProbeRadius(EntityHandle entityID, float probeRadius)
	{
		GetEntityByUUID(entityID).GetComponent<SpringArmComponent>().ProbeRadius = probeRadius;
	}

	static void SpringArmComponent_GetTargetOffset(EntityHandle entityID, glm::vec3* outTargetOffset)
	{
		*outTargetOffset = GetEntityByUUID(entityID).GetComponent<SpringArmComponent>().TargetOffset;
	}

	static void SpringArmComponent_SetTargetOffset(EntityHandle entityID, glm::vec3* targetOffset)
	{
		GetEntityByUUID(entityID).GetComponent<SpringArmComponent>().TargetOffset = *targetOffset;
	}

	static void SpringArmComponent_GetSocketOffset(EntityHandle entityID, glm::vec3* outSocketOffset)
	{
		*outSocketOffset = GetEntityByUUID(entityID).GetComponent<SpringArmComponent>().SocketOffset;
	}

	static void SpringArmComponent_SetSocketOffset(EntityHandle entityID, glm::vec3* socketOffset)
	{
		GetEntityByUUID(entityID).GetComponent<SpringArmComponent>().SocketOffset = *socketOffset;
	}

	static void SpringArmComponent_ExcludeEntity(EntityHandle entityID, EntityHandle exclusionID)
	{
		if (!ScriptEngine::GetSceneContext()->DoesEntityExist(exclusionID))
			return;

		GetEntityByUUID(entityID).GetComponent<SpringArmComponent>().ExclusionMask->insert(exclusionID);
	}

	static uint8_t FieldComponent_GetType(EntityHandle entityID)
	{
		return (uint8_t)GetEntityByUUID(entityID).GetComponent<FieldComponent>().Type;
	}

	static void FieldComponent_SetType(EntityHandle entityID, uint8_t type)
	{
		GetEntityByUUID(entityID).GetComponent<FieldComponent>().Type = (FieldComponent::FieldType)type;
	}

	static void FieldComponent_GetLayer(EntityHandle entityID, MonoString** outLayer)
	{
		Utils::GetMonoStringFromPhysicsLayer(GetEntityByUUID(entityID).GetComponent<FieldComponent>().Layer, outLayer);
	}

	static void FieldComponent_SetLayer(EntityHandle entityID, MonoString* layer)
	{
		GetEntityByUUID(entityID).GetComponent<FieldComponent>().Layer = Utils::GetPhysicsLayerFromMonoString(layer);
	}

	static void FieldComponent_GetForce(EntityHandle entityID, glm::vec3* force)
	{
		*force = GetEntityByUUID(entityID).GetComponent<FieldComponent>().Force;
	}

	static void FieldComponent_SetFroce(EntityHandle entityID, glm::vec3* force)
	{
		GetEntityByUUID(entityID).GetComponent<FieldComponent>().Force = *force;
	}

	static float FieldComponent_GetMagnitude(EntityHandle entityID)
	{
		return GetEntityByUUID(entityID).GetComponent<FieldComponent>().Magnitude;
	}

	static void FieldComponent_SetMagnitude(EntityHandle entityID, float magnitude)
	{
		GetEntityByUUID(entityID).GetComponent<FieldComponent>().Magnitude = magnitude;
	}

	static float FieldComponent_GetRadius(EntityHandle entityID)
	{
		return GetEntityByUUID(entityID).GetComponent<FieldComponent>().Radius;
	}

	static void FieldComponent_SetRadius(EntityHandle entityID, float radius)
	{
		GetEntityByUUID(entityID).GetComponent<FieldComponent>().Radius = radius;
	}

	static float FieldComponent_GetFallof(EntityHandle entityID)
	{
		return GetEntityByUUID(entityID).GetComponent<FieldComponent>().Falloff;
	}

	static void FieldComponent_SetFallof(EntityHandle entityID, float falloff)
	{
		GetEntityByUUID(entityID).GetComponent<FieldComponent>().Falloff = falloff;
	}

	static float FieldComponent_GetBuoyancy(EntityHandle entityID)
	{
		return GetEntityByUUID(entityID).GetComponent<FieldComponent>().Buoyancy;
	}

	static void FieldComponent_SetBuoyancy(EntityHandle entityID, float buoyancy)
	{
		GetEntityByUUID(entityID).GetComponent<FieldComponent>().Buoyancy = buoyancy;
	}

	static float FieldComponent_GetLinearDrag(EntityHandle entityID)
	{
		return GetEntityByUUID(entityID).GetComponent<FieldComponent>().LinearDrag;
	}

	static void FieldComponent_SetLinearDrag(EntityHandle entityID, float linearDrag)
	{
		GetEntityByUUID(entityID).GetComponent<FieldComponent>().LinearDrag = linearDrag;
	}

	static float FieldComponent_GetAngularDrag(EntityHandle entityID)
	{
		return GetEntityByUUID(entityID).GetComponent<FieldComponent>().AngularDrag;
	}

	static void FieldComponent_SetAngularDrag(EntityHandle entityID, float angularDrag)
	{
		GetEntityByUUID(entityID).GetComponent<FieldComponent>().AngularDrag = angularDrag;
	}

	static void FieldComponent_GetFluidVelocity(EntityHandle entityID, glm::vec3* fluidVelocity)
	{
		*fluidVelocity = GetEntityByUUID(entityID).GetComponent<FieldComponent>().FluidVelocity;
	}

	static void FieldComponent_SetFluidVelocity(EntityHandle entityID, glm::vec3* fluidVelocity)
	{
		GetEntityByUUID(entityID).GetComponent<FieldComponent>().FluidVelocity = *fluidVelocity;
	}

	static bool DistanceConstraintComponent_GetEnabled(EntityHandle entityID)
	{
		return GetEntityByUUID(entityID).GetComponent<DistanceConstraintComponent>().Enabled;
	}

	static void DistanceConstraintComponent_SetEnabled(EntityHandle entityID, bool enabled)
	{
		DistanceConstraintComponent& dcc = GetEntityByUUID(entityID).GetComponent<DistanceConstraintComponent>();
		dcc.Enabled = enabled;
		GetPhysicsScene()->UpdateConstraintEnabled(&dcc);
	}

	static MonoObject* DistanceConstraintComponent_GetTarget(EntityHandle entityID)
	{
		return ScriptEngine::GetManagedInstanceOrDefaultEntity(GetEntityByUUID(entityID).GetComponent<DistanceConstraintComponent>().Target);
	}

	static void DistanceConstraintComponent_SetTarget(EntityHandle entityID, EntityHandle target)
	{
		Entity entity = GetEntityByUUID(entityID);
		entity.GetComponent<DistanceConstraintComponent>().Target = target;
		GetPhysicsScene()->RebuildComponent<DistanceConstraintComponent>(entity);
	}

	static void DistanceConstraintComponent_SetDistance(EntityHandle entityID, float distance)
	{
		DistanceConstraintComponent& dcc = GetEntityByUUID(entityID).GetComponent<DistanceConstraintComponent>();
		dcc.Type = DistanceConstraintComponent::DistanceType::Fixed;
		dcc.Distance = distance;
		GetPhysicsScene()->UpdateDistanceConstraintDistance(dcc);
	}

	static void DistanceConstraintComponent_SetDistanceRange(EntityHandle entityID, float minDistance, float maxDistance)
	{
		DistanceConstraintComponent& dcc = GetEntityByUUID(entityID).GetComponent<DistanceConstraintComponent>();
		dcc.Type = DistanceConstraintComponent::DistanceType::Range;
		dcc.MinDistance = minDistance;
		dcc.MaxDistance = maxDistance;
		GetPhysicsScene()->UpdateDistanceConstraintDistance(dcc);
	}

	static bool FixedConstraintComponent_GetEnabled(EntityHandle entityID)
	{
		return GetEntityByUUID(entityID).GetComponent<FixedConstraintComponent>().Enabled;
	}

	static void FixedConstraintComponent_SetEnabled(EntityHandle entityID, bool enabled)
	{
		FixedConstraintComponent& dcc = GetEntityByUUID(entityID).GetComponent<FixedConstraintComponent>();
		dcc.Enabled = enabled;
		GetPhysicsScene()->UpdateConstraintEnabled(&dcc);
	}

	static MonoObject* FixedConstraintComponent_GetTarget(EntityHandle entityID)
	{
		return ScriptEngine::GetManagedInstanceOrDefaultEntity(GetEntityByUUID(entityID).GetComponent<FixedConstraintComponent>().Target);
	}

	static void FixedConstraintComponent_SetTarget(EntityHandle entityID, EntityHandle targetID)
	{
		Entity entity = GetEntityByUUID(entityID);
		entity.GetComponent<FixedConstraintComponent>().Target = targetID;
		GetPhysicsScene()->RebuildComponent<FixedConstraintComponent>(entity);
	}

	static void Input_GetKeyboardName(MonoString** outString)
	{
		*outString = mono_string_new(mono_domain_get(), Input::GetKeyboardName().c_str());
	}

	static bool Input_IsKeyDown(KeyCode keycode)
	{
		return Input::IsKeyPressed(keycode);
	}

	static void Input_GetMouseName(MonoString** outString)
	{
		*outString = mono_string_new(mono_domain_get(), Input::GetMouseName().c_str());
	}

	static bool Input_IsMouseButtonPressed(MouseCode button)
	{
		return Input::IsMouseButtonPressed(button);
	}
	
	static void Input_GetMousePosition(glm::vec2* outPosition)
	{
		*outPosition = Input::GetMousePosition();
	}

	static void Input_GetMouseDelta(glm::vec2* outDelta)
	{
		*outDelta = Input::GetMouseDelta();
	}

	static float Input_GetMouseX()
	{
		return Input::GetMouseX();
	}

	static float Input_GetMouseY()
	{
		return Input::GetMouseY();
	}

	static void Input_SetMouseLocked(bool locked)
	{
		Application::Get().GetWindow().LockCursor(locked);
	}

	static uint32_t Input_GetGamepadCount()
	{
		return Input::GetGamepadCount();
	}

	static bool Input_IsGamepadConnected(int gamepad)
	{
		return Input::IsGamepadConnected(gamepad);
	}

	static void Input_GetGamepadName(int gamepad, MonoString** outName)
	{
		*outName = mono_string_new(mono_domain_get(), Input::GetGamepadName(gamepad).c_str());
	}

	static int Input_GetGamepadPowerLevel(int gamepad)
	{
		return (int)Input::GetGamepadPowerLevel(gamepad);
	}

	static bool Input_IsGamepadButtonPressed(int gamepad, GamepadButtonCode button)
	{
		return Input::IsGamepadButtonPressed(gamepad, button);
	}

	static float Input_GetGamepadAxis(int gamepad, GamepadAxisCode axis)
	{
		return Input::GetGamepadAxis(gamepad, axis);
	}

	static void Input_GetGamepadSensor(int gamepad, GamepadSensorCode sensor, glm::vec3* outValue)
	{
		*outValue = Input::GetGamepadSensor(gamepad, sensor);
	}

	static void Input_GetGamepadTouchPad(int gamepad, glm::vec2* outValue)
	{
		*outValue = Input::GetGamepadTouchPad(gamepad);
	}

	static bool Input_SetGamepadRumble(int gamepad, float left, float right, float duration)
	{
		return Input::SetGamepadRumble(gamepad, left, right, duration);
	}

	static void Input_SetGamepadLED(int gamepad, glm::vec3* color)
	{
		Input::SetGamepadLED(gamepad, *color);
	}

	static void Input_SetGamepadPlayerLED(int gamepad, int brightnessLevel, int count, bool fadeIn)
	{
		Input::SetGamepadPlayerLED(gamepad, (Input::BrightnessLevel)brightnessLevel, count, fadeIn);
	}

	static void Input_SetGamepadMicrophoneLED(int gamepad, bool enabled, bool pulse)
	{
		Input::SetGamepadMicrophoneLED(gamepad, enabled, pulse);
	}
	
	static void Input_ClearTriggerEffect(int gamepad, GamepadButtonCode trigger)
	{
		Input::ClearTriggerEffect(gamepad, trigger);
	}

	static void Input_SetTriggerEffect(int gamepad, GamepadButtonCode trigger, float startPosition, bool keepEffect, float beginForce, float middleForce, float endForce, float frequency)
	{
		Input::SetTriggerEffect(gamepad, trigger, startPosition, keepEffect, beginForce, middleForce, endForce, frequency);
	}

	static void Input_SetTriggerEffectContinuous(int gamepad, GamepadButtonCode trigger, float startPosition, float force)
	{
		Input::SetTriggerEffectContinuous(gamepad, trigger, startPosition, force);
	}

	static void Input_SetTriggerEffectSection(int gamepad, GamepadButtonCode trigger, float startPosition, float endPosition)
	{
		Input::SetTriggerEffectSection(gamepad, trigger, startPosition, endPosition);
	}

	namespace Utils {
		
		static void ConvertRaycastHitEntityIDToMonoEntity(RaycastHit* hit)
		{
			// Warning: This is pretty bad. We directly reuse C++ side ID field to represent mono script pointer in C#. It does work though :)
			hit->EntityID = (uint64_t)ScriptEngine::GetManagedInstanceOrDefaultEntity(hit->EntityID);
		}

		static MonoArray* ConvertRaycastHitListToMono(std::vector<RaycastHit>& hits)
		{
			MonoClass* monoClass = mono_class_from_name(ScriptEngine::GetCoreAssemblyImage(), "Dymatic", "RaycastHit");
			MonoArray* monoArray = mono_array_new(ScriptEngine::GetApplicationDomain(), monoClass, hits.size());

			for (size_t i = 0; i < hits.size(); i++)
			{
				Utils::ConvertRaycastHitEntityIDToMonoEntity(&hits[i]);
				mono_array_set(monoArray, RaycastHit, i, hits[i]);
			}

			return monoArray;
		}

		template<typename ShapeFunction, typename... Args>
		static void ShapeCast(ShapeFunction function, RaycastHit* outHit, Args... args)
		{
			PhysicsScene& physicsScene = *ScriptEngine::GetSceneContext()->GetPhysicsScene();
			*outHit = (physicsScene.*function)(std::forward<Args>(args)...);
			Utils::ConvertRaycastHitEntityIDToMonoEntity(outHit);
		}

		template<typename ShapeFunction, typename... Args>
		static MonoArray* ShapeCastMultihit(ShapeFunction function, Args... args)
		{
			PhysicsScene& physicsScene = *ScriptEngine::GetSceneContext()->GetPhysicsScene();

			std::vector<RaycastHit> hits;
			(physicsScene.*function)(std::forward<Args>(args)..., std::ref(hits));
			return Utils::ConvertRaycastHitListToMono(hits);
		}

	}
	
	static void Physics_Raycast(glm::vec3* origin, glm::vec3* direction, float distance, RaycastHit* outHit)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);

		*outHit = scene->Raycast(*origin, *direction, distance);
		outHit->EntityID = (uint64_t)ScriptEngine::GetManagedInstanceOrDefaultEntity(outHit->EntityID);
	}

	static MonoArray* Physics_RaycastMultihit(glm::vec3* origin, glm::vec3* direction, float distance, uint64_t* outEntityID)
	{
		std::vector<RaycastHit> hits;
		ScriptEngine::GetSceneContext()->GetPhysicsScene()->RaycastMultihit(*origin, *direction, distance, hits);
		return Utils::ConvertRaycastHitListToMono(hits);
	}

	static void Physics_RaycastPoints(glm::vec3* start, glm::vec3* end, RaycastHit* outHit)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);

		*outHit = scene->Raycast(*start, *end);
		Utils::ConvertRaycastHitEntityIDToMonoEntity(outHit);
	}

	static void Physics_BoxShapeCast(glm::vec3* start, glm::vec3* direction, float distance, glm::vec3* halfSize, glm::vec3* orientation, RaycastHit* outHit)
	{
		Utils::ShapeCast(&PhysicsScene::BoxShapeCast, outHit, *start, *direction, distance, *halfSize, *orientation);
	}

	static MonoArray* Physics_BoxShapeCastMultihit(glm::vec3* start, glm::vec3* direction, float distance, glm::vec3* halfSize, glm::vec3* orientation)
	{
		return Utils::ShapeCastMultihit(&PhysicsScene::BoxShapeCastMultihit, *start, *direction, distance, *halfSize, *orientation);
	}

	static void Physics_CapsuleShapeCast(glm::vec3* start, glm::vec3* direction, float distance, float radius, float halfHeight, RaycastHit* outHit)
	{
		Utils::ShapeCast(&PhysicsScene::CapsuleShapeCast, outHit, *start, *direction, distance, radius, halfHeight);
	}

	static MonoArray* Physics_CapsuleShapeCastMultihit(glm::vec3* start, glm::vec3* direction, float distance, float radius, float halfHeight)
	{
		return Utils::ShapeCastMultihit(&PhysicsScene::CapsuleShapeCastMultihit, *start, *direction, distance, radius, halfHeight);
	}

	static void Physics_SphereShapeCast(glm::vec3* start, glm::vec3* direction, float distance, float radius, RaycastHit* outHit)
	{
		Utils::ShapeCast(&PhysicsScene::SphereShapeCast, outHit, *start, *direction, distance, radius);
	}

	static MonoArray* Physics_SphereShapeCastMultihit(glm::vec3* start, glm::vec3* direction, float distance, float radius)
	{
		return Utils::ShapeCastMultihit(&PhysicsScene::SphereShapeCastMultihit, *start, *direction, distance, radius);
	}

	static MonoArray* Physics_GetEntitiesInBounds(glm::vec3* min, glm::vec3* max)
	{
		std::vector<EntityHandle> entityIDs;
		GetPhysicsScene()->GetEntitiesInBounds(*min, *max, entityIDs);
		return Utils::GetEntityMonoArrayFromIDs(entityIDs);
	}

	static MonoArray* Physics_GetEntitiesInBoundsOnLayer(glm::vec3* min, glm::vec3* max, MonoString* layer)
	{
		std::vector<EntityHandle> entityIDs;
		GetPhysicsScene()->GetEntitiesInBounds(*min, *max, Utils::GetPhysicsLayerFromMonoString(layer), entityIDs);
		return Utils::GetEntityMonoArrayFromIDs(entityIDs);
	}
	
	static void Debug_DrawDebugLine(glm::vec3* start, glm::vec3* end, glm::vec3* color, float duration)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);

		scene->DrawDebugLine(*start, *end, glm::vec4(*color, 1.0f), duration);
	}

	static void Debug_DrawDebugCube(glm::vec3* position, glm::vec3* size, glm::vec3* color, float duration)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);

		scene->DrawDebugCube(*position, *size, glm::vec4(*color, 1.0f), duration);
	}

	static void Debug_DrawDebugSphere(glm::vec3* center, float radius, glm::vec3* color, float duration)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);

		scene->DrawDebugSphere(*center, radius, glm::vec4(*color, 1.0f), duration);
	}

	static void Debug_ClearDebugDrawing()
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		DY_CORE_ASSERT(scene);

		scene->ClearDebugDrawing();
	}

	static void Log_Trace(MonoString* message) 
	{
		if (!message)
			return;

		char* messageCStr = mono_string_to_utf8(message);
		DY_TRACE(messageCStr);
		mono_free(messageCStr);
	}
	
	static void Log_Info(MonoString* message)
	{
		if (!message)
			return;

		char* messageCStr = mono_string_to_utf8(message);
		DY_INFO(messageCStr);
		mono_free(messageCStr);
	}

	static void Log_Warn(MonoString* message)
	{
		if (!message)
			return;

		char* messageCStr = mono_string_to_utf8(message);
		DY_WARN(messageCStr);
		mono_free(messageCStr);
	}
	
	static void Log_Error(MonoString* message)
	{
		if (!message)
			return;

		char* messageCStr = mono_string_to_utf8(message);
		DY_ERROR(messageCStr);
		mono_free(messageCStr);
	}

	static void Log_Critical(MonoString* message)
	{
		if (!message)
			return;

		char* messageCStr = mono_string_to_utf8(message);
		DY_CRITICAL(messageCStr);
		mono_free(messageCStr);
	}

	static void Core_Assert(bool condition, MonoString* message)
	{
		if (!message)
			return;

		char* messageCStr = mono_string_to_utf8(message);
		DY_ASSERT(condition, messageCStr);
		mono_free(messageCStr);
	}

	static void Math_MultiplyRotationVector(glm::vec3* rotation, glm::vec3* vector, glm::vec3* result)
	{
		*result = glm::quat(glm::radians(*rotation)) * (*vector);
	}

	static Buffer s_ScratchBuffer(1024);

	static void Network_ReplicateMethod(UUID entityID, MonoString* methodName, MonoArray* parameters)
	{
		uint32_t size = mono_array_length(parameters);
		void* data = mono_array_addr_with_size(parameters, sizeof(void*), 0);

		char* methodNameCStr = mono_string_to_utf8(methodName);

		BufferStreamWriter stream(s_ScratchBuffer);
		stream.WriteRaw<PacketType>(PacketType::ScriptMessage);
		stream.WriteRaw<UUID>(entityID);
		stream.WriteString(methodNameCStr);
		stream.WriteData((char*)data, size);

		mono_free(methodNameCStr);
		
		NetworkManager::Send(s_ScratchBuffer);
	}

	template<typename... Component>
	static void RegisterComponent()
	{
		([]()
		{
			std::string_view typeName = typeid(Component).name();
			size_t pos = typeName.find_last_of(':');
			std::string_view structName = typeName.substr(pos + 1);
			std::string managedTypename = fmt::format("Dymatic.{}", structName);

			MonoType* managedType = mono_reflection_type_from_name(managedTypename.data(), ScriptEngine::GetCoreAssemblyImage());
			if (!managedType)
			{
				DY_CORE_ERROR("Could not find component type {}", managedTypename);
				return;
			}

			s_EntityAddComponentFuncs[managedType] = [](Entity entity) { entity.AddComponent<Component>(); };
			s_EntityHasComponentFuncs[managedType] = [](Entity entity) { return entity.HasComponent<Component>(); };
			s_EntityRemoveComponentFuncs[managedType] = [](Entity entity) { entity.RemoveComponent<Component>(); };
		}(), ...);
	}

	template<typename... Component>
	static void RegisterComponent(ComponentGroup<Component...>)
	{
		RegisterComponent<Component...>();
	}

	void ScriptGlue::RegisterComponents()
	{
		s_EntityAddComponentFuncs.clear();
		s_EntityHasComponentFuncs.clear();
		s_EntityRemoveComponentFuncs.clear();
		
		if (ScriptEngine::GetCoreAssemblyImage())
			RegisterComponent(AllComponents{});
	}

	void ScriptGlue::RegisterFunctions()
	{
		DY_ADD_INTERNAL_CALL(GetScriptInstance);

		DY_ADD_INTERNAL_CALL(Scene_OpenScene);
		
		DY_ADD_INTERNAL_CALL(Asset_RegisterScriptReference);
		DY_ADD_INTERNAL_CALL(Asset_UnregisterScriptReference);
		DY_ADD_INTERNAL_CALL(Asset_DoesAssetExist);
		DY_ADD_INTERNAL_CALL(Asset_GetAssetHandle);

		DY_ADD_INTERNAL_CALL(Prefab_Instantiate);
		DY_ADD_INTERNAL_CALL(Prefab_InstantiateAtTransform);

		DY_ADD_INTERNAL_CALL(Material_CreateInstance);
		DY_ADD_INTERNAL_CALL(MaterialInstance_SetParameterFloat);
		DY_ADD_INTERNAL_CALL(MaterialInstance_SetParameterVector2);
		DY_ADD_INTERNAL_CALL(MaterialInstance_SetParameterVector3);
		DY_ADD_INTERNAL_CALL(MaterialInstance_SetParameterVector4);

		DY_ADD_INTERNAL_CALL(VirtualTexture_Resize);
		DY_ADD_INTERNAL_CALL(VirtualTexture_Clear);
		DY_ADD_INTERNAL_CALL(VirtualTexture_GetSize);
		DY_ADD_INTERNAL_CALL(VirtualTexture_GetPixelCount);
		DY_ADD_INTERNAL_CALL(VirtualTexture_GetDataSize);
		DY_ADD_INTERNAL_CALL(VirtualTexture_GetData);
		DY_ADD_INTERNAL_CALL(VirtualTexture_SetData);

		DY_ADD_INTERNAL_CALL(VideoPlayer_SetTime);
		DY_ADD_INTERNAL_CALL(VideoPlayer_Update);
		DY_ADD_INTERNAL_CALL(VideoPlayer_GetSubtitle);
		
		DY_ADD_INTERNAL_CALL(Entity_AddComponent);
		DY_ADD_INTERNAL_CALL(Entity_HasComponent);
		DY_ADD_INTERNAL_CALL(Entity_RemoveComponent);
		DY_ADD_INTERNAL_CALL(Entity_Duplicate);
		DY_ADD_INTERNAL_CALL(Entity_Destroy);
		DY_ADD_INTERNAL_CALL(Entity_Parent);
		DY_ADD_INTERNAL_CALL(Entity_Unparent);
		DY_ADD_INTERNAL_CALL(Entity_HasParent);
		DY_ADD_INTERNAL_CALL(Entity_GetParent);
		DY_ADD_INTERNAL_CALL(Entity_GetChildCount);
		DY_ADD_INTERNAL_CALL(Entity_GetChildren);
		DY_ADD_INTERNAL_CALL(Entity_Attach);
		DY_ADD_INTERNAL_CALL(Entity_GetAttachment);
		DY_ADD_INTERNAL_CALL(Entity_GetEntityByID);
		DY_ADD_INTERNAL_CALL(Entity_FindEntityByName);
		DY_ADD_INTERNAL_CALL(Entity_Create);
		DY_ADD_INTERNAL_CALL(Entity_CreateWithScript);
		DY_ADD_INTERNAL_CALL(Entity_AddScriptComponent);
		DY_ADD_INTERNAL_CALL(Entity_AddDistanceConstraintComponentDefault);
		DY_ADD_INTERNAL_CALL(Entity_AddDistanceConstraintComponentFixed);
		DY_ADD_INTERNAL_CALL(Entity_AddDistanceConstraintComponentRange);
		DY_ADD_INTERNAL_CALL(Entity_AddFixedConstraintComponent);

		DY_ADD_INTERNAL_CALL(TagComponent_GetTag);
		DY_ADD_INTERNAL_CALL(TagComponent_SetTag);

		DY_ADD_INTERNAL_CALL(TransformComponent_GetTranslation);
		DY_ADD_INTERNAL_CALL(TransformComponent_SetTranslation);
		DY_ADD_INTERNAL_CALL(TransformComponent_GetRotation);
		DY_ADD_INTERNAL_CALL(TransformComponent_SetRotation);
		DY_ADD_INTERNAL_CALL(TransformComponent_GetScale);
		DY_ADD_INTERNAL_CALL(TransformComponent_GetWorldTranslation);
		DY_ADD_INTERNAL_CALL(TransformComponent_SetWorldTranslation);
		DY_ADD_INTERNAL_CALL(TransformComponent_GetWorldRotation);
		DY_ADD_INTERNAL_CALL(TransformComponent_SetWorldRotation);
		DY_ADD_INTERNAL_CALL(TransformComponent_GetWorldScale);
		DY_ADD_INTERNAL_CALL(TransformComponent_SetWorldScale);
		DY_ADD_INTERNAL_CALL(TransformComponent_GetWorldTransform);
		DY_ADD_INTERNAL_CALL(TransformComponent_SetWorldTransform);

		DY_ADD_INTERNAL_CALL(CameraComponent_GetProjectionType);
		DY_ADD_INTERNAL_CALL(CameraComponent_SetProjectionType);
		DY_ADD_INTERNAL_CALL(CameraComponent_GetPerspectiveFOV);
		DY_ADD_INTERNAL_CALL(CameraComponent_SetPerspectiveFOV);
		DY_ADD_INTERNAL_CALL(CameraComponent_GetPerspectiveNear);
		DY_ADD_INTERNAL_CALL(CameraComponent_SetPerspectiveNear);
		DY_ADD_INTERNAL_CALL(CameraComponent_GetPerspectiveFar);
		DY_ADD_INTERNAL_CALL(CameraComponent_SetPerspectiveFar);
		DY_ADD_INTERNAL_CALL(CameraComponent_GetOrthographicSize);
		DY_ADD_INTERNAL_CALL(CameraComponent_SetOrthographicSize);
		DY_ADD_INTERNAL_CALL(CameraComponent_GetOrthographicNear);
		DY_ADD_INTERNAL_CALL(CameraComponent_SetOrthographicNear);
		DY_ADD_INTERNAL_CALL(CameraComponent_GetOrthographicFar);
		DY_ADD_INTERNAL_CALL(CameraComponent_SetOrthographicFar);
		DY_ADD_INTERNAL_CALL(CameraComponent_GetPrimary);
		DY_ADD_INTERNAL_CALL(CameraComponent_SetPrimary);
		DY_ADD_INTERNAL_CALL(CameraComponent_GetFixedAspectRatio);
		DY_ADD_INTERNAL_CALL(CameraComponent_SetFixedAspectRatio);

		DY_ADD_INTERNAL_CALL(CaptureComponent_GetTarget);
		DY_ADD_INTERNAL_CALL(CaptureComponent_SetTarget);
		DY_ADD_INTERNAL_CALL(CaptureComponent_GetRenderType);
		DY_ADD_INTERNAL_CALL(CaptureComponent_SetRenderType);
		DY_ADD_INTERNAL_CALL(CaptureComponent_GetMaskType);
		DY_ADD_INTERNAL_CALL(CaptureComponent_SetMaskType);
		DY_ADD_INTERNAL_CALL(CaptureComponent_GetCapture);
		DY_ADD_INTERNAL_CALL(CaptureComponent_SetCapture);
		DY_ADD_INTERNAL_CALL(CaptureComponent_GetCumulative);
		DY_ADD_INTERNAL_CALL(CaptureComponent_SetCumulative);
		DY_ADD_INTERNAL_CALL(CaptureComponent_MaskAdd);
		DY_ADD_INTERNAL_CALL(CaptureComponent_MaskRemove);
		DY_ADD_INTERNAL_CALL(CaptureComponent_MaskClear);

		DY_ADD_INTERNAL_CALL(ScriptComponent_GetScriptName);
		DY_ADD_INTERNAL_CALL(ScriptComponent_SetScriptName);

		DY_ADD_INTERNAL_CALL(SpriteRendererComponent_GetTexture);
		DY_ADD_INTERNAL_CALL(SpriteRendererComponent_SetTexture);
		DY_ADD_INTERNAL_CALL(SpriteRendererComponent_GetColor);
		DY_ADD_INTERNAL_CALL(SpriteRendererComponent_SetColor);
		DY_ADD_INTERNAL_CALL(SpriteRendererComponent_GetTilingFactor);
		DY_ADD_INTERNAL_CALL(SpriteRendererComponent_SetTilingFactor);

		DY_ADD_INTERNAL_CALL(CircleRendererComponent_GetColor);
		DY_ADD_INTERNAL_CALL(CircleRendererComponent_SetColor);
		DY_ADD_INTERNAL_CALL(CircleRendererComponent_GetThickness);
		DY_ADD_INTERNAL_CALL(CircleRendererComponent_SetThickness);
		DY_ADD_INTERNAL_CALL(CircleRendererComponent_GetFade);
		DY_ADD_INTERNAL_CALL(CircleRendererComponent_SetFade);

		DY_ADD_INTERNAL_CALL(TextComponent_GetText);
		DY_ADD_INTERNAL_CALL(TextComponent_SetText);
		DY_ADD_INTERNAL_CALL(TextComponent_GetColor);
		DY_ADD_INTERNAL_CALL(TextComponent_SetColor);

		DY_ADD_INTERNAL_CALL(RigidBody2DComponent_ApplyLinearImpulse);
		DY_ADD_INTERNAL_CALL(RigidBody2DComponent_ApplyLinearImpulseToCenter);

		DY_ADD_INTERNAL_CALL(StaticMeshComponent_GetMesh);
		DY_ADD_INTERNAL_CALL(StaticMeshComponent_SetMesh);
		DY_ADD_INTERNAL_CALL(StaticMeshComponent_GetAnimationGraph);
		DY_ADD_INTERNAL_CALL(StaticMeshComponent_SetAnimationGraph);
		DY_ADD_INTERNAL_CALL(StaticMeshComponent_SetAnimationGraphParameterBool);
		DY_ADD_INTERNAL_CALL(StaticMeshComponent_SetAnimationGraphParameterInt);
		DY_ADD_INTERNAL_CALL(StaticMeshComponent_SetAnimationGraphParameterFloat);
		DY_ADD_INTERNAL_CALL(StaticMeshComponent_SetAnimationGraphParameterVector2);
		DY_ADD_INTERNAL_CALL(StaticMeshComponent_SetAnimationGraphParameterVector3);
		DY_ADD_INTERNAL_CALL(StaticMeshComponent_SetAnimationGraphParameterVector4);
		DY_ADD_INTERNAL_CALL(StaticMeshComponent_SetAnimationGraphParameterTransform);
		DY_ADD_INTERNAL_CALL(StaticMeshComponent_GetMaterial);
		DY_ADD_INTERNAL_CALL(StaticMeshComponent_SetMaterial);
		DY_ADD_INTERNAL_CALL(StaticMeshComponent_GetBoneTransform);

		DY_ADD_INTERNAL_CALL(DirectionalLightComponent_GetColor);
		DY_ADD_INTERNAL_CALL(DirectionalLightComponent_SetColor);
		DY_ADD_INTERNAL_CALL(DirectionalLightComponent_GetIntensity);
		DY_ADD_INTERNAL_CALL(DirectionalLightComponent_SetIntensity);

		DY_ADD_INTERNAL_CALL(PointLightComponent_GetColor);
		DY_ADD_INTERNAL_CALL(PointLightComponent_SetColor);
		DY_ADD_INTERNAL_CALL(PointLightComponent_GetIntensity);
		DY_ADD_INTERNAL_CALL(PointLightComponent_SetIntensity);
		DY_ADD_INTERNAL_CALL(PointLightComponent_GetRadius);
		DY_ADD_INTERNAL_CALL(PointLightComponent_SetRadius);
		DY_ADD_INTERNAL_CALL(PointLightComponent_GetCastsShadows);
		DY_ADD_INTERNAL_CALL(PointLightComponent_SetCastsShadows);

		DY_ADD_INTERNAL_CALL(SkyLightComponent_GetIntensity);
		DY_ADD_INTERNAL_CALL(SkyLightComponent_SetIntensity);

		DY_ADD_INTERNAL_CALL(AudioComponent_GetAudio);
		DY_ADD_INTERNAL_CALL(AudioComponent_SetAudio);
		DY_ADD_INTERNAL_CALL(AudioComponent_GetPosition);
		DY_ADD_INTERNAL_CALL(AudioComponent_SetPosition);
		DY_ADD_INTERNAL_CALL(AudioComponent_GetVolume);
		DY_ADD_INTERNAL_CALL(AudioComponent_SetVolume);
		DY_ADD_INTERNAL_CALL(AudioComponent_Get3D);
		DY_ADD_INTERNAL_CALL(AudioComponent_Set3D);
		DY_ADD_INTERNAL_CALL(AudioComponent_GetIsLooping);
		DY_ADD_INTERNAL_CALL(AudioComponent_SetIsLooping);
		DY_ADD_INTERNAL_CALL(AudioComponent_GetStartOnAwake);
		DY_ADD_INTERNAL_CALL(AudioComponent_SetStartOnAwake);
		DY_ADD_INTERNAL_CALL(AudioComponent_GetPan);
		DY_ADD_INTERNAL_CALL(AudioComponent_SetPan);
		DY_ADD_INTERNAL_CALL(AudioComponent_GetSpeed);
		DY_ADD_INTERNAL_CALL(AudioComponent_SetSpeed);
		DY_ADD_INTERNAL_CALL(AudioComponent_GetEcho);
		DY_ADD_INTERNAL_CALL(AudioComponent_SetEcho);

		DY_ADD_INTERNAL_CALL(SplineComponent_GetPosition);
		DY_ADD_INTERNAL_CALL(SplineComponent_SetPosition);
		DY_ADD_INTERNAL_CALL(SplineComponent_GetTangent);
		DY_ADD_INTERNAL_CALL(SplineComponent_SetTangent);
		DY_ADD_INTERNAL_CALL(SplineComponent_GetType);
		DY_ADD_INTERNAL_CALL(SplineComponent_SetType);
		DY_ADD_INTERNAL_CALL(SplineComponent_GetPointCount);
		DY_ADD_INTERNAL_CALL(SplineComponent_GetPositionWeighted);
		DY_ADD_INTERNAL_CALL(SplineComponent_GetPositionDistance);

		DY_ADD_INTERNAL_CALL(RigidBodyComponent_GetLayer);
		DY_ADD_INTERNAL_CALL(RigidBodyComponent_SetLayer);
		DY_ADD_INTERNAL_CALL(RigidBodyComponent_IsActive);
		DY_ADD_INTERNAL_CALL(RigidBodyComponent_Activate);
		DY_ADD_INTERNAL_CALL(RigidBodyComponent_Deactivate);
		DY_ADD_INTERNAL_CALL(RigidBodyComponent_SetAllowSleeping);
		DY_ADD_INTERNAL_CALL(RigidBodyComponent_ResetSleepTimer);
		DY_ADD_INTERNAL_CALL(RigidBodyComponent_SetPositionWithoutActivation);
		DY_ADD_INTERNAL_CALL(RigidBodyComponent_SetRotationWithoutActivation);
		DY_ADD_INTERNAL_CALL(RigidBodyComponent_AddForce);
		DY_ADD_INTERNAL_CALL(RigidBodyComponent_AddForceAtPosition);
		DY_ADD_INTERNAL_CALL(RigidBodyComponent_AddImpulse);
		DY_ADD_INTERNAL_CALL(RigidBodyComponent_AddImpulseAtPosition);
		DY_ADD_INTERNAL_CALL(RigidBodyComponent_AddAngularImpulse);
		DY_ADD_INTERNAL_CALL(RigidBodyComponent_AddTorque);
		DY_ADD_INTERNAL_CALL(RigidBodyComponent_AddForceAndTorque);
		DY_ADD_INTERNAL_CALL(RigidBodyComponent_ApplyBuoyancyImpulse);
		DY_ADD_INTERNAL_CALL(RigidBodyComponent_MoveKinematic);
		DY_ADD_INTERNAL_CALL(RigidBodyComponent_GetCenterOfMassPosition);
		DY_ADD_INTERNAL_CALL(RigidBodyComponent_GetAccumulatedForce);
		DY_ADD_INTERNAL_CALL(RigidBodyComponent_GetAccumulatedTorque);
		DY_ADD_INTERNAL_CALL(RigidBodyComponent_ResetForce);
		DY_ADD_INTERNAL_CALL(RigidBodyComponent_ResetTorque);
		DY_ADD_INTERNAL_CALL(RigidBodyComponent_ResetMotion);
		DY_ADD_INTERNAL_CALL(RigidBodyComponent_GetLinearVelocity);
		DY_ADD_INTERNAL_CALL(RigidBodyComponent_SetLinearVelocity);
		DY_ADD_INTERNAL_CALL(RigidBodyComponent_AddLinearVelocity);
		DY_ADD_INTERNAL_CALL(RigidBodyComponent_GetAngularVelocity);
		DY_ADD_INTERNAL_CALL(RigidBodyComponent_SetAngularVelocity);
		DY_ADD_INTERNAL_CALL(RigidBodyComponent_GetLinearAndAngularVelocity);
		DY_ADD_INTERNAL_CALL(RigidBodyComponent_SetLinearAndAngularVelocity);
		DY_ADD_INTERNAL_CALL(RigidBodyComponent_AddLinearAndAngularVelocity);
		DY_ADD_INTERNAL_CALL(RigidBodyComponent_GetPointVelocity);

		DY_ADD_INTERNAL_CALL(SoftBodyComponent_GetLayer);
		DY_ADD_INTERNAL_CALL(SoftBodyComponent_SetLayer);
		DY_ADD_INTERNAL_CALL(SoftBodyComponent_SetVertexPosition);
		DY_ADD_INTERNAL_CALL(SoftBodyComponent_SetVertexVelocity);
		DY_ADD_INTERNAL_CALL(SoftBodyComponent_SetPositionWeighted);
		DY_ADD_INTERNAL_CALL(SoftBodyComponent_GetCenterOfMassPosition);
		DY_ADD_INTERNAL_CALL(SoftBodyComponent_SetFixedPosition);

		DY_ADD_INTERNAL_CALL(CharacterMovementComponent_GetLayer);
		DY_ADD_INTERNAL_CALL(CharacterMovementComponent_SetLayer);
		DY_ADD_INTERNAL_CALL(CharacterMovementComponent_GetMaxWalkSpeed);
		DY_ADD_INTERNAL_CALL(CharacterMovementComponent_SetMaxWalkSpeed);
		DY_ADD_INTERNAL_CALL(CharacterMovementComponent_GetJumpSpeed);
		DY_ADD_INTERNAL_CALL(CharacterMovementComponent_SetJumpSpeed);
		DY_ADD_INTERNAL_CALL(CharacterMovementComponent_GetGravityScale);
		DY_ADD_INTERNAL_CALL(CharacterMovementComponent_SetGravityScale);
		DY_ADD_INTERNAL_CALL(CharacterMovementComponent_GetMaxSlopeAngle);
		DY_ADD_INTERNAL_CALL(CharacterMovementComponent_SetMaxSlopeAngle);
		DY_ADD_INTERNAL_CALL(CharacterMovementComponent_GetAirControl);
		DY_ADD_INTERNAL_CALL(CharacterMovementComponent_SetAirControl);
		DY_ADD_INTERNAL_CALL(CharacterMovementComponent_GetVelocityBlendWeight);
		DY_ADD_INTERNAL_CALL(CharacterMovementComponent_SetVelocityBlendWeight);
		DY_ADD_INTERNAL_CALL(CharacterMovementComponent_GetRotateToMotion);
		DY_ADD_INTERNAL_CALL(CharacterMovementComponent_SetRotateToMotion);
		DY_ADD_INTERNAL_CALL(CharacterMovementComponent_GetRotationRate);
		DY_ADD_INTERNAL_CALL(CharacterMovementComponent_SetRotationRate);
		DY_ADD_INTERNAL_CALL(CharacterMovementComponent_SetMovementInput);
		DY_ADD_INTERNAL_CALL(CharacterMovementComponent_Jump);
		DY_ADD_INTERNAL_CALL(CharacterMovementComponent_IsFalling);
		DY_ADD_INTERNAL_CALL(CharacterMovementComponent_GetLinearVelocity);
		DY_ADD_INTERNAL_CALL(CharacterMovementComponent_GetGroundVelocity);
		
		DY_ADD_INTERNAL_CALL(SpringArmComponent_GetTargetLength);
		DY_ADD_INTERNAL_CALL(SpringArmComponent_SetTargetLength);
		DY_ADD_INTERNAL_CALL(SpringArmComponent_GetProbeRadius);
		DY_ADD_INTERNAL_CALL(SpringArmComponent_SetProbeRadius);
		DY_ADD_INTERNAL_CALL(SpringArmComponent_GetTargetOffset);
		DY_ADD_INTERNAL_CALL(SpringArmComponent_SetTargetOffset);
		DY_ADD_INTERNAL_CALL(SpringArmComponent_GetSocketOffset);
		DY_ADD_INTERNAL_CALL(SpringArmComponent_SetSocketOffset);
		DY_ADD_INTERNAL_CALL(SpringArmComponent_ExcludeEntity);
		
		DY_ADD_INTERNAL_CALL(FieldComponent_GetType);
		DY_ADD_INTERNAL_CALL(FieldComponent_SetType);
		DY_ADD_INTERNAL_CALL(FieldComponent_GetLayer);
		DY_ADD_INTERNAL_CALL(FieldComponent_SetLayer);
		DY_ADD_INTERNAL_CALL(FieldComponent_GetForce);
		DY_ADD_INTERNAL_CALL(FieldComponent_SetFroce);
		DY_ADD_INTERNAL_CALL(FieldComponent_GetMagnitude);
		DY_ADD_INTERNAL_CALL(FieldComponent_SetMagnitude);
		DY_ADD_INTERNAL_CALL(FieldComponent_GetRadius);
		DY_ADD_INTERNAL_CALL(FieldComponent_SetRadius);
		DY_ADD_INTERNAL_CALL(FieldComponent_GetFallof);
		DY_ADD_INTERNAL_CALL(FieldComponent_SetFallof);
		DY_ADD_INTERNAL_CALL(FieldComponent_GetBuoyancy);
		DY_ADD_INTERNAL_CALL(FieldComponent_SetBuoyancy);
		DY_ADD_INTERNAL_CALL(FieldComponent_GetLinearDrag);
		DY_ADD_INTERNAL_CALL(FieldComponent_SetLinearDrag);
		DY_ADD_INTERNAL_CALL(FieldComponent_GetAngularDrag);
		DY_ADD_INTERNAL_CALL(FieldComponent_SetAngularDrag);
		DY_ADD_INTERNAL_CALL(FieldComponent_GetFluidVelocity);
		DY_ADD_INTERNAL_CALL(FieldComponent_SetFluidVelocity);

		DY_ADD_INTERNAL_CALL(DistanceConstraintComponent_GetEnabled);
		DY_ADD_INTERNAL_CALL(DistanceConstraintComponent_SetEnabled);
		DY_ADD_INTERNAL_CALL(DistanceConstraintComponent_GetTarget);
		DY_ADD_INTERNAL_CALL(DistanceConstraintComponent_SetTarget);
		DY_ADD_INTERNAL_CALL(DistanceConstraintComponent_SetDistance);
		DY_ADD_INTERNAL_CALL(DistanceConstraintComponent_SetDistanceRange);

		DY_ADD_INTERNAL_CALL(FixedConstraintComponent_GetEnabled);
		DY_ADD_INTERNAL_CALL(FixedConstraintComponent_SetEnabled);
		DY_ADD_INTERNAL_CALL(FixedConstraintComponent_GetTarget);
		DY_ADD_INTERNAL_CALL(FixedConstraintComponent_SetTarget);
		
		DY_ADD_INTERNAL_CALL(Input_GetKeyboardName);
		DY_ADD_INTERNAL_CALL(Input_IsKeyDown);
		DY_ADD_INTERNAL_CALL(Input_GetMouseName);
		DY_ADD_INTERNAL_CALL(Input_IsMouseButtonPressed);
		DY_ADD_INTERNAL_CALL(Input_GetMousePosition);
		DY_ADD_INTERNAL_CALL(Input_GetMouseDelta);
		DY_ADD_INTERNAL_CALL(Input_GetMouseX);
		DY_ADD_INTERNAL_CALL(Input_GetMouseY);
		DY_ADD_INTERNAL_CALL(Input_SetMouseLocked);
		DY_ADD_INTERNAL_CALL(Input_GetGamepadCount);
		DY_ADD_INTERNAL_CALL(Input_IsGamepadConnected);
		DY_ADD_INTERNAL_CALL(Input_GetGamepadName);
		DY_ADD_INTERNAL_CALL(Input_GetGamepadPowerLevel);
		DY_ADD_INTERNAL_CALL(Input_IsGamepadButtonPressed);
		DY_ADD_INTERNAL_CALL(Input_GetGamepadAxis);
		DY_ADD_INTERNAL_CALL(Input_GetGamepadSensor);
		DY_ADD_INTERNAL_CALL(Input_GetGamepadTouchPad);
		DY_ADD_INTERNAL_CALL(Input_SetGamepadRumble);
		DY_ADD_INTERNAL_CALL(Input_SetGamepadLED);
		DY_ADD_INTERNAL_CALL(Input_SetGamepadPlayerLED);
		DY_ADD_INTERNAL_CALL(Input_SetGamepadMicrophoneLED);
		DY_ADD_INTERNAL_CALL(Input_ClearTriggerEffect);
		DY_ADD_INTERNAL_CALL(Input_SetTriggerEffect);
		DY_ADD_INTERNAL_CALL(Input_SetTriggerEffectContinuous);
		DY_ADD_INTERNAL_CALL(Input_SetTriggerEffectSection);

		DY_ADD_INTERNAL_CALL(Physics_Raycast);
		DY_ADD_INTERNAL_CALL(Physics_RaycastMultihit);
		DY_ADD_INTERNAL_CALL(Physics_RaycastPoints);
		DY_ADD_INTERNAL_CALL(Physics_BoxShapeCast);
		DY_ADD_INTERNAL_CALL(Physics_BoxShapeCastMultihit);
		DY_ADD_INTERNAL_CALL(Physics_CapsuleShapeCast);
		DY_ADD_INTERNAL_CALL(Physics_CapsuleShapeCastMultihit);
		DY_ADD_INTERNAL_CALL(Physics_SphereShapeCast);
		DY_ADD_INTERNAL_CALL(Physics_SphereShapeCastMultihit);
		DY_ADD_INTERNAL_CALL(Physics_GetEntitiesInBounds);
		DY_ADD_INTERNAL_CALL(Physics_GetEntitiesInBoundsOnLayer);

		DY_ADD_INTERNAL_CALL(Debug_DrawDebugLine);
		DY_ADD_INTERNAL_CALL(Debug_DrawDebugCube);
		DY_ADD_INTERNAL_CALL(Debug_DrawDebugSphere);
		DY_ADD_INTERNAL_CALL(Debug_ClearDebugDrawing);

		DY_ADD_INTERNAL_CALL(Log_Trace);
		DY_ADD_INTERNAL_CALL(Log_Info);
		DY_ADD_INTERNAL_CALL(Log_Warn);
		DY_ADD_INTERNAL_CALL(Log_Error);
		DY_ADD_INTERNAL_CALL(Log_Critical);

		DY_ADD_INTERNAL_CALL(Core_Assert);

		DY_ADD_INTERNAL_CALL(Math_MultiplyRotationVector);

		DY_ADD_INTERNAL_CALL(Network_ReplicateMethod);
	}

	void ScriptGlue::SetOpenSceneCallback(const std::function<void(UUID)>& callback)
	{
		s_OpenSceneCallback = callback;
	}

	void ScriptGlue::OnRuntimeStart()
	{
		s_RegisteredScriptAssets.clear();
	}

	void ScriptGlue::OnRuntimeStop()
	{
		for (const auto& [handle, instance] : s_InstancedMaterials)
			AssetManager::RemoveAsset(handle);

		s_InstancedMaterials.clear();
	}

}