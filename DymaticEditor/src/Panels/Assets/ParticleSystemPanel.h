#pragma once
#include "Panels/EditorPanel.h"

#include "Dymatic/Renderer/ParticleSystem.h"

namespace Dymatic {

	class ParticleSystemPanel : public EditorPanel
	{
	public:
		ParticleSystemPanel(const Ref<ParticleSystem> particleSystem);

		virtual void OnUpdate(Timestep ts) override;
		virtual void OnImGuiRender(bool& open) override;
		inline virtual void Focus() override { m_Focused = true; }
	private:
		void CheckModified();
		void OnModify();
	private:
		Ref<ParticleSystem> m_ParticleSystem;
		bool m_Focused = false;
	};

}