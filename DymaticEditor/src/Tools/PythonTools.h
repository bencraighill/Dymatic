#pragma once

#include "Dymatic/Core/Timestep.h"

#include <filesystem>

namespace Dymatic {

	class EditorLayer;

	enum PythonUIRenderStage
	{
		Main = 0,
		MenuBar,
		MenuBar_File,
		MenuBar_Edit,
		MenuBar_Window,
		MenuBar_View,
		MenuBar_Script,
		MenuBar_Help
	};

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
		static void ReloadPlugin(const std::filesystem::path& path);
		
		static void OnUpdate(Timestep ts);
		static void OnImGuiRender(PythonUIRenderStage stage);
	};
	
}