#pragma once

#include "ResponseSender.h"
#include "HttpRequestParser.h"

#include <memory>
#include <string>

// #define BIT(n) (1 << (n))

// enum class RequestMethod
// {
// 	GET      = BIT(0),
// 	POST     = BIT(1),
// 	PUT      = BIT(2),
// 	PATCH    = BIT(3),
// 	DELETE   = BIT(4),
// 	HEAD     = BIT(5),
// 	OPTIONS  = BIT(6),

// 	UNKNOWN = -1
// };

class RequestHandler
{
public:
	RequestHandler() {};
	~RequestHandler() {};

	// RequestHandler(std::shared_ptr<ResponseSender> responseSender);


	// void handleRequest(const std::string& request);
	const std::string handleRequest(const std::string& request, int epollFd);

private:
	void parseRequest(const std::string& request);
	const std::string handleGetRequest();
	void handlePostRequest();
	void handlePutRequest();
	void handleDeleteRequest();
	void handleHeadRequest();
	void handleOptionsRequest();

private:
	// std::shared_ptr<ResponseSender> m_ResponseSender;

	int m_EpollFd = -1;

	RequestMethod m_RequestMethod = RequestMethod::UNKNOWN;

	std::string m_RequestPath;
	std::string m_ProtocalVersion;
	std::string m_RequestBody;


	HttpRequestParser m_RequestParser;
};