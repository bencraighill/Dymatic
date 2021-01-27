#pragma once

#include "Dymatic.h"
#include "Dymatic/Core/Base.h"
#include "../Preferences.h"
#include "Dymatic/Utils/PlatformUtils.h"
#include "PopupsAndNotifications.h"

namespace Dymatic {

	class PreferencesPannel
	{
	public:
		PreferencesPannel();
		void OnImGuiRender();

		std::string ColorVariableToText(ColorSchemeType colorVariable);
		void EditThemeColor(ColorSchemeType colorSchemeType, std::string tooltip = "");
		bool KeyBindInputButton(KeyBindEvent keyBindEvent);
		void NotificationEdit(std::string NotificationName, int varIndex, bool enabled = true);

		//Themes
		void ImportTheme();
		void ExportTheme();
		void RetryDythemeFile();
		void OpenThemeByPath(std::string path);
		std::string RoundFloatString(std::string stringValue);
		void SaveThemeByPath(std::string path);
		void SetThemePreferences(ColorScheme colorScheme);
		void UpdateThemePreferences();

		//Key Binds
		void ImportKeyBinds();
		void ExportKeyBinds();
		void OpenKeyBindsByFilepath(std::string filepath);
		void SaveKeyBindsByFilepath(std::string filepath);

		//Preferences
		void ImportPreferences();
		void ExportPreferences();
		void OpenPreferencesByFilepath(std::string filepath);
		void SavePreferencesByFilepath(std::string filepath);

		std::string DrawRoundedButtonStack(std::vector<std::string> buttonIds);
		Preferences& GetPreferences() { return m_Preferences; }

		void SetShowWindow(bool showWindow) { m_ShowWindow = showWindow; }
		void SetRecentDythemePath(std::string filepath) { m_RecentThemePath = filepath; }

		void SetFileColorsFromString(std::string colorString);

		PopupData m_PreferencesMessage;
		std::string ToLower(std::string inString);
	private:
		Preferences m_Preferences;
		bool m_ShowWindow = false;

		std::string KeyBindSearchBar;
		bool SearchByNameKey = true;

		KeyBindEvent m_ButtonActive = INVALID_BIND;
		KeyBindEvent m_KeySelectReturn = INVALID_BIND;

		std::string m_RecentThemePath = "";

		//Splitter Code
		float variation1 = 0.0f;
		float variation2 = 0.0f;

		std::string m_CurrentCatagory = "Input";

		//Icons
		Ref<Texture2D> m_IconMoreOptions = Texture2D::Create("assets/icons/SystemIcons/MoreOptionsIcon.png");
	};

}