#include "ProjectSettings.h"

#include "Dymatic/Project/Project.h"

#include <fstream>
#include <yaml-cpp/yaml.h>

#include "Dymatic/Core/UUID.h"
#include "Dymatic/Utils/YAMLUtils.h"

namespace Dymatic {

	static const char* s_ProjectSettingsDirectory = "Config/Editor";
	static const char* s_ProjectSettingsFilename = "ProjectSettings.dyconfig";

	static ProjectSettings::ProjectSettingsData s_ProjectSettingsData;

	ProjectSettings::ProjectSettingsData& ProjectSettings::GetData()
	{
		return s_ProjectSettingsData;
	}

	void ProjectSettings::Serialize()
	{
		if (!std::filesystem::exists(Project::GetProjectDirectory() / s_ProjectSettingsDirectory))
			std::filesystem::create_directories(Project::GetProjectDirectory() / s_ProjectSettingsDirectory);

		YAML::Emitter out;
		{
			out << YAML::BeginMap; // Root
			out << YAML::Key << "ProjectSettings" << YAML::Value;
			{
				out << YAML::BeginMap; // ProjectSettings
				
				out << YAML::Key << "RecentScenePaths" << YAML::Value;
				{
					out << YAML::BeginSeq; // RecentScenePaths
					for (auto& path : s_ProjectSettingsData.RecentScenePaths)
						out << YAML::Value << path.string();
					out << YAML::EndSeq; // RecentScenePaths
				}
				
				out << YAML::Key << "ColoredFolders" << YAML::Value;
				{
					out << YAML::BeginSeq; // ColoredFolderPaths
					for (auto& [path, color] : s_ProjectSettingsData.ColoredFolders)
					{
						if (color == glm::vec3(1.0f))
							continue;

						out << YAML::BeginMap;
						out << YAML::Key << "Path" << YAML::Value << path.string();
						out << YAML::Key << "Color" << YAML::Value << color;
						out << YAML::EndMap;
					}
					out << YAML::EndSeq; // ColoredFolderPaths
				}

				out << YAML::EndMap; // ProjectSettings
			}
			out << YAML::EndMap; // Root
		}

		// Output the file
		std::ofstream fout(Project::GetProjectDirectory() / s_ProjectSettingsDirectory / s_ProjectSettingsFilename);
		fout << out.c_str();
	}

	bool ProjectSettings::Deserialize()
	{
		std::filesystem::path filepath = Project::GetProjectDirectory() / s_ProjectSettingsDirectory / s_ProjectSettingsFilename;

		if (!std::filesystem::exists(filepath))
			return false;

		YAML::Node data;
		try
		{
			data = YAML::LoadFile(filepath.string());
		}
		catch (YAML::ParserException e)
		{
			DY_CORE_ERROR("Failed to load project settings file '{0}'\n     {1}", filepath.string(), e.what());
			return false;
		}

		auto projectSettingsNode = data["ProjectSettings"];
		if (!projectSettingsNode)
			return false;

		// Clear all previous data
		s_ProjectSettingsData.RecentScenePaths.clear();
		s_ProjectSettingsData.ColoredFolders.clear();

		if (auto& recentScenePathsNode = projectSettingsNode["RecentScenePaths"])
		{
			s_ProjectSettingsData.RecentScenePaths.resize(recentScenePathsNode.size());
			for (uint32_t i = 0; i < recentScenePathsNode.size(); i++)
				s_ProjectSettingsData.RecentScenePaths[i] = recentScenePathsNode[i].as<std::string>();
		}

		if (auto& coloredFolderNode = projectSettingsNode["ColoredFolders"])
		{
			for (auto& coloredFolder : coloredFolderNode)
			{
				std::filesystem::path path = coloredFolder["Path"].as<std::string>();
				glm::vec3 color = coloredFolder["Color"].as<glm::vec3>();
				s_ProjectSettingsData.ColoredFolders[path] = color;
			}
		}

		return true;
	}

	void ProjectSettings::AddRecentScenePath(const std::filesystem::path& path)
	{
		auto& paths = s_ProjectSettingsData.RecentScenePaths;
		for (uint32_t i = 0; i < paths.size(); i++)
			if (paths[i].lexically_normal() == path.lexically_normal())
				paths.erase(paths.begin() + i);
		paths.insert(paths.begin(), path.lexically_normal());

		Serialize();
	}

	bool ProjectSettings::HasFolderColor(const std::filesystem::path& path)
	{
		return s_ProjectSettingsData.ColoredFolders.find(path) != s_ProjectSettingsData.ColoredFolders.end();
	}

	void ProjectSettings::SetFolderColor(const std::filesystem::path& path, const glm::vec3& color)
	{
		s_ProjectSettingsData.ColoredFolders[path] = color;
		Serialize();
	}

	glm::vec3 ProjectSettings::GetFolderColor(const std::filesystem::path& path)
	{
		return s_ProjectSettingsData.ColoredFolders[path];
	}

}