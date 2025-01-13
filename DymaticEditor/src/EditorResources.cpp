#include "EditorResources.h"

#include "Dymatic/Editor/Material/MaterialCompiler.h"

namespace Dymatic {

	// Audio
	Ref<Audio> EditorResources::SoundPlay;
	Ref<Audio> EditorResources::SoundSimulate;
	Ref<Audio> EditorResources::SoundStop;
	Ref<Audio> EditorResources::SoundPause;
	Ref<Audio> EditorResources::SoundStep;
	Ref<Audio> EditorResources::SoundCompileSuccess;
	Ref<Audio> EditorResources::SoundCompileFailure;

	// General Editor
	Ref<Texture2D> EditorResources::IconPlay;
	Ref<Texture2D> EditorResources::IconSimulate;
	Ref<Texture2D> EditorResources::IconStop;
	Ref<Texture2D> EditorResources::IconPause;
	Ref<Texture2D> EditorResources::IconStep;

	Ref<Texture2D> EditorResources::DymaticSplash;
	Ref<Texture2D> EditorResources::DymaticLogo;
	Ref<Texture2D> EditorResources::QuestionMarkIcon;
	Ref<Texture2D> EditorResources::ErrorIcon;
	Ref<Texture2D> EditorResources::NotificationIcon;

	Ref<Texture2D> EditorResources::CheckerboardTexture;

	// Toolbar
	Ref<Texture2D> EditorResources::SaveIcon;
	Ref<Texture2D> EditorResources::SaveHoveredIcon;
	Ref<Texture2D> EditorResources::CompileIcon;
	Ref<Texture2D> EditorResources::CompileHoveredIcon;
	Ref<Texture2D> EditorResources::SourceControlIcon;
	Ref<Texture2D> EditorResources::LiveLinkIcon;
	Ref<Texture2D> EditorResources::LiveLinkHoveredIcon;
	Ref<Texture2D> EditorResources::IDEIcon;
	Ref<Texture2D> EditorResources::ToolbarSettingsIcon;

	// Scene Hierarchy Panel
	Ref<Texture2D> EditorResources::SearchbarIcon;
	Ref<Texture2D> EditorResources::ClearIcon;

	Ref<Texture2D> EditorResources::VisibleIcon;
	Ref<Texture2D> EditorResources::HiddenIcon;
	Ref<Texture2D> EditorResources::UnlockedIcon;
	Ref<Texture2D> EditorResources::LockedIcon;
	Ref<Texture2D> EditorResources::SelectableIcon;
	Ref<Texture2D> EditorResources::NonSelectableIcon;

	Ref<Texture2D> EditorResources::IconAddComponent;
	Ref<Texture2D> EditorResources::IconDuplicate;
	Ref<Texture2D> EditorResources::IconDelete;
	Ref<Texture2D> EditorResources::IconRevert;
	Ref<Texture2D> EditorResources::IconPicker;

	// Content Browser
	Ref<Texture2D> EditorResources::DirectoryIcon;
	Ref<Texture2D> EditorResources::FileIcon;

	Ref<Texture2D> EditorResources::SceneIcon;
	Ref<Texture2D> EditorResources::PrefabIcon;
	Ref<Texture2D> EditorResources::ScriptIcon;
	Ref<Texture2D> EditorResources::MeshIcon;
	Ref<Texture2D> EditorResources::MaterialIcon;
	Ref<Texture2D> EditorResources::TextureIcon;
	Ref<Texture2D> EditorResources::EnvironmentMapIcon;
	Ref<Texture2D> EditorResources::VirtualTextureIcon;
	Ref<Texture2D> EditorResources::FontIcon;
	Ref<Texture2D> EditorResources::AudioIcon;
	Ref<Texture2D> EditorResources::ParticleSystemIcon;
	Ref<Texture2D> EditorResources::SkeletonIcon;
	Ref<Texture2D> EditorResources::AnimationIcon;
	Ref<Texture2D> EditorResources::AnimationGraphIcon;
	Ref<Texture2D> EditorResources::VideoIcon;
	Ref<Texture2D> EditorResources::VideoPlayerIcon;
	Ref<Texture2D> EditorResources::SubtitleIcon;
	Ref<Texture2D> EditorResources::ZipArchiveIcon;
	Ref<Texture2D> EditorResources::SolutionIcon;

	Ref<Texture2D> EditorResources::NewIcon;
	Ref<Texture2D> EditorResources::BrowserSettingsIcon;

	Ref<Texture2D> EditorResources::RightArrowIcon;
	Ref<Texture2D> EditorResources::RefreshIcon;
	Ref<Texture2D> EditorResources::ForwardDirectoryIcon;
	Ref<Texture2D> EditorResources::BackDirectoryIcon;
	Ref<Texture2D> EditorResources::UpDirectoryIcon;

	Ref<Texture2D> EditorResources::SourceControlUntrackedIcon;
	Ref<Texture2D> EditorResources::SourceControlAddedIcon;
	Ref<Texture2D> EditorResources::SourceControlModifiedIcon;

	// Node Editors
	Ref<Texture2D> EditorResources::HeaderBackground;
	Ref<Texture2D> EditorResources::CommentIcon;
	Ref<Texture2D> EditorResources::PinIcon;
	Ref<Texture2D> EditorResources::PinnedIcon;

	// Third Party
	Ref<Texture2D> EditorResources::PythonLogo;

	// Asset Editor Panels
	Ref<EnvironmentMap> EditorResources::DefaultEnvironmentMap;
	Ref<Texture2D> EditorResources::DefaultSkyFlowMap;

	Ref<Model> EditorResources::BoneMesh;
	Ref<Model> EditorResources::CubeMesh;
	Ref<Model> EditorResources::SphereMesh;

	Ref<MaterialAsset> EditorResources::BasicMaterial;

	void EditorResources::Init()
	{
		// Audio
		SoundPlay = Audio::Create("Resources/Audio/EditorStart.wav");
		SoundSimulate = Audio::Create("Resources/Audio/EditorSimulate.wav");
		SoundStop = Audio::Create("Resources/Audio/EditorStop.wav");
		SoundPause = Audio::Create("Resources/Audio/EditorPause.wav");
		SoundStep = Audio::Create("Resources/Audio/EditorStep.wav");
		SoundCompileSuccess = Audio::Create("Resources/Audio/EditorCompileSuccess.wav");
		SoundCompileFailure = Audio::Create("Resources/Audio/EditorCompileFailure.wav");

		// General Editor
		IconPlay = Texture2D::Create("Resources/Icons/Toolbar/PlayButton.png");
		IconSimulate = Texture2D::Create("Resources/Icons/Toolbar/SimulateButton.png");
		IconStop = Texture2D::Create("Resources/Icons/Toolbar/StopButton.png");
		IconPause = Texture2D::Create("Resources/Icons/Toolbar/PauseButton.png");
		IconStep = Texture2D::Create("Resources/Icons/Toolbar/StepButton.png");

		DymaticSplash = Texture2D::Create("Resources/Icons/Branding/DymaticSplash.bmp");
		DymaticLogo = Texture2D::Create("Resources/Icons/Branding/DymaticLogo.png");
		QuestionMarkIcon = Texture2D::Create("Resources/Icons/General/QuestionMark.png");
		ErrorIcon = Texture2D::Create("Resources/Icons/General/Error.png");
		NotificationIcon = Texture2D::Create("Resources/Icons/General/NotificationIcon.png");

		CheckerboardTexture = Texture2D::Create("Resources/Textures/Checkerboard.png");

		// Toolbar
		SaveIcon = Texture2D::Create("Resources/Icons/Toolbar/SaveIcon.png");
		SaveHoveredIcon = Texture2D::Create("Resources/Icons/Toolbar/SaveHoveredIcon.png");
		CompileIcon = Texture2D::Create("Resources/Icons/Toolbar/CompileIcon.png");
		CompileHoveredIcon = Texture2D::Create("Resources/Icons/Toolbar/CompileHoveredIcon.png");
		SourceControlIcon = Texture2D::Create("Resources/Icons/Toolbar/SourceControlIcon.png");
		LiveLinkIcon = Texture2D::Create("Resources/Icons/Toolbar/LiveLinkIcon.png");
		LiveLinkHoveredIcon = Texture2D::Create("Resources/Icons/Toolbar/LiveLinkHoveredIcon.png");
		IDEIcon = Texture2D::Create("Resources/Icons/Toolbar/IDEIcon.png");
		ToolbarSettingsIcon = Texture2D::Create("Resources/Icons/Toolbar/SettingsIcon.png");

		// Scene Hierarchy Panel
		SearchbarIcon = Texture2D::Create("Resources/Icons/SceneHierarchy/SearchbarIcon.png");
		ClearIcon = Texture2D::Create("Resources/Icons/SceneHierarchy/ClearIcon.png");
		
		VisibleIcon = Texture2D::Create("Resources/Icons/SceneHierarchy/VisibleIcon.png");
		HiddenIcon = Texture2D::Create("Resources/Icons/SceneHierarchy/HiddenIcon.png");
		UnlockedIcon = Texture2D::Create("Resources/Icons/SceneHierarchy/UnlockedIcon.png");
		LockedIcon = Texture2D::Create("Resources/Icons/SceneHierarchy/LockedIcon.png");
		SelectableIcon = Texture2D::Create("Resources/Icons/SceneHierarchy/SelectableIcon.png");
		NonSelectableIcon = Texture2D::Create("Resources/Icons/SceneHierarchy/NonSelectableIcon.png");

		IconAddComponent = Texture2D::Create("Resources/Icons/Properties/PropertiesAddComponent.png");
		IconDuplicate = Texture2D::Create("Resources/Icons/Properties/PropertiesDuplicate.png");
		IconDelete = Texture2D::Create("Resources/Icons/Properties/PropertiesDelete.png");
		IconRevert = Texture2D::Create("Resources/Icons/Properties/PropertiesRevert.png");
		IconPicker = Texture2D::Create("Resources/Icons/Properties/PickerIcon.png");
		
		// Content Browser Panel
		DirectoryIcon = Texture2D::Create("Resources/Icons/ContentBrowser/DirectoryIcon.png");
		FileIcon = Texture2D::Create("Resources/Icons/ContentBrowser/FileIcon.png");
		
		SceneIcon = Texture2D::Create("Resources/Icons/ContentBrowser/SceneIcon.png");
		PrefabIcon = Texture2D::Create("Resources/Icons/ContentBrowser/PrefabIcon.png");
		ScriptIcon = Texture2D::Create("Resources/Icons/ContentBrowser/ScriptIcon.png");
		MeshIcon = Texture2D::Create("Resources/Icons/ContentBrowser/MeshIcon.png");
		MaterialIcon = Texture2D::Create("Resources/Icons/ContentBrowser/MaterialIcon.png");
		TextureIcon = Texture2D::Create("Resources/Icons/ContentBrowser/TextureIcon.png");
		EnvironmentMapIcon = Texture2D::Create("Resources/Icons/ContentBrowser/EnvironmentMapIcon.png");
		VirtualTextureIcon = Texture2D::Create("Resources/Icons/ContentBrowser/VirtualTextureIcon.png");
		FontIcon = Texture2D::Create("Resources/Icons/ContentBrowser/FontIcon.png");
		AudioIcon = Texture2D::Create("Resources/Icons/ContentBrowser/AudioIcon.png");
		ParticleSystemIcon = Texture2D::Create("Resources/Icons/ContentBrowser/ParticleSystemIcon.png");
		SkeletonIcon = Texture2D::Create("Resources/Icons/ContentBrowser/SkeletonIcon.png");
		AnimationIcon = Texture2D::Create("Resources/Icons/ContentBrowser/AnimationIcon.png");
		AnimationGraphIcon = Texture2D::Create("Resources/Icons/ContentBrowser/AnimationGraphIcon.png");
		VideoIcon = Texture2D::Create("Resources/Icons/ContentBrowser/VideoIcon.png");
		VideoPlayerIcon = Texture2D::Create("Resources/Icons/ContentBrowser/VideoPlayerIcon.png");
		SubtitleIcon = Texture2D::Create("Resources/Icons/ContentBrowser/SubtitleIcon.png");
		ZipArchiveIcon = Texture2D::Create("Resources/Icons/ContentBrowser/ZipArchiveIcon.png");
		SolutionIcon = Texture2D::Create("Resources/Icons/ContentBrowser/SolutionIcon.png");

		NewIcon = Texture2D::Create("Resources/Icons/ContentBrowser/NewIcon.png");
		BrowserSettingsIcon = Texture2D::Create("Resources/Icons/ContentBrowser/SettingsIcon.png");

		RightArrowIcon = Texture2D::Create("Resources/Icons/ContentBrowser/RightArrowIcon.png");
		RefreshIcon = Texture2D::Create("Resources/Icons/ContentBrowser/RefreshIcon.png");
		ForwardDirectoryIcon = Texture2D::Create("Resources/Icons/ContentBrowser/ForwardDirectoryIcon.png");
		BackDirectoryIcon = Texture2D::Create("Resources/Icons/ContentBrowser/BackDirectoryIcon.png");
		UpDirectoryIcon = Texture2D::Create("Resources/Icons/ContentBrowser/UpDirectoryIcon.png");

		SourceControlUntrackedIcon = Texture2D::Create("Resources/Icons/ContentBrowser/SourceControl/SourceControlUntrackedIcon.png");
		SourceControlAddedIcon = Texture2D::Create("Resources/Icons/ContentBrowser/SourceControl/SourceControlAddedIcon.png");
		SourceControlModifiedIcon = Texture2D::Create("Resources/Icons/ContentBrowser/SourceControl/SourceControlModifiedIcon.png");

		// Node Editors
		HeaderBackground = Texture2D::Create("Resources/Icons/NodeEditor/BlueprintBackground.png");
		CommentIcon = Texture2D::Create("Resources/Icons/NodeEditor/CommentIcon.png");
		PinIcon = Texture2D::Create("Resources/Icons/NodeEditor/PinIcon.png");
		PinnedIcon = Texture2D::Create("Resources/Icons/NodeEditor/PinnedIcon.png");

		// Third Party
		PythonLogo = Texture2D::Create("Resources/Icons/ThirdParty/Python.png");

		// Asset Editor Panels
		DefaultEnvironmentMap = EnvironmentMap::Create("Resources/Textures/Environment Maps/kloofendal_overcast_puresky_2k.hdr");
		DefaultSkyFlowMap = Texture2D::Create("Resources/Textures/Environment Maps/DefaultSkyFlowMap.png");

		BoneMesh = Model::Create("Resources/Objects/Bone/Bone.fbx");
		CubeMesh = Model::Create("Resources/Objects/Basic/Cube.fbx");
		SphereMesh = Model::Create("Resources/Objects/Basic/Sphere.fbx");

		// Generate a default material
		const Ref<Editor::MaterialGraph> graph = CreateRef<Editor::MaterialGraph>();
		graph->SpawnResultNode();
		Editor::MaterialCompiler compiler(graph);
		compiler.Compile();
		DY_CORE_ASSERT(compiler.GetCompilerResult().Success, "Failed to compile basic editor material graph!");
		BasicMaterial = compiler.GetMaterial();
	}

	void EditorResources::Shutdown()
	{
		DymaticLogo = nullptr;
	}

}