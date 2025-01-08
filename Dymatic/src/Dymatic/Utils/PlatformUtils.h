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
		enum MonitorOrientation
		{
			Landscape = 0,
			PortraitClockwise = 1,
			LandscapeFlipped = 2,
			PortraitCounterClockwise = 3
		};
		
		struct MonitorInfo
		{
			std::string Name;
			uint32_t DisplayFrequency;
			MonitorOrientation Orientation;
			glm::ivec2 Size;

			inline const char* GetOrientationString()
			{
				switch (Orientation)
				{
				case MonitorOrientation::Landscape: return "Landscape";
				case MonitorOrientation::PortraitClockwise: return "Portrait Clockwise";
				case MonitorOrientation::LandscapeFlipped: return "Landscape Flipped";
				case MonitorOrientation::PortraitCounterClockwise: return "Portrait Counter Clockwise";
				default: return "Unknown";
				}
			}
		};

	public:
		static int GetMonitorCount();
		static MonitorInfo GetMonitorInfo(int monitor);
		static std::vector<MonitorInfo> GetMonitorInfo();
		static glm::vec4 GetMonitorWorkArea();
	};

	class Network
	{
	public:
		struct NetworkInfo
		{
			std::string Name;
			int Strength;
		};

	public:
		static NetworkInfo GetNetworkInfo();
		static void OpenURL(const std::string& url);
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
			
			UUID Handle = UUID();
		};

		static void SetLoading(bool loading);
		static void SetProgress(float progress);
		
		static void FlashIcon();
		static void SetNotificationIcon(Ref<Texture2D> icon, glm::vec3 tint = glm::vec3(1.0f, 1.0f, 1.0f));

		static UUID AddThumbnailButton(const ThumbnailButton& button);
		static void RemoveThumbnailButton(UUID handle);
		static void RemoveThumbnailButtonAtIndex(uint32_t index);
		static void SetThumbnailButtons(const std::vector<ThumbnailButton>& buttons);
		static void UpdateThumbnailButtons();

	private:
		static void ThumbnailButtonCallback(uint32_t index);

		friend class WindowsWindow;
	};

	class Splash
	{
	public:
		static void Init(const std::string& name, const std::filesystem::path& splash, uint32_t rounding);
		static void Update(const std::string& message, uint32_t progress);
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
		struct ApplicationInstallDetails
		{
			std::string Name;
			std::filesystem::path Path;
			Ref<Texture2D> Icon;
		};

	public:
		static std::string Execute(const std::string& command);
		static std::vector<ApplicationInstallDetails> GetInstalledApplications();
		static const int GetCPUCores();
	};

	class Time
	{
	public:
		static float GetTime();
	};

}