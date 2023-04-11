#include <Dymatic.h>
#include <Dymatic/Core/EntryPoint.h>

#include "EditorLayer.h"

namespace Dymatic {

	class DymaticEditor : public Application
	{
	public:
		DymaticEditor(const ApplicationSpecification& spec)
			: Application(spec)
		{
			PushLayer(new EditorLayer());
		}
	};

	Application* CreateApplication(ApplicationCommandLineArgs args)
	{
		ApplicationSpecification spec;
		spec.Name = "Dymatic Editor";
		spec.CommandLineArgs = args;
#ifdef DY_DEBUG
		spec.ConsoleVisible = true;
#else
		spec.ConsoleVisible = false;
#endif
		
		if (args.Count > 1)
			spec.WorkingDirectory = args.Args[1];

		spec.WindowDecorated = false;
		spec.WindowStartHidden = true;
		spec.ApplicationIcon = "Resources/Icons/Branding/DymaticLogoBorderSmall.png";
		
		spec.SplashImage = "Resources/Icons/Branding/DymaticSplash.bmp";
		spec.SplashName = args.Count > 2 ? "Dymatic Editor - " + std::filesystem::path(args.Args[2]).stem().string() : "Dymatic Editor";

		return new DymaticEditor(spec);
	}

}