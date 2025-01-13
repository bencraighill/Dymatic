#include "VisualStudioInterface.h"

#include "Dymatic/Utils/PlatformUtils.h"

#include "Settings/Preferences.h"

#include <spdlog/fmt/fmt.h>

namespace Dymatic {

	std::filesystem::path VisualStudioInterface::GetIDEPath()
	{
		if (Preferences::GetData().ManualDevenv)
			return Preferences::GetData().DevenvPath;

		std::string devenvPath = System::Execute("\"\"vendor/vswhere/vswhere.exe\" -property productPath -latest\"");
		devenvPath.erase(devenvPath.find(".exe"));
		devenvPath += ".com";
		return devenvPath;
	}

	std::filesystem::path VisualStudioInterface::GetSolutionPath()
	{
		const std::filesystem::path& assetDirectory = Project::GetAssetDirectory();
		for (const auto& entry : std::filesystem::recursive_directory_iterator(assetDirectory))
		{
			if (entry.path().extension() == ".sln")
				return entry.path();
		}
		return std::filesystem::path();
	}

	void VisualStudioInterface::OpenSolution()
	{
		std::filesystem::path solutionPath = GetSolutionPath();
		if (std::filesystem::exists(solutionPath))
			System::Execute(fmt::format("\"start \"{}\" \"{}\"\"", GetIDEPath().string(), solutionPath.string()));
	}

	void VisualStudioInterface::OpenFile(const std::filesystem::path& path)
	{
		System::Execute(fmt::format("\"\"{}\" /edit \"{}\"\"", GetIDEPath().string(), path.string()));
	}

}