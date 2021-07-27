#include <Dymatic.h>
#include <Dymatic/Core/EntryPoint.h>

#include "EditorLayer.h"

namespace Dymatic {

	class DymaticEditor : public Application
	{
	public:
		DymaticEditor(ApplicationCommandLineArgs args)
			: Application("Dymatic Editor", args)
		{
			PushLayer(new EditorLayer());
		}

		~DymaticEditor()
		{
		}
	};

	Application* CreateApplication(ApplicationCommandLineArgs args)
	{
		return new DymaticEditor(args);
	}

}