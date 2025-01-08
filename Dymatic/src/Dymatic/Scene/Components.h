#pragma once

#include "SceneCamera.h"
#include "Dymatic/Core/UUID.h"
#include "Dymatic/Renderer/Texture.h"
#include "Dymatic/Renderer/EnvironmentMap.h"
#include "Dymatic/Renderer/Font.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include "Dymatic/Scene/Transform.h"

#include "Dymatic/Renderer/Model.h"
#include "Dymatic/Animation/AnimationGraphPlayer.h"
#include "Dymatic/Renderer/ParticleSystemPlayer.h"
#include "Dymatic/Audio/Audio.h"
#include "Dymatic/Renderer/SceneRendererContext.h"

#include "Dymatic/Asset/AssetManager.h"
#include "Dymatic/Physics/Axis.h"
#include "Dymatic/Math/Fraction.h"

namespace Dymatic {

	typedef uint64_t EntityHandle;
	typedef uint64_t PhysicsLayerID;

	// For internal use
	struct SceneComponent
	{
		UUID SceneID;
	};

	struct IDComponent
	{
		UUID ID;

		IDComponent() = default;
		IDComponent(const IDComponent&) = default;
	};

	struct TagComponent
	{
		std::string Tag;

		TagComponent() = default;
		TagComponent(const TagComponent&) = default;
		TagComponent(const std::string& tag)
			: Tag(tag) {}
	};

	struct RelationshipComponent
	{
		RelationshipComponent() = default;
		RelationshipComponent(const RelationshipComponent&) = default;

		EntityHandle ParentHandle = 0;
		std::vector<EntityHandle> Children;
	};

	struct AttachmentComponent
	{
		AttachmentComponent() = default;
		AttachmentComponent(const AttachmentComponent&) = default;

		std::string BoneName;
	};

	struct PrefabComponent
	{
		PrefabComponent() = default;
		PrefabComponent(const AssetHandle prefabID) : PrefabID(prefabID) {}
		PrefabComponent(const PrefabComponent&) = default;

		AssetHandle PrefabID = 0;
	};

	struct FolderComponent
	{
		glm::vec4 Color = { 1.0f, 1.0f, 1.0f, 1.0f };
	};

	struct TransformComponent
	{
		// Note: Only internal systems should directly modify these. Editor and script systems should use Scene SetEntity Transform methods.
		Transform Transform;

		TransformComponent() = default;
		TransformComponent(const TransformComponent&) = default;

		bool operator==(const TransformComponent& other) const
		{
			return Transform == other.Transform;
		}

		bool operator!=(const TransformComponent& other) const
		{
			return Transform != other.Transform;
		}
	};

	struct SpriteRendererComponent
	{
		glm::vec4 Color{ 1.0f, 1.0f, 1.0f, 1.0f };
		Ref<Texture2D> Texture;
		float TilingFactor = 1.0f;

		SpriteRendererComponent() = default;
		SpriteRendererComponent(const SpriteRendererComponent&) = default;
		SpriteRendererComponent(const glm::vec4& color)
			: Color(color) {}
		SpriteRendererComponent(Ref<Texture2D> texture)
			: Texture(texture) {}
	};

	struct ParticleSystemComponent
	{
		ParticleSystemComponent() = default;
		ParticleSystemComponent(const ParticleSystemComponent& other);

		void SetParticleSystem(const Ref<ParticleSystem> particleSystem, const Ref<MaterialAsset> material = nullptr);
		Ref<ParticleSystem> GetParticleSystem() const { return Player ? Player->GetParticleSystem() : nullptr; }

		Ref<ParticleSystemPlayer> Player = nullptr;
		Ref<MaterialAsset> Material = nullptr;
	};

	struct CircleRendererComponent
	{
		glm::vec4 Color{ 1.0f, 1.0f, 1.0f, 1.0f };
		float Thickness = 1.0f;
		float Fade = 0.005f;

		CircleRendererComponent() = default;
		CircleRendererComponent(const CircleRendererComponent&) = default;
	};

	struct TextComponent
	{
		std::string TextString;
		TextAlignment Alignment = TextAlignment::Left;
		glm::vec4 Color{ 1.0f, 1.0f, 1.0f, 1.0f };
		Ref<Font> Font;
		float Kerning = 0.0f;
		float LineSpacing = 0.0f;
		float MaxWidth = 0.0f;

		TextComponent() = default;
		TextComponent(const TextComponent&) = default;
	};

	struct CameraComponent
	{
		SceneCamera Camera;
		bool Primary = true;
		bool FixedAspectRatio = false;

		CameraComponent() = default;
		CameraComponent(const CameraComponent&) = default;
	};

	struct CaptureComponent
	{
		Ref<Texture2D> Target = nullptr;
		SceneRendererContext::RendererVisualizationMode Type = SceneRendererContext::RendererVisualizationMode::Rendered;
		SceneRendererContext::RenderMaskType MaskType = SceneRendererContext::RenderMaskType::None;
		bool Capture = true;
		bool Cumulative = false;

		// Runtime Only
		Ref<SceneRendererContext> RuntimeRendererContext = nullptr;

		CaptureComponent() = default;
		CaptureComponent(const CaptureComponent&) = default;
	};

	struct ScriptComponent
	{
		std::string ClassName;

		ScriptComponent() = default;
		ScriptComponent(const ScriptComponent&) = default;

		ScriptComponent(const std::string& className)
			: ClassName(className) {}
	};

	// Forward declaration
	class ScriptableEntity;

	struct NativeScriptComponent
	{
		ScriptableEntity* Instance = nullptr;
		ScriptableEntity* (*InstantiateScript)();
		void (*DestroyScript)(NativeScriptComponent*);

		template<typename T>
		void Bind()
		{
			InstantiateScript = []() { return static_cast<ScriptableEntity*>(new T()); };
			DestroyScript = [](NativeScriptComponent* nsc) { delete nsc->Instance; nsc->Instance = nullptr; };
		}
	};

	// Physics
	struct RigidBody2DComponent
	{
		enum class BodyType { Static = 0, Dynamic, Kinematic };
		BodyType Type;
		bool FixedRotation = false;

		// Storage for runtime
		void* RuntimeBody = nullptr;

		RigidBody2DComponent() = default;
		RigidBody2DComponent(const RigidBody2DComponent& other) = default;
	};

	struct BoxCollider2DComponent
	{
		glm::vec2 Offset = { 0.0f, 0.0f };
		glm::vec2 Size = { 0.5f, 0.5f };

		float Density = 1.0f;
		float Friction = 0.5f;
		float Restitution = 0.0f;
		float RestitutionThreshold = 0.5f;

		// Storage for runtime
		void* RuntimeFixture = nullptr;

		BoxCollider2DComponent() = default;
		BoxCollider2DComponent(const BoxCollider2DComponent& other) = default;
	};

	struct CircleCollider2DComponent
	{
		glm::vec2 Offset = { 0.0f, 0.0f };
		float Radius = 0.5f;

		float Density = 1.0f;
		float Friction = 0.5f;
		float Restitution = 0.0f;
		float RestitutionThreshold = 0.5f;

		// Storage for runtime
		void* RuntimeFixture = nullptr;

		CircleCollider2DComponent() = default;
		CircleCollider2DComponent(const CircleCollider2DComponent& other) = default;
	};

	struct StaticMeshComponent
	{
		Ref<Model> m_Model = nullptr;
		std::vector<Ref<MaterialAsset>> m_Materials;
		Ref<AnimationGraphPlayer> m_AnimationGraphPlayer = nullptr;

		StaticMeshComponent() {}

		StaticMeshComponent(Ref<Model> model)
		{
			SetModel(model);
		}

		StaticMeshComponent(const StaticMeshComponent& other);

		inline Ref<Model> GetModel() const { return m_Model; }
		inline Ref<AnimationGraphPlayer> GetAnimationPlayer() const { return m_AnimationGraphPlayer; }
		Ref<Skeleton> GetSkeleton() const { return m_Model ? m_Model->GetSkeleton() : nullptr; }

		void SetModel(Ref<Model> model);
		void SetAnimationGraph(Ref<AnimationGraph> animationGraph);

		void Update(Timestep ts);
	};

	struct DirectionalLightComponent
	{
		glm::vec3 Color = glm::vec3(1.0f, 1.0f, 1.0f);
		float Intensity = 1.0f;

		DirectionalLightComponent() = default;
		DirectionalLightComponent(const DirectionalLightComponent&) = default;
	};

	struct PointLightComponent
	{
		glm::vec3 Color = glm::vec3(1.0f, 1.0f, 1.0f);
		float Intensity = 1.0f;
		float Radius = 1.0f;
		bool CastsShadows = true;

		PointLightComponent() = default;
		PointLightComponent(const PointLightComponent&) = default;
	};

	struct SpotLightComponent
	{
		glm::vec3 Color = glm::vec3(1.0f, 1.0f, 1.0f);

		float CutOff = 12.5f;
		float OuterCutOff = 15.0f;

		float Constant = 1.0f;
		float Linear = 0.09f;
		float Quadratic = 0.032f;

		SpotLightComponent() = default;
		SpotLightComponent(const SpotLightComponent&) = default;
	};

	struct SkyLightComponent
	{
		enum class SkyType { EnvironmentMap, DynamicSky };

		SkyLightComponent() = default;
		SkyLightComponent(const SkyLightComponent&) = default;

		Ref<EnvironmentMap> EnvironmentMap = nullptr;
		Ref<Texture2D> FlowMap = nullptr;
		float Intensity = 1.0f;
		SkyType Type = SkyType::EnvironmentMap;
	};

	struct VolumeComponent
	{
		VolumeComponent() = default;
		VolumeComponent(const VolumeComponent& vc) = default;

		enum class BlendType
		{
			Set = 0, Add
		};

		BlendType Blend = BlendType::Set;

		glm::vec3 Color = glm::vec3(1.0f);
		float ScatteringDistribution = 0.5;
		float ScatteringIntensity = 1.0;
		float ExtinctionScale = 0.5;
	};

	struct DecalComponent
	{
		DecalComponent() = default;

		Ref<Texture2D> Texture;
		bool ConstrainAngle = false;
	};

	struct PostProcessVolumeComponent
	{
		PostProcessVolumeComponent() = default;
		PostProcessVolumeComponent(const PostProcessVolumeComponent& ppvc) = default;

		bool Enabled = true;
		bool Bounded = true;
		Ref<MaterialAsset> Material = nullptr;
	};

	struct AudioComponent
	{
		Ref<Audio> AudioSound = nullptr;
		bool Initialized = false;
		bool StartOnAwake = false;
		uint32_t StartPosition = 0;

		AudioComponent() = default;
		AudioComponent(const AudioComponent& ac) = default;

		void SetStartPosition(const uint32_t startPosition);
	};

	struct SplineComponent
	{
		enum class SplineType { Curve, Linear, Constant };

		struct SplinePoint
		{
			SplineType Type;
			glm::vec3 Position;
			glm::vec3 Tangent;

			SplinePoint(const glm::vec3& position = glm::vec3(0.0f), const glm::vec3& tangent = glm::vec3(0.0f), SplineType type = SplineType::Curve)
				: Position(position), Tangent(tangent), Type(type) {}
		};

		std::vector<SplinePoint> Points;

		SplineComponent();
		SplineComponent(const SplineComponent& sc) = default;

		glm::vec3 Sample(float t) const;
		glm::vec3 SampleDistance(float distance) const;

		void AddPoint();
		void RemovePoint(uint32_t index);
		void DuplicatePoint(uint32_t index);
		void SetPoints(const std::vector<SplinePoint>& points);

	private:
		void Invalidate();
		glm::vec3 SampleSegment(const SplinePoint& startPoint, const SplinePoint& endPoint, float localT) const;

		// Cached world space values
		std::vector<float> SegmentLengths;
		float TotalLength;
	};

	struct RigidBodyComponent
	{
		enum class BodyType { Static = 0, Dynamic, Kinematic };
		enum class MassMode { Density, Mass };

		BodyType Type;
		MassMode Mode;
		PhysicsLayerID Layer = 0;
		bool Sensor = false;

		union
		{
			float Density = 1.0f;
			float Mass;
		};

		float Friction = 0.2f;
		float Restitution = 0.1f;

		// Storage for runtime
		void* RuntimeBody = nullptr;

		RigidBodyComponent() = default;
		RigidBodyComponent(const RigidBodyComponent&) = default;
	};

	struct SoftBodyComponent
	{
		PhysicsLayerID Layer = 0;

		float Friction = 0.2f;
		float Restitution = 0.1f;
		float Pressure = 0.0f;

		float VertexMass = 40.0f;
		float VertexRadius = 0.0f;
		bool UseVertexColorAsWeight = false;

		// Storage for runtime
		void* RuntimeBody = nullptr;
		Ref<Model> RuntimeModel = nullptr;

		SoftBodyComponent() = default;
		SoftBodyComponent(const SoftBodyComponent&) = default;
	};

	struct BoxColliderComponent
	{
		glm::vec3 Size = { 1.0f, 1.0f, 1.0f };

		BoxColliderComponent() = default;
		BoxColliderComponent(const BoxColliderComponent&) = default;
	};

	struct SphereColliderComponent
	{
		float Radius = 1.0f;

		SphereColliderComponent() = default;
		SphereColliderComponent(const SphereColliderComponent&) = default;
	};

	struct CapsuleColliderComponent
	{
		float Radius = 1.0f;
		float HalfHeight = 1.0f;

		CapsuleColliderComponent() = default;
		CapsuleColliderComponent(const CapsuleColliderComponent&) = default;
	};

	struct MeshColliderComponent
	{
		enum class MeshType { Triangle = 0, Convex };
		MeshType Type = MeshType::Convex;

		MeshColliderComponent() = default;
		MeshColliderComponent(const MeshColliderComponent&) = default;
	};

	enum class ConstraintSpace
	{
		LocalSpace,
		WorldSpace,
		Automatic
	};

	enum class ConstraintSwingType
	{
		Cone,
		Pyramid
	};

	struct MotorComponent
	{
		enum class MotorStateType
		{
			Off,
			Velocity,
			Position
		};

		MotorStateType MotorState;
		float MaxMotorAcceleration = 20.0f;

		// TOOD: Just use spring component part
		float Frequency = 2.0f;
		float Damping = 1.0f;
	};

	struct ConstraintComponentBase
	{
		EntityHandle Target;
		bool Enabled = true;

		// Runtime Only
		void* RuntimeConstraint;

		ConstraintComponentBase() = default;
		ConstraintComponentBase(EntityHandle target, bool enabled = true)
			: Target(target), Enabled(enabled) {}
	};

	struct PointConstraintComponent : ConstraintComponentBase
	{
		ConstraintSpace Space = ConstraintSpace::LocalSpace;
		glm::vec3 LocalPoint = glm::vec3(0.0f);
		glm::vec3 TargetPoint = glm::vec3(0.0f);

		PointConstraintComponent() = default;
		PointConstraintComponent(EntityHandle target)
			: ConstraintComponentBase(target) {}
	};

	struct ConeConstraintComponent : ConstraintComponentBase
	{
		ConstraintSpace Space = ConstraintSpace::LocalSpace;

		struct ReferenceFrame
		{
			glm::vec3 Offset = glm::vec3(0.0f);
			glm::vec3 TwistAxis = c_AxisX;
		};

		ReferenceFrame LocalReferenceFrame;
		ReferenceFrame TargetReferenceFrame;
		
		float HalfConeAngle = 0.0f;

		ConeConstraintComponent() = default;
		ConeConstraintComponent(const ConeConstraintComponent& other) = default;

		ConeConstraintComponent(EntityHandle target)
			: ConstraintComponentBase(target) {}
	};

	struct DistanceConstraintComponent : ConstraintComponentBase
	{
		enum class DistanceType { Default, Fixed, Range };
		DistanceType Type = DistanceType::Default;

		union
		{
			// Fixed Only
			float Distance;

			// Range Only
			struct
			{
				float MinDistance;
				float MaxDistance;
			};
		};

		DistanceConstraintComponent() = default;

		void UpdateType();

		DistanceConstraintComponent(EntityHandle target)
			: ConstraintComponentBase(target), Type(DistanceType::Default) {}

		DistanceConstraintComponent(EntityHandle target, float distance)
			: ConstraintComponentBase(target), Type(DistanceType::Fixed), Distance(distance) {}

		DistanceConstraintComponent(EntityHandle target, float minDistance, float maxDistance)
			: ConstraintComponentBase(target), Type(DistanceType::Fixed), MinDistance(minDistance), MaxDistance(maxDistance) {}
	};

	struct SpringConstraintComponent
	{
		enum class SpringType { FrequencyAndDamping, StiffnessAndDamping };
		SpringType Type = SpringType::FrequencyAndDamping;

		union
		{
			float Frequency = 0.0f;
			float Stiffness;
		};

		float Damping = 0.0f;
	};

	struct HingeConstraintComponent : ConstraintComponentBase
	{
		struct ReferenceFrame
		{
			glm::vec3 Point = glm::vec3(0.0f);
			glm::vec3 HingeAxis = c_AxisY;
			glm::vec3 NormalAxis = c_AxisX;
		};

		ConstraintSpace Space = ConstraintSpace::LocalSpace;
		ReferenceFrame LocalReferenceFrame;
		ReferenceFrame TargetReferenceFrame;

		float MinRotation = -180.0f;
		float MaxRotation = 180.0f;

		float MaxFrictionTorque = 0.0f;
	};

	struct FixedConstraintComponent : ConstraintComponentBase
	{
		struct ReferenceFrame
		{
			glm::vec3 Point = glm::vec3(0.0f);
			glm::vec3 AxisX = c_AxisX;
			glm::vec3 AxisY = c_AxisY;
		};

		ConstraintSpace Type = ConstraintSpace::Automatic;

		// Manual World/Local Space specification only
		ReferenceFrame LocalReferenceFrame;
		ReferenceFrame TargetReferenceFrame;

		FixedConstraintComponent() = default;
		FixedConstraintComponent(EntityHandle target)
			: ConstraintComponentBase(target) {}
	};

	struct GearConstraintComponent : ConstraintComponentBase
	{
		ConstraintSpace Space = ConstraintSpace::LocalSpace;
		glm::vec3 LocalHingeAxis = c_AxisX;
		glm::vec3 TargetHingeAxis = c_AxisX;
		Fraction Ratio;

		GearConstraintComponent() = default;
		GearConstraintComponent(EntityHandle target)
			: ConstraintComponentBase(target) {}
	};

	struct PulleyConstraintComponent : ConstraintComponentBase
	{
		static constexpr float AutomaticLengthCalculationFlag = -1.0f;

		struct ReferenceFrame
		{
			glm::vec3 BodyPoint = glm::vec3(0.0f);
			glm::vec3 FixedPoint = glm::vec3(0.0f);
		};

		ConstraintSpace Space = ConstraintSpace::LocalSpace;
		ReferenceFrame LocalReferenceFrame;
		ReferenceFrame TargetReferenceFrame;

		Fraction Ratio;
		float MinLength = 0.0f;
		float MaxLength = AutomaticLengthCalculationFlag;

		PulleyConstraintComponent() = default;
		PulleyConstraintComponent(EntityHandle target)
			: ConstraintComponentBase(target) {}
	};

	struct RackAndPinionConstraintComponent : ConstraintComponentBase
	{
		enum class RatioMode { Properties, Ratio };

		ConstraintSpace Space = ConstraintSpace::LocalSpace;
		RatioMode Mode = RatioMode::Properties;

		glm::vec3 HingeAxis = c_AxisX;
		glm::vec3 SliderAxis = c_AxisX;

		union
		{
			struct
			{
				uint32_t RackTeethCount;
				uint32_t PinionTeethCount;
				float RackLength;
			};

			float Ratio;
		};

		void UpdateMode();

		RackAndPinionConstraintComponent();
		RackAndPinionConstraintComponent(EntityHandle target)
			: ConstraintComponentBase(target) {}
	};

	struct SwingTwistConstraintComponent : ConstraintComponentBase
	{
		struct ReferenceFrame
		{
			glm::vec3 Position = glm::vec3(0.0f);
			glm::vec3 TwistAxis = c_AxisX;
			glm::vec3 PlaneAxis = c_AxisY;
		};

		ConstraintSpace Space = ConstraintSpace::LocalSpace;
		ConstraintSwingType SwingType = ConstraintSwingType::Cone;
		ReferenceFrame LocalReferenceFrame;
		ReferenceFrame TargetReferenceFrame;

		float NormalHalfConeAngle = 0.0f;
		float PlaneHalfConeAngle = 0.0f;
		float TwistMinAngle = 0.0f;
		float TwistMaxAngle = 0.0f;
		float MaxFrictionTorque = 0.0f;

		SwingTwistConstraintComponent() = default;
		SwingTwistConstraintComponent(EntityHandle target)
			: ConstraintComponentBase(target) {}
	};

	struct SliderConstraintComponent : ConstraintComponentBase
	{
		static constexpr float SliderMaxBound = FLT_MAX;

		struct ReferenceFrame
		{
			glm::vec3 Point = glm::vec3(0.0f);
			glm::vec3 SliderAxis = c_AxisX;
			glm::vec3 NormalAxis = c_AxisY;
		};

		ConstraintSpace Space = ConstraintSpace::LocalSpace;
		ReferenceFrame LocalReferenceFrame;
		ReferenceFrame TargetReferenceFrame;

		float SliderMin = -SliderMaxBound;
		float SliderMax = SliderMaxBound;
		float MaxFrictionForce = 0.0f;

		SliderConstraintComponent() = default;
		SliderConstraintComponent(EntityHandle target)
			: ConstraintComponentBase(target) {}
	};

	struct SixDOFConstraintComponent : ConstraintComponentBase
	{
		enum Axis
		{
			TranslationX,
			TranslationY,
			TranslationZ,

			RotationX,
			RotationY,
			RotationZ,

			AxisCount
		};

		enum class AxisStatus
		{
			Free,
			Locked,
			Custom
		};

		struct ReferenceFrame
		{
			glm::vec3 Position = glm::vec3(0.0f);
			glm::vec3 AxisX = c_AxisX;
			glm::vec3 AxisY = c_AxisY;
		};

		ConstraintSpace Space = ConstraintSpace::LocalSpace;
		ReferenceFrame LocalReferenceFrame;
		ReferenceFrame TargetReferenceFrame;
		ConstraintSwingType SwingType = ConstraintSwingType::Cone;

		std::array<float, Axis::AxisCount> MaxFriction = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
		std::array<float, Axis::AxisCount> LimitMin = { -FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX };
		std::array<float, Axis::AxisCount> LimitMax = {  FLT_MAX,  FLT_MAX,  FLT_MAX,  FLT_MAX,  FLT_MAX,  FLT_MAX };

		SixDOFConstraintComponent() = default;
		SixDOFConstraintComponent(EntityHandle target)
			: ConstraintComponentBase(target) {}

		AxisStatus GetAxisStatus(const Axis axis) const;
		void SetAxisStatus(const Axis axis, const AxisStatus status);
	};

	struct FollowConstraintComponent : ConstraintComponentBase
	{
		enum class RotationConstraintType
		{
			Free,
			AroundTangent,
			AroundNormal,
			AroundBinormal,
			ToPath,
			Constrained,
		};

		bool Looping = true;
		glm::vec3 Normal = c_AxisY;

		float StartFraction = 0.0f;
		float MaxFrictionForce = 0.0f;

		RotationConstraintType RotationConstraint = RotationConstraintType::Free;

		EntityHandle BaseTarget = 0;
		void* BaseRuntimeBody = nullptr;

		// Motor
		MotorComponent Motor;
		float TargetVelocity = 0.0f;
		float TargetPathFraction = 0.0f;
		float MaxFrictionAcceleration = 0.0f;

		FollowConstraintComponent() = default;
		FollowConstraintComponent(EntityHandle target)
			: ConstraintComponentBase(target) {}
	};

	struct RagdollComponent
	{
		RagdollComponent() = default;
		RagdollComponent(const RagdollComponent&) = default;

		PhysicsLayerID Layer = 0;

		// Runtime Only
		void* RuntimeBody = nullptr;
		Ref<BoneMatrixList> RuntimePose = nullptr;
	};

	struct CharacterMovementComponent
	{
		CharacterMovementComponent() = default;
		CharacterMovementComponent(const CharacterMovementComponent&) = default;

		PhysicsLayerID Layer;

		float Mass = 70.0f;
		float CapsuleRadius = 0.42f;
		float CapsuleHeight = 1.92f;
		float InnerShapeFraction = 0.9f;

		float MaxWalkSpeed = 6.0f;
		float JumpSpeed = 4.0f;
		float GravityScale = 1.0f;
		float MaxSlopeAngle = 45.0f;
		float AirControl = 0.2f;
		float VelocityBlendWeight = 0.25f;

		bool RotateToMotion = true;
		float RotationRate = 540.0f;

		float MaxStrength = 100.0f;
		float Friction = 0.5f;
		
		float MaxStepHeight = 0.45f;
		float MinStepForward = 0.02f;
		bool StickToFloor = true;
		
		// Runtime storage
		glm::vec3 RuntimeMovementDirection;
		glm::vec3 PreviousMovementDirection;
		glm::vec3 RuntimeLinearVelocity;
		glm::vec3 RuntimeGroundVelocity;
		bool RuntimeJump;
		bool IsFalling;
		void* RuntimeController = nullptr;
	};
	
	struct VehicleMovementComponent
	{
		VehicleMovementComponent() = default;
		VehicleMovementComponent(const VehicleMovementComponent&) = default;

		struct Wheel
		{

		};

		std::vector<Wheel> Wheels;

		// Runtime storage
		void* VehicleController = nullptr;
	};

	struct SpringArmComponent
	{
		SpringArmComponent()
		{
			ExclusionMask = CreateRef<std::unordered_set<EntityHandle>>();
		}
		
		SpringArmComponent(const SpringArmComponent& other)
		{
			ExclusionMask = CreateRef<std::unordered_set<EntityHandle>>(*other.ExclusionMask);
		}
		
		float TargetLength = 3.0f;
		float ProbeRadius = 0.12f;
		glm::vec3 TargetOffset = glm::vec3(0.0f);
		glm::vec3 SocketOffset = glm::vec3(0.0f);

		// Runtime Storage
		float CurrentLength = -1.0f;
		Ref<std::unordered_set<EntityHandle>> ExclusionMask;
	};

	struct FieldComponent
	{
		enum class FieldType { Directional, Radial, Buoyancy };
		FieldType Type;

		PhysicsLayerID Layer = 0;

		union
		{
			// Directional
			glm::vec3 Force;

			// Radial
			struct
			{
				float Magnitude;
				float Radius;
				float Falloff;
			};

			// Bouyancy
			struct
			{
				float Buoyancy;
				float LinearDrag;
				float AngularDrag;
				glm::vec3 FluidVelocity;
			};
		};

		FieldComponent()
		{
			SetType(FieldType::Directional);
		}

		FieldComponent(const FieldComponent&) = default;
		void SetType(FieldType type);
	};

	struct LandscapeComponent
	{
		LandscapeComponent();
		LandscapeComponent(const LandscapeComponent&) = default;

		void Allocate();
		void Build();

		glm::uvec2 Resolution = glm::uvec2(32);
		Ref<ScopedBuffer> Data;

		Ref<Model> LandscapeMesh;
		Ref<MaterialAsset> Material = nullptr;

		bool Physics = true;
		PhysicsLayerID Layer;
		void* RuntimeBody = nullptr;

		// TODO: Foliage Assets (mesh, size variation, distribution etc)
	};

	struct NavigationMeshComponent
	{
		Ref<Model> DebugMesh;
	};

	struct NavigationModifierComponent
	{
		float Weight;
	};

	struct NavigationLinkComponent
	{
		EntityHandle TargetLink;
	};

	// UI Components

	//struct UIComponent
	//{
	//	enum UIType
	//	{
	//		Canvas,
	//		Image,
	//		Button
	//	};
	//
	//	struct UICanvasComponent
	//	{
	//		bool Enabled;
	//		glm::vec2 Min;
	//		glm::vec2 Max;
	//
	//		UICanvasComponent() = default;
	//		UICanvasComponent(const UICanvasComponent&) = default;
	//	};
	//
	//	struct UIImageComponent
	//	{
	//		Ref<Texture2D> Image;
	//
	//
	//		glm::vec2 Anchor;
	//		glm::vec2 Position;
	//		glm::vec2 Size;
	//
	//		UIImageComponent() = default;
	//		UIImageComponent(const UIImageComponent&) = default;
	//	};
	//
	//	struct UIButtonComponent
	//	{
	//		glm::vec4 Color;
	//
	//		UIButtonComponent() = default;
	//		UIButtonComponent(const UIButtonComponent&) = default;
	//	};
	//
	//	UIComponent(UIType type)
	//	{
	//		switch (type)
	//		{
	//		case Canvas: Element = new UICanvasComponent;
	//		case Image: Element = new UIImageComponent;
	//		case Button: Element = new UIButtonComponent;
	//		}
	//	}
	//	~UIComponent()
	//	{
	//		if (Element)
	//			delete Element;
	//	}
	//
	//	UIType GetType() { return Type; }
	//	void* GetElement() { return Element; }
	//
	//	UIComponent(const UIComponent&) = default;
	//
	//private:
	//	UIType Type;
	//	void* Element = nullptr;
	//};

	struct UICanvasComponent
	{
		bool Enabled = true;

		glm::vec2 Min { -1.0f, -1.0f };
		glm::vec2 Max {  1.0f,  1.0f };

		UICanvasComponent() = default;
		UICanvasComponent(const UICanvasComponent&) = default;
	};

	struct UIImageComponent
	{
		Ref<Texture2D> Image;


		glm::vec2 Anchor;
		glm::vec2 Position;
		glm::vec2 Size;

		UIImageComponent() = default;
		UIImageComponent(const UIImageComponent&) = default;
	};

	struct UIButtonComponent 
	{
		glm::vec4 Color;

		UIButtonComponent() = default;
		UIButtonComponent(const UIButtonComponent&) = default;
	};

	template<typename... Component>
	struct ComponentGroup
	{
	};

	// Note: The order these are listed is important for registration (e.g. A collider must be registered before the rigid body that uses it)
	using AllComponents =
		ComponentGroup<
			PrefabComponent, AttachmentComponent, FolderComponent, TransformComponent,
			SpriteRendererComponent, CircleRendererComponent, TextComponent,
			CameraComponent, CaptureComponent, ScriptComponent, NativeScriptComponent,
			RigidBody2DComponent, BoxCollider2DComponent, CircleCollider2DComponent,
			StaticMeshComponent, DirectionalLightComponent, PointLightComponent, 
			SpotLightComponent, SkyLightComponent, VolumeComponent, PostProcessVolumeComponent,
			AudioComponent, SplineComponent,
			ParticleSystemComponent, DecalComponent,
			BoxColliderComponent, SphereColliderComponent, CapsuleColliderComponent, MeshColliderComponent,
			RigidBodyComponent, SoftBodyComponent,
			PointConstraintComponent, ConeConstraintComponent, DistanceConstraintComponent, SpringConstraintComponent, HingeConstraintComponent,
			FixedConstraintComponent, GearConstraintComponent, PulleyConstraintComponent, RackAndPinionConstraintComponent,
			SwingTwistConstraintComponent, SliderConstraintComponent, SixDOFConstraintComponent, FollowConstraintComponent,
			RagdollComponent, CharacterMovementComponent, VehicleMovementComponent, SpringArmComponent, FieldComponent,
			LandscapeComponent, NavigationMeshComponent, NavigationModifierComponent, NavigationLinkComponent,
			UICanvasComponent, UIImageComponent, UIButtonComponent
		>;

}
