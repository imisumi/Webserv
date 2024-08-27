#include "RequestHandler.h"

#include "Core/Log.h"

// RequestHandler::RequestHandler()
// {
// }

// RequestHandler::~RequestHandler()
// {
// }

// RequestHandler::RequestHandler(std::shared_ptr<ResponseSender> responseSender)
// 	// : m_ResponseSender(responseSender)
// {
// }


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
	else if (request.find("PUT") != std::string::npos)
		return RequestType::PUT;
	else if (request.find("PATCH") != std::string::npos)
		return RequestType::PATCH;
	else if (request.find("DELETE") != std::string::npos)
		return RequestType::DELETE;
	else if (request.find("HEAD") != std::string::npos)
		return RequestType::HEAD;
	else if (request.find("OPTIONS") != std::string::npos)
		return RequestType::OPTIONS;

	return RequestType::UNKNOWN;
}

static std::string RequestTypeToString(RequestType type)
{
	switch (type)
	{
	case RequestType::GET:			return "GET";
	case RequestType::POST:			return "POST";
	case RequestType::PUT:			return "PUT";
	case RequestType::PATCH:		return "PATCH";
	case RequestType::DELETE:		return "DELETE";
	case RequestType::HEAD:			return "HEAD";
	case RequestType::OPTIONS:		return "OPTIONS";
	default:					return "UNKNOWN";
	}
}


void RequestHandler::parseRequest(const std::string& request)
{
	//TODO: fix
	size_t methodEnd = request.find(' ');
	if (methodEnd != std::string::npos)
	{
		m_RequestType = GetRequestType(request.substr(0, methodEnd));
		LOG_INFO("Request type: {}", RequestTypeToString(m_RequestType));
		size_t pathEnd = request.find(' ', methodEnd + 1);
		if (pathEnd != std::string::npos)
		{
			m_RequestPath = request.substr(methodEnd + 1, pathEnd - methodEnd - 1);
			LOG_INFO("Request path: {}", m_RequestPath);

			size_t protocalEnd = request.find('\r', pathEnd + 1);
			if (protocalEnd != std::string::npos)
			{
				m_ProtocalVersion = request.substr(pathEnd + 1, protocalEnd - pathEnd - 1);
				LOG_INFO("Protocal version: {}", m_ProtocalVersion);
			}
		}
	}
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