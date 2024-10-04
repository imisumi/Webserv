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
#include "NewHttpParser.h"

std::string extractHeaders(const std::string& response) {
    // Find the position of the "\r\n\r\n" which separates headers from the body
    size_t headerEndPos = response.find("\r\n\r\n");
    
    if (headerEndPos != std::string::npos) {
        // Extract everything up to and including the headers
        return response.substr(0, headerEndPos + 4);
    } else {
        // If "\r\n\r\n" is not found, assume the response contains only headers
        return response;
    }
}


// const std::string RequestHandler::HandleRequest(Client& client, const std::string& request)
const std::string RequestHandler::HandleRequest(Client& client)
{
	// parseRequest(request);
	NewHttpRequest parsedRequest = client.GetNewRequest();
	// if (parsedRequest.parse(request) == -1)
	// {
	// 	LOG_ERROR("Failed to parse request");
	// 	return ResponseGenerator::InternalServerError();
	// }

	HttpRequest req = m_RequestParser.getRequest();
	LOG_INFO("URI: {}", req.getUri().string());

	std::filesystem::path locationPrefix = parsedRequest.path;
	LOG_INFO("Directory: {}", locationPrefix.string());

	//? find the longest path prefix that has location settings
	for (std::filesystem::path longestPathPrefix = parsedRequest.path; longestPathPrefix != "/"; longestPathPrefix = longestPathPrefix.parent_path())
	{
		LOG_INFO("Longest path prefix: {}", longestPathPrefix.string());
		if (client.GetConfig()->hasLocationSettings(longestPathPrefix))
		{
			locationPrefix = longestPathPrefix;
			LOG_DEBUG("Location settings found for: {}", locationPrefix.string());
			break;
		}
	}

	LOG_INFO("Location prefix: {}", locationPrefix.string());
	const ServerSettings::LocationSettings& location = client.GetConfig()->GetLocationSettings(locationPrefix);
	client.SetLocationSettings(location);
	
	// parsedRequest.path = location.root / std::filesystem::relative(parsedRequest.path, "/");
	parsedRequest.mappedPath = location.root / std::filesystem::relative(parsedRequest.mappedPath, "/");
	parsedRequest.print();

	req.setUri(location.root / std::filesystem::relative(req.getUri(), "/"));
	LOG_INFO("URI: {}", req.getUri().string());

	client.SetNewRequest(parsedRequest);
	client.SetRequest(req);


	//TODO: find better solution
	const uint8_t allowedMethods = client.GetLocationSettings().httpMethods;

	static const uint8_t GET = 1;
	static const uint8_t POST = 1 << 1;
	static const uint8_t DELETE = 1 << 2;

	if (parsedRequest.method == "GET")
	{
		LOG_ERROR("GET request");
		if ((allowedMethods & GET))
		{
			return ResponseGenerator::handleGetRequest(client);
		}
		LOG_ERROR("Method not allowed");
		return ResponseGenerator::MethodNotAllowed();
	}
	else if (parsedRequest.method == "HEAD")
	{
		LOG_ERROR("GET request");
		if ((allowedMethods & GET))
		{
			std::string response = ResponseGenerator::handleGetRequest(client);

			response = extractHeaders(response);
			return response;

			return ResponseGenerator::handleGetRequest(client);
		}
		LOG_ERROR("Method not allowed");
		return ResponseGenerator::MethodNotAllowed();
	}
	else if (parsedRequest.method == "POST")
	{
		LOG_ERROR("POST request");
		if (allowedMethods & POST)
		{
			return ResponseGenerator::handlePostRequest(client);
		}
		LOG_ERROR("Method not allowed");
		return ResponseGenerator::MethodNotAllowed();
	}
	else if (parsedRequest.method == "DELETE")
	{
		LOG_ERROR("DELETE request");
		if (allowedMethods & DELETE)
		{
			return ResponseGenerator::handleDeleteRequest(client);
		}
		LOG_ERROR("Method not allowed");
		return ResponseGenerator::MethodNotAllowed();
	}
	else
	{
		LOG_ERROR("Unsupported method");
		LOG_ERROR("Allowed methods: {}", parsedRequest.method);
		return ResponseGenerator::MethodNotImplemented();
	}
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
		// std::cout << GREEN << "Body: " << WHITE << request.body << RESET << std::endl;
	}
	else
	{
		std::cerr << "Failed to parse request" << std::endl;
	}
}
