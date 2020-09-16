#pragma once

#ifdef DY_PLATFORM_WINDOWS

extern Dymatic::Application* Dymatic::CreateApplication();

int main(int argc, char** argv)
{
	Dymatic::Log::Init();
	DY_CORE_WARN("Initialized Core Log!");
	DY_INFO("Initialized Client Log!");

	printf("Dymatic Engine\n");
	auto app = Dymatic::CreateApplication();
	app->Run();
	delete app;
}

#endif
