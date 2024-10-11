#pragma once

#include "Config/Config.h"

#include "Server/Client.h"
class Cgi
{
public:

	static bool isValidCGI(const Config& config, const std::filesystem::path& path);

	static std::string executeCGI(const Client& client, const HttpRequest& request);

private:
	static void handleChildProcess(const Client& client, int pipefd[]);
	static const std::string handleParentProcess(int pipefd[], pid_t pid);
};