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
		Ref<SceneRendererContext> m_SceneRendererContext;
		Ref<Scene> m_Scene;

		Ref<Texture2D> m_StartupLogo = nullptr;
		float m_StartupDisplayTime = 0.0f;

		std::vector<UUID> m_PostUpdateQueue;

#ifndef DY_DIST
		struct DebugMessage
		{
			std::string Text;
			int Level;
		};
		std::vector<DebugMessage> m_DebugMessages;

		float m_DeltaTime;
#endif
	};

}