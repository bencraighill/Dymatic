#include "SourceControl.h"
#include "Dymatic/Core/Base.h"

#include "Dymatic/Project/Project.h"
#include "Settings/Preferences.h"
#include "PopupsAndNotifications.h"

#include "../TextSymbols.h"

#include "Dymatic/Math/StringUtils.h"

#include <git2.h>

#include <sstream>
#include <unordered_map>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

namespace Dymatic {
	
	static git_repository* g_ActiveRepository = nullptr;
	
	struct SourceControlData
	{
		std::string Name;
		std::string Email;
		
		std::string RemoteURL;

		struct FileStatus
		{
			std::filesystem::path File;
			SourceControl::Status UnstagedStatus;
			SourceControl::Status StagedStatus;
		};
		
		std::unordered_map<std::filesystem::path, FileStatus> FileStatuses;
	};
	static SourceControlData s_Data;

	static std::string Execute(const std::string& command)
	{
		std::array<char, 256> buffer;
		std::string result;
		std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(command.c_str(), "r"), _pclose);
		if (!pipe)
			throw std::runtime_error("popen() failed!");
		while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
			result += buffer.data();
		return result;
	}

	static void UpdateUserData()
	{
		// Setup
		git_config* config;
		git_config_open_default(&config);
		git_buf buffer;
		buffer.ptr = new char[256];
		buffer.size = 256;

		// Retrieve Global Values
		memset(buffer.ptr, 0, buffer.size);
		git_config_get_string_buf(&buffer, config, "user.name");
		s_Data.Name = buffer.ptr;

		memset(buffer.ptr, 0, buffer.size);
		git_config_get_string_buf(&buffer, config, "user.email");
		s_Data.Email = buffer.ptr;

		// Cleanup
		git_config_free(config);
		delete[] buffer.ptr;

		if (g_ActiveRepository)
		{
			// Setup
			git_remote* remote;
			if (git_remote_lookup(&remote, g_ActiveRepository, "origin") == 0)
			{
				// Retrieve Remote URL
				s_Data.RemoteURL = git_remote_url(remote);

				git_remote_free(remote);
			}
			else
				s_Data.RemoteURL = "Invalid";
		}
	};

	static void ShutdownRepository()
	{
		if (g_ActiveRepository)
		{
			git_repository_free(g_ActiveRepository);
			g_ActiveRepository = nullptr;
		}
	}

	void SourceControl::Setup(const std::string& remoteURL)
	{
		if (!CheckGitPath())
			return;

		ShutdownRepository();

		if (Project::GetActive())
		{
			if (!std::filesystem::exists(Project::GetProjectDirectory() / ".git"))
			{
				GitCommand("init", false);
				GitCommand("remote add origin \"" + remoteURL + "\"");
				GitCommand("fetch origin master -q");
				GitCommand("merge origin/master");

				Connect();
			}
		}

		UpdateUserData();
	}

	void SourceControl::Init()
	{
		git_libgit2_init();
		Connect();
	}

	void SourceControl::Connect()
	{		
		ShutdownRepository();

		if (!CheckGitPath())
			return;

		if (Project::GetActive())
		{
			if (std::filesystem::exists(Project::GetProjectDirectory() / ".git"))
			{
				git_repository_open(&g_ActiveRepository, Project::GetProjectDirectory().string().c_str());
				UpdateStatus();
			}

			UpdateUserData();
		}
	}

	static void CharacterCodeToFileStatus(SourceControl::Status& unstaged, SourceControl::Status& staged, const std::string& code)
	{
		char stagedCode = code[0];
		char unstagedCode = code[1];
		
		switch (stagedCode)
		{
		case '?': staged = SourceControl::Status::None; break;
		case 'A': staged = SourceControl::Status::Added; break;
		case 'D': staged = SourceControl::Status::Deleted; break;
		case 'M': staged = SourceControl::Status::Modified; break;
		case 'R': staged = SourceControl::Status::Renamed; break;
		}

		switch (unstagedCode)
		{
		case '?': unstaged = SourceControl::Status::Added; break;
		case 'A': unstaged = SourceControl::Status::Added; break;
		case 'D': unstaged = SourceControl::Status::Deleted; break;
		case 'M': unstaged = SourceControl::Status::Modified; break;
		case 'R': unstaged = SourceControl::Status::Renamed; break;
		}
	}

	void SourceControl::UpdateStatus()
	{
		if (!CheckGitPath() || !g_ActiveRepository)
			return;
		
		s_Data.FileStatuses.clear();
		std::istringstream f(GitCommand("status -s --untracked-files=all"));
		std::string line;
		while (std::getline(f, line)) {

			String::ReplaceAll(line, "\"", "");

			{
				std::string statusString = line.substr(0, 2);
				std::filesystem::path file = std::filesystem::path(line.substr(3)).make_preferred();
				SourceControl::Status unstagedStatus = SourceControl::Status::None;
				SourceControl::Status stagedStatus = SourceControl::Status::None;
				
				CharacterCodeToFileStatus(unstagedStatus, stagedStatus, statusString);
				s_Data.FileStatuses[file] = { file, unstagedStatus, stagedStatus };
			}
		}
	}

	void SourceControl::StageFile(const std::filesystem::path& path)
	{
		if (!CheckGitPath() || !g_ActiveRepository)
			return;
		GitCommand("add \"" + path.string() + "\"");
		UpdateStatus();
	}

	void SourceControl::UnstageFile(const std::filesystem::path& path)
	{
		if (!CheckGitPath() || !g_ActiveRepository)
			return;
		GitCommand("rm --cached \"" + path.string() + "\"");
		UpdateStatus();
	}

	void SourceControl::DiscardFile(const std::filesystem::path& path)
	{
		if (!CheckGitPath() || !g_ActiveRepository)
			return;
		if (GetFileStagedStatus(path) == Status::None && GetFileUnstagedStatus(path) == Status::Added)
			std::filesystem::remove(Project::GetProjectDirectory() / path);
		else
		{
			GitCommand("checkout -- \"" + path.string() + "\"");
		}
		UpdateStatus();
	}

	void SourceControl::Commit(const std::string& message, const std::string& description)
	{
		if (!CheckGitPath() || !g_ActiveRepository)
			return;
		GitCommand("commit -m \"" + message + "\" -m \"Dymatic Engine Source Control:\n\n" + description + "\"");
		UpdateStatus();
	}

	void SourceControl::Commit(const std::string& fileString, const std::string& message, const std::string& description)
	{
		if (!CheckGitPath() || !g_ActiveRepository)
			return;
		GitCommand("commit -m \"" + message + "\" -m \"Dymatic Engine Source Control: " + description + "\" " + fileString);
		UpdateStatus();
	}

	void SourceControl::Push()
	{
		if (!CheckGitPath() || !g_ActiveRepository)
			return;
		GitCommand("push origin master");
	}

	void SourceControl::Pull()
	{
		if (!CheckGitPath() || !g_ActiveRepository)
			return;

		GitCommand("pull origin master");
	}

	bool SourceControl::CheckGitPath()
	{
		return std::filesystem::exists(Preferences::GetData().GitExecutablePath);
	}

	bool SourceControl::IsStaged(const std::filesystem::path& path)
	{
		if (!CheckGitPath())
			return false;
		return GetFileStagedStatus(path) != Status::None;
	}

	bool SourceControl::IsUnstaged(const std::filesystem::path& path)
	{
		if (!CheckGitPath())
			return false;
		return GetFileUnstagedStatus(path) != Status::None;
	}

	std::string SourceControl::GitCommand(const std::string& command, bool requireInit)
	{
		if (Project::GetActive())
		{
			if ((std::filesystem::exists(Project::GetProjectDirectory() / ".git")) || !requireInit)
			{
				auto workdir = std::filesystem::current_path();
				std::filesystem::current_path(Project::GetProjectDirectory());
				auto& output = Execute(("\"\"" + Preferences::GetData().GitExecutablePath + "\" " + command + "\""));
				std::filesystem::current_path(workdir);
				return output;
			}
		}
	}

	SourceControl::Status SourceControl::GetFileStagedStatus(const std::filesystem::path& file)
	{
		if (!CheckGitPath())
			return SourceControl::Status::None;

		auto key = file;
		key.make_preferred();
		if (s_Data.FileStatuses.find(key.string()) != s_Data.FileStatuses.end())
			return s_Data.FileStatuses[key.string()].StagedStatus;
		return SourceControl::Status::None;
	}

	SourceControl::Status SourceControl::GetFileUnstagedStatus(const std::filesystem::path& file)
	{
		if (!CheckGitPath())
			return SourceControl::Status::None;
		
		auto key = file;
		key.make_preferred();
		if (s_Data.FileStatuses.find(key.string()) != s_Data.FileStatuses.end())
			return s_Data.FileStatuses[key.string()].UnstagedStatus;
		return SourceControl::Status::None;
	}

	void SourceControl::Shutdown()
	{
		git_libgit2_shutdown();
		ShutdownRepository();
	}

	bool SourceControl::IsActive()
	{
		if (!CheckGitPath())
			return false;

		return g_ActiveRepository;
	}

	bool SourceControl::IsAuth()
	{
		if (!CheckGitPath())
			return false;

		return !s_Data.Name.empty() && !s_Data.Email.empty();
	}


	static ImVec4 GetFileStatusColor(SourceControl::Status status)
	{
		switch (status)
		{
		case SourceControl::Status::Added: return { 0.2f, 0.8f, 0.3f, 1.0f };
		case SourceControl::Status::Deleted: return { 0.9f, 0.1f, 0.2f, 1.0f };
		case SourceControl::Status::Modified: return { 0.9f, 0.8f, 0.1f, 1.0f };
		case SourceControl::Status::Renamed: return { 0.2f, 0.1f, 0.9f, 1.0f };
		}
		return { 0.5f, 0.5f, 0.5f, 1.0f };
	}

	void SourceControlPanel::OnImGuiRender()
	{
		if (!m_SourceControlPannelVisible)
			return;

		auto& io = ImGui::GetIO();
		auto& style = ImGui::GetStyle();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 10.0f));
		ImGui::Begin("Source Control", &m_SourceControlPannelVisible, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

		if (ImGui::IsWindowAppearing())
		{
			ImVec2 size(800.0f, 600.0f);
			ImGui::SetWindowSize(size);
			ImGui::SetWindowPos(ImGui::GetMainViewport()->Pos + (ImGui::GetMainViewport()->Size - size) * 0.5f);

			m_RemoteCommitInputBuffer.clear();
			UpdateUserData();
		}

		{
			ImGui::PushFont(io.Fonts->Fonts[0]);
			const char* text = "Source Control";
			ImGui::Dummy(ImVec2((ImGui::GetWindowContentRegionWidth() - ImGui::CalcTextSize(text).x) * 0.5f, 0.0f));
			ImGui::SameLine();
			ImGui::TextDisabled(text);
			ImGui::PopFont();
		}

		ImGui::Dummy(ImVec2(0.0f, 10.0f));
		if (Project::GetActive())
			if (ImGui::CloseButton(ImGui::GetID("##ProjectBrowserCloseButton"), ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x - style.WindowPadding.x - 10.0f, ImGui::GetWindowPos().y + ImGui::GetWindowContentRegionMin().y)))
				m_SourceControlPannelVisible = false;

		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg) * ImVec4(1.25f, 1.25f, 1.25f, 1.0f));
		ImGui::BeginChild("##RecentProjects", ImVec2(0.0f, ImGui::GetContentRegionAvail().y - 60.0f), false, ImGuiWindowFlags_AlwaysUseWindowPadding);
		ImGui::PopStyleColor();
		
		ImGui::TextDisabled("Name: %s", s_Data.Name.c_str());
		ImGui::TextDisabled("Email: %s", s_Data.Email.c_str());

		ImGui::Dummy(ImVec2(0.0f, 5.0f));
		ImGui::Separator();
		ImGui::Dummy(ImVec2(0.0f, 5.0f));

		static bool s_LastCheckGitPath;
		bool lastGitPath = SourceControl::CheckGitPath();
		if (lastGitPath)
		{
			if (!s_LastCheckGitPath && lastGitPath)
			{
				SourceControl::Connect();
			}
			
			if (g_ActiveRepository)
			{
				ImGui::Dummy(ImVec2((ImGui::GetContentRegionAvailWidth() - ImGui::CalcTextSize(CHARACTER_SYMBOL_TICK " Source Control Active").x) * 0.5f, 0.0f));
				ImGui::SameLine();
				ImGui::Text(CHARACTER_SYMBOL_TICK " Source Control Active");
				ImGui::TextDisabled("Remote URL: %s", s_Data.RemoteURL.c_str());

				ImGui::Dummy(ImVec2(0.0f, 20.0f));
				ImGui::Separator();
				ImGui::Dummy(ImVec2(0.0f, 20.0f));

				if (ImGui::IsWindowAppearing() || ImGui::Button(CHARACTER_ICON_REFRESH, ImVec2(60.0f, 30.0f)))
					SourceControl::UpdateStatus();

				{
					ImGui::BeginChild("##UnstagedWindow", ImVec2(ImGui::GetContentRegionAvailWidth() * 0.5f, 0.0f));
					ImGui::Text("Unstaged:");
					auto drawList = ImGui::GetWindowDrawList();

					std::filesystem::path actionPath;
					int command = 0;

					for (auto& [key, status] : s_Data.FileStatuses)
					{
						if (status.UnstagedStatus == SourceControl::Status::None)
							continue;

						auto color = ImGui::GetColorU32(GetFileStatusColor(status.UnstagedStatus));
						ImGui::Button(status.File.string().c_str(), ImVec2(ImGui::GetContentRegionAvailWidth(), 30.0f));
						auto size = ImGui::GetItemRectSize().y * 0.5f;
						auto center = ImGui::GetItemRectMin() + ImVec2(size, size);
						drawList->AddRectFilled(center - ImVec2(7.5f, 7.5f), center + ImVec2(7.5f, 7.5f), color, 2.5f);

						if (ImGui::IsItemClicked() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
						{
							command = 1;
							actionPath = key;
						}

						if (ImGui::BeginPopupContextItem())
						{
							if (ImGui::MenuItem("Stage"))
							{
								command = 1;
								actionPath = key;
							}
							if (ImGui::MenuItem("Discard"))
							{
								command = 2;
								actionPath = key;
							}
							ImGui::EndPopup();
						}
					}

					if (command == 1)
						SourceControl::StageFile(actionPath);
					else if (command == 2)
						SourceControl::DiscardFile(actionPath);

					ImGui::EndChild();
				}

				ImGui::SameLine();

				{
					ImGui::BeginChild("##StagedWindow", ImVec2(ImGui::GetContentRegionAvailWidth(), 0.0f));
					ImGui::Text("Staged:");
					auto drawList = ImGui::GetWindowDrawList();

					std::filesystem::path actionPath;
					int command = 0;

					for (auto& [key, status] : s_Data.FileStatuses)
					{
						if (status.StagedStatus == SourceControl::Status::None)
							continue;

						auto color = ImGui::GetColorU32(GetFileStatusColor(status.StagedStatus));
						ImGui::Button(status.File.string().c_str(), ImVec2(ImGui::GetContentRegionAvailWidth(), 30.0f));
						auto size = ImGui::GetItemRectSize().y * 0.5f;
						auto center = ImGui::GetItemRectMin() + ImVec2(size, size);
						drawList->AddRectFilled(center - ImVec2(7.5f, 7.5f), center + ImVec2(7.5f, 7.5f), color, 2.5f);

						if (ImGui::IsItemClicked() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
						{
							command = 1;
							actionPath = key;
						}

						if (ImGui::BeginPopupContextItem())
						{
							if (ImGui::MenuItem("Unstage"))
							{
								command = 1;
								actionPath = key;
							}
							if (ImGui::MenuItem("Discard"))
							{
								command = 2;
								actionPath = key;
							}
							ImGui::EndPopup();
						}
					}

					if (command == 1)
						SourceControl::UnstageFile(actionPath);
					else if (command == 2)
						SourceControl::DiscardFile(actionPath);

					ImGui::EndChild();
				}
			}
			else
			{
				ImGui::Text("Setup Source Control:");
				ImGui::Separator();

				ImGui::Text("Service Provider");
				ImGui::SameLine();
				ImGui::SetNextItemWidth(-1);
				if (ImGui::BeginCombo("##ProviderInput", "Git"))
				{
					ImGui::MenuItem("Git");
					ImGui::EndCombo();
				}

				ImGui::Text("Remote URL");
				ImGui::SameLine();

				char buffer[256];
				memset(buffer, 0, sizeof(buffer));
				std::strncpy(buffer, m_RemoteCommitInputBuffer.c_str(), sizeof(buffer));
				ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());
				if (ImGui::InputText("##SourceControlRemoteInput", buffer, sizeof(buffer)))
					m_RemoteCommitInputBuffer = std::string(buffer);

				ImGui::Dummy(ImVec2(0.0f, ImGui::GetContentRegionAvail().y - 35.0f - style.FramePadding.y * 2.0f));
				ImGui::Dummy(ImVec2((ImGui::GetContentRegionAvailWidth() - 300.0f) * 0.5f, 0.0f));
				ImGui::SameLine();
				ImGui::BeginDisabled(m_RemoteCommitInputBuffer.empty() || s_Data.Name.empty() || s_Data.Email.empty());
				if (ImGui::Button("Setup Source Control", ImVec2(300.0f, 35.0f)))
					SourceControl::Setup(m_RemoteCommitInputBuffer);
				ImGui::EndDisabled();
			}
		}
		else
		{
			ImGui::Text("The Git executable path specified in preferences was invalid.");
			ImGui::TextDisabled("DYMATIC EDITOR: [Source Control] - Connection to Git failed");
		}

		ImGui::EndChild();

		if (g_ActiveRepository)
		{
			{
				char buffer[256];
				memset(buffer, 0, sizeof(buffer));
				std::strncpy(buffer, m_RemoteCommitInputBuffer.c_str(), sizeof(buffer));
				ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth() - 100.0f - style.FramePadding.x * 2.0f);
				if (ImGui::InputText("##SourceControlCommitInput", buffer, sizeof(buffer)))
					m_RemoteCommitInputBuffer = buffer;
			}

			ImGui::SameLine();

			if (ImGui::Button("Commit", ImVec2(100.0f, ImGui::GetTextLineHeight() + style.FramePadding.y * 2.0f)))
			{
				SourceControl::Commit(m_RemoteCommitInputBuffer);
				m_RemoteCommitInputBuffer.clear();
			}

			ImGui::Dummy(ImVec2((ImGui::GetContentRegionAvailWidth() - 200.0f - style.FramePadding.x * 2.0f) * 0.5f, 0.0f));
			ImGui::SameLine();
			
			if (ImGui::Button("Push", ImVec2(100.0f, 35.0f)))
				SourceControl::Push();
			ImGui::SameLine();
			if (ImGui::Button("Pull", ImVec2(100.0f, 35.0f)))
				SourceControl::Pull();
		}

		s_LastCheckGitPath = lastGitPath;

		ImGui::PopStyleVar();

		ImGui::End();
	}

}