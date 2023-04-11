#pragma once
#include "Dymatic/Renderer/Texture.h"

#include <string>
#include <filesystem>

namespace Dymatic {

	class ProjectLauncher
	{
	public:
		void OnImGuiRender(std::filesystem::path* openPath);
		void Open(bool create = false);

		void AddRecentProject(const std::filesystem::path& path);
		bool CreateProject(const std::filesystem::path& path, const std::string& name);

	private:
		void LoadRecentProjects();
		void SaveRecentProjcets();

		Ref<Texture2D> GetProjectIcon(const std::filesystem::path& path);

	private:
		bool m_Open = false;
		
		Ref<Texture2D> m_DymaticProjectIcon = Texture2D::Create("Resources/Icons/Branding/DymaticLogo.png");
		
		struct RecentProject
		{
			std::filesystem::path Path;
			Ref<Texture2D> Icon;

			bool operator==(const std::filesystem::path& path) { return Path == path; }
		};

		std::vector<RecentProject> m_RecentProjects;
		std::filesystem::path m_SelectedProject;

		bool m_CreateState = false;
		bool m_ShowThumbnails = true;

		std::string m_ProjectLocationBuffer;
		std::string m_ProjectNameBuffer;
	};
	
}