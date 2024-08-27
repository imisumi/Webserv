#include "Log.h"

#include "spdlog/sinks/stdout_color_sinks.h"

#ifdef LOGGING_ENABLED
	std::shared_ptr<spdlog::logger> Log::s_ServerLogger;
#endif

void Log::Init()
{
#ifdef LOGGING_ENABLED
	spdlog::set_pattern("%^[%T] %n: %v%$");

	s_ServerLogger = spdlog::stdout_color_mt("SERVER");
	s_ServerLogger->set_level(spdlog::level::trace);
#endif
}
