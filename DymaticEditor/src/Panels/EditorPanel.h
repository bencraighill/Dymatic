#pragma once

#include "Dymatic/Core/Base.h"
#include "Dymatic/Core/Timestep.h"
#include "Dymatic/Events/Event.h"

namespace Dymatic {

	class EditorPanel
	{
	protected:
		EditorPanel() = default;

	public:
		virtual void OnUpdate(Timestep ts) = 0;
		virtual void OnImGuiRender(bool& open) = 0;

		virtual void OnEvent(Event& e) {}

		virtual void Focus() {}
	};

}