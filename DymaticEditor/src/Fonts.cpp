#include "Fonts.h"

#include <imgui.h>

namespace Dymatic::UI {

	void PushFont(FontType font)
	{
		ImGui::PushFont(GetFont(font));
	}

	void PopFont()
	{
		ImGui::PopFont();
	}

	ImFont* GetFont(FontType font)
	{
		return ImGui::GetIO().Fonts->Fonts[(uint8_t)font];
	}

}