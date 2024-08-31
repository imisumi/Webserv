#pragma once

#include <string>
#include <filesystem>
#include <fstream>

#include "Core/Log.h"


//TODO: rename to Config, or abstract to ConfigParser
#include "Config/ConfigParser.h"

#include "HttpRequestParser.h"


#include "Core/Core.h"


enum class HTTPStatusCode
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
	InternalServerError = 500	// server error
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

	static const std::string generateResponse(const Config& config, const HttpRequest& request);

private:
	static std::string readFileContents(const std::filesystem::path& path);


	static std::string determineContentType(const std::filesystem::path& file);

	static std::string HTTPStatusCodeToString(HTTPStatusCode code);

	static std::string ContentTypeToString(ContentType type);
	
	static std::string getCurrentDateAndTime();

	static std::string buildHttpResponse(const std::filesystem::path& path, const std::string& body, HTTPStatusCode code);

	static std::string buildHttpResponse(ContentType type, const std::string& body, HTTPStatusCode code);

	static const std::string handleGetRequest(const Config& config, const HttpRequest& request);

	static std::string generateOKResponse(const std::filesystem::path& path);

	static std::string generateFileResponse(const std::filesystem::path& path);

	// static std::string generateForbiddenResponse(const std::filesystem::path& path);
	static std::string generateForbiddenResponse();

	static std::string generateNotFoundResponse();
};