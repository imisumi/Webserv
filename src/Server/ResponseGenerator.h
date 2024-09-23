#pragma once

#include <string>
#include <filesystem>
#include <fstream>
#include <optional>

#include "Core/Log.h"


//TODO: rename to Config, or abstract to ConfigParser
// #include "Config/ConfigParser.h"
#include "Config/Config.h"

#include "HttpRequestParser.h"


#include "Core/Core.h"
#include "Client.h"


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
	InternalServerError = 500,	// server error
	NotImplemented = 501,		// not implemented
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

	static const std::string generateResponse(const Client& client, const HttpRequest& request);


	static const std::string InternalServerError(const Config& config);
	static const std::string InternalServerError();
	static const std::string MethodNotAllowed();
	static const std::string OkResponse();
	

private:

	static const std::string generateDirectoryListingResponse(const std::filesystem::path& path);

	static std::optional<std::string> readFileContents(const std::filesystem::path& path);

	static std::string determineContentType(const std::filesystem::path& file);

	static std::string HTTPStatusCodeToString(HTTPStatusCode code);

	static std::string ContentTypeToString(ContentType type);
	
	static std::string getCurrentDateAndTime();

	static std::string buildHttpResponse(const std::string& body, HTTPStatusCode code, const HttpRequest& request);

	static std::string buildHttpResponse(ContentType type, const std::string& body, HTTPStatusCode code);

	static const std::string handleGetRequest(const Client& client);

	static const std::string handlePostRequest(const Client& client, const HttpRequest& request);

	static const std::string handleDeleteRequest(const Client& client, const HttpRequest& request);

	static std::string generateOKResponse(const HttpRequest& request);
	static std::string generateOKResponse(const std::filesystem::path& path, const HttpRequest& request);

	static std::string generateFileResponse(const HttpRequest& request);

	static std::string generateForbiddenResponse();

	static std::string generateNotFoundResponse();


	static std::string generateNotModifiedResponse();

	static std::string generateNotImplementedResponse();
	static std::string generateBadRequestResponse();
	static std::string generateInternalServerErrorResponse();






};