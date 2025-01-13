#pragma once

namespace Dymatic {

	class RendererContext
	{
	public:
		virtual ~RendererContext() = default;

		virtual void Init() = 0;
		virtual void Shutdown() = 0;
		virtual void SwapBuffers() = 0;

		static Scope<RendererContext> Create(void* window);
	};

}