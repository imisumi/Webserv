#pragma once

#include "Config/Config.h"

#include "Server/Client.h"
class Cgi
{
public:
	static std::string executeCGI(const Client& client, const HttpRequest& request);

private:
	// static void handleChildProcess(const Client& client, int pipefd[]);
	static void handleChildProcess(const Client& client, int inPipe[], int outPipe[]);
};