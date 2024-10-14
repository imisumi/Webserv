#pragma once

#include "Config/Config.h"

#include "Server/Client.h"
class Cgi
{
public:
	static std::string executeCGI(const Client& client, const HttpRequest& request);

private:
	static void handleChildProcess(const Client& client, int pipefd[]);
	static const std::string handleParentProcess(int pipefd[], pid_t pid);
};