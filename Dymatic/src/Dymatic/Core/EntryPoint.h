#pragma once
#include "Dymatic/Core/Base.h"

#ifdef DY_PLATFORM_WINDOWS

extern Dymatic::Application* Dymatic::CreateApplication();

int main(int argc, char** argv)
{
	Dymatic::Log::Init();

	DY_PROFILE_BEGIN_SESSION("Startup", "DymaticProfile-Startup.json");
	auto app = Dymatic::CreateApplication();
	DY_PROFILE_END_SESSION();

	DY_PROFILE_BEGIN_SESSION("Runtime", "DymaticProfile-Runtime.json");
	app->Run();
	DY_PROFILE_END_SESSION();

	DY_PROFILE_BEGIN_SESSION("Shutdown", "DymaticProfile-Shutdown.json");
	delete app;
	DY_PROFILE_END_SESSION();
}

#endif
