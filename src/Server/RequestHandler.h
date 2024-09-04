#pragma once

#include "ResponseSender.h"
#include "HttpRequestParser.h"

#include "Config/ConfigParser.h"

#include <memory>
#include <string>

class RequestHandler
{
public:
	RequestHandler() {};
	~RequestHandler() {};

	const std::string handleRequest(const Config& config, const std::string& request);

private:
	void parseRequest(const std::string& request);

private:
	HttpRequestParser m_RequestParser;
};