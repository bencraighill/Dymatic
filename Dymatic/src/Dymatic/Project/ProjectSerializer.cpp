#include "dypch.h"
#include "Dymatic/Project/ProjectSerializer.h"

#include "Dymatic/Core/UUID.h"

#include <fstream>
#include <yaml-cpp/yaml.h>

namespace Dymatic {

	ProjectSerializer::ProjectSerializer(Ref<Project> project)
		: m_Project(project)
	{
	}

	bool ProjectSerializer::Serialize(const std::filesystem::path& filepath)
	{
		const auto& config = m_Project->GetConfig();

		YAML::Emitter out;
		{
			out << YAML::BeginMap; // Root
			out << YAML::Key << "Project" << YAML::Value;
			{
				out << YAML::BeginMap; // Project
				out << YAML::Key << "Name" << YAML::Value << config.Name;
				out << YAML::Key << "StartScene" << YAML::Value << config.StartScene;
				out << YAML::Key << "AssetDirectory" << YAML::Value << config.AssetDirectory.string();
				out << YAML::Key << "CoreModulePath" << YAML::Value << config.CoreModulePath.string();
				out << YAML::Key << "ScriptModulePath" << YAML::Value << config.ScriptModulePath.string();
				out << YAML::Key << "CacheDirectory" << YAML::Value << config.CacheDirectory.string();

				out << YAML::Key << "Physics" << YAML::Value;
				{
					const auto& physics = config.PhysicsSettings;

					out << YAML::BeginMap; // Physics
					out << YAML::Key << "AllowSleeping" << YAML::Value << physics.AllowSleeping;
					out << YAML::Key << "SleepTimer" << YAML::Value << physics.SleepTimer;
					out << YAML::Key << "PositionSteps" << YAML::Value << physics.PositionSteps;
					out << YAML::Key << "VelocitySteps" << YAML::Value << physics.VelocitySteps;
					out << YAML::Key << "Deterministic" << YAML::Value << physics.Deterministic;

					out << YAML::Key << "Layers" << YAML::Value;
					out << YAML::BeginSeq; // Layers
					for (const auto& [id, layer] : physics.Layers)
					{
						out << YAML::BeginMap; // Layer
						out << YAML::Key << "ID" << YAML::Value << id;
						out << YAML::Key << "Name" << YAML::Value << layer.Name;

						out << YAML::Key << "ExclusionMask" << YAML::Value;
						out << YAML::BeginSeq; // Mask
						for (const auto excluded : layer.ExclusionMask)
							if (physics.Layers.find(excluded) != physics.Layers.end())
								out << YAML::Value << excluded;
						out << YAML::EndSeq; // Mask

						out << YAML::EndMap; // Layer
					}
					out << YAML::EndSeq; // Layers

					out << YAML::EndMap; // Physics
				}

				out << YAML::EndMap; // Project
			}
			out << YAML::EndMap; // Root
		}

		// Output the file
		std::ofstream fout(filepath);
		fout << out.c_str();

		return true;
	}

	bool ProjectSerializer::Deserialize(const std::filesystem::path& filepath)
	{
		if (!std::filesystem::exists(filepath))
			return false;

		auto& config = m_Project->GetConfig();

		YAML::Node data;
		try
		{
			data = YAML::LoadFile(filepath.string());
		}
		catch (YAML::ParserException e)
		{
			DY_CORE_ERROR("Failed to load project file '{0}'\n     {1}", filepath.string(), e.what());
			return false;
		}

		auto projectNode = data["Project"];
		if (!projectNode)
			return false;

		auto nameNode = projectNode["Name"];
		if (!nameNode)
			return false;
		config.Name = nameNode.as<std::string>();

		auto startSceneNode = projectNode["StartScene"];
		if (startSceneNode)
			config.StartScene = startSceneNode.as<AssetHandle>();

		auto assetDirectoryNode = projectNode["AssetDirectory"];
		if (!assetDirectoryNode)
			return false;

		config.AssetDirectory = assetDirectoryNode.as<std::string>();

		if (auto coreModulePathNode = projectNode["CoreModulePath"])
			config.CoreModulePath = coreModulePathNode.as<std::string>();

		if (auto scriptModulePathNode = projectNode["ScriptModulePath"])
			config.ScriptModulePath = scriptModulePathNode.as<std::string>();

		if (auto cacheDirectoryNode = projectNode["CacheDirectory"])
			config.CacheDirectory = cacheDirectoryNode.as<std::string>();

		auto& physics = config.PhysicsSettings;
		physics.Layers.clear();

		if (auto physicsNode = projectNode["Physics"])
		{
			physics.AllowSleeping = physicsNode["AllowSleeping"].as<bool>();
			physics.SleepTimer = physicsNode["SleepTimer"].as<float>();
			physics.PositionSteps = physicsNode["PositionSteps"].as<uint32_t>();
			physics.VelocitySteps = physicsNode["VelocitySteps"].as<uint32_t>();
			physics.Deterministic = physicsNode["Deterministic"].as<bool>();

			if (auto layersNode = physicsNode["Layers"])
			{
				for (auto layerNode : layersNode)
				{
					const uint64_t id = layerNode["ID"].as<uint64_t>();
					auto& layer = physics.Layers[id];

					layer.Name = layerNode["Name"].as<std::string>();

					auto exclusionMaskNode = layerNode["ExclusionMask"];
					for (auto exclusionNode : exclusionMaskNode)
						layer.ExclusionMask.insert(exclusionNode.as<uint64_t>());
				}
			}
		}

		// Ensure we always have a default physics layer
		if (physics.Layers.find(0) == physics.Layers.end())
			physics.Layers[0] = PhysicsLayer{ "Default" };

		return true;
	}
	
}