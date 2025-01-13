#pragma once
#include "Dymatic/Core/Base.h"
#include "Dymatic/Core/Application.h"

#ifdef DY_PLATFORM_WINDOWS

extern Dymatic::Application* Dymatic::CreateApplication(ApplicationCommandLineArgs args);

int main(int argc, char** argv)
{
	Dymatic::Log::Init();

	DY_PROFILE_BEGIN_SESSION("Startup", "DymaticProfile-Startup.json");
	auto app = Dymatic::CreateApplication({ argc, argv });
	DY_PROFILE_END_SESSION();

	DY_PROFILE_BEGIN_SESSION("Runtime", "DymaticProfile-Runtime.json");
	app->Run();
	DY_PROFILE_END_SESSION();

	DY_PROFILE_BEGIN_SESSION("Shutdown", "DymaticProfile-Shutdown.json");
	delete app;
	DY_PROFILE_END_SESSION();
}

#if defined(_WIN32) && defined(DY_DIST)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	return main(__argc, __argv);
}
#endif

#endif
