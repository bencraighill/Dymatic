#pragma once
#include "Dymatic/Core/Base.h"

#include "Dymatic/Asset/AssetHandle.h"
#include "Dymatic/Physics/PhysicsSettings.h"

#include <string>
#include <filesystem>

namespace Dymatic {
	
	struct ProjectConfig
	{
		std::string Name = "Untitled";

		AssetHandle StartScene;
		
		std::filesystem::path AssetDirectory;
		std::filesystem::path CoreModulePath;
		std::filesystem::path ScriptModulePath;
		std::filesystem::path CacheDirectory;

		PhysicsSettings PhysicsSettings;
	};

	class Project
	{
	public:
		static const std::filesystem::path& GetProjectFilepath()
		{
			DY_CORE_ASSERT(s_ActiveProject);
			return s_ActiveProject->m_ProjectFilepath;
		}

		static const std::filesystem::path& GetProjectDirectory()
		{
			DY_CORE_ASSERT(s_ActiveProject);
			return s_ActiveProject->m_ProjectDirectory;
		}

		static std::string GetName()
		{
			DY_CORE_ASSERT(s_ActiveProject);
			return s_ActiveProject->m_Config.Name;
		}

		static std::filesystem::path GetAssetDirectory()
		{
			DY_CORE_ASSERT(s_ActiveProject);
			return GetProjectDirectory() / s_ActiveProject->GetConfig().AssetDirectory;
		}

		static std::filesystem::path GetRelativeAssetDirectory()
		{
			DY_CORE_ASSERT(s_ActiveProject);
			return s_ActiveProject->GetConfig().AssetDirectory;
		}

		static std::filesystem::path GetCoreModulePath()
		{
			DY_CORE_ASSERT(s_ActiveProject);

			if (s_ActiveProject->GetConfig().CoreModulePath.empty())
				return "";

			return GetAssetDirectory() / s_ActiveProject->GetConfig().CoreModulePath;
		}

		static std::filesystem::path GetScriptModulePath()
		{
			DY_CORE_ASSERT(s_ActiveProject);

			if (s_ActiveProject->GetConfig().ScriptModulePath.empty())
				return "";

			return GetAssetDirectory() / s_ActiveProject->GetConfig().ScriptModulePath;
		}

		static std::filesystem::path GetCacheDirectory()
		{
			DY_CORE_ASSERT(s_ActiveProject);
			const auto& cacheDirectory = s_ActiveProject->GetConfig().CacheDirectory;
			return GetProjectDirectory() / (cacheDirectory.empty() ? "Cache" : cacheDirectory);
		}

		// Should be inside an asset manager
		static std::filesystem::path GetAssetFileSystemPath(const std::filesystem::path& path)
		{
			DY_CORE_ASSERT(s_ActiveProject);
			return GetAssetDirectory() / path;
		}

		static ProjectConfig& GetActiveConfig()
		{
			DY_CORE_ASSERT(s_ActiveProject);
			return s_ActiveProject->GetConfig();
		}

		ProjectConfig& GetConfig() { return m_Config; }

		static Ref<Project> GetActive() { return s_ActiveProject; }

		static Ref<Project> New();
		static Ref<Project> Load(const std::filesystem::path& path);
		static bool Save();
		static bool Save(const std::filesystem::path& path);

	private:
		ProjectConfig m_Config;
		std::filesystem::path m_ProjectDirectory;
		std::filesystem::path m_ProjectFilepath;

		inline static Ref<Project> s_ActiveProject = nullptr;
	};

}