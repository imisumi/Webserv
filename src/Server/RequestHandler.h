#pragma once

#include "ResponseSender.h"

#include <memory>
#include <string>

enum class RequestType
{
	GET,
	POST,
	PUT,
	PATCH,
	DELETE,
	HEAD,
	OPTIONS,

	UNKNOWN
};

class RequestHandler
{
public:
	RequestHandler() {};
	~RequestHandler() {};

	// RequestHandler(std::shared_ptr<ResponseSender> responseSender);


	// void handleRequest(const std::string& request);
	void handleRequest(const std::string& request, int epollFd);

private:
	void parseRequest(const std::string& request);
	void handleGetRequest();
	void handlePostRequest();
	void handlePutRequest();
	void handleDeleteRequest();
	void handleHeadRequest();
	void handleOptionsRequest();

private:
	// std::shared_ptr<ResponseSender> m_ResponseSender;

	int m_EpollFd = -1;

	RequestType m_RequestType = RequestType::UNKNOWN;

	std::string m_RequestPath;
	std::string m_ProtocalVersion;
	std::string m_RequestBody;
};