#include <Dymatic.h>
#include <Dymatic/Core/EntryPoint.h>

#include "EditorLayer.h"

namespace Dymatic {

	class DymaticEditor : public Application
	{
	public:
		DymaticEditor()
			: Application("Dymatic Editor")
		{
			PushLayer(new EditorLayer());
		}

		~DymaticEditor()
		{
		}
	};

	Application* CreateApplication()
	{
		return new DymaticEditor();
	}

}