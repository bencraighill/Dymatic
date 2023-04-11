#pragma once

#include <filesystem>

namespace Dymatic {

	class EditorLayer;

	class PythonTools
	{
	public:
		PythonTools() = delete;

		static void Init(EditorLayer* context);
		static void Shutdown();

		static void RunScript(const std::filesystem::path& path);
		static void RunGlobalCommand(const std::string& command);

		static void LoadPlugin(const std::filesystem::path& path);
		static void UnloadPlugin(std::filesystem::path path);
		static void ReloadPlugin(std::filesystem::path path);
		
		static void OnUpdate();
		static void OnImGuiRender();
	};
	
}