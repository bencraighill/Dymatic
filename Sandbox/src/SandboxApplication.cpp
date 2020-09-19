#include <Dymatic.h>

#include "imgui/imgui.h"

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

	virtual void OnImGuiRender() override
	{
		ImGui::Begin("Test");
		ImGui::Text("Hello World!");
		ImGui::End();
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
	}

};

class Sandbox : public Dymatic::Application
{
public:
	Sandbox()
	{
		PushLayer(new ExampleLayer());
	}

	~Sandbox()
	{
	
	}

};

Dymatic::Application* Dymatic::CreateApplication()
{
	return new Sandbox();
}