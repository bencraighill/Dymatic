#include "ProjectBrowser.h"
#include "Dymatic/Project/Project.h"

#include "Dymatic/Core/Application.h"
#include "Dymatic/Utils/PlatformUtils.h"

#include "Dymatic/UI/UI.h"

#include "../TextSymbols.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <fstream>
#include <string>

namespace Dymatic {

	static const std::filesystem::path s_RecentProjectsFilepath = "saved/RecentProjects.txt";

	void ProjectLauncher::OnImGuiRender(std::filesystem::path* openPath)
	{		
		if (!m_Open)
			return;

		if (ImGui::GetFrameCount() == 1)
			return;

		auto& style = ImGui::GetStyle();
		auto& io = ImGui::GetIO();

		const char* name = "Dymatic Project Browser";
		ImGui::OpenPopup(name);
		ImGui::SetNextWindowBgAlpha(1.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 10.0f));
		// Window can only be closed if a project is currently loaded

		if (!Project::GetActive())
			io.ConfigViewportsNoAutoMerge = true;

		if (ImGui::BeginPopupModal(name, nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings))
		{
			const float optionsHeight = 75.0f;
			const ImVec2 size(800.0f, 600.0f);
			
			if (ImGui::IsWindowAppearing())
			{				
				if (Project::GetActive())
					ImGui::SetWindowPos(ImGui::GetMainViewport()->Pos + ((ImGui::GetMainViewport()->Size - size) * 0.5f));
				else
				{
					glm::vec4 monitorRect = Monitor::GetMonitorWorkArea();
					ImGui::SetWindowPos((ImVec2(monitorRect.z, monitorRect.w) - size) * 0.5f);
				}
				ImGui::SetWindowSize(size);
			}

			{
				ImGui::PushFont(io.Fonts->Fonts[0]);
				const char* text = "Dymatic Project Browser";
				ImGui::Dummy(ImVec2((ImGui::GetWindowContentRegionWidth() - ImGui::CalcTextSize(text).x) * 0.5f, 0.0f));
				ImGui::SameLine();
				ImGui::TextDisabled(text);
				ImGui::PopFont();

				ImGui::Dummy(ImVec2(0.0f, 10.0f));

				if (ImGui::CloseButton(ImGui::GetID("##ProjectBrowserCloseButton"), ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x - style.WindowPadding.x - 10.0f, ImGui::GetWindowPos().y + ImGui::GetWindowContentRegionMin().y)))
				{
					if (Project::GetActive())
						m_Open = false;
					else
						Application::Get().Close();
				}
			}

			{
				const float newButtonHeight = 35.0f;

				ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg) * ImVec4(1.25f, 1.25f, 1.25f, 1.0f));
				ImGui::BeginChild("##RecentProjects", ImVec2(0.0f, ImGui::GetContentRegionAvail().y - optionsHeight), false, ImGuiWindowFlags_AlwaysUseWindowPadding);

				ImGui::Text("Recent Projects");
				ImGui::Separator();

				// Grid
				ImGui::BeginChild("##ProjectGrid", ImVec2(0.0f, ImGui::GetContentRegionAvail().y - newButtonHeight - style.FramePadding.y * 2.0f));

				auto drawList = ImGui::GetWindowDrawList();
				if (ImGui::BeginTable("##ProjectTable", 6))
				{

					ImGui::TableNextColumn();
					for (uint32_t index = 0; index < m_RecentProjects.size(); index++)
					{
						ImGui::PushID(index);

						auto& project = m_RecentProjects[index];

						const float width = ImGui::GetContentRegionAvailWidth();
						const ImVec2 size(width, width * 1.5f);

						ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, style.FrameRounding * 3.0f);
						ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_Header));
						ImGui::BeginDisabled(m_CreateState);
						if (ImGui::ButtonCornersEx("##ItemButton", size, ImGuiButtonFlags_None, ImDrawFlags_RoundCornersBottom))
							m_SelectedProject = project.Path;

						if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && ImGui::IsItemClicked())
						{
							m_Open = false;
							*openPath = m_SelectedProject;
						}

						ImGui::EndDisabled();
						ImGui::PopStyleColor();
						ImGui::PopStyleVar();

						const auto min = ImGui::GetItemRectMin();
						const auto max = ImGui::GetItemRectMax();

						// Draw background
						drawList->AddRectFilled(min, min + ImVec2(width, width), ImGui::GetColorU32(ImGuiCol_Button));

						drawList->AddImage((ImTextureID)(m_ShowThumbnails ? project.Icon : m_DymaticProjectIcon)->GetRendererID(), min, min + ImVec2(width, width), { 0, 1 }, { 1, 0 });
						drawList->AddText(min + ImVec2(style.FramePadding.x, width + style.FramePadding.y), ImGui::GetColorU32(ImGuiCol_Text), project.Path.filename().stem().string().c_str());

						// Render Item Outline
						auto selected = project.Path == m_SelectedProject;
						ImGui::GetWindowDrawList()->AddRect(min, max, selected ? ImGui::GetColorU32(ImVec4(1.0f, 0.92f, 0.57f, 1.0f)) : (ImGui::IsItemHovered() ? ImGui::GetColorU32(ImVec4(0.5f, 0.5f, 0.5f, 1.0f)) : ImGui::GetColorU32(ImGuiCol_WindowShadow)), style.FrameRounding * 3.0f, ImDrawFlags_RoundCornersBottom, ImGui::IsItemHovered() ? 2.0f : 1.0f);

						// Render Item Shadow
						ImDrawFlags draw_flags = ImDrawFlags_RoundCornersBottom | ImDrawFlags_ShadowCutOutShapeBackground;
						ImGui::GetWindowDrawList()->AddShadowRect(min, max, ImGui::GetColorU32(ImGuiCol_WindowShadow), 20.0f, ImVec2(5.0f, 5.0f), draw_flags, style.FrameRounding * 3.0f);

						ImGui::TableNextColumn();
						ImGui::PopID();
					}
					ImGui::EndTable();
				}

				if (m_CreateState)
					drawList->AddRectFilled(ImGui::GetWindowPos(), ImGui::GetWindowPos() + ImGui::GetWindowSize(), ImGui::GetColorU32(ImGuiCol_ModalWindowDimBg));

				ImGui::EndChild();

				const float width = ImGui::GetContentRegionAvailWidth();
				ImGui::Dummy(ImVec2(width * 0.25f, 0.0f));
				ImGui::SameLine();
				if (ImGui::Button(m_CreateState ? "Open Recent Project" : "Create New Project", ImVec2(width * 0.5f, newButtonHeight)))
				{
					m_CreateState = !m_CreateState;
					m_SelectedProject.clear();
				}

				ImGui::EndChild();
				ImGui::PopStyleColor();
			}

			{
				UI::ScopedStyleColor  textColor(ImGuiCol_Text, ImGui::GetColorU32(ImGuiCol_TextDisabled), !m_CreateState);

				ImGui::Text("Project Location");
				ImGui::SameLine();
				{
					char buffer[256];
					memset(buffer, 0, sizeof(buffer));
					std::strncpy(buffer, (m_CreateState ? m_ProjectLocationBuffer : m_SelectedProject.string()).c_str(), sizeof(buffer));
					ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth() * 0.5f);
					if (ImGui::InputText("##ProjectLocationInput", buffer, sizeof(buffer), m_CreateState ? 0 : ImGuiInputTextFlags_ReadOnly))
						m_ProjectLocationBuffer = std::string(buffer);
				}

				if (m_CreateState)
				{
					ImGui::SameLine();
					const float lineHeight = ImGui::GetTextLineHeight();
					if (ImGui::Button(CHARACTER_ICON_FOLDER, ImVec2(lineHeight + style.FramePadding.x * 4.0f, lineHeight + style.FramePadding.y * 2.0f)))
					{
						std::string filepath = FileDialogs::SelectFolder();
						if (!filepath.empty())
							m_ProjectLocationBuffer = filepath;
					}
				}

				ImGui::SameLine();

				ImGui::Text("Project Name");
				ImGui::SameLine();
				{
					char buffer[256];
					memset(buffer, 0, sizeof(buffer));
					std::strncpy(buffer, (m_CreateState ? m_ProjectNameBuffer : m_SelectedProject.filename().stem().string()).c_str(), sizeof(buffer));
					ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());
					if (ImGui::InputText("##ProjectNameInput", buffer, sizeof(buffer), m_CreateState ? 0 : ImGuiInputTextFlags_ReadOnly))
						m_ProjectNameBuffer = std::string(buffer);
				}
			}

			ImGui::PushFont(io.Fonts->Fonts[1]);
			ImGui::TextDisabled("Dymatic Project Launcher - " DY_VERSION);
			ImGui::PopFont();

			ImGui::SameLine();

			{
				ImGui::Text("Show Thumbnails");
				ImGui::SameLine();
				ImGui::Checkbox("##ShowThumbnailsCheckbox", &m_ShowThumbnails);
				ImGui::SameLine();

				const auto size = ImVec2(100.0f, 25.0f);
				if (m_CreateState)
				{
					ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvailWidth() - 200.0f - 4.0f * style.FramePadding.x, 0.0f));
					ImGui::SameLine();

					ImGui::BeginDisabled(!std::filesystem::exists(m_ProjectLocationBuffer) || m_ProjectNameBuffer.empty());
					if (ImGui::Button("Create", size))
					{
						CreateProject(std::filesystem::path(m_ProjectLocationBuffer), m_ProjectNameBuffer);
						m_Open = false;
						
						*openPath = std::filesystem::path(m_ProjectLocationBuffer) / m_ProjectNameBuffer / (m_ProjectNameBuffer + ".dyproject");
					}
					ImGui::EndDisabled();
				}
				else
				{
					ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvailWidth() - 300.0f - 6.0f * style.FramePadding.x, 0.0f));
					ImGui::SameLine();

					if (ImGui::Button("Browse...", size))
					{
						std::string filepath = FileDialogs::OpenFile("Dymatic Project (*.dyproject)\0*.dyproject\0");
						if (!filepath.empty())
							AddRecentProject(filepath);
					}
					ImGui::SameLine();
					ImGui::BeginDisabled(m_SelectedProject.empty());
					if (ImGui::Button("Open", size))
					{
						m_Open = false;
						*openPath = m_SelectedProject;
					}
					ImGui::EndDisabled();
				}
				ImGui::SameLine();
				if (ImGui::Button("Cancel", size))
				{
					m_Open = false;
					if (!Project::GetActive())
						Application::Get().Close();
				}
			}

			ImGui::EndPopup();
		}
		ImGui::PopStyleVar();

		if (!Project::GetActive())
			io.ConfigViewportsNoAutoMerge = false;
	}

	void ProjectLauncher::Open(bool create)
	{
		m_Open = true;
		m_CreateState = create;
		m_ProjectLocationBuffer.clear();
		m_ProjectNameBuffer.clear();

		LoadRecentProjects();
	}

	void ProjectLauncher::AddRecentProject(const std::filesystem::path& path)
	{
		// Ensure we have latest project list
		LoadRecentProjects();

		// Remove if already exists
		if (auto index = std::find(m_RecentProjects.begin(), m_RecentProjects.end(), path); index != m_RecentProjects.end())
			m_RecentProjects.erase(index);

		// Insert at beginning
		m_RecentProjects.insert(m_RecentProjects.begin(), { path, GetProjectIcon(path) });
		SaveRecentProjcets();
	}

	static void RenameAllInFile(const std::filesystem::path& source, const std::filesystem::path& destination, const std::string& find, const std::string& replace)
	{
		std::ifstream in(source);
		std::ofstream out(destination);

		std::string line;
		size_t len = find.length();
		while (getline(in, line))
		{
			while (true)
			{
				size_t pos = line.find(find);
				if (pos != std::string::npos)
					line.replace(pos, len, replace);
				else
					break;
			}

			out << line << "\n";
		}
	}

	bool ProjectLauncher::CreateProject(const std::filesystem::path& path, const std::string& name)
	{
		if (!std::filesystem::exists(path))
			return false;

		std::filesystem::copy("saved/presets/projects/DefaultProject", path / name, std::filesystem::copy_options::recursive);
		
		RenameAllInFile(path / name / "DefaultProject.dyproject", path / name / (name + ".dyproject"), "{projectName}", name);
		std::filesystem::remove(path / name / "DefaultProject.dyproject");
		
		RenameAllInFile(path / name / "Assets/Scripts/premakeTemplate.lua", path / name / "Assets/Scripts/premake5.lua", "{projectName}", name);
		std::filesystem::remove(path / name / "Assets/Scripts/premakeTemplate.lua");
	}

	void ProjectLauncher::LoadRecentProjects()
	{
		m_RecentProjects.clear();
		m_SelectedProject.clear();

		if (!std::filesystem::exists(s_RecentProjectsFilepath))
			return;

		std::ifstream file(s_RecentProjectsFilepath);
		if (file.is_open())
		{
			std::string line;
			while (getline(file, line))
			{
				bool found = false;
				for (auto& project : m_RecentProjects)
				{
					if (project.Path.string() == line)
					{
						found = true;
						break;
					}
				}

				if (!found && std::filesystem::exists(line))
					m_RecentProjects.push_back({ std::filesystem::path(line).make_preferred(), GetProjectIcon(line) });
			}
		}

		if (!m_RecentProjects.empty() && !m_CreateState)
			m_SelectedProject = m_RecentProjects[0].Path;
	}

	void ProjectLauncher::SaveRecentProjcets()
	{
		std::ofstream file(s_RecentProjectsFilepath);
		for (auto& project : m_RecentProjects)
			file << project.Path.make_preferred().string() << std::endl;
	}

	Ref<Texture2D> ProjectLauncher::GetProjectIcon(const std::filesystem::path& path)
	{
		std::filesystem::path iconPath = path.parent_path() / "Saved" / "DefaultScreenshot.png";
		if (std::filesystem::exists(iconPath))
			return Texture2D::Create(iconPath.string());
		return m_DymaticProjectIcon;
	}

}