#pragma once

#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>

class Logger;

class LoggerRegistry
{
public:
	static LoggerRegistry& instance()
	{
		static LoggerRegistry instance;	 // Guaranteed to be destroyed, instantiated on first use
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

class Logger
{
public:
	enum class LogLevel
	{
		TRACE,
		DEBUG,
		INFO,
		WARN,
		ERROR,
		CRITICAL,
		RELEASE
	};

	Logger(const std::string& name) : m_Name(name) {}

	static std::shared_ptr<Logger>& getLogger(const std::string& name)
	{
		return LoggerRegistry::instance().getLogger(name);
	}

	template <typename... Args>
	void log(LogLevel level, const std::string& format, Args&&... args)
	{
		if (level < current_log_level)
		{
			return;	 // Skip logs lower than current log level
		}

		std::string formatted_message = format_message(format, std::forward<Args>(args)...);
		std::string timestamp = get_timestamp();
		std::string color = get_color_for_level(level);
		std::string level_str = get_level_string(level);

		std::string full_message = color + timestamp + " " + m_Name + ": " + formatted_message + reset_color;

		std::lock_guard<std::mutex> lock(log_mutex);
		std::cout << full_message << std::endl;
	}

	template <typename... Args>
	void trace(const std::string& format, Args&&... args)
	{
		log(LogLevel::TRACE, format, std::forward<Args>(args)...);
	}

	template <typename... Args>
	void debug(const std::string& format, Args&&... args)
	{
		log(LogLevel::DEBUG, format, std::forward<Args>(args)...);
	}

	template <typename... Args>
	void info(const std::string& format, Args&&... args)
	{
		log(LogLevel::INFO, format, std::forward<Args>(args)...);
	}

	template <typename... Args>
	void warn(const std::string& format, Args&&... args)
	{
		log(LogLevel::WARN, format, std::forward<Args>(args)...);
	}

	template <typename... Args>
	void error(const std::string& format, Args&&... args)
	{
		log(LogLevel::ERROR, format, std::forward<Args>(args)...);
	}

	template <typename... Args>
	void critical(const std::string& format, Args&&... args)
	{
		log(LogLevel::CRITICAL, format, std::forward<Args>(args)...);
	}

	template <typename... Args>
	void release(const std::string& format, Args&&... args)
	{
		log(LogLevel::RELEASE, format, std::forward<Args>(args)...);
	}

	void set_log_level(LogLevel level) { current_log_level = level; }

private:
	std::mutex log_mutex;
	LogLevel current_log_level = LogLevel::TRACE;
	std::string m_Name;

	constexpr static const char* reset_color = "\033[0m";

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
			case LogLevel::TRACE:
				return "\033[0;97m";  // White
			case LogLevel::DEBUG:
				return "\033[0;36m";  // Cyan
			case LogLevel::INFO:
				return "\033[0;32m";  // Green
			case LogLevel::WARN:
				return "\033[0;33m";  // Yellow
			case LogLevel::ERROR:
				return "\033[0;31m";  // Red
			case LogLevel::CRITICAL:
				return "\033[0;97;41m";	 // White text, red background
			case LogLevel::RELEASE:
				return "\033[0;97m";  // White
			default:
				return reset_color;
		}
	}

	std::string get_level_string(LogLevel level) const
	{
		switch (level)
		{
			case LogLevel::TRACE:
				return "TRACE";
			case LogLevel::DEBUG:
				return "DEBUG";
			case LogLevel::INFO:
				return "INFO";
			case LogLevel::WARN:
				return "WARN";
			case LogLevel::ERROR:
				return "ERROR";
			case LogLevel::CRITICAL:
				return "CRITICAL";
			default:
				return "UNKNOWN";
		}
	}

	template <typename T>
	std::string replace_first_placeholder(std::string_view format, T&& value)
	{
		std::ostringstream oss;
		size_t pos = format.find("{}");

		if (pos == std::string::npos)
		{
			return std::string(format);	 // Ignore args
		}

		oss << format.substr(0, pos) << value << format.substr(pos + 2);
		return oss.str();
	}

	template <typename T, typename... Args>
	std::string format_message(std::string_view format, T&& value, Args&&... args)
	{
		std::string updated_format = replace_first_placeholder(format, std::forward<T>(value));

		if constexpr (sizeof...(args) == 0)
		{
			return updated_format;
		}

		return format_message(updated_format, std::forward<Args>(args)...);
	}

	std::string format_message(std::string_view format) { return std::string(format); }
};

class Log
{
public:
	static void trace(const std::string& format, auto&&... args)
	{
		Logger::getLogger("SERVER")->trace(format, std::forward<decltype(args)>(args)...);
	}

	static void info(const std::string& format, auto&&... args)
	{
		Logger::getLogger("SERVER")->info(format, std::forward<decltype(args)>(args)...);
	}

	static void debug(const std::string& format, auto&&... args)
	{
		Logger::getLogger("SERVER")->debug(format, std::forward<decltype(args)>(args)...);
	}

	static void warn(const std::string& format, auto&&... args)
	{
		Logger::getLogger("SERVER")->warn(format, std::forward<decltype(args)>(args)...);
	}

	static void error(const std::string& format, auto&&... args)
	{
		Logger::getLogger("SERVER")->error(format, std::forward<decltype(args)>(args)...);
	}

	static void critical(const std::string& format, auto&&... args)
	{
		Logger::getLogger("SERVER")->critical(format, std::forward<decltype(args)>(args)...);
	}

	static void release(const std::string& format, auto&&... args)
	{
		Logger::getLogger("SERVER")->release(format, std::forward<decltype(args)>(args)...);
	}
};