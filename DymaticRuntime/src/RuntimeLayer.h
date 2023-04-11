#pragma once
#include "Dymatic.h"

namespace Dymatic {

	class RuntimeLayer : public Layer
	{
	public:
		RuntimeLayer();
		virtual ~RuntimeLayer() = default;

		virtual void OnAttach() override;
		virtual void OnDetach() override;

		virtual void OnUpdate(Timestep ts) override;
		virtual void OnImGuiRender() override;
		virtual void OnEvent(Event& e) override;

	private:
		Ref<Framebuffer> m_Framebuffer;
		Ref<Scene> m_Scene;

		float m_DeltaTime;
	};

}