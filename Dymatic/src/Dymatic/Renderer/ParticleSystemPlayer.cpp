#include "dypch.h"
#include "Dymatic/Renderer/ParticleSystemPlayer.h"

#include "Dymatic/Renderer/Shader.h"
#include "Dymatic/Renderer/Renderer.h"

#include "Dymatic/Renderer/RendererConstants.h"

namespace Dymatic {
	
	ParticleSystemPlayer::ParticleSystemPlayer(const Ref<ParticleSystem> particleSystem)
		: m_ParticleSystem(particleSystem)
	{
		AllocateBuffer();
	}

	uint32_t ParticleSystemPlayer::Update()
	{
		// Check if we need to reallocate
		const UUID modificationID = m_ParticleSystem->GetModificationID();

		auto& data = m_ParticleSystem->GetData();
		if (m_ActiveMaxParticleCount != data.MaxParticles)
			AllocateBuffer();
		else if (m_ModificationID != modificationID)
		{
			// Check if we need to invalidate and re-upload data to the GPU
			m_ModificationID = modificationID;
			m_ParticleBuffer->SetData(&data, sizeof(ParticleSystem::ParticleSystemData), 4 * sizeof(uint32_t));
		}

		Bind();
		Renderer::GetShaderLibrary()->Get("Renderer3D_ParticleUpdate")->Dispatch(m_ParticleCount / 16 + 1, 1, 1);
		
		// TODO: OpenGL throws a performance warning here for GPU read backs! We could possibility optimizes this by calling GL_MAP_PERSISTENT_BIT and GL_MAP_COHERENT_BIT to map the data but this does seem like overkill.
		// Perhaps moving the ParticleCount to its StorageBuffer would be more performant so we have only a single read?
		m_ParticleBuffer->GetData(&m_ParticleCount, sizeof(uint32_t));
		return m_ParticleCount;
	}

	void ParticleSystemPlayer::Bind()
	{
		m_ParticleBuffer->Bind(RendererConstants::Buffers::ParticleSystem);
	}

	void ParticleSystemPlayer::AllocateBuffer()
	{
		auto& data = m_ParticleSystem->GetData();

		m_ActiveMaxParticleCount = data.MaxParticles;
		m_ModificationID = m_ParticleSystem->GetModificationID();

		// Allocate GPU memory for the SSBO
		const size_t bufferSize = 4 * sizeof(uint32_t) + sizeof(ParticleSystem::ParticleSystemData) + sizeof(Particle) * data.MaxParticles;
		m_ParticleBuffer = ShaderStorageBuffer::Create(bufferSize, ShaderStorageBufferUsage::DYNAMIC_COPY);
		
		m_ParticleCount = 0;
		m_ParticleBuffer->SetData(&m_ParticleCount, sizeof(uint32_t));
		m_ParticleBuffer->SetData(&data, sizeof(ParticleSystem::ParticleSystemData), 4 * sizeof(uint32_t));
	}

}