#pragma once
#include <Dymatic.h>
#include "Dymatic/Core/Base.h"

namespace Dymatic {

	class LogPanel
	{
	public:
		LogPanel();

		void OnEvent(Event& e);
		void OnImGuiRender(Timestep ts);

		void OnLog(const Log::Message& message);
		void ClearLog();

	private:
		void UpdateDisplayList();

	private:
		std::vector<Log::Message> m_LogMessages;
		std::vector<Log::Message> m_LogDisplayList;
		bool m_ScrollToBottom = false;
	};

}
