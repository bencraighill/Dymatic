#pragma once

#include "Core.h"

namespace Dymatic {

	class DYMATIC_API Application
	{
	public:
		Application();
		virtual ~Application();

		void Run();
	};

	//Define In Client
	Application* CreateApplication();

}
