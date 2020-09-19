#pragma once

namespace Dymatic {

	class DYMATIC_API GraphicsContext
	{
	public:
		virtual void Init() = 0;
		virtual void SwapBuffers() = 0;
	};

}
