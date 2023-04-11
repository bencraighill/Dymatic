#pragma once

#include <string>

namespace Dymatic {

	class AssetManagerPanel
	{
	public:
		AssetManagerPanel() = default;

		void OnImGuiRender();
	private:
		std::string m_SearchBuffer;
	};

}