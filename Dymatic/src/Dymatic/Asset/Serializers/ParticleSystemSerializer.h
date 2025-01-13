#pragma once
#include "Dymatic/Asset/AssetSerializer.h"
#include "Dymatic/Renderer/ParticleSystem.h"

namespace Dymatic {

	class ParticleSystemSerializer : public AssetSerializer
	{
	public:
		virtual void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const override
		{
			const Ref<ParticleSystem> particleSystem = As<ParticleSystem>(asset);

			YAML::Emitter out;
			out << YAML::BeginMap << YAML::Key << "Particle System" << YAML::Value << YAML::BeginMap;

			out << YAML::Key << "Default Material" << YAML::Value << particleSystem->GetMaterialHandle();

			auto& data = particleSystem->GetData();
			out << YAML::Key << "Max Particles" << YAML::Value << data.MaxParticles;
			out << YAML::Key << "Emission Count" << YAML::Value << data.EmissionCount;
			out << YAML::Key << "Lifetime" << YAML::Value << data.Lifetime;
			out << YAML::Key << "Collision Radius" << YAML::Value << data.CollisionRadius;
			out << YAML::Key << "Minimum Position" << YAML::Value << data.MinimumPosition;
			out << YAML::Key << "Maximum Position" << YAML::Value << data.MaximumPosition;
			out << YAML::Key << "Minimum Velocity" << YAML::Value << data.MinimumVelocity;
			out << YAML::Key << "Maximum Velocity" << YAML::Value << data.MaximumVelocity;
			out << YAML::Key << "Acceleration" << YAML::Value << data.Acceleration;

			out << YAML::EndMap; // Particle System
			out << YAML::EndMap;

			std::ofstream fout(Project::GetAssetFileSystemPath(metadata.FilePath));
			fout << out.c_str();
		}

		virtual bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset, bool multithreaded) const override
		{
			YAML::Node rootNode;
			if (!Utils::TryLoadYAMLFromFile(metadata, rootNode))
				return false;

			auto particleSystemNode = rootNode["Particle System"];
			if (!particleSystemNode)
				return false;

			const Ref<ParticleSystem> particleSystem = CreateRef<ParticleSystem>();
			auto& data = particleSystem->GetData();

			if (auto& defaultMaterialNode = particleSystemNode["Default Material"])
				particleSystem->GetMaterialHandle() = defaultMaterialNode.as<AssetHandle>();

			if (auto& maxParticlesNode = particleSystemNode["Max Particles"])
				data.MaxParticles = maxParticlesNode.as<uint32_t>();

			if (auto& emissionCountNode = particleSystemNode["Emission Count"])
				data.EmissionCount = emissionCountNode.as<uint32_t>();

			if (auto& lifetimeNode = particleSystemNode["Lifetime"])
				data.Lifetime = lifetimeNode.as<float>();

			if (auto& collisionRadiusNode = particleSystemNode["Collision Radius"])
				data.CollisionRadius = collisionRadiusNode.as<float>();

			if (auto& minimumPositionNode = particleSystemNode["Minimum Position"])
				data.MinimumPosition = minimumPositionNode.as<glm::vec3>();

			if (auto& maximumPosition = particleSystemNode["Maximum Position"])
				data.MaximumPosition = maximumPosition.as<glm::vec3>();

			if (auto& minimumVelocityNode = particleSystemNode["Minimum Velocity"])
				data.MinimumVelocity = minimumVelocityNode.as<glm::vec3>();

			if (auto& maximumVelocityNode = particleSystemNode["Maximum Velocity"])
				data.MaximumVelocity = maximumVelocityNode.as<glm::vec3>();

			if (auto& accelerationNode = particleSystemNode["Acceleration"])
				data.Acceleration = accelerationNode.as<glm::vec3>();

			asset = particleSystem;
			return true;
		}

		virtual bool SerializeToAssetPack(const AssetMetadata& metadata, const AssetHandle handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo) const override
		{
			const Ref<ParticleSystem> particleSystem = AssetManager::GetAsset<ParticleSystem>(handle);

			stream.WriteRaw<AssetHandle>(particleSystem->GetMaterialHandle());
			
			auto& data = particleSystem->GetData();
			stream.WriteRaw<uint32_t>(data.MaxParticles);
			stream.WriteRaw<uint32_t>(data.EmissionCount);
			stream.WriteRaw<float>(data.Lifetime);
			stream.WriteRaw<float>(data.CollisionRadius);
			stream.WriteRaw<glm::vec3>(data.MinimumPosition);
			stream.WriteRaw<glm::vec3>(data.MaximumPosition);
			stream.WriteRaw<glm::vec3>(data.MinimumVelocity);
			stream.WriteRaw<glm::vec3>(data.MaximumVelocity);
			stream.WriteRaw<glm::vec3>(data.Acceleration);

			return true;
		}

		virtual bool DeserializeFromAssetPack(const AssetMetadata& metadata, Ref<Asset>& asset, FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo) const override
		{
			const Ref<ParticleSystem> particleSystem = ParticleSystem::Create();

			stream.ReadRaw<AssetHandle>(particleSystem->GetMaterialHandle());

			auto& data = particleSystem->GetData();
			stream.ReadRaw<uint32_t>(data.MaxParticles);
			stream.ReadRaw<uint32_t>(data.EmissionCount);
			stream.ReadRaw<float>(data.Lifetime);
			stream.ReadRaw<float>(data.CollisionRadius);
			stream.ReadRaw<glm::vec3>(data.MinimumPosition);
			stream.ReadRaw<glm::vec3>(data.MaximumPosition);
			stream.ReadRaw<glm::vec3>(data.MinimumVelocity);
			stream.ReadRaw<glm::vec3>(data.MaximumVelocity);
			stream.ReadRaw<glm::vec3>(data.Acceleration);

			asset = particleSystem;
			return true;
		}
	};

}