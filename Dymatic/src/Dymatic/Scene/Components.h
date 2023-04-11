#pragma once

#include "SceneCamera.h"
#include "Dymatic/Core/UUID.h"
#include "Dymatic/Renderer/Texture.h"
#include "Dymatic/Renderer/Font.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

//----Particles------//
#include <glm/gtc/constants.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/compatibility.hpp>

#include <random>
//-------------------//

#include "Dymatic/Renderer/Model.h"
#include "Dymatic/Renderer/Animator.h"
#include "Dymatic/Audio/Audio.h"

#include "Dymatic/Asset/AssetManager.h"

namespace Dymatic {

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

	struct TransformComponent
	{
		glm::vec3 Translation = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Rotation = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Scale = { 1.0f, 1.0f, 1.0f };

		TransformComponent() = default;
		TransformComponent(const TransformComponent&) = default;
		TransformComponent(const glm::vec3& translation)
			: Translation(translation) {}

		glm::mat4 GetTransform() const
		{
			glm::mat4 rotation = glm::toMat4(glm::quat(Rotation));

			return glm::translate(glm::mat4(1.0f), Translation)
				* rotation
				* glm::scale(glm::mat4(1.0f), Scale);
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
	public:
		ParticleSystemComponent(uint32_t maxParticles = 100000)
			: m_PoolIndex(maxParticles - 1)
		{
			m_ParticlePool.resize(maxParticles);
		}

		struct ColorPoint
		{
			unsigned int id = 0;
			float point = 1.0f;
			glm::vec4 color = glm::vec4(1.0f);

			unsigned int GetId() { return id; }

			ColorPoint(unsigned int id)
				: id(id)
			{
			}

			ColorPoint(unsigned int id, float point, glm::vec4 color)
				: id(id), point(point), color(color)
			{
			}
		};

		struct Particle
		{
			glm::vec3 Position;
			glm::vec3 Velocity;
			glm::vec4 ColorBegin, ColorEnd, ColorConstant;
			float Rotation = 0.0f;
			float SizeBegin, SizeEnd;

			std::vector<ColorPoint> ColorPoints;

			float LifeTime = 1.0f;
			float LifeRemaining = 0.0f;

			bool Active = false;
		};

		void OnUpdate(Timestep ts)
		{
			for (auto& particle : m_ParticlePool)
			{
				if (!particle.Active)
					continue;

				if (particle.LifeRemaining <= 0.0f)
				{
					particle.Active = false;
					continue;
				}

				particle.LifeRemaining -= ts;

				particle.Velocity += Gravity * (float)ts;

				particle.Position += particle.Velocity * (float)ts;
				particle.Rotation += 0.01f * ts;
			}
		}

		void Emit()
		{
			if (Active)
			{
				for (int i = 0; i < EmissionNumber; i++)
				{
					Particle& particle = m_ParticlePool[m_PoolIndex];
					particle.Active = true;
					particle.Position = Position;
					particle.Rotation = RandomFloat(0, 1) * 2.0f * glm::pi<float>();

					// Velocity
					particle.Velocity = Velocity;
					particle.Velocity.x += VelocityVariation.x * (RandomFloat(0, 1) - 0.5f);
					particle.Velocity.y += VelocityVariation.y * (RandomFloat(0, 1) - 0.5f);
					particle.Velocity.z += VelocityVariation.z * (RandomFloat(0, 1) - 0.5f);

					// Color
					particle.ColorBegin = ColorBegin;
					particle.ColorEnd = ColorEnd;

					particle.ColorConstant = ColorConstant;

					particle.ColorPoints = ColorPoints;

					particle.LifeTime = LifeTime;
					particle.LifeRemaining = LifeTime;
					particle.SizeBegin = SizeBegin + SizeVariation * (RandomFloat(0, 1) - 0.5f);
					particle.SizeEnd = SizeEnd;

					m_PoolIndex = --m_PoolIndex % m_ParticlePool.size();
				}
			}
		}

		void DuplicateColorPoint(int index) { ColorPoints.insert(ColorPoints.begin() + index, ColorPoints[index]); ColorPoints[index].id = GetNextColorPointId(); }

		struct ColorPointOrderKey
		{
			inline bool operator() (const ColorPoint& colorPoint1, const ColorPoint& colorPoint2)
			{
				return (colorPoint1.point < colorPoint2.point);
			}
		};

		void RecalculateColorPointOrder()
		{
			std::sort(ColorPoints.begin(), ColorPoints.end(), ColorPointOrderKey());
		}

		glm::vec3 Offset = glm::vec3(0.0f, 0.0f, 0.0f);
		glm::vec3 Position = glm::vec3(0.0f, 0.0f, 0.0f);
		glm::vec3 Velocity = glm::vec3(0.0f, 0.0f, 0.0f);
		glm::vec3 VelocityVariation = glm::vec3(0.0f, 0.0f, 0.0f);
		glm::vec3 Gravity = glm::vec3(0.0f, -9.8f, 0.0f);
		glm::vec4 ColorBegin = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		glm::vec4 ColorEnd = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		glm::vec4 ColorConstant = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		float SizeBegin = 1.0f;
		float SizeEnd = 1.0f;
		float SizeVariation = 0.0f;
		float LifeTime = 1.0f;
		int EmissionNumber = 10;
		bool Active = true;
		bool FaceCamera = true;

		int ColorMethod = 0;
		unsigned int nextColorPointId = 1;
		unsigned int GetNextColorPointId() { nextColorPointId++; return nextColorPointId; }
		std::vector<ColorPoint> ColorPoints;

		std::vector<Particle>& GetParticlePool() { return m_ParticlePool; }
		void ClearParticlePool() { std::fill(m_ParticlePool.begin(), m_ParticlePool.end(), Particle()); }

	private:

		float RandomFloat(float a, float b) {
			float random = ((float)rand()) / (float)RAND_MAX;
			float diff = b - a;
			float r = random * diff;
			return a + r;
		}

		std::vector<Particle> m_ParticlePool;
		uint32_t m_PoolIndex;
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

	struct ScriptComponent
	{
		std::string ClassName;

		ScriptComponent() = default;
		ScriptComponent(const ScriptComponent&) = default;
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
	struct Rigidbody2DComponent
	{
		enum class BodyType { Static = 0, Dynamic, Kinematic };
		BodyType Type;
		bool FixedRotation = false;

		// Storage for runtime
		void* RuntimeBody = nullptr;

		Rigidbody2DComponent() = default;
		Rigidbody2DComponent(const Rigidbody2DComponent& other) = default;
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

	// For internal use
	struct SceneComponent
	{
		UUID SceneID;
	};

	// For internal use
	struct FolderComponent
	{
		glm::vec4 Color = { 1.0f, 1.0f, 1.0f, 1.0f };
	};

	struct StaticMeshComponent
	{
		Ref<Model> m_Model = nullptr;
		std::vector<Ref<Material>> m_Materials;
		Ref<Animator> m_Animator;

		StaticMeshComponent() 
		{ 
			m_Animator = Animator::Create();
		}
		
		StaticMeshComponent(Ref<Model> model) 
		{
			m_Animator = Animator::Create(); 
			SetModel(model);
		}
		
		StaticMeshComponent(const StaticMeshComponent& other) = default;

		inline Ref<Model> GetModel() { return m_Model; }
		inline Ref<Animator> GetAnimator() { return m_Animator; }

		void SetModel(Ref<Model> model)
		{
			m_Model = model;
			m_Materials.clear();

			if (model && model->IsLoaded())
				m_Materials.resize(m_Model->GetMeshes().size());
		}

		void LoadAnimation(const std::string& path)
		{
			if (m_Model)
			{
				Ref<Animation> animation = Animation::Create(path, m_Model);
				if (animation->IsLoaded())
					m_Animator->SetAnimation(animation);
				else
					DY_CORE_WARN("Could not load model {0}", path);
			}
			else
				DY_CORE_WARN("Model must be loaded before animation");
		}

		void Update(Timestep ts)
		{
			if (m_Animator)
				m_Animator->UpdateAnimation(ts.GetSeconds());
		}
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
		Ref<Texture2D> SkyboxHDRI;
		Ref<Texture2D> SkyboxFlowMap;
		std::string Filepath;
		float Intensity;
		int Type = 0;

		SkyLightComponent() = default;
		SkyLightComponent(const SkyLightComponent&) = default;

		void Load(const std::string& filepath)
		{
			Ref<Texture2D> hdri = Texture2D::Create(filepath);
			if (hdri->IsLoaded())
			{
				SkyboxHDRI = hdri;
				Filepath = filepath;
			}
			else
				DY_CORE_WARN("Could not load HDRI '{0}'", filepath);
		}
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
		float ScatteringDistribution = 0.5;
		float ScatteringIntensity = 1.0;
		float ExtinctionScale = 0.5;
	};

	struct AudioComponent
	{
		Ref<Audio> AudioSound = nullptr;
		bool Initialized = false;
		bool StartOnAwake = false;
		uint32_t StartPosition = 0;

		AudioComponent() = default;
		AudioComponent(const AudioComponent& ac) = default;
		AudioComponent(const std::string& path)
		{
			Load(path);
		}

		void Load(const std::string& path)
		{
			Ref<Audio> audio = AssetManager::GetAsset<Audio>(path);
			if (audio)
				AudioSound = audio;
		}
	};

	struct RigidbodyComponent
	{
		enum class BodyType { Static = 0, Dynamic };

		BodyType Type;
		float Density = 1.0f;
		float StaticFriction = 0.5f;
		float DynamicFriction = 0.5f;
		float Restitution = 0.1f;

		// Storage for runtime
		void* RuntimeBody = nullptr;

		RigidbodyComponent() = default;
		RigidbodyComponent(const RigidbodyComponent&) = default;
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
		MeshType Type;

		MeshColliderComponent() = default;
		MeshColliderComponent(const MeshColliderComponent&) = default;
	};

	struct CharacterMovementComponent
	{
		CharacterMovementComponent() = default;
		CharacterMovementComponent(const CharacterMovementComponent&) = default;

		float Density = 75.0f;
		float CapsuleRadius = 0.42f;
		float CapsuleHeight = 1.92f;
		
		float GravityScale = 1.0f;
		float StepOffset = 0.45f;
		float MaxWalkableSlope = 0.45f;
		float MaxWalkSpeed = 6.0f;
		float MaxAcceleration = 20.48f;
		float BrakingDeceleration = 20.48f;
		float GroundFriction = 8.0f;
		float AirControl = 0.2f;
		
		// Runtime storage
		glm::vec3 Velocity = glm::vec3(0.0f);
		void* CharacterController = nullptr;
	};
	
	struct VehicleMovementComponent
	{
		VehicleMovementComponent() = default;
		VehicleMovementComponent(const VehicleMovementComponent&) = default;

		UUID Chasis;
		UUID Wheels[4];

		float Mass;

		// Runtime storage
		uint32_t RuntimeVehicleID = 0;
		void* VehicleController = nullptr;
	};

	struct DirectionalFieldComponent
	{
		DirectionalFieldComponent() = default;
		DirectionalFieldComponent(const DirectionalFieldComponent&) = default;

		glm::vec3 Force;
	};

	struct RadialFieldComponent
	{
		RadialFieldComponent() = default;
		RadialFieldComponent(const RadialFieldComponent&) = default;

		float Magnitude = 1.0f;
		float Radius = 1.0f;
		float Falloff = 0.0f;
	};

	struct BouyancyFieldComponent
	{
		BouyancyFieldComponent() = default;
		BouyancyFieldComponent(const BouyancyFieldComponent&) = default;

		float FluidDensity = 1.0f;
		float LinearDamping = 2.5f;
		float AngularDamping = 0.5f;
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

	using AllComponents =
		ComponentGroup<
			FolderComponent, TransformComponent,
			SpriteRendererComponent, CircleRendererComponent, TextComponent,
			CameraComponent, ScriptComponent, NativeScriptComponent,
			Rigidbody2DComponent, BoxCollider2DComponent, CircleCollider2DComponent,
			StaticMeshComponent, DirectionalLightComponent, PointLightComponent, 
			SpotLightComponent, SkyLightComponent, VolumeComponent,
			AudioComponent,
			RigidbodyComponent, CharacterMovementComponent, VehicleMovementComponent, 
			BouyancyFieldComponent, DirectionalFieldComponent, RadialFieldComponent,
			BoxColliderComponent, SphereColliderComponent, CapsuleColliderComponent, MeshColliderComponent,
			UICanvasComponent, UIImageComponent, UIButtonComponent
		>;

}
