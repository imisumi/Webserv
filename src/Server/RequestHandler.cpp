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

	std::filesystem::path locationPrefix = parsedRequest.path;
	Log::info("Directory: {}", locationPrefix.string());

	//? find the longest path prefix that has location settings
	for (std::filesystem::path longestPathPrefix = parsedRequest.path; longestPathPrefix != "/"; longestPathPrefix = longestPathPrefix.parent_path())
	{
		Log::info("Longest path prefix: {}", longestPathPrefix.string());
		if (client.GetServerConfig()->hasLocationSettings(longestPathPrefix))
		{
			locationPrefix = longestPathPrefix;
			Log::debug("Location settings found for: {}", locationPrefix.string());
			break;
		}
	}

	Log::info("Location prefix: {}", locationPrefix.string());
	const ServerSettings::LocationSettings& location = client.GetServerConfig()->GetLocationSettings(locationPrefix);
	Log::error("ALLOWED METHODS: {}", location.httpMethods);
	client.SetLocationSettings(location);

	parsedRequest.mappedPath = location.root / std::filesystem::relative(parsedRequest.mappedPath, "/");
	parsedRequest.print();

	client.SetNewRequest(parsedRequest);

	//TODO: find better solution
	// const uint32_t allowedMethods =  static_cast<uint32_t>(client.GetLocationSettings().httpMethods);
	const uint8_t allowedMethods = client.GetLocationSettings().httpMethods;
	// std::cerr << client.GetLocationSettings().root << '\n';
	// std::cerr << client.GetLocationSettings().httpMethods << '\n';
	// std::cerr << static_cast<int>(client.GetLocationSettings().httpMethods) << '\n';
	// std::cerr << allowedMethods << '\n';

	static const uint8_t GET = 1;
	static const uint8_t POST = 1 << 1;
	static const uint8_t DELETE = 1 << 2;

	if (parsedRequest.method == "GET")
	{
		Log::error("GET request");
		if ((allowedMethods & GET))
		{
			return ResponseGenerator::handleGetRequest(client);
		}
		Log::error("Method not allowed");
		return ResponseGenerator::MethodNotAllowed();
	}
	else if (parsedRequest.method == "HEAD")
	{
		Log::error("GET request");
		if ((allowedMethods & GET))
		{
			std::string response = ResponseGenerator::handleGetRequest(client);

			response = extractHeaders(response);
			return response;

			return ResponseGenerator::handleGetRequest(client);
		}
		Log::error("Method not allowed");
		return ResponseGenerator::MethodNotAllowed();
	}
	else if (parsedRequest.method == "POST")
	{
		Log::error("POST request");
		if (allowedMethods & POST)
		{
			return ResponseGenerator::handlePostRequest(client);
		}
		Log::error("Method not allowed");
		return ResponseGenerator::MethodNotAllowed();
	}
	else if (parsedRequest.method == "DELETE")
	{
		Log::error("DELETE request");
		if (allowedMethods & DELETE)
		{
			return ResponseGenerator::handleDeleteRequest(client);
		}
		Log::error("Method not allowed");
		return ResponseGenerator::MethodNotAllowed();
	}
	else
	{
		Log::error("Unsupported method");
		Log::error("Allowed methods: {}", parsedRequest.method);
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
