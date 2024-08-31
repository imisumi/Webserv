#include "Core/Core.h"
#include "Server/Server.h"


#include <iostream>
#include <csignal>
#include <cstdlib>  // For setenv


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
	if (setenv("HTML_ROOT_DIR", "/home/imisumi-wsl/dev/Webserv/root/html", 1) != 0)
	{
		std::cerr << "Error setting environment variable" << std::endl;
		return 1;
	}

	Server::Init();
	Server::Run();
	Server::Shutdown();
}