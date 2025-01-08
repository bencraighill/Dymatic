#include "dypch.h"
#include "Dymatic/Core/Log.h"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/base_sink.h>

namespace Dymatic {

	Ref<spdlog::logger> Log::s_CoreLogger;
	Ref<spdlog::logger> Log::s_ClientLogger;

	std::vector<spdlog::sink_ptr> LogSinks;

	std::function<void(const Log::Message&)> LogCallback = nullptr;

	class callback_sink : public spdlog::sinks::base_sink<std::mutex>
	{
	protected:
		void sink_it_(const spdlog::details::log_msg& msg) override
		{
			if (LogCallback)
			{
				spdlog::memory_buf_t formatted;
				spdlog::sinks::base_sink<std::mutex>::formatter_->format(msg, formatted);
				
				std::string text(msg.payload.data(), msg.payload.size());
				
				std::time_t time = std::chrono::system_clock::to_time_t(msg.time);
				std::tm* tm = std::localtime(&time);
				std::stringstream sstream;
				sstream << std::put_time(tm, "%H:%M:%S");

				LogCallback({ fmt::to_string(formatted), text, sstream.str(), msg.logger_name == "DYMATIC", msg.level });
			}
		}

		void flush_() override {}
	};

	static void LoadConfig()
	{
	}

	void Log::Init()
	{
		LoadConfig();

		LogSinks.emplace_back(CreateRef<spdlog::sinks::stdout_color_sink_mt>());
		LogSinks.emplace_back(CreateRef<spdlog::sinks::basic_file_sink_mt>("logs/Dymatic.log", true));
		LogSinks.emplace_back(CreateRef<callback_sink>());

		LogSinks[0]->set_pattern("%^[%T] %n: %v%$");
		LogSinks[1]->set_pattern("[%T] [%l] %n: %v");
		LogSinks[2]->set_pattern("[%T] [%l] %n: %v");

		s_CoreLogger = CreateRef<spdlog::logger>("DYMATIC", begin(LogSinks), end(LogSinks));
		spdlog::register_logger(s_CoreLogger);
		s_CoreLogger->set_level(spdlog::level::trace);
		s_CoreLogger->flush_on(spdlog::level::trace);

		s_ClientLogger = CreateRef<spdlog::logger>("APPLICATION", begin(LogSinks), end(LogSinks));
		spdlog::register_logger(s_ClientLogger);
		s_ClientLogger->set_level(spdlog::level::trace);
		s_ClientLogger->flush_on(spdlog::level::trace);
	}

	void Log::HideConsole()
	{
		::ShowWindow(::GetConsoleWindow(), SW_HIDE);
	}

	void Log::ShowConsole()
	{
		::ShowWindow(::GetConsoleWindow(), SW_SHOW);
	}

	bool Log::IsConsoleVisible()
	{
		return ::IsWindowVisible(::GetConsoleWindow()) != FALSE;
	}

	static std::unordered_map<const char*, int> s_TagDisplayLevels;

	void Log::SetTagDisplayLevel(const char* tag, int level)
	{
		if (level == SPDLOG_LEVEL_OFF)
			s_TagDisplayLevels.erase(tag);
		else
			s_TagDisplayLevels[tag] = level;
	}

	bool Log::ShouldDisplayTag(const char* tag, int level)
	{
		if (s_TagDisplayLevels.find(tag) == s_TagDisplayLevels.end())
			return true;

		return s_TagDisplayLevels[tag] <= level;
	}

	void Log::SetCallback(const std::function<void(const Message&)>& callback)
	{
		LogCallback = callback;
	}

}

