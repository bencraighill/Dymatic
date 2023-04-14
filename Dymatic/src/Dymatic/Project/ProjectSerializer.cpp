#include "dypch.h"
#include "Dymatic/Project/ProjectSerializer.h"

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
				out << YAML::Key << "StartScene" << YAML::Value << config.StartScene.string();
				out << YAML::Key << "AssetDirectory" << YAML::Value << config.AssetDirectory.string();
				out << YAML::Key << "CoreModulePath" << YAML::Value << config.CoreModulePath.string();
				out << YAML::Key << "ScriptModulePath" << YAML::Value << config.ScriptModulePath.string();
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
			config.StartScene = startSceneNode.as<std::string>();

		auto assetDirectoryNode = projectNode["AssetDirectory"];
		if (!assetDirectoryNode)
			return false;
		config.AssetDirectory = assetDirectoryNode.as<std::string>();

		auto coreModulePathNode = projectNode["CoreModulePath"];
		if (coreModulePathNode)
			config.CoreModulePath = coreModulePathNode.as<std::string>();

		auto scriptModulePathNode = projectNode["ScriptModulePath"];
		if (scriptModulePathNode)
			config.ScriptModulePath = scriptModulePathNode.as<std::string>();

		return true;
	}
	
}