#include "ResponseGenerator.h"

#include "Core/Core.h"



static const char s_ForbiddenResponse[] = 
		"HTTP/1.1 403 Forbidden\r\n"
		"Content-Length: 28\r\n" //? length has to match the length of the body
		"Connection: close\r\n"
		"Content-Type: text/plain\r\n"
		"\r\n"
		"403 Forbidden: Access Denied"; //? body of the response

static const char s_NotFoundResponse[] = 
		"HTTP/1.1 404 Not Found\r\n"
		"Content-Length: 33\r\n" //? length has to match the length of the body
		"Connection: close\r\n"
		"Content-Type: text/plain\r\n"
		"\r\n"
		"404 Not Found: Resource Not Found"; //? body of the response


//? Multipurpose Internet Mail Extensions (MIME) type
static const std::unordered_map<std::string, std::string> s_SupportedMineTypes = {
			{ ".html", "text/html" },
			{ ".ico", "image/x-icon" },
			{ ".jpg", "image/jpeg" },
			{ ".jpeg", "image/jpeg" },
			{ ".png", "image/png" }
		};

const std::string ResponseGenerator::generateResponse(const Config& config, const HttpRequest& request)
{
	//TODO: validate request
	//? fo now only GET is supported

	
	//TODO: use switch statement once method is changed from string to enum class
	// if (request.method == "GET")
	// {
	// 	return handleGetRequest(config, request);
	// }

	switch (request.method)
	{
		case RequestMethod::GET:			return handleGetRequest(config, request);
		case RequestMethod::POST:			break;
		case RequestMethod::PUT:			break;
		case RequestMethod::PATCH:			break;
		case RequestMethod::DELETE:			break;
		case RequestMethod::HEAD:			break;
		case RequestMethod::OPTIONS:		break;
		default:							break;
	}

	return generateNotFoundResponse();
}








//? assume that the file exists
//? return the contents of the file or nullptr if the file cannot be opened due to permissions
std::string ResponseGenerator::readFileContents(const std::filesystem::path& path)
{
	LOG_DEBUG("Reading file contents: {}", path.string());
	std::ifstream file(path);
	if (!file.is_open())
	{
		LOG_ERROR("Failed to open file: {}", path.string());
		return std::string();
	}

	std::stringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}

std::string ResponseGenerator::determineContentType(const std::filesystem::path& file)
{
	// log file
	LOG_INFO("File: {}", file.string());
	std::string extension = file.extension().string();
	LOG_INFO("Extension: {}", extension);
	auto it = s_SupportedMineTypes.find(extension);

	if (it != s_SupportedMineTypes.end())
	{
		LOG_INFO("Content-Type: {}", it->second);
		return it->second;
	}
	return std::string();
}

std::string ResponseGenerator::HTTPStatusCodeToString(HTTPStatusCode code)
{
	switch (code)
	{
		case HTTPStatusCode::OK:					return "200 OK";
		case HTTPStatusCode::OKCreated:				return "201 Created";
		case HTTPStatusCode::MovedToNewURL:			return "301 Moved Permanently";
		case HTTPStatusCode::NotModified:			return "304 Not Modified";
		case HTTPStatusCode::BadRequest:			return "400 Bad Request";
		case HTTPStatusCode::Unauthorized:			return "401 Unauthorized";
		case HTTPStatusCode::Forbidden:				return "403 Forbidden";
		case HTTPStatusCode::NotFound:				return "404 Not Found";
		case HTTPStatusCode::MethodNotAllowed:		return "405 Method Not Allowed";
		case HTTPStatusCode::InternalServerError:	return "500 Internal Server Error";
		default:									return "";
	}
}

std::string ResponseGenerator::ContentTypeToString(ContentType type)
{
	switch (type)
	{
		case ContentType::HTML:		return "text/html";
		case ContentType::TEXT:		return "text/plain";
		case ContentType::IMAGE:	return "image/x-icon";
		default:					return "";
	}
}

std::string ResponseGenerator::getCurrentDateAndTime()
{
	// Get current time
	auto now = std::chrono::system_clock::now();
	std::time_t current_time = std::chrono::system_clock::to_time_t(now);
	std::tm* local_time = std::localtime(&current_time);

	// Format the date and time
	std::ostringstream oss;
	oss << std::put_time(local_time, "%a, %d %b %Y %H:%M:%S CET");

	return oss.str();
}

std::string ResponseGenerator::buildHttpResponse(const std::filesystem::path& path, const std::string& body, HTTPStatusCode code)
{
	std::ostringstream response;

	std::string contentType = determineContentType(path);
	if (contentType.empty())
	{
		// return generateForbiddenResponse(path);
		return generateForbiddenResponse();
	}

	const std::string statusCode = HTTPStatusCodeToString(code);

	WEB_ASSERT(!statusCode.empty(), "Invalid HTTP status code! (add a custom code or use a valid one)");

	// response << "HTTP/1.1 200 OK\r\n"
	response << "HTTP/1.1 " << statusCode << "\r\n"
			<< "Server: Webserv/1.0\r\n"
			// << "Date: " << getCurrentDateAndTime() << "\r\n"



			<< "Content-Type: " << contentType << "\r\n"
			// << "Content-Type: text/html\r\n"
			// << "Content-Type: image/x-icon\r\n"


			<< "Content-Length: " << body.size() << "\r\n"
			// << "Last-Modified: " << getFileModificationTime(path) << "\r\n"
			//TODO: hard coded values should check this
			// << "Connection: keep-alive\r\n"
			// << "ETag: \"" << generateETag(body) << "\"\r\n"
			<< "Accept-Ranges: bytes\r\n"
			// Disable caching
			// << "Cache-Control: no-store, no-cache, must-revalidate, proxy-revalidate, max-age=0\r\n"
			// << "Pragma: no-cache\r\n"
			// << "Expires: 0\r\n"
			<< "\r\n";  // End of headers

	if (!body.empty())
		response << body;

	return response.str();
}


std::string ResponseGenerator::buildHttpResponse(ContentType type, const std::string& body, HTTPStatusCode code)
{
	std::ostringstream response;

	// response << "HTTP/1.1 200 OK\r\n"
	response << "HTTP/1.1 " << HTTPStatusCodeToString(code) << "\r\n"
			<< "Server: Webserv/1.0\r\n"
			<< "Date: " << getCurrentDateAndTime() << "\r\n"



			<< "Content-Type: " << ContentTypeToString(type) << "\r\n"
			// << "Content-Type: text/html\r\n"
			// << "Content-Type: image/x-icon\r\n"


			<< "Content-Length: " << body.size() << "\r\n"
			// << "Last-Modified: " << getFileModificationTime(path) << "\r\n"
			//TODO: hard coded values should check this
			<< "Connection: keep-alive\r\n"
			// << "ETag: \"" << generateETag(body) << "\"\r\n"
			// << "Accept-Ranges: bytes\r\n"
			// Disable caching
			// << "Cache-Control: no-store, no-cache, must-revalidate, proxy-revalidate, max-age=0\r\n"
			// << "Pragma: no-cache\r\n"
			// << "Expires: 0\r\n"
			<< "\r\n";  // End of headers

	if (!body.empty())
		response << body;

	return response.str();
}


std::string ReadImageFile(const std::filesystem::path& path)
{
	std::ifstream file(path, std::ios::binary);
	if (!file.is_open())
	{
		LOG_ERROR("Failed to open file: {}", path.string());
		return std::string();
	}
	// Read the file content into a string
	std::ostringstream oss;
	oss << file.rdbuf();
	std::string favicon_content = oss.str();
	file.close();
	return favicon_content;
}

const std::string ResponseGenerator::handleGetRequest(const Config& config, const HttpRequest& request)
{
	LOG_INFO("Handling GET request");
	const std::filesystem::path root = config.getRoot();
	const std::filesystem::path uri = root / std::filesystem::relative(request.uri, "/");
		LOG_DEBUG("Requested path: {}", uri.string());

	//? Validate the requested path
	if (std::filesystem::exists(uri))
	{
		//? Check if the requested path is a directory or a file
		if (std::filesystem::is_directory(uri))
		{
			LOG_DEBUG("Requested path is a directory");

			//TODO: see if config maps to the given uri, if so see if index is overwriten esle use default

			const std::filesystem::path indexPath = uri / "index.html";
			if (std::filesystem::exists(indexPath))
			{
				LOG_DEBUG("index.html exists");

				// const std::string fileContents = readFileContents(indexPath);
				std::string fileContents = readFileContents(indexPath);
				if (fileContents.empty())
				{
					LOG_CRITICAL("Failed to read file contents: {}", indexPath.string());
					return generateForbiddenResponse();
				}
				std::string response = buildHttpResponse(indexPath, fileContents, HTTPStatusCode::OK);
				return response;
			}
			else
			{
				LOG_DEBUG("index.html does not exist");
				//? send 404 response
				return generateNotFoundResponse();
			}

		}
		else if (std::filesystem::is_regular_file(uri))
		{
			LOG_DEBUG("Requested path is a file");

			return generateFileResponse(uri);
		}
		else
		{
			//TODO: send 405 response???
			LOG_ERROR("Requested path is not a file or directory");

			return generateNotFoundResponse();
		}
	}
	LOG_DEBUG("Requested path does not exist");
	return generateNotFoundResponse();
}

std::string ResponseGenerator::generateOKResponse(const std::filesystem::path& path)
{
	return "";
}

std::string ResponseGenerator::generateForbiddenResponse()
{
	LOG_INFO("Generating 403 Forbidden response");

	const char* root = std::getenv("HTML_ROOT_DIR");
	if (root == nullptr)
	{
		WEB_ASSERT(false, "HTML_ROOT_DIR environment variable not set!");
		return nullptr;
	}

	std::filesystem::path value(root);

	std::string fileContents = readFileContents(std::filesystem::path(value / "403-Forbidden.html"));
	if (fileContents.empty())
	{
		LOG_CRITICAL("Failed to read file contents: {}",  "403-Forbidden.html");
		return std::string(s_ForbiddenResponse);
	}
	return buildHttpResponse(ContentType::HTML, fileContents, HTTPStatusCode::Forbidden);
}

std::string ResponseGenerator::generateFileResponse(const std::filesystem::path& path)
{
	std::string fileContents = ReadImageFile(path);
	if (fileContents.empty())
	{
		LOG_CRITICAL("Failed to read file contents: {}", path.string());
		// return generateForbiddenResponse(path);
		return generateForbiddenResponse();
	}
	std::string response = buildHttpResponse(path, fileContents, HTTPStatusCode::OK);

	// std::string response = buildHttpResponse(path, "");
	return response;
}










std::string ResponseGenerator::generateNotFoundResponse()
{
	LOG_INFO("Generating 404 Not Found response");

	const char* root = std::getenv("HTML_ROOT_DIR");
	if (root == nullptr)
	{
		WEB_ASSERT(false, "HTML_ROOT_DIR environment variable not set!");
		return nullptr;
	}

	std::filesystem::path value(root);

	std::string fileContents = readFileContents(std::filesystem::path(value / "404-NotFound.html"));
	if (fileContents.empty())
	{
		LOG_CRITICAL("Failed to read file contents: {}",  "404-NotFound.html");
		return std::string(s_NotFoundResponse);
	}
	return buildHttpResponse(ContentType::HTML, fileContents, HTTPStatusCode::NotFound);
}