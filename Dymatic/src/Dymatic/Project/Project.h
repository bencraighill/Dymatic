#pragma once
#include "Dymatic/Core/Base.h"

#include <string>
#include <filesystem>

namespace Dymatic {
	
	struct ProjectConfig
	{
		std::string Name = "Untitled";

		std::filesystem::path StartScene;
		
		std::filesystem::path AssetDirectory;
		std::filesystem::path ScriptModulePath;
	};

	class Project
	{
	public:
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

		static std::filesystem::path GetScriptModulePath()
		{
			DY_CORE_ASSERT(s_ActiveProject);
			return GetAssetDirectory() / s_ActiveProject->GetConfig().ScriptModulePath;
		}

		// Should be inside an asset manager
		static std::filesystem::path GetAssetFileSystemPath(const std::filesystem::path& path)
		{
			DY_CORE_ASSERT(s_ActiveProject);
			return GetAssetDirectory() / path;
		}

		ProjectConfig& GetConfig() { return m_Config; }

		static Ref<Project> GetActive() { return s_ActiveProject; }

		static Ref<Project> New();
		static Ref<Project> Load(const std::filesystem::path& path);
		static bool Save(const std::filesystem::path& path);

	private:
		ProjectConfig m_Config;
		std::filesystem::path m_ProjectDirectory;

		inline static Ref<Project> s_ActiveProject = nullptr;
	};

}