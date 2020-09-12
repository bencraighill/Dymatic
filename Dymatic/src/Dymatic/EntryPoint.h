#pragma once

#ifdef DY_PLATFORM_WINDOWS

extern Dymatic::Application* Dymatic::CreateApplication();

int main(int argc, char** argv)
{
	printf("Dymatic Engine\n");
	auto app = Dymatic::CreateApplication();
	app->Run();
	delete app;
}

#endif
