#pragma once
#include "Dymatic.h"

#include "Filesystem/FileManager.h"
#include "Dymatic/Renderer/EditorCamera.h"

namespace Dymatic {

	class Preferences
	{
	public:
		struct PythonPluginInformation
		{
		public:
			PythonPluginInformation(const std::filesystem::path& pluginPath, bool enabled = true);

		public:
			struct PluginMetadata
			{
				std::string Name;
				std::string Description;
				Ref<Texture2D> Icon;
				std::string Author;
				std::string CompanyName;
				std::string Version;
				std::string Dependencies;
				std::string BuildDate;
				std::string EngineVersionRequirements;
				std::string LegalCopyright;
				std::string LegalTrademarks;
				std::string License;
			};

		public:
			std::filesystem::path PluginPath;
			bool Enabled = true;
			PluginMetadata Metadata;
		};

		struct ViewportBookmark
		{
			std::string Name;
			EditorCamera::EditorCameraTransform Transform;
		};

		enum class BoneAttachmentEditMode
		{
			Hierarchy,
			Component,
		};

		struct PreferencesData
		{
			PreferencesData() = default;

			// Main Preferences
			bool AutosavePreferences = true;

			bool ShowSplashStartup = true;
			bool AdvancedEditMode = false;
			BoneAttachmentEditMode BoneAttachmentEditMode = BoneAttachmentEditMode::Hierarchy;
			bool LockViewportMouse = true;

			int TooltipHoverDelay = 500;
			int DoubleClickSpeed = 300;
			bool EmulateNumpad = false;

			bool AutosaveEnabled = true;
			int AutosaveTime = 5;

			int RecentFileCount = 10;

			bool ManualDevenv = false;
			std::string DevenvPath;
			std::string GitExecutablePath;

			std::vector<PythonPluginInformation> PythonPlugins;

			std::unordered_map<std::string, std::filesystem::path> DefaultApplications;

			// Editor Preferences
			float EditorVolume = 1.0f;
			bool ShowTransformGizmo = true;
			bool ShowGrid = true;

			// Viewport
			bool ShowViewportUI = true;
			int FrameStepCount = 1;
			bool ShowFPS = false;
			bool ShowCameraPreview = true;
			std::vector<ViewportBookmark> ViewportBookmarks;

			// Content Browser
			enum class ContentBrowserLayoutType
			{
				Grid,
				List
			};
			ContentBrowserLayoutType LayoutType = ContentBrowserLayoutType::Grid;
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
			bool Filters[FILE_TYPE_SIZE];
			bool ShowThumbnails = true;
			int ThumbnailSize = 128;
			int ListItemSpacing = 0;

			// Log
			bool LogClearOnPlay = true;
			bool LogScrollToBottom = true;
			bool LogFilters[6]{true};
		};

		enum EditorWindow
		{
			None = -1,
			Viewport,
			Toolbar,
			Statistics,
			Info,
			Profiler,
			ScriptEditor,
			SceneSettings,
			SceneHierarchy,
			Properties,
			Notifications,
			ContentBrowser,
			TextEditor,
			CurveEditor,
			ImageEditor,
			Log,
			AssetManager,
			EDITOR_WINDOW_COUNT
		};

	public:
		static void Init();
		static void Shutdown();
		
		static PreferencesData& GetData();

		// Preferences
		static bool LoadPreferences(const std::filesystem::path& filepath = "");
		static void SavePreferences(const std::filesystem::path& filepath = "");

		// Themes
		static std::string GetThemeColorName(uint8_t idx);
		static uint8_t GetThemeColor(const std::string& name);
		static bool LoadTheme(const std::filesystem::path& filepath);
		static void SaveTheme(const std::filesystem::path& filepath);

		// Keymap
		static bool LoadKeymap(const std::filesystem::path& filepath);
		static void SaveKeymap(const std::filesystem::path& filepath);

		// Workspace
		static std::string GetEditorWindowName(EditorWindow editorWindow);
		static EditorWindow GetEditorWindow(const std::string& name);
		static bool& GetEditorWindowVisible(EditorWindow editorWindow);
		static bool LoadWorkspace();
		static bool LoadWorkspace(const std::filesystem::path& filepath);
		static void SaveWorkspace();
		static void SaveWorkspace(const std::filesystem::path& filepath);

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
				SceneSimulateBind,
				SceneStopBind,
				FocusBind,
				ReloadAssembly,
				GizmoNoneBind,
				GizmoTranslateBind,
				GizmoRotateBind,
				GizmoScaleBind,
				CreateBind,
				DuplicateBind,
				DeleteBind,
				UndoBind,
				RedoBind,
				VisualizationRenderedBind,
				VisualizationWireframeBind,
				VisualizationLightingOnlyBind,
				VisualizationAlbedoBind,
				VisualizationNormalBind,
				VisualizationEntityIDBind,
				ToggleVisualizationBind,
				ViewFrontBind,
				ViewSideBind,
				ViewTopBind,
				ViewFlipBind,
				ViewProjectionBind,
				OpenCommandLineBind,
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
	};
}
