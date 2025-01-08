#pragma once
#include <filesystem>

namespace Dymatic {

	class VisualStudioInterface
	{
	public:
		static std::filesystem::path GetIDEPath();
		static std::filesystem::path GetSolutionPath();
		static void OpenSolution();
		static void OpenFile(const std::filesystem::path& path);
	};

}