#include <Dymatic.h>

class ExampleLayer : public Dymatic::Layer
{
public:
	ExampleLayer()
		: Layer("Example")
	{

	}

	void OnUpdate() override
	{
		if (Dymatic::Input::IsKeyPressed(DY_KEY_TAB))

			DY_TRACE("Tab key is pressed! (poll)");
	
	}

	void OnEvent(Dymatic::Event& event) override
	{
		if (event.GetEventType() == Dymatic::EventType::KeyPressed)
		{
			Dymatic::KeyPressedEvent& e = (Dymatic::KeyPressedEvent&)event;
			if (e.GetKeyCode() == DY_KEY_TAB)
				DY_TRACE("Tab key is pressed! (event)");
			DY_TRACE("{0}", (char)e.GetKeyCode());
		}
		DY_TRACE("{0}", event);
	}

};

class Sandbox : public Dymatic::Application
{
public:
	Sandbox()
	{
		PushLayer(new ExampleLayer());
		PushOverlay(new Dymatic::ImGuiLayer());
	}

	~Sandbox()
	{
	
	}

};

Dymatic::Application* Dymatic::CreateApplication()
{
	return new Sandbox();
}