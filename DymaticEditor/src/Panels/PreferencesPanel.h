#pragma once

#include "Dymatic.h"
#include "Dymatic/Core/Base.h"

#include "Dymatic/Utils/PlatformUtils.h"

#include "Settings/Preferences.h"
#include "Tools/PluginLoader.h"

namespace Dymatic {

	enum PreferencesCategory
	{
		Interface,
		Themes,
		Viewport,
		Editing,
		Input,
		Navigation,
		Keymap,
		Plugins,
		System,
		SaveLoad,
		FilePaths
	};

	class PreferencesPanel
	{
	public:
		struct PreferencesPreset
		{
			std::string Name;
			std::filesystem::path Path;
		};

	public:
		PreferencesPanel();
		void OnImGuiRender();
		bool KeyBindInputButton(Preferences::Keymap::KeyBindEvent event);

		void OnEvent(Event& e);
		bool OnKeyPressed(KeyPressedEvent& e);
		bool OnMouseButtonPressed(MouseButtonPressedEvent& e);

		//Themes
		void ImportTheme();
		void ExportTheme();

		//Key Binds
		void ImportKeymap();
		void ExportKeymap();

		//Preferences
		void ImportPreferences();
		void ExportPreferences();

		bool& GetPreferencesPanelVisible() { return m_PreferencesPanelVisible; }

		void LoadAvailablePresets();
	private:
		void RefreshPlugins();
		void LoadPluginManifest();
		void WritePluginManifest();
	private:
		bool m_PreferencesPanelVisible = false;

		std::string m_KeyBindSearchBar;
		bool m_SearchByNameKey = true;

		Preferences::Keymap::KeyBindEvent m_ButtonActive = Preferences::Keymap::KeyBindEvent::INVALID_BIND;

		std::vector<PluginInfo> m_PluginInfo;

		PreferencesCategory m_CurrentCategory = Input;

		// Presets
		std::vector<PreferencesPreset> m_ThemePresets;
		std::vector<PreferencesPreset> m_KeymapPresets;

		// Input device data
		std::vector<Monitor::MonitorInfo> m_MonitorInfo;
		Network::NetworkInfo m_NetworkInfo;

		std::string m_DefaultApplicationBuffer;
	};

}