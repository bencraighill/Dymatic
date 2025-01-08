#pragma once

#include "Dymatic/Asset/Asset.h"

namespace Dymatic {

	class ParticleSystem : public Asset
	{
	public:
		struct ParticleSystemData
		{
			// Note: GPU Buffer reserves 16 bytes in front of this to track particle counts
			glm::vec3 MinimumPosition = glm::vec3(0.0f, 0.0f, 0.0f);
			float _padd0 = 1.0f;
			glm::vec3 MaximumPosition = glm::vec3(0.0f, 0.0f, 0.0f);
			float _padd1 = 1.0f;
			glm::vec3 MinimumVelocity = glm::vec3(-1.0f, -1.0f, -1.0f);
			float _padd2 = 1.0f;
			glm::vec3 MaximumVelocity = glm::vec3(1.0f, 1.0f, 1.0f);
			float _padd3 = 1.0f;
			glm::vec3 Acceleration = glm::vec3(0.0f, 0.0f, 0.0f);
			float _padd4 = 1.0f;

			uint32_t MaxParticles = 10000; // Warning: This must match the size allocated for the u_Particles uniform in the ParticleSystemData on the GPU
			uint32_t EmissionCount = 5;
			float Lifetime = 1.0f;
			float CollisionRadius = 1.0f;
		};

	public:
		static Ref<ParticleSystem> Create() { return CreateRef<ParticleSystem>(); }
		ParticleSystem() = default;

		inline ParticleSystemData& GetData() { return m_Data; }

		inline AssetHandle& GetMaterialHandle() { return m_MaterialHandle; }
		inline const AssetHandle GetMaterialHandle() const { return m_MaterialHandle; }

		inline UUID GetModificationID() const { return m_ModificationID; }
		inline void Modify() { m_ModificationID = UUID(); }

		static AssetType GetStaticType() { return AssetType::ParticleSystem; }
		inline virtual AssetType GetAssetType() const override { return GetStaticType(); }

	private:
		ParticleSystemData m_Data;
		AssetHandle m_MaterialHandle = 0;
		UUID m_ModificationID;
	};

}