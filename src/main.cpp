#include "Core/Core.h"
#include "Server/Server.h"


#include <iostream>
#include <csignal>
#include <cstdlib>  // For setenv

#include "Utils/Utils.h"
// #include "Config/ConfigParser.h"
#include "Config/Config.h"


static void SignalHandler(int signal)
{
	LOG_INFO("Signal {} received!", signal);

	Server::Stop();
}

int main()
{
	Log::Init();

	// {
	// 	Config conf = Config::CreateDefaultConfig();
	// 	conf.Print();
	// 	return 0;
	// }

	// std::signal(SIGINT, SignalHandler);

	//TODOl: get root for config

	// WEB_ASSERT(false, "make sure correct path is set");

	// print working directory
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

	Config conf = ConfigParser::createDefaultConfig();
	{
		Utils::Timer timer;
		Server::Init(conf);
	}
	Server::Run();
	Server::Shutdown();
}