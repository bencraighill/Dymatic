#include <Dymatic.h>
#include <Dymatic/Core/EntryPoint.h>

#include "LauncherLayer.h"

namespace Dymatic {

	class DymaticLauncher : public Application
	{
	public:
		DymaticLauncher()
			: Application("Dymatic Launcher")
		{
			PushLayer(new LauncherLayer());
		}

		~DymaticLauncher()
		{
		}
	};

	Application* CreateApplication(ApplicationCommandLineArgs args)
	{
		return new DymaticLauncher();
	}

}