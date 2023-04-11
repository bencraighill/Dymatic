#include "dypch.h"
#include "Dymatic/Core/Log.h"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/base_sink.h>

namespace spdlog::sinks
{
	class vector_sink : public spdlog::sinks::base_sink<std::mutex>
	{
	public:
		const std::vector<Dymatic::Log::Message>& GetMessages() { return m_Messages; }
	private:
		std::vector<Dymatic::Log::Message> m_Messages;
	protected:
		void sink_it_(const spdlog::details::log_msg& msg) override
		{
			spdlog::memory_buf_t formatted;
			spdlog::sinks::base_sink<std::mutex>::formatter_->format(msg, formatted);
			m_Messages.push_back({ fmt::to_string(formatted), msg.level });
		}

		void flush_() override {}
	};
}

namespace Dymatic {

	Ref<spdlog::logger> Log::s_CoreLogger;
	Ref<spdlog::logger> Log::s_ClientLogger;

	std::vector<spdlog::sink_ptr> LogSinks;

	void Log::Init()
	{
		LogSinks.emplace_back(CreateRef<spdlog::sinks::stdout_color_sink_mt>());
		LogSinks.emplace_back(CreateRef<spdlog::sinks::basic_file_sink_mt>("Dymatic.log", true));
		LogSinks.emplace_back(CreateRef<spdlog::sinks::vector_sink>());

		LogSinks[0]->set_pattern("%^[%T] %n: %v%$");
		LogSinks[1]->set_pattern("[%T] [%l] %n: %v");
		LogSinks[2]->set_pattern("%^[%T] %n: %v%$");

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
		::SetForegroundWindow(::GetConsoleWindow());
		::ShowWindow(::GetForegroundWindow(), SW_HIDE);
	}

	void Log::ShowConsole()
	{
		::ShowWindow(::GetConsoleWindow(), SW_SHOW);
	}

	bool Log::IsConsoleVisible()
	{
		return ::IsWindowVisible(::GetConsoleWindow()) != FALSE;
	}

	const std::vector<Log::Message>& Log::GetMessages()
	{
		return As<spdlog::sinks::vector_sink>(LogSinks[2])->GetMessages();
	}

}

