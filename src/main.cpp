#include "Core/Core.h"
#include "Server/Server.h"


#include <iostream>
#include <csignal>

static void SignalHandler(int signal)
{
	LOG_INFO("Signal {} received!", signal);

	Server::Stop();
}

int main()
{
	Log::Init();

	// std::signal(SIGINT, SignalHandler);

	Server::Init();
	Server::Run();
	Server::Shutdown();
}