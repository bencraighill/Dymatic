#pragma once

#include <stdint.h>

struct ImFont;

namespace Dymatic {

	enum class FontType : uint8_t
	{
		// Default font and all 'font icons'
		Default = 0,

		// Font variants
		Bold = 1,
		Small = 2,
		ExtraLarge = 3,

		// Regular version of font awesome icons that cannot be merged
		FASolidIcons = 4,
		FARegularIcons = 5,
	};

	namespace UI
	{
		void PushFont(FontType font);
		void PopFont();
		ImFont* GetFont(FontType font);
	}

}