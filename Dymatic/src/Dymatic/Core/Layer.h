#pragma once

#include "Dymatic/Core/Core.h"
#include "Dymatic/Core/Timestep.h"
#include "Dymatic/Events/Event.h"

namespace Dymatic {

	class DYMATIC_API Layer
	{
	public:
		Layer(const std::string& name = "Layer");
		virtual ~Layer();

		virtual void OnAttach() {}
		virtual void OnDetach() {}
		virtual void OnUpdate(Timestep ts) {}
		virtual void OnImGuiRender() {}
		virtual void OnEvent(Event& event) {}

		inline const std::string& GetName() const { return m_DebugName; }
	protected:
		std::string m_DebugName;
	};

}
