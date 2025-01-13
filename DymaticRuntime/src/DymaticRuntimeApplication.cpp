#include <Dymatic.h>
#include <Dymatic/Core/EntryPoint.h>

#include "RuntimeLayer.h"

namespace Dymatic {

	class DymaticRuntime : public Application
	{
	public:
		DymaticRuntime(const ApplicationSpecification& spec)
			: Application(spec)
		{
			PushLayer(new RuntimeLayer());
		}
	};

	Application* CreateApplication(ApplicationCommandLineArgs args)
	{
		ApplicationSpecification spec;
		
		spec.Name = "Dymatic Runtime";
		spec.CommandLineArgs = args;
		spec.Runtime = true;
		spec.ConsoleVisible = false;
		spec.WindowDecorated = true;
		spec.WindowStartHidden = true;
		spec.ApplicationIcon = "Resources/Icons/Branding/DymaticLogoBorderSmall.png";

		RendererConfig config;
		config.ShaderPackPath = "Resources/ShaderPack.dysp";
		Renderer::SetConfig(config);

		AssetManager::DeserializeAssetPack("Assets/AssetPack.dyap");

#ifdef DY_DIST
		spec.EnableImGui = false;
#endif

		return new DymaticRuntime(spec);
	}

}