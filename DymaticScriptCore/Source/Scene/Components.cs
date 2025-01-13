using System.Linq.Expressions;
using System.Management.Instrumentation;
using System.Security.Policy;

namespace Dymatic
{
    public abstract class Component
    {
        public Entity Entity { get; internal set; }
    }

    public class TagComponent : Component
    {
        public string Tag
        {
            get
            {
                InternalCalls.TagComponent_GetTag(Entity.ID, out string tag);
                return tag;
            }
            set
            {
                InternalCalls.TagComponent_SetTag(Entity.ID, value);
            }
        }
    }

    public class TransformComponent : Component
    {
        public Vector3 Translation
        {
            get
            {
                InternalCalls.TransformComponent_GetTranslation(Entity.ID, out Vector3 translation);
                return translation;
            }
            set
            {
                InternalCalls.TransformComponent_SetTranslation(Entity.ID, ref value);
            }
        }

        public Vector3 Rotation
        {
            get
            {
                InternalCalls.TransformComponent_GetRotation(Entity.ID, out Vector3 rotation);
                return rotation;
            }
            set
            {
                InternalCalls.TransformComponent_SetRotation(Entity.ID, ref value);
            }
        }

        public Vector3 Scale
        {
            get
            {
                InternalCalls.TransformComponent_GetScale(Entity.ID, out Vector3 scale);
                return scale;
            }
            set
            {
                InternalCalls.TransformComponent_SetScale(Entity.ID, ref value);
            }
        }

        public Vector3 GetWorldTranslation()
        {
            InternalCalls.TransformComponent_GetWorldTranslation(Entity.ID, out Vector3 translation);
            return translation;
        }

        public void SetWorldTranslation(Vector3 translation)
        {
            InternalCalls.TransformComponent_SetWorldTranslation(Entity.ID, ref translation);
        }

        public Vector3 GetWorldRotation()
        {
            InternalCalls.TransformComponent_GetWorldRotation(Entity.ID, out Vector3 rotation);
            return rotation;
        }

        public void SetWorldRotation(Vector3 rotation)
        {
            InternalCalls.TransformComponent_SetWorldRotation(Entity.ID, ref rotation);
        }

        public Vector3 GetWorldScale()
        {
            InternalCalls.TransformComponent_GetWorldScale(Entity.ID, out Vector3 scale);
            return scale;
        }

        public void SetWorldScale(Vector3 scale)
        {
            InternalCalls.TransformComponent_SetWorldScale(Entity.ID, ref scale);
        }

        public Transform GetWorldTransform()
        {
            InternalCalls.TransformComponent_GetWorldTransform(Entity.ID, out Vector3 translation, out Vector3 rotation, out Vector3 scale);
            return new Transform(translation, rotation, scale);
        }

        public void SetWorldTransform(Transform transform)
        {
            InternalCalls.TransformComponent_SetWorldTransform(Entity.ID, ref transform.Translation, ref transform.Rotation, ref transform.Scale);
        }
    }

    public enum ProjectionType : byte
    {
        Perspective = 0,
        Orthographic = 1
    }

    public class CameraComponent : Component
    {
        public ProjectionType ProjectionType
        {
            get
            {
                return (ProjectionType)InternalCalls.CameraComponent_GetProjectionType(Entity.ID);
            }
            set
            {
                InternalCalls.CameraComponent_SetProjectionType(Entity.ID, (byte)value);
            }
        }

        public float PerspectiveFOV
        {
            get
            {
                return InternalCalls.CameraComponent_GetPerspectiveFOV(Entity.ID);
            }
            set
            {
                InternalCalls.CameraComponent_SetPerspectiveFOV(Entity.ID, value);
            }
        }

        public float PerspectiveNear
        {
            get
            {
                return InternalCalls.CameraComponent_GetPerspectiveNear(Entity.ID);
            }
            set
            {
                InternalCalls.CameraComponent_SetPerspectiveNear(Entity.ID, value);
            }
        }

        public float PerspectiveFar
        {
            get
            {
                return InternalCalls.CameraComponent_GetPerspectiveFar(Entity.ID);
            }
            set
            {
                InternalCalls.CameraComponent_SetPerspectiveFar(Entity.ID, value);
            }
        }

        public float OrthographicSize
        {
            get
            {
                return InternalCalls.CameraComponent_GetOrthographicSize(Entity.ID);
            }
            set
            {
                InternalCalls.CameraComponent_SetOrthographicSize(Entity.ID, value);
            }
        }

        public float OrthographicNear
        {
            get
            {
                return InternalCalls.CameraComponent_GetOrthographicNear(Entity.ID);
            }
            set
            {
                InternalCalls.CameraComponent_SetOrthographicNear(Entity.ID, value);
            }
        }

        public float OrthographicFar
        {
            get
            {
                return InternalCalls.CameraComponent_GetOrthographicFar(Entity.ID);
            }
            set
            {
                InternalCalls.CameraComponent_SetOrthographicFar(Entity.ID, value);
            }
        }

        public bool Primary
        {
            get
            {
                return InternalCalls.CameraComponent_GetPrimary(Entity.ID);
            }
            set
            {
                InternalCalls.CameraComponent_SetPrimary(Entity.ID, value);
            }
        }

        public bool FixedAspectRatio
        {
            get
            {
                return InternalCalls.CameraComponent_GetFixedAspectRatio(Entity.ID);
            }
            set
            {
                InternalCalls.CameraComponent_SetFixedAspectRatio(Entity.ID, value);
            }
        }
    }

    // Replicated from SceneRendererContext.cpp
    public enum CaptureRenderType : byte
    {
        Rendered = 0, Wireframe, LightingOnly, PrePostProcessing, PathTraced,
        Albedo, Depth, LinearDepth, Position, Normal, Emissive, Roughness, Metallic, Specular, AmbientOcclusion, Velocity, EntityID, SubmeshIndex
    }

    public enum CaptureMaskType : byte
    {
        None,
        Exclusive,
        Inclusive
    }

    public class CaptureComponent : Component
    {
        public VirtualTexture Target
        {
            get
            {
                ulong handle = InternalCalls.CaptureComponent_GetTarget(Entity.ID);
                return handle == 0 ? null : new VirtualTexture(handle);
            }
            set
            {
                InternalCalls.CaptureComponent_SetTarget(Entity.ID, value == null ? 0 : value.Handle);
            }
        }

        public CaptureRenderType RenderType
        {
            get { return (CaptureRenderType)InternalCalls.CaptureComponent_GetRenderType(Entity.ID); }
            set { InternalCalls.CaptureComponent_SetRenderType(Entity.ID, (byte)value); }
        }

        public CaptureMaskType MaskType
        {
            get { return (CaptureMaskType)InternalCalls.CaptureComponent_GetMaskType(Entity.ID); }
            set { InternalCalls.CaptureComponent_SetMaskType(Entity.ID, (byte)value); }
        }

        public bool Capture
        {
            get { return InternalCalls.CaptureComponent_GetCapture(Entity.ID); }
            set { InternalCalls.CaptureComponent_SetCapture(Entity.ID, value); }
        }

        public bool Cumulative
        {
            get { return InternalCalls.CaptureComponent_GetCumulative(Entity.ID); }
            set { InternalCalls.CaptureComponent_SetCumulative(Entity.ID, value); }
        }

        public void AddToMask(Entity entity)
        {
            InternalCalls.CaptureComponent_MaskAdd(Entity.ID, entity.ID);
        }

        public void RemoveFromMask(Entity entity)
        {
            InternalCalls.CaptureComponent_MaskRemove(Entity.ID, entity.ID);
        }

        public void ClearMask()
        {
            InternalCalls.CaptureComponent_MaskClear(Entity.ID);
        }
    }

        public class ScriptComponent : Component
    {
        public ScriptComponent(ulong entityID, string script)
        {
            InternalCalls.Entity_AddScriptComponent(entityID, script);
        }

        public string ScriptName
        {
            get
            {
                InternalCalls.ScriptComponent_GetScriptName(Entity.ID, out string scriptName);
                return ScriptName;
            }
            set
            {
                InternalCalls.ScriptComponent_SetScriptName(Entity.ID, value);
            }
        }
    }

    public class SpriteRendererComponent : Component
    {
        public Texture Texture
        {
            get
            {
                ulong handle = InternalCalls.SpriteRendererComponent_GetTexture(Entity.ID);
                return handle == 0 ? null : new Texture(handle);
            }
            set
            {
                InternalCalls.SpriteRendererComponent_SetTexture(Entity.ID, value == null ? 0 : value.Handle);
            }
        }

        public Vector4 Color
        {
            get
            {
                InternalCalls.SpriteRendererComponent_GetColor(Entity.ID, out Vector4 color);
                return color;
            }
            set
            {
                InternalCalls.SpriteRendererComponent_SetColor(Entity.ID, ref value);
            }
        }

        public float TilingFactor
        {
            get
            {
                return InternalCalls.SpriteRendererComponent_GetTilingFactor(Entity.ID);
            }
            set
            {
                InternalCalls.SpriteRendererComponent_SetTilingFactor(Entity.ID, value);
            }
        }
    }

    public class CircleRendererComponent : Component
    {
        public Vector4 Color
        {
            get
            {
                InternalCalls.CircleRendererComponent_GetColor(Entity.ID, out Vector4 color);
                return color;
            }
            set
            {
                InternalCalls.CircleRendererComponent_SetColor(Entity.ID, ref value);
            }
        }

        public float Thickness
        {
            get
            {
                return InternalCalls.CircleRendererComponent_GetThickness(Entity.ID);
            }
            set
            {
                InternalCalls.CircleRendererComponent_SetThickness(Entity.ID, value);
            }
        }

        public float Fade
        {
            get
            {
                return InternalCalls.CircleRendererComponent_GetFade(Entity.ID);
            }
            set
            {
                InternalCalls.CircleRendererComponent_SetFade(Entity.ID, value);
            }
        }
    }

    public class TextComponent : Component
    {
        public string Text
        {
            get
            {
                InternalCalls.TextComponent_GetText(Entity.ID, out string text);
                return text;
            }
            set
            {
                InternalCalls.TextComponent_SetText(Entity.ID, value);
            }
        }

        public Vector3 Color
        {
            get
            {
                InternalCalls.TextComponent_GetColor(Entity.ID, out Vector3 color);
                return color;
            }
            set
            {
                InternalCalls.TextComponent_SetColor(Entity.ID, ref value);
            }
        }
    }
    
    public class RigidBody2DComponent : Component
    {

        public void ApplyLinearImpulse(Vector2 impulse, Vector2 worldPosition, bool wake)
        {
            InternalCalls.RigidBody2DComponent_ApplyLinearImpulse(Entity.ID, ref impulse, ref worldPosition, wake);
        }

        public void ApplyLinearImpulse(Vector2 impulse, bool wake)
        {
            InternalCalls.RigidBody2DComponent_ApplyLinearImpulseToCenter(Entity.ID, ref impulse, wake);
        }

    }

    public class MaterialTable
    {
        internal MaterialTable(ulong id)
        {
            EntityID = id;
        }

        private readonly ulong EntityID;

        public Material this[uint slot]
        {
            get
            {
                ulong handle = InternalCalls.StaticMeshComponent_GetMaterial(EntityID, slot);
                return handle == 0 ? null : new Material(handle);
            }
            set
            {
                InternalCalls.StaticMeshComponent_SetMaterial(EntityID, value == null ? 0 : value.Handle, slot);
            }
        }
    }

    public class AnimationPlayer
    {
        internal AnimationPlayer(ulong id)
        {
            EntityID = id;
        }

        private readonly ulong EntityID;

        public AnimationGraph AnimationGraph
        {
            get
            {
                ulong handle = InternalCalls.StaticMeshComponent_GetAnimationGraph(EntityID);
                return handle == 0 ? null : new AnimationGraph(handle);
            }
            set
            {
                InternalCalls.StaticMeshComponent_SetAnimationGraph(EntityID, value == null ? 0 : value.Handle);
            }
        }

        public bool SetParameter(string name, bool value)       { return InternalCalls.StaticMeshComponent_SetAnimationGraphParameterBool(EntityID, name, value); }
        public bool SetParameter(string name, int value)        { return InternalCalls.StaticMeshComponent_SetAnimationGraphParameterInt(EntityID, name, value); }
        public bool SetParameter(string name, float value)      { return InternalCalls.StaticMeshComponent_SetAnimationGraphParameterFloat(EntityID, name, value); }
        public bool SetParameter(string name, Vector2 value)    { return InternalCalls.StaticMeshComponent_SetAnimationGraphParameterVector2(EntityID, name, ref value); }
        public bool SetParameter(string name, Vector3 value)    { return InternalCalls.StaticMeshComponent_SetAnimationGraphParameterVector3(EntityID, name, ref value); }
        public bool SetParameter(string name, Vector4 value)    { return InternalCalls.StaticMeshComponent_SetAnimationGraphParameterVector4(EntityID, name, ref value); }
        public bool SetParameter(string name, Transform value)  { return InternalCalls.StaticMeshComponent_SetAnimationGraphParameterTransform(EntityID, name, ref value); }
    }
    public class StaticMeshComponent : Component
    {
        public Mesh Mesh
        {
            get
            {
                ulong handle = InternalCalls.StaticMeshComponent_GetMesh(Entity.ID);
                return handle == 0 ? null : new Mesh(handle);
            }
            set
            {
                InternalCalls.StaticMeshComponent_SetMesh(Entity.ID, value == null ? 0 : value.Handle);
            }
        }

        public AnimationPlayer AnimationPlayer
        {
            get
            {
                return new AnimationPlayer(Entity.ID);
            }
        }

        public MaterialTable Materials
        {
            get
            {
                return new MaterialTable(Entity.ID);
            }
        }

        public Transform GetBoneTransform(string name)
        {
            InternalCalls.StaticMeshComponent_GetBoneTransform(Entity.ID, name, out Transform transform);
            return transform;
        }
    }

    public class DirectionalLightComponent : Component
    {
        public Vector3 Color
        {
            get
            {
                InternalCalls.DirectionalLightComponent_GetColor(Entity.ID, out Vector3 color);
                return color;
            }
            set
            {
                InternalCalls.DirectionalLightComponent_SetColor(Entity.ID, ref value);
            }
        }

        public float Intensity
        {
            get
            {
                return InternalCalls.DirectionalLightComponent_GetIntensity(Entity.ID);
            }
            set
            {
                InternalCalls.DirectionalLightComponent_SetIntensity(Entity.ID, value);
            }
        }
    }

    public class PointLightComponent : Component
    {
        public Vector3 Color
        {
            get
            {
                InternalCalls.PointLightComponent_GetColor(Entity.ID, out Vector3 color);
                return color;
            }
            set
            {
                InternalCalls.PointLightComponent_SetColor(Entity.ID, ref value);
            }
        }

        public float Intensity
        {
            get
            {
                return InternalCalls.PointLightComponent_GetIntensity(Entity.ID);
            }
            set
            {
                InternalCalls.PointLightComponent_SetIntensity(Entity.ID, value);
            }
        }

        public float Radius
        {
            get
            {
                return InternalCalls.PointLightComponent_GetRadius(Entity.ID);
            }
            set
            {
                InternalCalls.PointLightComponent_SetRadius(Entity.ID, value);
            }
        }

        public bool CastsShadows
        {
            get
            {
                return InternalCalls.PointLightComponent_GetCastsShadows(Entity.ID);
            }
            set
            {
                InternalCalls.PointLightComponent_SetCastsShadows(Entity.ID, value);
            }
        }
    }

    public class SkyLightComponent : Component
    {
        public float Intensity
        {
            get
            {
                return InternalCalls.SkyLightComponent_GetIntensity(Entity.ID);
            }
            set
            {
                InternalCalls.SkyLightComponent_SetIntensity(Entity.ID, value);
            }
        }
    }

    public class AudioComponent : Component
    {
        public Audio Audio
        {
            get
            {
                ulong handle = InternalCalls.AudioComponent_GetAudio(Entity.ID);
                return handle == 0 ? null : new Audio(handle);
            }
            set
            {
                InternalCalls.AudioComponent_SetAudio(Entity.ID, value == null ? 0 : value.Handle);
            }
        }

        public uint Position
        {
            get
            {
                return InternalCalls.AudioComponent_GetPosition(Entity.ID);
            }
            set
            {
                InternalCalls.AudioComponent_SetPosition(Entity.ID, value);
            }
        }

        public float Volume
        {
            get
            {
                return InternalCalls.AudioComponent_GetVolume(Entity.ID);
            }
            set
            {
                InternalCalls.AudioComponent_SetVolume(Entity.ID, value);
            }
        }

        public bool Spatial
        {
            get
            {
                return InternalCalls.AudioComponent_Get3D(Entity.ID);
            }
            set
            {
                InternalCalls.AudioComponent_Set3D(Entity.ID, value);
            }
        }

        public bool Loop
        {
            get
            {
                return InternalCalls.AudioComponent_GetIsLooping(Entity.ID);
            }
            set
            {
                InternalCalls.AudioComponent_SetIsLooping(Entity.ID, value);
            }
        }

        public bool StartOnAwake
        {
            get
            {
                return InternalCalls.AudioComponent_GetStartOnAwake(Entity.ID);
            }
            set
            {
                InternalCalls.AudioComponent_SetStartOnAwake(Entity.ID, value);
            }
        }

        public float Pan
        {
            get
            {
                return InternalCalls.AudioComponent_GetPan(Entity.ID);
            }
            set
            {
                InternalCalls.AudioComponent_SetPan(Entity.ID, value);
            }
        }

        public float Speed
        {
            get
            {
                return InternalCalls.AudioComponent_GetSpeed(Entity.ID);
            }
            set
            {
                InternalCalls.AudioComponent_SetSpeed(Entity.ID, value);
            }
        }

        public float Echo
        {
            get
            {
                return InternalCalls.AudioComponent_GetEcho(Entity.ID);
            }
            set
            {
                InternalCalls.AudioComponent_SetEcho(Entity.ID, value);
            }
        }
    }

    public enum SplineType : byte
    {
        Curve = 0,
        Linear,
        Constant
    }

    public struct SplinePoint
    {
        private ulong EntityID;
        private uint Index;

        internal SplinePoint(ulong entityID, uint index)
        {
            EntityID = entityID;
            Index = index;
        }

        public Vector3 Position
        {
            get
            {
                InternalCalls.SplineComponent_GetPosition(EntityID, Index, out Vector3 position);
                return position;
            }

            set { InternalCalls.SplineComponent_SetPosition(EntityID, Index, ref value); }
        }

        public Vector3 Tangent
        {
            get
            {
                InternalCalls.SplineComponent_GetTangent(EntityID, Index, out Vector3 tangent);
                return tangent;
            }

            set { InternalCalls.SplineComponent_SetTangent(EntityID, Index, ref value); }
        }

        public SplineType Type
        {
            get { return (SplineType)InternalCalls.SplineComponent_GetType(EntityID, Index); }

            set { InternalCalls.SplineComponent_SetType(EntityID, Index, (byte)value); }
        }
    }

    public class SplineComponent : Component
    {
        public SplinePoint[] Points
        {
            get
            {
                uint pointCount = InternalCalls.SplineComponent_GetPointCount(Entity.ID);
                SplinePoint[] points = new SplinePoint[pointCount];

                for (uint pointIndex = 0; pointIndex < pointCount; pointIndex++)
                    points[pointIndex] = new SplinePoint(Entity.ID, pointIndex);

                return points;
            }
        }

        public Vector3 GetPositionWeighted(float weight)
        {
            InternalCalls.SplineComponent_GetPositionWeighted(Entity.ID, weight, out Vector3 position);
            return position;
        }

        public Vector3 GetPositionDistance(float distance)
        {
            InternalCalls.SplineComponent_GetPositionDistance(Entity.ID, distance, out Vector3 position);
            return position;
        }
    }

    public class RigidBodyComponent : Component
    {
        public string Layer
        {
            get
            {
                InternalCalls.RigidBodyComponent_GetLayer(Entity.ID, out string layer);
                return layer;
            }

            set { InternalCalls.RigidBodyComponent_SetLayer(Entity.ID, value); }
        }

        public bool IsActive()
        {
            return InternalCalls.RigidBodyComponent_IsActive(Entity.ID);
        }

        public void Activate()
        {
            InternalCalls.RigidBodyComponent_Activate(Entity.ID);
        }

        public void Deactivate()
        {
            InternalCalls.RigidBodyComponent_Deactivate(Entity.ID);
        }

        public void SetAllowSleeping(bool allowSleeping)
        {
            InternalCalls.RigidBodyComponent_SetAllowSleeping(Entity.ID, allowSleeping);
        }

        public void ResetSleepTimer()
        {
            InternalCalls.RigidBodyComponent_ResetSleepTimer(Entity.ID);
        }

        public void SetPositionWithoutActivation(Vector3 position)
        {
            InternalCalls.RigidBodyComponent_SetPositionWithoutActivation(Entity.ID, ref position);
        }

        public void SetRotationWithoutActivation(Vector3 rotation)
        {
            InternalCalls.RigidBodyComponent_SetRotationWithoutActivation(Entity.ID, ref rotation);
        }

        public void AddForce(Vector3 force)
        {
            InternalCalls.RigidBodyComponent_AddForce(Entity.ID, ref force);
        }

        public void AddForce(Vector3 force, Vector3 position)
        {
            InternalCalls.RigidBodyComponent_AddForceAtPosition(Entity.ID, ref force, ref position);
        }

        public void AddImpulse(Vector3 impulse)
        {
            InternalCalls.RigidBodyComponent_AddImpulse(Entity.ID, ref impulse);
        }

        public void AddImpulse(Vector3 impulse, Vector3 position)
        {
            InternalCalls.RigidBodyComponent_AddImpulseAtPosition(Entity.ID, ref impulse, ref position);
        }

        public void AddAngularImpulse(Vector3 angularImpulse)
        {
            InternalCalls.RigidBodyComponent_AddAngularImpulse(Entity.ID, ref angularImpulse);
        }

        public void AddTorque(Vector3 torque)
        {
            InternalCalls.RigidBodyComponent_AddTorque(Entity.ID, ref torque);
        }

        public void AddForceAndTorque(Vector3 force, Vector3 torque)
        {
            InternalCalls.RigidBodyComponent_AddForceAndTorque(Entity.ID, ref force, ref torque);
        }

        public void ApplyBuoyancyImpulse(Vector3 surfacePosition, Vector3 surfaceNormal, float buoyancy, float linearDrag, float angularDrag, Vector3 fluidVelocity, float deltaTime)
        {
            InternalCalls.RigidBodyComponent_ApplyBuoyancyImpulse(Entity.ID, ref surfacePosition, ref surfaceNormal, buoyancy, linearDrag, angularDrag, ref fluidVelocity, deltaTime);
        }

        public void MoveKinematic(Vector3 position, Vector3 rotation, float deltaTime)
        {
            InternalCalls.RigidBodyComponent_MoveKinematic(Entity.ID, ref position, ref rotation, deltaTime);
        }

        public Vector3 GetCenterOfMassPosition()
        {
            InternalCalls.RigidBodyComponent_GetCenterOfMassPosition(Entity.ID, out Vector3 position);
            return position;
        }

        public Vector3 GetAccumulatedForce()
        {
            InternalCalls.RigidBodyComponent_GetAccumulatedForce(Entity.ID, out Vector3 force);
            return force;
        }

        public Vector3 GetAccumulatedTorque()
        {
            InternalCalls.RigidBodyComponent_GetAccumulatedTorque(Entity.ID, out Vector3 torque);
            return torque;
        }

        public void ResetForce()
        {
            InternalCalls.RigidBodyComponent_ResetForce(Entity.ID);
        }

        public void ResetTorque()
        {
            InternalCalls.RigidBodyComponent_ResetTorque(Entity.ID);
        }

        public void ResetMotion()
        {
            InternalCalls.RigidBodyComponent_ResetMotion(Entity.ID);
        }

        public Vector3 GetLinearVelocity()
        {
            InternalCalls.RigidBodyComponent_GetLinearVelocity(Entity.ID, out Vector3 linearVelocity);
            return linearVelocity;
        }

        public void SetLinearVelocity(Vector3 linearVelocity)
        {
            InternalCalls.RigidBodyComponent_SetLinearVelocity(Entity.ID, ref linearVelocity);
        }

        public void AddLinearVelocity(Vector3 linearVelocity)
        {
            InternalCalls.RigidBodyComponent_AddLinearVelocity(Entity.ID, ref linearVelocity);
        }

        public Vector3 GetAngularVelocity()
        {
            InternalCalls.RigidBodyComponent_GetAngularVelocity(Entity.ID, out Vector3 angularVelocity);
            return angularVelocity;
        }

        public void SetAngularVelocity(Vector3 angularVelocity)
        {
            InternalCalls.RigidBodyComponent_SetAngularVelocity(Entity.ID, ref angularVelocity);
        }

        public void GetLinearAndAngularVelocity(out Vector3 linearVelocity, out Vector3 angularVelocity)
        {
            InternalCalls.RigidBodyComponent_GetLinearAndAngularVelocity(Entity.ID, out linearVelocity, out angularVelocity);
        }

        public void SetLinearAndAngularVelocity(Vector3 linearVelocity, Vector3 angularVelocity)
        {
            InternalCalls.RigidBodyComponent_SetLinearAndAngularVelocity(Entity.ID, ref linearVelocity, ref angularVelocity);
        }

        public void AddLinearAndAngularVelocity(Vector3 linearVelocity, Vector3 angularVelocity)
        {
            InternalCalls.RigidBodyComponent_AddLinearAndAngularVelocity(Entity.ID, ref linearVelocity, ref angularVelocity);
        }

        public Vector3 GetPointVelocity(Vector3 point)
        {
            InternalCalls.RigidBodyComponent_GetPointVelocity(Entity.ID, ref point, out Vector3 position);
            return position;
        }
    }

    public class SoftBodyComponent : Component
    {
        public string Layer
        {
            get
            {
                InternalCalls.SoftBodyComponent_GetLayer(Entity.ID, out string layer);
                return layer;
            }

            set { InternalCalls.SoftBodyComponent_SetLayer(Entity.ID, value); }
        }

        public void SetVertexPosition(uint index, Vector3 position)
        {
            InternalCalls.SoftBodyComponent_SetVertexPosition(Entity.ID, index, ref position);
        }

        public void SetVertexVelocity(uint index, Vector3 velocity)
        {
            InternalCalls.SoftBodyComponent_SetVertexVelocity(Entity.ID, index, ref velocity);
        }

        public void SetPositionWeighted(Vector3 position)
        {
            InternalCalls.SoftBodyComponent_SetPositionWeighted(Entity.ID, ref position);
        }

        public Vector3 GetCenterOfMassPosition()
        {
            InternalCalls.SoftBodyComponent_GetCenterOfMassPosition(Entity.ID, out Vector3 position);
            return position;
        }

        public void SetFixedPosition(bool fixedPosition)
        {
            InternalCalls.SoftBodyComponent_SetFixedPosition(Entity.ID, fixedPosition);
        }

        // TODO: Add all methods present on the RigidBodyComponent
    }

    public class CharacterMovementComponent : Component
    {
        public string Layer
        {
            get
            {
                InternalCalls.CharacterMovementComponent_GetLayer(Entity.ID, out string layer);
                return layer;
            }

            set { InternalCalls.CharacterMovementComponent_SetLayer(Entity.ID, value); }
        }

        public float MaxWalkSpeed
        {
            get { return InternalCalls.CharacterMovementComponent_GetMaxWalkSpeed(Entity.ID); }
            set { InternalCalls.CharacterMovementComponent_SetMaxWalkSpeed(Entity.ID, value); }
        }

        public float JumpSpeed
        {
            get { return InternalCalls.CharacterMovementComponent_GetJumpSpeed(Entity.ID); }
            set { InternalCalls.CharacterMovementComponent_SetJumpSpeed(Entity.ID, value); }
        }

        public float GravityScale
        {
            get { return InternalCalls.CharacterMovementComponent_GetGravityScale(Entity.ID); }
            set { InternalCalls.CharacterMovementComponent_SetGravityScale(Entity.ID, value); }
        }

        public float MaxSlopeAngle
        {
            get { return InternalCalls.CharacterMovementComponent_GetMaxSlopeAngle(Entity.ID); }
            set { InternalCalls.CharacterMovementComponent_SetMaxSlopeAngle(Entity.ID, value); }
        }

        public float AirControl
        {
            get { return InternalCalls.CharacterMovementComponent_GetAirControl(Entity.ID); }
            set { InternalCalls.CharacterMovementComponent_SetAirControl(Entity.ID, value); }
        }

        public float VelocityBlendWeight
        {
            get { return InternalCalls.CharacterMovementComponent_GetVelocityBlendWeight(Entity.ID); }
            set { InternalCalls.CharacterMovementComponent_SetVelocityBlendWeight(Entity.ID, value); }
        }

        public bool RotateToMotion
        {
            get { return InternalCalls.CharacterMovementComponent_GetRotateToMotion(Entity.ID); }
            set { InternalCalls.CharacterMovementComponent_SetRotateToMotion(Entity.ID, value); }
        }

        public float RotationRate
        {
            get { return InternalCalls.CharacterMovementComponent_GetRotationRate(Entity.ID); }
            set { InternalCalls.CharacterMovementComponent_SetRotationRate(Entity.ID, value); }
        }

        public void SetMovementInput(Vector3 movement)
        {
            InternalCalls.CharacterMovementComponent_SetMovementInput(Entity.ID, ref movement);
        }

        public void Jump()
        {
            InternalCalls.CharacterMovementComponent_Jump(Entity.ID);
        }

        public bool IsFalling()
        {
            return InternalCalls.CharacterMovementComponent_IsFalling(Entity.ID);
        }

        public Vector3 GetLinearVelocity()
        {
            InternalCalls.CharacterMovementComponent_GetLinearVelocity(Entity.ID, out Vector3 linearVelocity);
            return linearVelocity;
        }

        public Vector3 GetGroundVelocity()
        {
            InternalCalls.CharacterMovementComponent_GetGroundVelocity(Entity.ID, out Vector3 groundVelocity);
            return groundVelocity;
        }
    }

    public class SpringArmComponent : Component
    {
        public float TargetLength
        {
            get { return InternalCalls.SpringArmComponent_GetTargetLength(Entity.ID); }
            set { InternalCalls.SpringArmComponent_SetTargetLength(Entity.ID, value); }
        }

        public float ProbeRadius
        {
            get { return InternalCalls.SpringArmComponent_GetProbeRadius(Entity.ID); }
            set { InternalCalls.SpringArmComponent_SetProbeRadius(Entity.ID, value); }
        }

        public Vector3 TargetOffset
        {
            get
            {
                InternalCalls.SpringArmComponent_GetTargetOffset(Entity.ID, out Vector3 targetOffset);
                return targetOffset;
            }

            set { InternalCalls.SpringArmComponent_SetTargetOffset(Entity.ID, ref value); }
        }

        public Vector3 SocketOffset
        {
            get
            {
                InternalCalls.SpringArmComponent_GetSocketOffset(Entity.ID, out Vector3 socketOffset);
                return socketOffset;
            }

            set { InternalCalls.SpringArmComponent_SetSocketOffset(Entity.ID, ref value); }
        }

        public void ExcludeEntity(Entity entity)
        {
            InternalCalls.SpringArmComponent_ExcludeEntity(Entity.ID, entity.ID);
        }
    }

    public enum FieldType : byte
    {
        Directional,
        Radial,
        Buoyancy
    }

    public class FieldComponent : Component
    {
        public FieldType Type
        {
            get { return (FieldType)InternalCalls.FieldComponent_GetType(Entity.ID); }

            set { InternalCalls.FieldComponent_SetType(Entity.ID, (byte)value); }
        }

        public string Layer
        {
            get {
                InternalCalls.FieldComponent_GetLayer(Entity.ID, out string layer);
                return layer;
            }

            set { InternalCalls.FieldComponent_SetLayer(Entity.ID, value); }
        }

        public Vector3 Force
        {
            get
            {
                InternalCalls.FieldComponent_GetForce(Entity.ID, out Vector3 force);
                return force;
            }

            set
            {
                InternalCalls.FieldComponent_SetForce(Entity.ID, ref value);
            }
        }

        public float Magnitude
        {
            get { return InternalCalls.FieldComponent_GetMagnitude(Entity.ID); }

            set { InternalCalls.FieldComponent_SetMagnitude(Entity.ID, value); }
        }

        public float Radius
        {
            get { return InternalCalls.FieldComponent_GetRadius(Entity.ID); }

            set { InternalCalls.FieldComponent_SetRadius(Entity.ID, value); }
        }

        public float Falloff
        {
            get { return InternalCalls.FieldComponent_GetFallof(Entity.ID); }

            set { InternalCalls.FieldComponent_SetFallof(Entity.ID, value); }
        }

        public float Buoyancy
        {
            get { return InternalCalls.FieldComponent_GetBuoyancy(Entity.ID); }

            set { InternalCalls.FieldComponent_SetBuoyancy(Entity.ID, value); }
        }

        public float LinearDrag
        {
            get { return InternalCalls.FieldComponent_GetLinearDrag(Entity.ID); }

            set { InternalCalls.FieldComponent_SetLinearDrag(Entity.ID, value); }
        }

        public float AngularDrag
        {
            get { return InternalCalls.FieldComponent_GetAngularDrag(Entity.ID); }

            set { InternalCalls.FieldComponent_SetAngularDrag(Entity.ID, value); }
        }

        public Vector3 FluidVelocity
        {
            get
            {
                InternalCalls.FieldComponent_GetFluidVelocity(Entity.ID, out Vector3 fluidVelocity);
                return fluidVelocity;
            }

            set { InternalCalls.FieldComponent_SetFluidVelocity(Entity.ID, ref value); }
        }
    }

    public class DistanceConstraintComponent : Component
    {
        public DistanceConstraintComponent() {}

        public DistanceConstraintComponent(ulong entityID, Entity target)
        {
            InternalCalls.Entity_AddDistanceConstraintComponentDefault(entityID, target.ID);
        }

        public DistanceConstraintComponent(ulong entityID, Entity target, float distance)
        {
            InternalCalls.Entity_AddDistanceConstraintComponentFixed(entityID, target.ID, distance);
        }

        public DistanceConstraintComponent(ulong entityID, Entity target, float minDistance, float maxDistance)
        {
            InternalCalls.Entity_AddDistanceConstraintComponentRange(entityID, target.ID, minDistance, maxDistance);
        }

        public bool Enabled
        {
            get { return InternalCalls.DistanceConstraintComponent_GetEnabled(Entity.ID); }
            set { InternalCalls.DistanceConstraintComponent_SetEnabled(Entity.ID, value); }
        }

        public Entity Target
        {
            get { return InternalCalls.DistanceConstraintComponent_GetTarget(Entity.ID); }
            set { InternalCalls.DistanceConstraintComponent_SetTarget(Entity.ID, value.ID); }
        }

        public void SetDistance(float distance)
        {
            InternalCalls.DistanceConstraintComponent_SetDistance(Entity.ID, distance);
        }

        public void SetDistanceRange(float minDistance, float maxDistance)
        {
            InternalCalls.DistanceConstraintComponent_SetDistanceRange(Entity.ID, minDistance, maxDistance);
        }
    }

    public class FixedConstraintComponent : Component
    {
        public FixedConstraintComponent() {}

        public FixedConstraintComponent(ulong entityID, Entity target)
        {
            InternalCalls.Entity_AddFixedConstraintComponent(entityID, target.ID);
        }

        public bool Enabled
        {
            get { return InternalCalls.FixedConstraintComponent_GetEnabled(Entity.ID); }
            set { InternalCalls.FixedConstraintComponent_SetEnabled(Entity.ID, value); }
        }

        public Entity Target
        {
            get { return InternalCalls.FixedConstraintComponent_GetTarget(Entity.ID); }
            set { InternalCalls.FixedConstraintComponent_SetTarget(Entity.ID, value.ID); }
        }
    }

    public class ParticleSystemComponent : Component
    {
    }
}