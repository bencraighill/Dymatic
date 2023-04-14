#pragma once

#include <string>
#include <filesystem>

#include "Dymatic/Renderer/Texture.h"

namespace Dymatic {

	class PlatformUtils
	{
	public:
		static void Init();
		static void Shutdown();
	};

	class FileDialogs
	{
	public:
		static std::string OpenFile(const char* filter);
		static std::vector<std::string> OpenFileMultiple(const char* filter);
		static std::string SaveFile(const char* filter);
		static std::string SelectFolder();
	};

	class Monitor
	{
	public:
		static int GetMonitorCount();
		static glm::vec4 GetMonitorWorkArea();
	};

	class Taskbar
	{
	public:
		struct ThumbnailButton
		{
			Ref<Texture2D> Icon;
			std::string Tooltip;
			std::function<void(ThumbnailButton&)> Callback = nullptr;
			bool Enabled = true;
		};

		static void SetLoading(bool loading);
		static void SetProgress(float progress);
		
		static void FlashIcon();
		static void SetNotificationIcon(Ref<Texture2D> icon, glm::vec3 tint = glm::vec3(1.0f, 1.0f, 1.0f));

		static void AddThumbnailButton(const ThumbnailButton& button);
		static void RemoveThumbnailButton(uint32_t index);
		static void SetThumbnailButtons(const std::vector<ThumbnailButton>& buttons);
		static void UpdateThumbnailButtons();
		static void ThumbnailButtonCallback(uint32_t index);
	};

	class Splash
	{
	public:
		static void Init(const std::string& name, const std::string& splash, int rounding);
		static void Update(const std::string& message, int progress);
		static void Shutdown();
	};

	class Process
	{
	public:
		static void CreateApplicationProcess(const std::filesystem::path& path, std::vector<std::string> args);
	};

	class System
	{
	public:
		static std::string Execute(const std::string& command);
	};

	class Time
	{
	public:
		static float GetTime();
	};

}