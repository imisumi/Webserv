#pragma once

#include "ResponseSender.h"

#include <memory>
#include <string>

enum class RequestType
{
	GET,
	POST,
	DELETE,

	UNKNOWN
};

class RequestHandler
{
public:
	RequestHandler();
	~RequestHandler();

	RequestHandler(std::shared_ptr<ResponseSender> responseSender);


	void handleRequest(const std::string& request);

private:
	void parseRequest(const std::string& request);
	void handleGetRequest();
	void handlePostRequest();
	void handleDeleteRequest();

private:
	std::shared_ptr<ResponseSender> m_ResponseSender;

	RequestType m_RequestType = RequestType::UNKNOWN;

	std::string m_RequestPath;
	std::string m_RequestBody;
};