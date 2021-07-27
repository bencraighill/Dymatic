#pragma once
#include <Dymatic.h>
#include "Dymatic/Core/Base.h"

#include "ContentBrowser.h"

#include "../Preferences.h"

namespace Dymatic {

	class FilePrompt
	{
	public:
		FilePrompt(Preferences* preferencesRef);

		void OnImGuiRender(Timestep ts);
	private:
		Preferences* m_PreferencesReference;
		ContentBrowser m_InternalContentBrowser = ContentBrowser(m_PreferencesReference, NULL);

		std::string m_SaveFileNameBar = "";
	};

}
