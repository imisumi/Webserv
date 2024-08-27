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

	LOG_INFO("Starting server...");
	LOG_WARN("This is a warning!");
	LOG_ERROR("This is an error! {} {}", 1, "sadsad");

	// std::signal(SIGINT, SignalHandler);

	Server::Init();
	Server::Run();
	Server::Shutdown();
}