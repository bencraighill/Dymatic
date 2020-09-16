#pragma once



#include "Core.h"
#include "spdlog/spdlog.h"
#include "spdlog/fmt/ostr.h"

namespace Dymatic {

	class DYMATIC_API Log
	{
	public:
		static void Init();

		inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
		inline static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }
	private:
		static std::shared_ptr<spdlog::logger> s_CoreLogger;
		static std::shared_ptr<spdlog::logger> s_ClientLogger;

	};

}


// Core Log Macros for use
#define DY_CORE_TRACE(...)   ::Dymatic::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define DY_CORE_INFO(...)    ::Dymatic::Log::GetCoreLogger()->info(__VA_ARGS__)
#define DY_CORE_WARN(...)    ::Dymatic::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define DY_CORE_ERROR(...)   ::Dymatic::Log::GetCoreLogger()->error(__VA_ARGS__)
#define DY_CORE_FATAL(...)   ::Dymatic::Log::GetCoreLogger()->fatal(__VA_ARGS__)

// Client Log Macros for use
#define DY_TRACE(...)        ::Dymatic::Log::GetClientLogger()->trace(__VA_ARGS__)
#define DY_INFO(...)         ::Dymatic::Log::GetClientLogger()->info(__VA_ARGS__)
#define DY_WARN(...)         ::Dymatic::Log::GetClientLogger()->warn(__VA_ARGS__)
#define DY_ERROR(...)        ::Dymatic::Log::GetClientLogger()->error(__VA_ARGS__)
#define DY_FATAL(...)        ::Dymatic::Log::GetClientLogger()->fatal(__VA_ARGS__)




