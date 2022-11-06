#pragma once

namespace Dymatic {

	enum class TextureFormat
	{
		None = 0,

		// Color
		RGBA8,
		RGB8,
		RG8,
		R8,

		RGBA16F,
		RGB16F,
		RG16F,
		R16F,

		RED_INTEGER,

		// Depth/Stencil
		DEPTH24STENCIL8,

		// Defaults
		Depth = DEPTH24STENCIL8
	};

}