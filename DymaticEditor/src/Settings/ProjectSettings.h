#pragma once

#include <vector>
#include <filesystem>
#include <unordered_map>

#include <glm/glm.hpp>

namespace Dymatic {

	class ProjectSettings
	{
	public:
		struct ProjectSettingsData
		{
			std::vector<std::filesystem::path> RecentScenePaths;
			std::unordered_map<std::filesystem::path, glm::vec3> ColoredFolders;
		};

		ProjectSettings() = delete;

		static ProjectSettingsData& GetData();
		static void Serialize();
		static bool Deserialize();
		
		static void AddRecentScenePath(const std::filesystem::path& path);
		
		static bool HasFolderColor(const std::filesystem::path& path);
		static glm::vec3 GetFolderColor(const std::filesystem::path& path);
		static void SetFolderColor(const std::filesystem::path& path, const glm::vec3& color);
	};

}