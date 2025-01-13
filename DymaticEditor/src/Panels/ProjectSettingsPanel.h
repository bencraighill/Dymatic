#pragma once

#include <cinttypes>

namespace Dymatic {

	class ProjectSettingsPanel
	{
	public:
		ProjectSettingsPanel() = default;
		void OnImGuiRender();

		inline bool& GetVisible() { return m_Visible; }

	private:
		bool m_Visible = false;

		uint64_t m_SelectedPhysicsLayer = -1;
	};

}