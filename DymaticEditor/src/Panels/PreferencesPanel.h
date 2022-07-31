#pragma once

#include "Dymatic.h"
#include "Dymatic/Core/Base.h"

#include "../Plugins/PluginLoader.h"
#include "../Preferences.h"

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

	class PreferencesPannel
	{
	public:
		PreferencesPannel();
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

		void LoadPresetLayout();
	private:
		void RefreshPlugins();
		void LoadPluginManifest();
		void WritePluginManifest();
	private:
		bool m_PreferencesPanelVisible = false;

		std::string m_KeyBindSearchBar;
		bool m_SearchByNameKey = true;

		Preferences::Keymap::KeyBindEvent m_ButtonActive = Preferences::Keymap::KeyBindEvent::INVALID_BIND;

		std::string m_RecentThemePath = "";

		std::vector<PluginInfo> m_PluginInfo;

		PreferencesCategory m_CurrentCategory = Input;

		//Icons
		Ref<Texture2D> m_IconMoreOptions = Texture2D::Create("assets/icons/SystemIcons/MoreOptionsIcon.png");

		std::vector<std::string> m_SelectableThemeNames;
		std::vector<std::string> m_SelectableThemePaths;

		std::vector<std::string> m_SelectableKeybindNames;
		std::vector<std::string> m_SelectableKeybindPaths;
	};

}