#pragma once
#include "Dymatic.h"

namespace Dymatic {

	class Preferences
	{
	public:
		struct PreferencesData
		{
			PreferencesData();

			//--- Main Preferences ---//
			int DoubleClickSpeed = 300;
			bool EmulateNumpad = false;

			bool AutosavePreferences = true;
			bool AutosaveEnabled = true;

			int AutosaveTime = 5;
			int RecentFileCount = 10;

			bool ShowSplashStartup = true;

			bool ManualDevenv = false;
			std::string DevenvPath;

			//--- Editor Preferences ---//

			// Content Browser
			bool DirectoriesFirst = true;
			bool Ascending = true;
			enum ContentBrowserSortType
			{
				Name = 0,
				Date,
				Type,
				Size
			};
			ContentBrowserSortType ContentBrowserSortType;
			bool Filters[6];
			bool ShowThumbnails = true;

		};

	public:
		static PreferencesData& GetData();

		// Themes
		static std::string GetThemeColorName(uint8_t idx);
		static uint8_t GetThemeColor(std::string name);
		static bool LoadTheme(const std::filesystem::path& filepath);
		static void SaveTheme(const std::filesystem::path& filepath);

		// Keymap
		static bool LoadKeymap(const std::filesystem::path& filepath);
		static void SaveKeymap(const std::filesystem::path& filepath);

		// Preferences
		static bool LoadPreferences(const std::filesystem::path& filepath);
		static void SavePreferences(const std::filesystem::path& filepath);

	public:

		class Keymap
		{
		public:
			enum BindCategory
			{
				Invalid = -1,
				Keyboard = 0,
				MouseButton,
				BIND_CATEGORY_SIZE
			};

			struct KeyBindData
			{
				KeyCode KeyCode = Key::Space;
				MouseCode MouseCode = Mouse::ButtonLeft;
				BindCategory BindCategory = Keyboard;
				bool Ctrl = false;
				bool Shift = false;
				bool Alt = false;
				bool Enabled = true;
				bool Repeats = false;
			};

			enum KeyBindEvent
			{
				INVALID_BIND = -1,
				NewSceneBind = 0,
				OpenSceneBind,
				SaveSceneBind,
				SaveSceneAsBind,
				QuitBind,
				SelectObjectBind,
				SceneStartBind,
				SceneStopBind,
				GizmoNoneBind,
				GizmoTranslateBind,
				GizmoRotateBind,
				GizmoScaleBind,
				CreateBind,
				DuplicateBind,
				DeleteBind,
				ShadingTypeWireframeBind,
				ShadingTypeUnlitBind,
				ShadingTypeSolidBind,
				ShadingTypeRenderedBind,
				ToggleShadingTypeBind,
				ViewFrontBind,
				ViewSideBind,
				ViewTopBind,
				ViewFlipBind,
				ViewProjectionBind,
				RenameBind,
				ClosePopupBind,
				TextEditorDuplicate,
				TextEditorSwapLineUp,
				TextEditorSwapLineDown,
				TextEditorSwitchHeader,
				BIND_EVENT_SIZE
			};

		public:

			static std::vector<Preferences::Keymap::KeyBindEvent> CheckKey(KeyPressedEvent event);
			static std::vector<Preferences::Keymap::KeyBindEvent> CheckMouseButton(MouseButtonPressedEvent event);

			static std::string GetBindString(KeyBindEvent event);

			static KeyBindData& GetKeyBind(KeyBindEvent event);
			static std::array<KeyBindData, BIND_EVENT_SIZE>& GetKeymap();

			static std::string GetBindEventName(KeyBindEvent event);
			static KeyBindEvent GetBindEventByName(const std::string& name);
			static std::string GetKeyName(KeyCode key);
			static KeyCode GetKeyByName(const std::string& name);
			static std::string GetMouseButtonName(MouseCode button);
			static MouseCode GetMouseButtonByName(const std::string& name);
			static std::string GetBindCategoryName(BindCategory category);
			static BindCategory GetBindCategoryByName(const std::string& name);

			static const std::array<KeyCode, 120>& GetAllKeys();
			static const std::array<MouseCode, 12>& GetAllMouseButtons();
		};

	private:
	};
}
