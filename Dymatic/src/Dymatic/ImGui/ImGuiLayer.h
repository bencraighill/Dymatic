#pragma once

#include "Dymatic/Core/Layer.h"

#include "Dymatic/Events/ApplicationEvent.h"
#include "Dymatic/Events/KeyEvent.h"
#include "Dymatic/Events/MouseEvent.h"


namespace Dymatic {

	class DYMATIC_API ImGuiLayer : public Layer
	{
	public:
		ImGuiLayer();
		~ImGuiLayer();

		virtual void OnAttach() override;
		virtual void OnDetach() override;

		void Begin();
		void End();
	private:
		float m_Time = 0.0f;
	};

}