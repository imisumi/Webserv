#pragma once

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
	// HttpRequestParser m_RequestParser;
};