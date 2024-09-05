#include "Core/Core.h"
#include "Server/Server.h"


#include <iostream>
#include <csignal>
#include <cstdlib>  // For setenv

#include "Utils/Utils.h"
#include "Config/ConfigParser.h"


static void SignalHandler(int signal)
{
	LOG_INFO("Signal {} received!", signal);

	Server::Stop();
}

int main()
{
	Log::Init();

	// std::signal(SIGINT, SignalHandler);

	//TODOl: get root for config

	// WEB_ASSERT(false, "make sure correct path is set");
	if (setenv("HTML_ROOT_DIR", "/home/kaltevog/Desktop/Webserv/root/html", 1) != 0)
	{
		std::cerr << "Error setting environment variable" << std::endl;
		return 1;
	}

	Config conf = Config::CreateDefaultConfig();
	{
		Utils::Timer timer;
		Server::Init(conf);
	}
	Server::Run();
	Server::Shutdown();
}