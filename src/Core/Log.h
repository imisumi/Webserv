#pragma once

#include <memory>
#include <sstream>
#include <iostream>
#include <cstdarg>

#define LOGGING_ENABLED

#ifdef LOGGING_ENABLED
	#include "spdlog/spdlog.h"
#endif


class  Log
{
public:
	static void Init();

#ifdef LOGGING_ENABLED
	inline static std::shared_ptr<spdlog::logger>& GetServerLogger() { return s_ServerLogger; }
#endif
private:
#ifdef LOGGING_ENABLED
	static std::shared_ptr<spdlog::logger> s_ServerLogger;
#endif
};

#ifdef LOGGING_ENABLED
	#define LOG_TRACE(...)	        ::Log::GetServerLogger()->trace(__VA_ARGS__)
	#define LOG_INFO(...)	        ::Log::GetServerLogger()->info(__VA_ARGS__)
	#define LOG_WARN(...)	        ::Log::GetServerLogger()->warn(__VA_ARGS__)
	#define LOG_ERROR(...)	        ::Log::GetServerLogger()->error(__VA_ARGS__)
#else
	#define LOG_TRACE(...)      { std::ostringstream oss; oss << "TRACE: " << __VA_ARGS__; std::cout << oss.str() << std::endl; }
	#define LOG_INFO(...)       { std::ostringstream oss; oss << "INFO: " << __VA_ARGS__; std::cout << oss.str() << std::endl; }
	#define LOG_WARN(...)       { std::ostringstream oss; oss << "WARN: " << __VA_ARGS__; std::cout << oss.str() << std::endl; }
	#define LOG_ERROR(...)      { std::ostringstream oss; oss << "ERROR: " << __VA_ARGS__; std::cout << oss.str() << std::endl; }
#endif

