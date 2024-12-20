#pragma once

#include <string>
#include <filesystem>
#include <fstream>
#include <optional>

#include "Core/Log.h"


//TODO: rename to Config, or abstract to ConfigParser
// #include "Config/ConfigParser.h"
#include "Config/Config.h"

// #include "Server/HttpRequestParser.h"
#include "Server/HttpParser.h"


#include "Core/Core.h"
#include "Server/Client.h"

enum class HTTPStatusCode : uint16_t
{
	OK = 200,					// request successful
	OKCreated = 201,			// resource created
	MovedToNewURL = 301,		// resource moved to new URL
	NotModified = 304,			// cached version
	BadRequest = 400,			// invalid request
	Unauthorized = 401,			// authentication required
	Forbidden = 403,			// access denied
	NotFound = 404,				// resource not found
	MethodNotAllowed = 405,		// method not allowed
	PayloadTooLarge = 413,		// payload too large
	InternalServerError = 500,	// server error
	NotImplemented = 501,		// not implemented
};

static std::map<HTTPStatusCode, std::string> HTTPStatusMap = {
	{HTTPStatusCode::OK, "200 OK"},
	{HTTPStatusCode::OKCreated, "201 Created"},
	{HTTPStatusCode::MovedToNewURL, "301 Moved Permanently"},
	{HTTPStatusCode::NotModified, "304 Not Modified"},
	{HTTPStatusCode::BadRequest, "400 Bad Request"},
	{HTTPStatusCode::Unauthorized, "401 Unauthorized"},
	{HTTPStatusCode::Forbidden, "403 Forbidden"},
	{HTTPStatusCode::NotFound, "404 Not Found"},
	{HTTPStatusCode::MethodNotAllowed, "405 Method Not Allowed"},
	{HTTPStatusCode::PayloadTooLarge, "413 Payload Too Large"},
	{HTTPStatusCode::InternalServerError, "500 Internal Server Error"},
	{HTTPStatusCode::NotImplemented, "501 Not Implemented"},
};

enum class ContentType
{
	HTML,
	TEXT,
	IMAGE
};

class ResponseGenerator
{
public:
	ResponseGenerator() {};
	~ResponseGenerator() {};

	static std::string GenerateRedirectResponse(const uint16_t redirectCode, const std::string& location);
	static std::string GenerateErrorResponse(const HTTPStatusCode code, const Client& client);
	static std::string InternalServerError();
	static std::string MethodNotAllowed();
	static std::string Forbidden();
	static std::string OkResponse();
	static std::string MethodNotImplemented();
	static std::string BadRequest();
	static std::string NotFound();
	static std::string Timeout();


	static const std::string handleGetRequest(const Client& client);
	static const std::string handleDeleteRequest(const Client& client);
	static const std::string handlePostRequest(const Client& client);

private:

	static bool isFileModified(const HttpRequest& request);

	static const std::string generateDirectoryListingResponse(const std::filesystem::path& path, const Client& client);

	static std::optional<std::string> readFileContents(const std::filesystem::path& path);

	static std::string determineContentType(const std::filesystem::path& file);

	static std::string HTTPStatusCodeToString(HTTPStatusCode code);

	static std::string ContentTypeToString(ContentType type);
	
	static std::string getCurrentDateAndTime();

	static std::string buildHttpResponse(const std::string& body, HTTPStatusCode code, const HttpRequest& request, const Client& client);
	// static std::string buildHttpResponse(const std::string& body, HTTPStatusCode code, const HttpRequest& request);

	static std::string buildHttpResponse(ContentType type, const std::string& body, HTTPStatusCode code);

	static std::string parseMultipartContentType(const std::string& body, const std::string& boundary, const std::string& fieldName);


	static std::string generateOKResponse(const Client& client);
	static std::string generateOKResponse(const std::filesystem::path& path, const HttpRequest& request, const Client& client);

	static std::string generateFileResponse(const HttpRequest& request, const Client& client);
	// static std::string generateFileResponse(const HttpRequest& request);


	static std::string generateNotModifiedResponse();
	// static std::string InternalServerError();

	static std::string generatePayloadTooLargeResponse();

};