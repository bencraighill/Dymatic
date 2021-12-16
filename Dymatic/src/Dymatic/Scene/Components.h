#pragma once

#include "SceneCamera.h"
#include "Dymatic/Core/UUID.h"
#include "Dymatic/Renderer/Texture.h"

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
	};

	struct ParticleSystemComponent
	{
	public:
		ParticleSystemComponent::ParticleSystemComponent(uint32_t maxParticles = 100000)
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
	
		void ParticleSystemComponent::OnUpdate(Dymatic::Timestep ts)
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
		
		std::vector<Particle> &GetParticlePool() { return m_ParticlePool; }
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

	struct CameraComponent
	{
		SceneCamera Camera;
		bool Primary = true;
		bool FixedAspectRatio = false;

		CameraComponent() = default;
		CameraComponent(const CameraComponent&) = default;
	};

	// Forward declaration
	class ScriptableEntity;

	struct NativeScriptComponent
	{
		ScriptableEntity* Instance = nullptr;

		ScriptableEntity*(*InstantiateScript)();
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

}
