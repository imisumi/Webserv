#pragma once



// #include "Config/ConfigParser.h"
#include "Config/Config.h"
#include "Server/HttpRequestParser.h"

#include "Server/Client.h"
class Cgi
{
public:

	static bool isValidCGI(const Config& config, const std::filesystem::path& path);

	static std::string executeCGI(const Client& client, const HttpRequest& request);

	// file path should be the first argument in argv
	static std::string RunCgi(const Client& client, char* argv[]);
private:
	static void handleChildProcess(const HttpRequest& request, int pipefd[]);
	static const std::string handleParentProcess(int pipefd[], pid_t pid);
};