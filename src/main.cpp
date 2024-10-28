#include "Core/Core.h"
#include "Server/Server.h"


#include <iostream>
#include <csignal>
#include <cstdlib>  // For setenv

#include "Utils/Utils.h"
// #include "Config/ConfigParser.h"
#include "Config/Config.h"


int main(int argc, char **argv)
{
	Config conf;
	if (argc > 2)
	{
		Log::error("Too many arguments");
		return 0;
	}
	try
	{
		conf = ConfigParser::createConfig(argv[1]);
	}
	catch (const std::exception& e)
	{
		Log::error("Error creating config: {}", e.what());
		return 1;
	}

	char cwd[1024];
	if (getcwd(cwd, sizeof(cwd)) != NULL)
	{
		std::cout << "Current working dir: " << cwd << std::endl;
	}
	std::string root = cwd;
	if (setenv("WEBSERV_ROOT", cwd, 1) != 0)
	{
		Log::error("Error setting environment variable");
		return 1;
	}

	// Logger::set_log_level(Logger::LogLevel::CRITICAL);
// #ifdef WEBSERV_RELEASE
// 	auto logger = Logger::getLogger("SERVER");
// 	logger->set_log_level(Logger::LogLevel::RELEASE);
// #endif

	Log::trace("trace");
	Log::info("info");
	Log::debug("debug");
	Log::warn("warn");
	Log::error("error");
	Log::critical("critical {}", 123, 1);

	Log::trace("This is a trace message {}: {}: {}", 123, "Hello", 10.12f);
	Log::trace("This is an info message {}: {}: {}", 123, "Hello", 10.12f);
	Log::trace("This is a debug message {}: {}: {}", 123, "Hello", 10.12f);
	Log::trace("This is a warning message {}: {}: {}", 123, "Hello", 10.12f);
	Log::trace("This is an error message {}: {}: {}", 123, "Hello", 10.12f);
	Log::trace("This is a critical message {}: {}: {}", 123, "Hello", 10.12f);
	// Log::trace("This is a critical message {}: {}: {", 123, "Hello", 10.12f);
	Log::trace("trace: ", 123);

	{
		Utils::Timer timer;
		Server::Init(conf);
	}
	Server::Run();
	Server::Shutdown();
}
