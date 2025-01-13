#pragma once

#include "Dymatic/Renderer/Texture.h"
#include "Dymatic/Renderer/EnvironmentMap.h"
#include "Dymatic/Renderer/Model.h"
#include "Dymatic/Audio/Audio.h"

namespace Dymatic {

	class EditorResources
	{
	public:
		static void Init();
		static void Shutdown();

	public:
		// Audio
		static Ref<Audio> SoundPlay, SoundSimulate, SoundStop, SoundPause, SoundStep, SoundCompileSuccess, SoundCompileFailure;

		// General Editor
		static Ref<Texture2D> IconPlay, IconSimulate, IconStop, IconPause, IconStep;

		static Ref<Texture2D> DymaticSplash;
		static Ref<Texture2D> DymaticLogo;
		static Ref<Texture2D> QuestionMarkIcon;
		static Ref<Texture2D> ErrorIcon;
		static Ref<Texture2D> NotificationIcon;

		static Ref<Texture2D> CheckerboardTexture;
		
		// Toolbar
		static Ref<Texture2D> SaveIcon;
		static Ref<Texture2D> SaveHoveredIcon;
		static Ref<Texture2D> CompileIcon;
		static Ref<Texture2D> CompileHoveredIcon;
		static Ref<Texture2D> SourceControlIcon;
		static Ref<Texture2D> LiveLinkIcon;
		static Ref<Texture2D> LiveLinkHoveredIcon;
		static Ref<Texture2D> IDEIcon;
		static Ref<Texture2D> ToolbarSettingsIcon;

		// Scene Hierarchy Panel
		static Ref<Texture2D> SearchbarIcon;
		static Ref<Texture2D> ClearIcon;

		static Ref<Texture2D> VisibleIcon;
		static Ref<Texture2D> HiddenIcon;
		static Ref<Texture2D> UnlockedIcon;
		static Ref<Texture2D> LockedIcon;
		static Ref<Texture2D> SelectableIcon;
		static Ref<Texture2D> NonSelectableIcon;
		
		static Ref<Texture2D> IconAddComponent;
		static Ref<Texture2D> IconDuplicate;
		static Ref<Texture2D> IconDelete;
		static Ref<Texture2D> IconRevert;
		static Ref<Texture2D> IconPicker;

		// Content Browser
		static Ref<Texture2D> DirectoryIcon;
		static Ref<Texture2D> FileIcon;
		
		static Ref<Texture2D> SceneIcon;
		static Ref<Texture2D> PrefabIcon;
		static Ref<Texture2D> ScriptIcon;
		static Ref<Texture2D> MeshIcon;
		static Ref<Texture2D> MaterialIcon;
		static Ref<Texture2D> TextureIcon;
		static Ref<Texture2D> EnvironmentMapIcon;
		static Ref<Texture2D> VirtualTextureIcon;
		static Ref<Texture2D> FontIcon;
		static Ref<Texture2D> AudioIcon;
		static Ref<Texture2D> ParticleSystemIcon;
		static Ref<Texture2D> SkeletonIcon;
		static Ref<Texture2D> AnimationIcon;
		static Ref<Texture2D> AnimationGraphIcon;
		static Ref<Texture2D> VideoIcon;
		static Ref<Texture2D> VideoPlayerIcon;
		static Ref<Texture2D> SubtitleIcon;
		static Ref<Texture2D> ZipArchiveIcon;
		static Ref<Texture2D> SolutionIcon;

		static Ref<Texture2D> NewIcon;
		static Ref<Texture2D> BrowserSettingsIcon;

		static Ref<Texture2D> RightArrowIcon;
		static Ref<Texture2D> RefreshIcon;
		static Ref<Texture2D> ForwardDirectoryIcon;
		static Ref<Texture2D> BackDirectoryIcon;
		static Ref<Texture2D> UpDirectoryIcon;

		static Ref<Texture2D> SourceControlUntrackedIcon;
		static Ref<Texture2D> SourceControlAddedIcon;
		static Ref<Texture2D> SourceControlModifiedIcon;

		// Node Editors
		static Ref<Texture2D> HeaderBackground;
		static Ref<Texture2D> CommentIcon;
		static Ref<Texture2D> PinIcon;
		static Ref<Texture2D> PinnedIcon;

		// Third Party
		static Ref<Texture2D> PythonLogo;

		// Asset Editor Panels
		static Ref<EnvironmentMap> DefaultEnvironmentMap;
		static Ref<Texture2D> DefaultSkyFlowMap;

		static Ref<Model> BoneMesh;
		static Ref<Model> CubeMesh;
		static Ref<Model> SphereMesh;

		static Ref<MaterialAsset> BasicMaterial;
	};

}