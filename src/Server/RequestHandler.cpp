#include "RequestHandler.h"

#include "ResponseGenerator.h"

#include "Core/Log.h"

#include <filesystem>
#include <fstream>

#define GREEN "\033[32m"
#define WHITE "\033[37m"
#define RESET "\033[0m"


/*
	GET
	- parameters in the URL
	- used for fetching documents/files
	- maximum length of the URL is 2048 characters
	- okay to cache
	- should not change the server

	POST
	- parameters in the body
	- used for updating data
	- no maximum length
	- not okay to cache
	- can change the server
*/
const std::string RequestHandler::handleRequest(const Client& client, const Config& config, const std::string& request)
{
	parseRequest(request);

	// Config config = Config::CreateDefaultConfig();

	HttpRequest req = m_RequestParser.getRequest();
	req.setUri(config.getRoot() / std::filesystem::relative(req.getUri(), "/"));

	return ResponseGenerator::generateResponse(client, config, req);
}



std::string RequestMethodToString(RequestMethod type)
{
	switch (type)
	{
	case RequestMethod::GET:		return "GET";
	case RequestMethod::POST:		return "POST";
	case RequestMethod::PUT:		return "PUT";
	case RequestMethod::PATCH:		return "PATCH";
	case RequestMethod::DELETE:		return "DELETE";
	case RequestMethod::HEAD:		return "HEAD";
	case RequestMethod::OPTIONS:	return "OPTIONS";
	default:						return "UNKNOWN";
	}
}

void RequestHandler::parseRequest(const std::string& requestBuffer)
{
	m_RequestParser.reset();
	if (m_RequestParser.parse(requestBuffer))
	{
		const HttpRequest& request = m_RequestParser.getRequest();
		std::cout << GREEN << "Method: " << WHITE << RequestMethodToString(request.method) << RESET << std::endl;
		std::cout << GREEN << "URI: " << WHITE << request.getUri().string() << RESET << std::endl;
		std::cout << GREEN << "Version: " << WHITE << request.version << RESET << std::endl;
		for (const auto& header : request.headers) {
			std::cout << GREEN << header.first << ": " << WHITE << header.second << RESET << std::endl;
		}
		std::cout << GREEN << "Body: " << WHITE << request.body << RESET << std::endl;
	}
	else
	{
		std::cerr << "Failed to parse request" << std::endl;
	}


}