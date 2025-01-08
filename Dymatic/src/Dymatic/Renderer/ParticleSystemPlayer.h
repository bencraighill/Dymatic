#pragma once

#include "Dymatic/Core/Timestep.h"

#include "Dymatic/Renderer/MaterialAsset.h"
#include "Dymatic/Renderer/ShaderStorageBuffer.h"
#include "Dymatic/Renderer/ParticleSystem.h"

namespace Dymatic {

	class ParticleSystemPlayer
	{
	public:
		// Note: These structures reflect the GPU layout of Particles and their data
		struct Particle
		{
			glm::vec4 Position;
			glm::vec4 Velocity;
			float LifeTime;
			float LifeRemaining;
		};

	public:
		static Ref<ParticleSystemPlayer> Create(const Ref<ParticleSystem> particleSystem) { return CreateRef<ParticleSystemPlayer>(particleSystem); }

		ParticleSystemPlayer(const Ref<ParticleSystem> particleSystem);

		uint32_t Update();

		void Bind();

		// Warning: This is not a performant operation and should only be called sparingly
		void AllocateBuffer();

		inline Ref<ParticleSystem> GetParticleSystem() const { return m_ParticleSystem; }
		inline uint32_t GetParticleCount() const { return m_ParticleCount; }
		
	private:
		Ref<ShaderStorageBuffer> m_ParticleBuffer;
		Ref<ParticleSystem> m_ParticleSystem;
		UUID m_ModificationID;
		size_t m_ActiveMaxParticleCount;
		uint32_t m_ParticleCount;
	};

}