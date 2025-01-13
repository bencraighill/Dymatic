#pragma once

#include "Dymatic/Core/Base.h"

// This ignores all warnings raised inside External headers
#pragma warning(push, 0)
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#pragma warning(pop)

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/string_cast.hpp"

namespace Dymatic {

	class Log
	{
	public:
		struct Message
		{
			std::string FormattedText;
			std::string Text;
			std::string Time;
			bool IsCore;
			int Level;
		};

		static void Init();

		static void HideConsole();
		static void ShowConsole();
		static bool IsConsoleVisible();

		static void SetTagDisplayLevel(const char* tag, int level);
		static bool ShouldDisplayTag(const char* tag, int level);
		static void SetCallback(const std::function<void(const Message&)>& callback);

		static Ref<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
		static Ref<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }
	private:
		static Ref<spdlog::logger> s_CoreLogger;
		static Ref<spdlog::logger> s_ClientLogger;
	};

}

template<typename OStream, glm::length_t L, typename T, glm::qualifier Q>
inline OStream& operator<<(OStream& os, const glm::vec<L, T, Q>& vector)
{
	return os << glm::to_string(vector);
}

template<typename OStream, glm::length_t C, glm::length_t R, typename T, glm::qualifier Q>
inline OStream& operator<<(OStream& os, const glm::mat<C, R, T, Q>& matrix)
{
	return os << glm::to_string(matrix);
}

template<typename OStream, typename T, glm::qualifier Q>
inline OStream& operator<<(OStream& os, glm::qua<T, Q> quaternion)
{
	return os << glm::to_string(quaternion);
}

#ifndef DY_DIST
// Core log macros
#define DY_CORE_TRACE(...)    ::Dymatic::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define DY_CORE_INFO(...)     ::Dymatic::Log::GetCoreLogger()->info(__VA_ARGS__)
#define DY_CORE_WARN(...)     ::Dymatic::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define DY_CORE_ERROR(...)    ::Dymatic::Log::GetCoreLogger()->error(__VA_ARGS__)
#define DY_CORE_CRITICAL(...) ::Dymatic::Log::GetCoreLogger()->critical(__VA_ARGS__)

// Tag log macros (TODO: should verify will log config that the tag level is greater than the specified enabled. TODO: Add this and implement on a per project setting basis)
#define DY_CORE_TRACE_TAG(tag, ...)		if (::Dymatic::Log::ShouldDisplayTag(tag, SPDLOG_LEVEL_TRACE)) DY_CORE_TRACE("[{}] {}", tag, __VA_ARGS__)
#define DY_CORE_INFO_TAG(tag, ...)		if (::Dymatic::Log::ShouldDisplayTag(tag, SPDLOG_LEVEL_INFO)) DY_CORE_INFO("[{}] {}", tag, __VA_ARGS__)
#define DY_CORE_WARN_TAG(tag, ...)		if (::Dymatic::Log::ShouldDisplayTag(tag, SPDLOG_LEVEL_WARN)) DY_CORE_WARN("[{}] {}", tag, __VA_ARGS__)
#define DY_CORE_ERROR_TAG(tag, ...)		if (::Dymatic::Log::ShouldDisplayTag(tag, SPDLOG_LEVEL_ERROR)) DY_CORE_ERROR("[{}] {}", tag, __VA_ARGS__)
#define DY_CORE_CRITICAL_TAG(tag, ...)	if (::Dymatic::Log::ShouldDisplayTag(tag, SPDLOG_LEVEL_CRITICAL)) DY_CORE_CRITICAL("[{}] {}", tag, __VA_ARGS__)

// Client log macros
#define DY_TRACE(...)         ::Dymatic::Log::GetClientLogger()->trace(__VA_ARGS__)
#define DY_INFO(...)          ::Dymatic::Log::GetClientLogger()->info(__VA_ARGS__)
#define DY_WARN(...)          ::Dymatic::Log::GetClientLogger()->warn(__VA_ARGS__)
#define DY_ERROR(...)         ::Dymatic::Log::GetClientLogger()->error(__VA_ARGS__)
#define DY_CRITICAL(...)      ::Dymatic::Log::GetClientLogger()->critical(__VA_ARGS__)

#else
// Core log macros
#define DY_CORE_TRACE(...)
#define DY_CORE_INFO(...)
#define DY_CORE_WARN(...)
#define DY_CORE_ERROR(...)
#define DY_CORE_CRITICAL(...)

// Tag log macros
#define DY_CORE_TRACE_TAG(tag, ...)   
#define DY_CORE_INFO_TAG(tag, ...)    
#define DY_CORE_WARN_TAG(tag, ...)    
#define DY_CORE_ERROR_TAG(tag, ...)   
#define DY_CORE_CRITICAL_TAG(tag, ...)

// Client log macros
#define DY_TRACE(...)
#define DY_INFO(...)
#define DY_WARN(...)
#define DY_ERROR(...)
#define DY_CRITICAL(...)
#endif
