#pragma once

#include <memory>

#include "Dymatic/Core/PlatformDetection.h"

#ifdef DY_DEBUG
#if defined(DY_PLATFORM_WINDOWS)
#define DY_DEBUGBREAK() __debugbreak()
#elif defined(DY_PLATFORM_LINUX)
#include <signal.h>
#define DY_DEBUGBREAK() raise(SIGTRAP)
#else
#error "Platform doesn't support debugbreak yet!"
#endif
#define DY_ENABLE_ASSERTS
#else
#define DY_DEBUGBREAK()
#endif

#define DY_EXPAND_MACRO(x) x
#define DY_STRINGIFY_MACRO(x) #x

#define BIT(x) (1 << x)

#define DY_BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

namespace Dymatic {

	template<typename T>
	using Scope = std::unique_ptr<T>;
	template<typename T, typename ... Args>
	constexpr Scope<T> CreateScope(Args&& ... args)
	{
		return std::make_unique<T>(std::forward<Args>(args)...);
	}

	template<typename T>
	using Ref = std::shared_ptr<T>;
	template<typename T, typename ... Args>
	constexpr Ref<T> CreateRef(Args&& ... args)
	{
		return std::make_shared<T>(std::forward<Args>(args)...);
	}

}

#include "Dymatic/Core/Log.h"
#include "Dymatic/Core/Assert.h"
