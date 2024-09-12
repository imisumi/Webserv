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
const std::string RequestHandler::handleRequest(const Client& client, const Config& c, const std::string& request)
{
	parseRequest(request);

	// Config config = Config::CreateDefaultConfig();


	HttpRequest req = m_RequestParser.getRequest();
	LOG_INFO("URI: {}", req.getUri().string());

	auto normalize_uri = [](const std::filesystem::path& uri) {
		std::filesystem::path result = uri;

		// Remove trailing slash
		// if (result.has_filename() && result.filename() == ".") {
		// 	result.remove_filename();
		// }
		while (!result.empty() && result.filename() == ".") {
			result.remove_filename();
		}

		// Remove leading slashes by iterating through the relative path
		// result = result.lexically_relative("/");

		return result;
	};

	// req.setUri(normalize_uri(req.getUri()));
	LOG_INFO("Normalized URI: {}", req.getUri().string());


	ServerSettings* serverSettings = client.GetConfig();
	ServerSettings::LocationSettings location = (*serverSettings)[req.getUri()];

	if (!location.root.is_absolute())
	{
		//? if the root is not absolute, prepend the webserv root
		static const std::filesystem::path webservRoot = getenv("WEBSERV_ROOT");
		LOG_INFO("Webserv root: {}", webservRoot.string());
		location.root = webservRoot / location.root;
		req.setUri(webservRoot / location.root / std::filesystem::relative(req.getUri(), "/"));
		LOG_INFO("Location root: {}", location.root.string());
		LOG_INFO("Request URI {}", req.getUri().string());
	}
	else
	{
		req.setUri(location.root / std::filesystem::relative(req.getUri(), "/"));
	}


	uint8_t allowedMethods = client.GetConfig()->GetAllowedMethods(req.getUri());

	LOG_INFO("Allowed methods: {}", allowedMethods);

	//TODO: this is not working correctly
	if (allowedMethods & static_cast<uint8_t>(req.method))
	{
		return ResponseGenerator::generateResponse(client, c, req);
	}
	return ResponseGenerator::MethodNotAllowed();
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