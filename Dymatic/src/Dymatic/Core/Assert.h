#pragma once

#include "Dymatic/Core/Base.h"
#include "Dymatic/Core/Log.h"
#include <filesystem>

#ifdef DY_ENABLE_ASSERTS

	// Alteratively we could use the same "default" message for both "WITH_MSG" and "NO_MSG" and
	// provide support for custom formatting by concatenating the formatting string instead of having the format inside the default message
	#define DY_INTERNAL_ASSERT_IMPL(type, check, msg, ...) { if(!(check)) { DY##type##ERROR(msg, __VA_ARGS__); DY_DEBUGBREAK(); } }
	#define DY_INTERNAL_ASSERT_WITH_MSG(type, check, ...) DY_INTERNAL_ASSERT_IMPL(type, check, "Assertion failed: {0}", __VA_ARGS__)
	#define DY_INTERNAL_ASSERT_NO_MSG(type, check) DY_INTERNAL_ASSERT_IMPL(type, check, "Assertion '{0}' failed at {1}:{2}", DY_STRINGIFY_MACRO(check), std::filesystem::path(__FILE__).filename().string(), __LINE__)

	#define DY_INTERNAL_ASSERT_GET_MACRO_NAME(arg1, arg2, macro, ...) macro
	#define DY_INTERNAL_ASSERT_GET_MACRO(...) DY_EXPAND_MACRO( DY_INTERNAL_ASSERT_GET_MACRO_NAME(__VA_ARGS__, DY_INTERNAL_ASSERT_WITH_MSG, DY_INTERNAL_ASSERT_NO_MSG) )

	// Currently accepts at least the condition and one additional parameter (the message) being optional
	#define DY_ASSERT(...) DY_EXPAND_MACRO( DY_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(_, __VA_ARGS__) )
	#define DY_CORE_ASSERT(...) DY_EXPAND_MACRO( DY_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(_CORE_, __VA_ARGS__) )
#else
	#define DY_ASSERT(...)
	#define DY_CORE_ASSERT(...)
#endif
