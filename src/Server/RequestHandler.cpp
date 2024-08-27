#include "RequestHandler.h"


RequestHandler::RequestHandler()
{
}

RequestHandler::~RequestHandler()
{
}

RequestHandler::RequestHandler(std::shared_ptr<ResponseSender> responseSender)
	: m_ResponseSender(responseSender)
{
}


void RequestHandler::handleRequest(const std::string& request)
{
	parseRequest(request);

	switch (m_RequestType)
	{
	case RequestType::GET:			handleGetRequest();		break;
	case RequestType::POST:			handlePostRequest();	break;
	case RequestType::DELETE:		handleDeleteRequest();	break;
	default:												break;
	}

}

static RequestType GetRequestType(const std::string& request)
{
	if (request.find("GET") != std::string::npos)
		return RequestType::GET;
	else if (request.find("POST") != std::string::npos)
		return RequestType::POST;
	else if (request.find("DELETE") != std::string::npos)
		return RequestType::DELETE;

	return RequestType::UNKNOWN;
}


void RequestHandler::parseRequest(const std::string& request)
{
	//TODO: fix
	// size_t methodEnd = request.find(' ');
	// if (methodEnd != std::string::npos)
	// {
	// 	// m_RequestType = request.substr(0, methodEnd);
	// 	m_RequestType = GetRequestType(request.substr(0, methodEnd));
	// 	size_t pathEnd = request.find(' ', methodEnd + 1);
	// 	if (pathEnd != std::string::npos)
	// 	{
	// 		m_RequestPath = request.substr(methodEnd + 1, pathEnd - methodEnd - 1);
	// 		// Handle body parsing if necessary
	// 		m_RequestBody = request.substr(pathEnd + 1);
	// 	}
	// }
}

void RequestHandler::handleGetRequest()
{
}

void RequestHandler::handlePostRequest()
{
}

void RequestHandler::handleDeleteRequest()
{
}