#pragma once

#include <string>
#include <filesystem>

namespace Dymatic {

	class FileDialogs
	{
	public:
		// These return empty strings if cancelled
		static std::string OpenFile(const char* filter);
		static std::string SaveFile(const char* filter);
	};

	class Process
	{
	public:
		static void CreateApplicationProcess(const std::filesystem::path& path, std::vector<std::string> args);
	};

	class Time
	{
	public:
		static float GetTime();
	};

}