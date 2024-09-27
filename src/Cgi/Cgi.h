#pragma once



// #include "Config/ConfigParser.h"
#include "Config/Config.h"
#include "Server/HttpRequestParser.h"

#include "Server/Client.h"
class Cgi
{
public:

	static bool isValidCGI(const Config& config, const std::filesystem::path& path);

	static std::string executeCGI(const Client& client, const NewHttpRequest& request);

private:
	static void handleChildProcess(const Client& client, int pipefd[]);
	static const std::string handleParentProcess(int pipefd[], pid_t pid);
};