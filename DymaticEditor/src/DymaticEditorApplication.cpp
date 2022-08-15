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

		~DymaticEditor()
		{
		}
	};

	Application* CreateApplication(ApplicationCommandLineArgs args)
	{
		ApplicationSpecification spec;
		spec.Name = "Dymatic Editor";
		spec.CommandLineArgs = args;

		if (args.Count > 1)
			spec.WorkingDirectory = args.Args[1];

		return new DymaticEditor(spec);
	}

}