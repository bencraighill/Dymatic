#pragma once

#include "Dymatic.h"
#include "Dymatic/ImGui/ImGuiLayer.h"
#include "imgui/imgui.h"
#include "Themes.h"
#include "KeyBinds.h"


namespace Dymatic {

	struct FileColor
	{
		ImVec4 Color = ImVec4{1.0f, 1.0f, 1.0f, 1.0f};
		char Extension[20] = {};
	};

	struct PreferenceData
	{
		ColorScheme colorScheme;
		KeyBinds keyBinds;
		//Autosave

		int doubleClickSpeed = 350;

		bool autosavePreferences = true;
		bool autosaveEnabled = true;
		int autosaveTime = 5;

		std::vector<FileColor> fileColors;

		bool showSplash = true;

		int NotificationPreset = 0;
		std::array<int, 2>&& NotificationToastEnabled = {};
		std::array<bool, 2>&& NotificationEnabled = {};
	};

	class Preferences
	{
	public:
		Preferences();
		virtual ~Preferences() = default;

		PreferenceData m_PreferenceData;
	};

}
