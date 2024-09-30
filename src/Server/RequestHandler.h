#pragma once

#include "ResponseSender.h"
#include "HttpRequestParser.h"

// #include "Config/ConfigParser.h"
#include "Config/Config.h"

#include <memory>
#include <string>

#include "Client.h"

class RequestHandler
{
public:
	RequestHandler() {};
	~RequestHandler() {};

	const std::string HandleRequest(Client& client);

private:
	void parseRequest(const std::string& request);

private:
	HttpRequestParser m_RequestParser;
};