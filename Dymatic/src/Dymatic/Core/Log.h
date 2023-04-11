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
			std::string Text;
			int Level;
		};

		static void Init();

		static void HideConsole();
		static void ShowConsole();
		static bool IsConsoleVisible();

		static const std::vector<Message>& GetMessages();

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
