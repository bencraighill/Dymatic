#pragma once

#include <string>
#include <filesystem>

namespace Dymatic {

	class SourceControl
	{
	public:

		enum Status
		{
			None = 0,
			Added,
			Deleted,
			Modified,
			Renamed
		};

		SourceControl() = delete;

		static void Init();
		static void Shutdown();

		static void Setup(const std::string& remoteURL);
		static void Connect();

		static void UpdateStatus();

		static void StageFile(const std::filesystem::path& path);
		static void UnstageFile(const std::filesystem::path& path);
		static void DiscardFile(const std::filesystem::path& path);
		static void Commit(const std::string& message, const std::string& description = "");
		static void Commit(const std::string& fileString, const std::string& message, const std::string& description = "");
		static void Push();
		static void Pull();

		static bool CheckGitPath();

		static bool IsStaged(const std::filesystem::path& path);
		static bool IsUnstaged(const std::filesystem::path& path);
		
		static std::string GitCommand(const std::string& command, bool requireInit = true);
		
		static Status GetFileStagedStatus(const std::filesystem::path& file);
		static Status GetFileUnstagedStatus(const std::filesystem::path& file);
		
		static bool IsActive();
		static bool IsAuth();
	};

	class SourceControlPanel
	{
	public:
		void OnImGuiRender();
		bool& GetSourceControlPannelVisible() { return m_SourceControlPannelVisible; }

	private:
		bool m_SourceControlPannelVisible = false;

		std::string m_RemoteCommitInputBuffer;
	};

}