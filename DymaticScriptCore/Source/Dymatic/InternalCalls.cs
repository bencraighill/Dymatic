using System;
using System.Reflection;
using System.Reflection.Emit;
using System.Runtime.CompilerServices;
using System.Security.Cryptography;

namespace Dymatic
{
    public static class InternalCalls
    {
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static object GetScriptInstance(ulong entityID);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Scene_OpenScene(ulong assetID);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static bool Asset_RegisterScriptReference(ulong handle);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static bool Asset_UnregisterScriptReference(ulong handle);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static bool Asset_DoesAssetExist(ulong handle);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static ulong Asset_GetAssetHandle(string filepath);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static Entity Prefab_Instantiate(ulong handle);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static Entity Prefab_InstantiateAtTransform(ulong handle, ref Transform transform);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static ulong Material_CreateInstance(ulong handle);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static bool MaterialInstance_SetParameterFloat(ulong handle, string name, float value);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static bool MaterialInstance_SetParameterVector2(ulong handle, string name, ref Vector2 value);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static bool MaterialInstance_SetParameterVector3(ulong handle, string name, ref Vector3 value);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static bool MaterialInstance_SetParameterVector4(ulong handle, string name, ref Vector4 value);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void VirtualTexture_Resize(ulong handle, ref Vector2 size);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void VirtualTexture_Clear(ulong handle);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void VirtualTexture_GetSize(ulong handle, out Vector2 size);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static uint VirtualTexture_GetPixelCount(ulong handle);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static uint VirtualTexture_GetDataSize(ulong handle);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static byte[] VirtualTexture_GetData(ulong handle);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static bool VirtualTexture_SetData(ulong handle, Array data);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void VideoPlayer_SetTime(ulong handle, float time);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void VideoPlayer_Update(ulong handle, float ts);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void VideoPlayer_GetSubtitle(ulong handle, out string subtitle);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Entity_AddComponent(ulong entityID, Type componentType);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static bool Entity_HasComponent(ulong entityID, Type componentType);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Entity_RemoveComponent(ulong entityID, Type componentType);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static ulong Entity_Duplicate(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Entity_Destroy(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static bool Entity_HasParent(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Entity_Parent(ulong entityID, ulong parentID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Entity_Unparent(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static Entity Entity_GetParent(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static uint Entity_GetChildCount(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static Entity[] Entity_GetChildren(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Entity_Attach(ulong entityID, string boneName);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Entity_GetAttachment(ulong entityID, out string boneName);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static Entity Entity_GetEntityByID(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static Entity Entity_FindEntityByName(string name);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static Entity Entity_Create(string name);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static Entity Entity_CreateWithScript(string name, Type scriptType);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static ulong Entity_AddScriptComponent(ulong entityID, string script);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static ulong Entity_AddDistanceConstraintComponentDefault(ulong entityID, ulong targetID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static ulong Entity_AddDistanceConstraintComponentFixed(ulong entityID, ulong targetID, float distance);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static ulong Entity_AddDistanceConstraintComponentRange(ulong entityID, ulong targetID, float minDistance, float maxDistance);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static ulong Entity_AddFixedConstraintComponent(ulong entityID, ulong targetID);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static string TagComponent_GetTag(ulong entityID, out string tag);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void TagComponent_SetTag(ulong entityID, string tag);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void TransformComponent_GetTranslation(ulong entityID, out Vector3 translation);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void TransformComponent_SetTranslation(ulong entityID, ref Vector3 translation);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void TransformComponent_GetRotation(ulong entityID, out Vector3 rotation);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void TransformComponent_SetRotation(ulong entityID, ref Vector3 rotation);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void TransformComponent_GetScale(ulong entityID, out Vector3 scale);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void TransformComponent_SetScale(ulong entityID, ref Vector3 scale);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void TransformComponent_GetWorldTranslation(ulong entityID, out Vector3 translation);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void TransformComponent_SetWorldTranslation(ulong entityID, ref Vector3 translation);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void TransformComponent_GetWorldRotation(ulong entityID, out Vector3 rotation);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void TransformComponent_SetWorldRotation(ulong entityID, ref Vector3 rotation);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void TransformComponent_GetWorldScale(ulong entityID, out Vector3 scale);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void TransformComponent_SetWorldScale(ulong entityID, ref Vector3 scale);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void TransformComponent_GetWorldTransform(ulong entityID, out Vector3 translation, out Vector3 rotation, out Vector3 scale);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void TransformComponent_SetWorldTransform(ulong entityID, ref Vector3 translation, ref Vector3 rotation, ref Vector3 scale);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static byte CameraComponent_GetProjectionType(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void CameraComponent_SetProjectionType(ulong entityID, byte projectionType);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static float CameraComponent_GetPerspectiveFOV(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void CameraComponent_SetPerspectiveFOV(ulong entityID, float perspectiveFOV);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static float CameraComponent_GetPerspectiveNear(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void CameraComponent_SetPerspectiveNear(ulong entityID, float perspectiveNear);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static float CameraComponent_GetPerspectiveFar(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void CameraComponent_SetPerspectiveFar(ulong entityID, float perspectiveFar);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static float CameraComponent_GetOrthographicSize(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void CameraComponent_SetOrthographicSize(ulong entityID, float orthographicSize);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static float CameraComponent_GetOrthographicNear(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void CameraComponent_SetOrthographicNear(ulong entityID, float orthographicNear);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static float CameraComponent_GetOrthographicFar(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void CameraComponent_SetOrthographicFar(ulong entityID, float orthographicFar);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static bool CameraComponent_GetPrimary(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void CameraComponent_SetPrimary(ulong entityID, bool primary);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static bool CameraComponent_GetFixedAspectRatio(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void CameraComponent_SetFixedAspectRatio(ulong entityID, bool fixedAspectRatio);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static ulong CaptureComponent_GetTarget(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void CaptureComponent_SetTarget(ulong entityID, ulong targetID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static byte CaptureComponent_GetRenderType(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void CaptureComponent_SetRenderType(ulong entityID, byte renderType);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static byte CaptureComponent_GetMaskType(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void CaptureComponent_SetMaskType(ulong entityID, byte renderType);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static bool CaptureComponent_GetCapture(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void CaptureComponent_SetCapture(ulong entityID, bool capture);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static bool CaptureComponent_GetCumulative(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void CaptureComponent_SetCumulative(ulong entityID, bool capture);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void CaptureComponent_MaskAdd(ulong entityID, ulong targetID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void CaptureComponent_MaskRemove(ulong entityID, ulong targetID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void CaptureComponent_MaskClear(ulong entityID);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static ulong ScriptComponent_GetScriptName(ulong entityID, out string scriptName);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static ulong ScriptComponent_SetScriptName(ulong entityID, string scriptName);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static ulong SpriteRendererComponent_GetTexture(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void SpriteRendererComponent_SetTexture(ulong entityID, ulong handle);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void SpriteRendererComponent_GetColor(ulong entityID, out Vector4 color);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void SpriteRendererComponent_SetColor(ulong entityID, ref Vector4 color);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static float SpriteRendererComponent_GetTilingFactor(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void SpriteRendererComponent_SetTilingFactor(ulong entityID, float tilingFactor);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void CircleRendererComponent_GetColor(ulong entityID, out Vector4 color);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void CircleRendererComponent_SetColor(ulong entityID, ref Vector4 color);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static float CircleRendererComponent_GetThickness(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void CircleRendererComponent_SetThickness(ulong entityID, float thickness);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static float CircleRendererComponent_GetFade(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void CircleRendererComponent_SetFade(ulong entityID, float fade);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void TextComponent_GetText(ulong entityID, out string text);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void TextComponent_SetText(ulong entityID, string text);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void TextComponent_GetColor(ulong entityID, out Vector3 color);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void TextComponent_SetColor(ulong entityID, ref Vector3 color);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void RigidBody2DComponent_ApplyLinearImpulse(ulong entityID, ref Vector2 impulse, ref Vector2 point, bool wake);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void RigidBody2DComponent_ApplyLinearImpulseToCenter(ulong entityID, ref Vector2 impulse, bool wake);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static ulong StaticMeshComponent_GetMesh(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void StaticMeshComponent_SetMesh(ulong entityID, ulong handle);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static ulong StaticMeshComponent_GetAnimationGraph(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void StaticMeshComponent_SetAnimationGraph(ulong entityID, ulong handle);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static bool StaticMeshComponent_SetAnimationGraphParameterBool(ulong entityID, string name, bool value);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static bool StaticMeshComponent_SetAnimationGraphParameterInt(ulong entityID, string name, int value);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static bool StaticMeshComponent_SetAnimationGraphParameterFloat(ulong entityID, string name, float value);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static bool StaticMeshComponent_SetAnimationGraphParameterVector2(ulong entityID, string name, ref Vector2 value);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static bool StaticMeshComponent_SetAnimationGraphParameterVector3(ulong entityID, string name, ref Vector3 value);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static bool StaticMeshComponent_SetAnimationGraphParameterVector4(ulong entityID, string name, ref Vector4 value);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static bool StaticMeshComponent_SetAnimationGraphParameterTransform(ulong entityID, string name, ref Transform value);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static ulong StaticMeshComponent_GetMaterial(ulong entityID, uint slot);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void StaticMeshComponent_SetMaterial(ulong entityID, ulong handle, uint slot);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void StaticMeshComponent_GetBoneTransform(ulong entityID, string name, out Transform transform);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void DirectionalLightComponent_GetColor(ulong entityID, out Vector3 color);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void DirectionalLightComponent_SetColor(ulong entityID, ref Vector3 color);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static float DirectionalLightComponent_GetIntensity(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void DirectionalLightComponent_SetIntensity(ulong entityID, float intensity);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void PointLightComponent_GetColor(ulong entityID, out Vector3 color);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void PointLightComponent_SetColor(ulong entityID, ref Vector3 color);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static float PointLightComponent_GetIntensity(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void PointLightComponent_SetIntensity(ulong entityID, float intensity);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static float PointLightComponent_GetRadius(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void PointLightComponent_SetRadius(ulong entityID, float radius);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static bool PointLightComponent_GetCastsShadows(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void PointLightComponent_SetCastsShadows(ulong entityID, bool radius);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static float SkyLightComponent_GetIntensity(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void SkyLightComponent_SetIntensity(ulong entityID, float intensity);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static ulong AudioComponent_GetAudio(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void AudioComponent_SetAudio(ulong entityID, ulong handle);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static uint AudioComponent_GetPosition(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void AudioComponent_SetPosition(ulong entityID, uint position);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static float AudioComponent_GetVolume(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void AudioComponent_SetVolume(ulong entityID, float volume);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static bool AudioComponent_Get3D(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void AudioComponent_Set3D(ulong entityID, bool is3D);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static bool AudioComponent_GetIsLooping(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void AudioComponent_SetIsLooping(ulong entityID, bool isLooping);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static bool AudioComponent_GetStartOnAwake(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void AudioComponent_SetStartOnAwake(ulong entityID, bool startOnAwake);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static float AudioComponent_GetPan(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void AudioComponent_SetPan(ulong entityID, float pan);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static float AudioComponent_GetSpeed(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void AudioComponent_SetSpeed(ulong entityID, float speed);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static float AudioComponent_GetEcho(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void AudioComponent_SetEcho(ulong entityID, float echo);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void SplineComponent_GetPosition(ulong entityID, uint index, out Vector3 position);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void SplineComponent_SetPosition(ulong entityID, uint index, ref Vector3 position);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void SplineComponent_GetTangent(ulong entityID, uint index, out Vector3 tangent);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void SplineComponent_SetTangent(ulong entityID, uint index, ref Vector3 tangent);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static byte SplineComponent_GetType(ulong entityID, uint index);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void SplineComponent_SetType(ulong entityID, uint index, byte type);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static uint SplineComponent_GetPointCount(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void SplineComponent_GetPositionWeighted(ulong entityID, float weight, out Vector3 position);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void SplineComponent_GetPositionDistance(ulong entityID, float distance, out Vector3 position);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void RigidBodyComponent_GetLayer(ulong entityID, out string layer);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void RigidBodyComponent_SetLayer(ulong entityID, string layer);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static bool RigidBodyComponent_IsActive(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void RigidBodyComponent_Activate(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void RigidBodyComponent_Deactivate(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void RigidBodyComponent_SetAllowSleeping(ulong entityID, bool allowSleeping);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void RigidBodyComponent_ResetSleepTimer(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void RigidBodyComponent_SetPositionWithoutActivation(ulong entityID, ref Vector3 position);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void RigidBodyComponent_SetRotationWithoutActivation(ulong entityID, ref Vector3 rotation);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void RigidBodyComponent_AddForce(ulong entityID, ref Vector3 force);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void RigidBodyComponent_AddForceAtPosition(ulong entityID, ref Vector3 force, ref Vector3 position);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void RigidBodyComponent_AddImpulse(ulong entityID, ref Vector3 impulse);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void RigidBodyComponent_AddImpulseAtPosition(ulong entityID, ref Vector3 impulse, ref Vector3 position);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void RigidBodyComponent_AddAngularImpulse(ulong entityID, ref Vector3 angularImpulse);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void RigidBodyComponent_AddTorque(ulong entityID, ref Vector3 torque);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void RigidBodyComponent_AddForceAndTorque(ulong entityID, ref Vector3 force, ref Vector3 torque);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void RigidBodyComponent_ApplyBuoyancyImpulse(ulong entityID, ref Vector3 surfacePosition, ref Vector3 surfaceNormal, float buoyancy, float linearDrag, float angularDrag, ref Vector3 fluidVelocity, float deltaTime);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void RigidBodyComponent_MoveKinematic(ulong entityID, ref Vector3 position, ref Vector3 rotation, float deltaTime);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void RigidBodyComponent_GetCenterOfMassPosition(ulong entityID, out Vector3 position);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void RigidBodyComponent_GetAccumulatedForce(ulong entityID, out Vector3 force);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void RigidBodyComponent_GetAccumulatedTorque(ulong entityID, out Vector3 torque);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void RigidBodyComponent_ResetForce(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void RigidBodyComponent_ResetTorque(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void RigidBodyComponent_ResetMotion(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void RigidBodyComponent_GetLinearVelocity(ulong entityID, out Vector3 linearVelocity);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void RigidBodyComponent_SetLinearVelocity(ulong entityID, ref Vector3 linearVelocity);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void RigidBodyComponent_AddLinearVelocity(ulong entityID, ref Vector3 linearVelocity);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void RigidBodyComponent_GetAngularVelocity(ulong entityID, out Vector3 angularVelocity);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void RigidBodyComponent_SetAngularVelocity(ulong entityID, ref Vector3 angularVelocity);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void RigidBodyComponent_GetLinearAndAngularVelocity(ulong entityID, out Vector3 linearVelocity, out Vector3 angularVelocity);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void RigidBodyComponent_SetLinearAndAngularVelocity(ulong entityID, ref Vector3 linearVelocity, ref Vector3 angularVelocity);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void RigidBodyComponent_AddLinearAndAngularVelocity(ulong entityID, ref Vector3 linearVelocity, ref Vector3 angularVelocity);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void RigidBodyComponent_GetPointVelocity(ulong entityID, ref Vector3 point, out Vector3 velocity);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void SoftBodyComponent_GetLayer(ulong entityID, out string layer);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void SoftBodyComponent_SetLayer(ulong entityID, string layer);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void SoftBodyComponent_SetVertexPosition(ulong entityID, uint index, ref Vector3 position);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void SoftBodyComponent_SetVertexVelocity(ulong entityID, uint index, ref Vector3 velocity);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void SoftBodyComponent_SetPositionWeighted(ulong entityID, ref Vector3 position);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void SoftBodyComponent_GetCenterOfMassPosition(ulong entityID, out Vector3 position);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void SoftBodyComponent_SetFixedPosition(ulong entityID, bool fixedPosition);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void CharacterMovementComponent_GetLayer(ulong entityID, out string layer);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void CharacterMovementComponent_SetLayer(ulong entityID, string layer);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static float CharacterMovementComponent_GetMaxWalkSpeed(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void CharacterMovementComponent_SetMaxWalkSpeed(ulong entityID, float maxWalkSpeed);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static float CharacterMovementComponent_GetJumpSpeed(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void CharacterMovementComponent_SetJumpSpeed(ulong entityID, float jumpSpeed);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static float CharacterMovementComponent_GetGravityScale(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void CharacterMovementComponent_SetGravityScale(ulong entityID, float gravityScale);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static float CharacterMovementComponent_GetMaxSlopeAngle(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void CharacterMovementComponent_SetMaxSlopeAngle(ulong entityID, float maxSlopeAngle);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static float CharacterMovementComponent_GetAirControl(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void CharacterMovementComponent_SetAirControl(ulong entityID, float airControl);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static float CharacterMovementComponent_GetVelocityBlendWeight(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void CharacterMovementComponent_SetVelocityBlendWeight(ulong entityID, float velocityBlendWeight);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static bool CharacterMovementComponent_GetRotateToMotion(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void CharacterMovementComponent_SetRotateToMotion(ulong entityID, bool rotateToMotion);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static float CharacterMovementComponent_GetRotationRate(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void CharacterMovementComponent_SetRotationRate(ulong entityID, float rotationRate);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void CharacterMovementComponent_SetMovementInput(ulong entityID, ref Vector3 movement);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void CharacterMovementComponent_Jump(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static bool CharacterMovementComponent_IsFalling(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void CharacterMovementComponent_GetLinearVelocity(ulong entityID, out Vector3 linearVelocity);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void CharacterMovementComponent_GetGroundVelocity(ulong entityID, out Vector3 groundVelocity);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static float SpringArmComponent_GetTargetLength(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void SpringArmComponent_SetTargetLength(ulong entityID, float targetLength);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static float SpringArmComponent_GetProbeRadius(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void SpringArmComponent_SetProbeRadius(ulong entityID, float probeRadius);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void SpringArmComponent_GetTargetOffset(ulong entityID, out Vector3 targetOffset);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void SpringArmComponent_SetTargetOffset(ulong entityID, ref Vector3 targetOffset);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void SpringArmComponent_GetSocketOffset(ulong entityID, out Vector3 socketOffset);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void SpringArmComponent_SetSocketOffset(ulong entityID, ref Vector3 socketOffset);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void SpringArmComponent_ExcludeEntity(ulong entityID, ulong exlusionID);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static byte FieldComponent_GetType(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void FieldComponent_SetType(ulong entityID, byte type);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void FieldComponent_GetLayer(ulong entityID, out string layer);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void FieldComponent_SetLayer(ulong entityID, string layer);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void FieldComponent_GetForce(ulong entityID, out Vector3 force);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void FieldComponent_SetForce(ulong entityID, ref Vector3 force);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static float FieldComponent_GetMagnitude(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void FieldComponent_SetMagnitude(ulong entityID, float magnitude);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static float FieldComponent_GetRadius(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void FieldComponent_SetRadius(ulong entityID, float radius);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static float FieldComponent_GetFallof(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void FieldComponent_SetFallof(ulong entityID, float falloff);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static float FieldComponent_GetBuoyancy(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void FieldComponent_SetBuoyancy(ulong entityID, float buoyancy);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static float FieldComponent_GetLinearDrag(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void FieldComponent_SetLinearDrag(ulong entityID, float linearDrag);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static float FieldComponent_GetAngularDrag(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void FieldComponent_SetAngularDrag(ulong entityID, float angularDrag);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void FieldComponent_GetFluidVelocity(ulong entityID, out Vector3 fluidVelocity);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void FieldComponent_SetFluidVelocity(ulong entityID, ref Vector3 fluidVelocity);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static bool DistanceConstraintComponent_GetEnabled(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void DistanceConstraintComponent_SetEnabled(ulong entityID, bool enabled);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static Entity DistanceConstraintComponent_GetTarget(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void DistanceConstraintComponent_SetTarget(ulong entityID, ulong targetID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void DistanceConstraintComponent_SetDistance(ulong entityID, float distance);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void DistanceConstraintComponent_SetDistanceRange(ulong entityID, float minDistance, float maxDistance);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static bool FixedConstraintComponent_GetEnabled(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void FixedConstraintComponent_SetEnabled(ulong entityID, bool enabled);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static Entity FixedConstraintComponent_GetTarget(ulong entityID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void FixedConstraintComponent_SetTarget(ulong entityID, ulong targetID);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Input_GetKeyboardName(out string name);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static bool Input_IsKeyDown(KeyCode keycode);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Input_GetMouseName(out string name);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static bool Input_IsMouseButtonPressed(MouseCode mouseCode);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Input_GetMousePosition(out Vector2 position);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Input_GetMouseDelta(out Vector2 delta);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static float Input_GetMouseX();
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static float Input_GetMouseY();
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Input_SetMouseLocked(bool locked);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static uint Input_GetGamepadCount();
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static bool Input_IsGamepadConnected(int gamepadIndex);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Input_GetGamepadName(int gamepadIndex, out string name);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static int Input_GetGamepadPowerLevel(int gamepadIndex);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static bool Input_IsGamepadButtonPressed(int gamepadIndex, GamepadButtonCode gamepadButton);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static float Input_GetGamepadAxis(int gamepadIndex, GamepadAxisCode gamepadAxis);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Input_GetGamepadSensor(int gamepadIndex, GamepadSensorCode gamepadSensor, out Vector3 value);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Input_GetGamepadTouchPad(int gamepadIndex, out Vector2 value);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static bool Input_SetGamepadRumble(int gamepadIndex, float left, float right, float duration);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Input_SetGamepadLED(int gamepadIndex, ref Vector3 color);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Input_SetGamepadPlayerLED(int gamepadIndex, int brightnessLevel, int count, bool fadeIn);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Input_SetGamepadMicrophoneLED(int gamepadIndex, bool enabled, bool pulse);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Input_ClearTriggerEffect(int gamepadIndex, GamepadButtonCode trigger);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Input_SetTriggerEffect(int gamepadIndex, GamepadButtonCode trigger, float startPosition, bool keepEffect, float beginForce, float middleForce, float endForce, float frequency);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Input_SetTriggerEffectContinuous(int gamepadIndex, GamepadButtonCode trigger, float startPosition, float force);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Input_SetTriggerEffectSection(int gamepadIndex, GamepadButtonCode trigger, float startPosition, float endPosition);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Physics_Raycast(ref Vector3 origin, ref Vector3 direction, float distance, out RaycastHit hit);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Physics_RaycastPoints(ref Vector3 start, ref Vector3 end, out RaycastHit hit);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static RaycastHit[] Physics_RaycastMultihit(ref Vector3 origin, ref Vector3 direction, float distance);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Physics_BoxShapeCast(ref Vector3 origin, ref Vector3 direction, float distance, ref Vector3 halfSize, ref Vector3 orientation, out RaycastHit hit);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static RaycastHit[] Physics_BoxShapeCastMultihit(ref Vector3 origin, ref Vector3 direction, float distance, ref Vector3 halfSize, ref Vector3 orientation);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Physics_CapsuleShapeCast(ref Vector3 origin, ref Vector3 direction, float distance, float radius, float halfHeight, out RaycastHit hit);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static RaycastHit[] Physics_CapsuleShapeCastMultihit(ref Vector3 origin, ref Vector3 direction, float distance, float radius, float halfHeight);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Physics_SphereShapeCast(ref Vector3 origin, ref Vector3 direction, float distance, float radius, out RaycastHit hit);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static RaycastHit[] Physics_SphereShapeCastMultihit(ref Vector3 origin, ref Vector3 direction, float distance, float radius);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static Entity[] Physics_GetEntitiesInBounds(ref Vector3 min, ref Vector3 max);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static Entity[] Physics_GetEntitiesInBoundsOnLayer(ref Vector3 min, ref Vector3 max, string layer);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Debug_DrawDebugLine(ref Vector3 start, ref Vector3 end, ref Vector3 color, float duration);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Debug_DrawDebugCube(ref Vector3 position, ref Vector3 size, ref Vector3 color, float duration);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Debug_DrawDebugSphere(ref Vector3 center, float radius, ref Vector3 color, float duration);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Debug_ClearDebugDrawing();
        
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Log_Trace(string message);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Log_Info(string message);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Log_Warn(string message);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Log_Error(string message);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Log_Critical(string message);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Core_Assert(bool condition, string message);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Math_MultiplyRotationVector(ref Vector3 rotation, ref Vector3 vector, out Vector3 result);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Network_ReplicateMethod(ulong entityID, string methodName, params object[] parameterData);

    }
}