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
		spec.ConsoleVisible = false;
		spec.WindowDecorated = true;
		spec.WindowStartHidden = true;
		spec.ApplicationIcon = "Resources/Icons/Branding/DymaticLogoBorderSmall.png";

		return new DymaticRuntime(spec);
	}

}