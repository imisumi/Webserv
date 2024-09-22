#pragma once



#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <ctime>
#include <chrono>
#include <iomanip>
#include <mutex>
#include <stdexcept>
#include <assert.h>
#include <unordered_map>
#include <memory>

#include <string.h>

class Logger;

class LoggerRegistry
{
public:
	static LoggerRegistry& instance()
	{
		static LoggerRegistry instance;  // Guaranteed to be destroyed, instantiated on first use
		return instance;
	}

	std::shared_ptr<Logger>& getLogger(const std::string& name)
	{
		if (m_Loggers.find(name) == m_Loggers.end())
		{
			m_Loggers[name] = std::make_shared<Logger>(name);
		}
		return m_Loggers[name];
	}

private:
	LoggerRegistry() = default;
	std::unordered_map<std::string, std::shared_ptr<Logger>> m_Loggers;
};

//TODO: Add support for log file
class Logger
{
public:
	enum class LogLevel { TRACE, DEBUG, INFO, WARN, ERROR, CRITICAL };

	Logger(const std::string& name)
		: m_Name(name)
	{
	}

	static std::shared_ptr<Logger>& getLogger(const std::string& name)
	{
		return LoggerRegistry::instance().getLogger(name);
	}

	template<typename... Args>
	void log(LogLevel level, const std::string& format, Args&&... args)
	{
		if (level < current_log_level)
		{
			return; // Skip logs lower than current log level
		}

		std::string formatted_message = format_message(format, std::forward<Args>(args)...);
		std::string timestamp = get_timestamp();
		std::string color = get_color_for_level(level);
		std::string level_str = get_level_string(level);
		
		std::string full_message = color + timestamp + " " + m_Name + ": " + formatted_message + reset_color;

		std::lock_guard<std::mutex> lock(log_mutex);
		// log_file << full_message << std::endl;
		std::cout << full_message << std::endl;
	}

	template<typename... Args>
	void trace(const std::string& format, Args&&... args)
	{
		log(LogLevel::TRACE, format, std::forward<Args>(args)...);
	}

	template<typename... Args>
	void debug(const std::string& format, Args&&... args)
	{
		log(LogLevel::DEBUG, format, std::forward<Args>(args)...);
	}

	template<typename... Args>
	void info(const std::string& format, Args&&... args)
	{
		log(LogLevel::INFO, format, std::forward<Args>(args)...);
	}

	template<typename... Args>
	void warn(const std::string& format, Args&&... args)
	{
		log(LogLevel::WARN, format, std::forward<Args>(args)...);
	}

	template<typename... Args>
	void error(const std::string& format, Args&&... args)
	{
		log(LogLevel::ERROR, format, std::forward<Args>(args)...);
	}

	template<typename... Args>
	void critical(const std::string& format, Args&&... args)
	{
		log(LogLevel::CRITICAL, format, std::forward<Args>(args)...);
	}

	void set_name(const std::string& name)
	{
		m_Name = name;
	}

	void set_log_level(LogLevel level)
	{
		current_log_level = level;
	}

private:
	// std::ofstream log_file;
	std::mutex log_mutex;
	LogLevel current_log_level = LogLevel::TRACE;
	std::string m_Name;

	const char reset_color[8] = "\033[0m";

	std::string get_timestamp()
	{
		auto now = std::chrono::system_clock::now();
		auto now_c = std::chrono::system_clock::to_time_t(now);
		std::ostringstream oss;
		oss << std::put_time(std::localtime(&now_c), "[%H:%M:%S]");
		return oss.str();
	}

	std::string get_color_for_level(LogLevel level) const
	{
		switch (level)
		{
			case LogLevel::TRACE:		return "\033[0;97m";		// White
			case LogLevel::DEBUG:		return "\033[0;36m";		// Cyan
			case LogLevel::INFO:		return "\033[0;32m";		// Green
			case LogLevel::WARN:		return "\033[0;33m";		// Yellow
			case LogLevel::ERROR:		return "\033[0;31m";		// Red
			case LogLevel::CRITICAL:	return "\033[0;97;41m";		// White text, red background
			default:					return reset_color;
		}
	}

	std::string get_level_string(LogLevel level) const
	{
		switch (level)
		{
			case LogLevel::TRACE:		return "TRACE";
			case LogLevel::DEBUG:		return "DEBUG";
			case LogLevel::INFO:		return "INFO";
			case LogLevel::WARN:		return "WARN";
			case LogLevel::ERROR:		return "ERROR";
			case LogLevel::CRITICAL:	return "CRITICAL";
			default:					return "UNKNOWN";
		}
	}

	template<typename T>
	std::string replace_first_placeholder(std::string_view format, T&& value)
	{
		std::ostringstream oss;
		size_t pos = format.find("{}");
		
		// If no placeholders are found, return the original format
		if (pos == std::string::npos)
		{
			return std::string(format); // Ignore args
		}

		// Construct the output string
		oss << format.substr(0, pos) << value << format.substr(pos + 2);
		return oss.str();
	}

	// Recursive function to process the variadic template arguments
	template<typename T, typename... Args>
	std::string format_message(std::string_view format, T&& value, Args&&... args)
	{
		std::string updated_format = replace_first_placeholder(format, std::forward<T>(value));

		// If no more arguments, return the updated format
		if constexpr (sizeof...(args) == 0)
		{
			return updated_format;
		}

		// Recur for the remaining arguments
		return format_message(updated_format, std::forward<Args>(args)...);
	}

	// Base case for variadic recursion (when there are no more arguments)
	std::string format_message(std::string_view format)
	{
		return std::string(format);
	}
};

#define LOG_TRACE(...)	        ::LoggerRegistry::instance().getLogger("SERVER")->trace(__VA_ARGS__)
#define LOG_INFO(...)	        ::LoggerRegistry::instance().getLogger("SERVER")->info(__VA_ARGS__)
#define LOG_DEBUG(...)	        ::LoggerRegistry::instance().getLogger("SERVER")->debug(__VA_ARGS__)
#define LOG_WARN(...)	        ::LoggerRegistry::instance().getLogger("SERVER")->warn(__VA_ARGS__)
#define LOG_ERROR(...)	        ::LoggerRegistry::instance().getLogger("SERVER")->error(__VA_ARGS__)
#define LOG_CRITICAL(...)	    ::LoggerRegistry::instance().getLogger("SERVER")->critical(__VA_ARGS__)
