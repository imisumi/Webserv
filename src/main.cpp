#include "Core/Core.h"
#include "Server/Server.h"


#include <iostream>
#include <csignal>
#include <cstdlib>  // For setenv

#include "Utils/Utils.h"
// #include "Config/ConfigParser.h"
#include "Config/Config.h"


int main()
{
	char cwd[1024];
	if (getcwd(cwd, sizeof(cwd)) != NULL)
	{
		std::cout << "Current working dir: " << cwd << std::endl;
	}
	std::string root = cwd;
	if (setenv("WEBSERV_ROOT", cwd, 1) != 0)
	{
		std::cerr << "Error setting environment variable" << std::endl;
		return 1;
	}

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


	root += "/root/html";
	
	if (setenv("WORKING_DIR", cwd, 1) != 0)
	{
		std::cerr << "Error setting environment variable" << std::endl;
		return 1;
	}

	if (setenv("HTML_ROOT_DIR", root.c_str(), 1) != 0)
	{
		std::cerr << "Error setting environment variable" << std::endl;
		return 1;
	}

	std::string cgiRoot = cwd;
	cgiRoot += "/root/webserv/cgi-bin";
	if (setenv("CGI_ROOT_DIR", cgiRoot.c_str(), 1) != 0)
	{
		std::cerr << "Error setting environment variable" << std::endl;
		return 1;
	}
	Config conf;
	try
	{
		conf = ConfigParser::createDefaultConfig();
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error while creating config." << std::endl;
		return 1;
	}
	{
		Utils::Timer timer;
		Server::Init(conf);
	}
	Server::Run();
	Server::Shutdown();
}
