#include <Dymatic.h>

class Sandbox : public Dymatic::Application
{
public:
	Sandbox()
	{

	}

	~Sandbox()
	{
	
	}

};

Dymatic::Application* Dymatic::CreateApplication()
{
	return new Sandbox();
}