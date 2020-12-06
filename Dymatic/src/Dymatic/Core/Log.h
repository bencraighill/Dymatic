#pragma once

#include "Dymatic/Core/Base.h"

// This ignores all warnings raised inside External headers
#pragma warning(push, 0)
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#pragma warning(pop)


namespace Dymatic {

	class Log
	{
	public:
		static void Init();

		static Ref<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
		static Ref<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }
	private:
		static Ref<spdlog::logger> s_CoreLogger;
		static Ref<spdlog::logger> s_ClientLogger;
	};

}

// Core log macros
#define DY_CORE_TRACE(...)    ::Dymatic::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define DY_CORE_INFO(...)     ::Dymatic::Log::GetCoreLogger()->info(__VA_ARGS__)
#define DY_CORE_WARN(...)     ::Dymatic::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define DY_CORE_ERROR(...)    ::Dymatic::Log::GetCoreLogger()->error(__VA_ARGS__)
#define DY_CORE_CRITICAL(...) ::Dymatic::Log::GetCoreLogger()->critical(__VA_ARGS__)

// Client log macros
#define DY_TRACE(...)         ::Dymatic::Log::GetClientLogger()->trace(__VA_ARGS__)
#define DY_INFO(...)          ::Dymatic::Log::GetClientLogger()->info(__VA_ARGS__)
#define DY_WARN(...)          ::Dymatic::Log::GetClientLogger()->warn(__VA_ARGS__)
#define DY_ERROR(...)         ::Dymatic::Log::GetClientLogger()->error(__VA_ARGS__)
#define DY_CRITICAL(...)      ::Dymatic::Log::GetClientLogger()->critical(__VA_ARGS__)
