#pragma once

#include "Dymatic/Core/Layer.h"

#include "Dymatic/Events/ApplicationEvent.h"
#include "Dymatic/Events/KeyEvent.h"
#include "Dymatic/Events/MouseEvent.h"
#include "Dymatic/Events/GamepadEvent.h"

namespace Dymatic {

	class ImGuiLayer : public Layer
	{
	public:
		ImGuiLayer();
		~ImGuiLayer() = default;

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnEvent(Event& e) override;

		void Begin();
		void End();

		void BlockEvents(bool block) { m_BlockEvents = block; }

		void AddIconFont(const char* path, float size, int min_range, int max_range);
	private:
		bool m_BlockEvents = true;
	};

}