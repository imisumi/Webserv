#include "ResponseGenerator.h"

#include "Core/Core.h"


#include <sys/stat.h>


#include "Cgi/Cgi.h"


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

static const char s_InternalServerErrorResponse[] = 
		"HTTP/1.1 500 Internal Server Error\r\n"
		"Content-Length: 39\r\n" //? length has to match the length of the body
		"Connection: close\r\n"
		"Content-Type: text/plain\r\n"
		"\r\n"
		"500 Internal Server Error: Server Error"; //? body of the response

static const char s_MethodNotAllowedResponse[] =
		"HTTP/1.1 405 Method Not Allowed\r\n"
		"Content-Length: 42\r\n" //? length has to match the length of the body
		"Connection: close\r\n"
		"Content-Type: text/plain\r\n"
		"\r\n"
		"405 Method Not Allowed: Method Not Allowed"; //? body of the response


//? Multipurpose Internet Mail Extensions (MIME) type
static const std::unordered_map<std::string, std::string> s_SupportedMineTypes = {
			{ ".html", "text/html" },
			{ ".css", "text/css" },
			{ ".ico", "image/x-icon" },
			{ ".jpg", "image/jpeg" },
			{ ".jpeg", "image/jpeg" },
			{ ".png", "image/png" }
		};

const std::string ResponseGenerator::generateResponse(const Client& client, const HttpRequest& request)
{
	//TODO: validate request
	//? fo now only GET is supported


	switch (request.method)
	{
		case RequestMethod::GET:			return handleGetRequest(client, request);
		case RequestMethod::POST:			return handlePostRequest(client, request);
		case RequestMethod::PUT:			break;
		case RequestMethod::PATCH:			return generateInternalServerErrorResponse(); //TODO: also temp
		case RequestMethod::DELETE:			return handleDeleteRequest(client, request);
		case RequestMethod::HEAD:			break ;
		case RequestMethod::OPTIONS:		return generateBadRequestResponse(); //TODO: this is just for testing, bad request in incase of a invalid request
		default:							break;
	}

	// return generateNotFoundResponse();
	return generateNotImplementedResponse();
}


//? assume that the file exists
//? return the contents of the file or nullopt if the file cannot be opened due to permissions
std::optional<std::string> ResponseGenerator::readFileContents(const std::filesystem::path& path)
{
	LOG_DEBUG("Reading file contents: {}", path.string());
	std::ifstream file(path);
	if (!file.is_open())
	{
		LOG_ERROR("Failed to open file: {}", path.string());
		return std::nullopt;
	}

	std::stringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}

std::string ResponseGenerator::determineContentType(const std::filesystem::path& file)
{
	// log file
	// LOG_INFO("File: {}", file.string());
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

static std::string getFileModificationTime(const std::filesystem::path& filePath)
{
	//? Since this is the body of the response, we assume the file exists
	// Get the last write time of the file
	auto ftime = std::filesystem::last_write_time(filePath);

	// Convert file time to system clock time
	auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
		ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now()
	);

	// Convert to time_t for easy formatting
	std::time_t time = std::chrono::system_clock::to_time_t(sctp);

	// Format the time as a string
	std::ostringstream oss;
	oss << std::put_time(std::localtime(&time), "%a, %d %b %Y %H:%M:%S GMT");

	return oss.str();
}

static std::string generateETag(const std::string& content)
{
	// Use std::hash to create a hash value for the content
	std::hash<std::string> hasher;
	size_t hash = hasher(content);

	// Convert the hash to a hexadecimal string
	std::ostringstream ss;
	ss << std::hex << hash;
	return ss.str();
}

static std::string generateETagv2(const std::filesystem::path& path)
{
	struct stat fileStat;

	if (stat(path.c_str(), &fileStat) != 0)
	{
		LOG_ERROR("Failed to get file stats: {}", path.string());
		return std::string();
	}

	auto inode = fileStat.st_ino;
	auto fileSize = fileStat.st_size;
	auto fileTime = fileStat.st_mtime;
	auto permissions = fileStat.st_mode; //? unconventional but prevents a check for has read permissions before returning 304

	// Combine these into the ETag
	std::ostringstream ss;
	ss << std::hex << inode << "-" << fileSize << "-" << fileTime << "-" << permissions;
	// return ss.str();
	return "\"" + ss.str() + "\"";
}

std::string ResponseGenerator::buildHttpResponse(const std::string& body, HTTPStatusCode code, const HttpRequest& request)
{
	std::ostringstream response;

	std::string contentType = determineContentType(request.getUri());
	if (contentType.empty())
	{
		return generateForbiddenResponse();
	}

	const std::string statusCode = HTTPStatusCodeToString(code);

	WEB_ASSERT(!statusCode.empty(), "Invalid HTTP status code! (add a custom code or use a valid one)");

	std::string connection = request.getHeader("Connection");
	if (connection.empty())
	{
		// connection = "keep-alive";
		connection = "close";
	}
	// connection = "keep-alive";


	// response << "HTTP/1.1 200 OK\r\n"
	response << "HTTP/1.1 " << statusCode << "\r\n"
			<< "Server: Webserv/1.0\r\n"
			<< "Date: " << getCurrentDateAndTime() << "\r\n"
			<< "Content-Type: " << contentType << "\r\n"
			<< "Content-Length: " << body.size() << "\r\n"
			<< "Last-Modified: " << getFileModificationTime(request.getUri()) << "\r\n"
			//TODO: hard coded values should check this
			// << "Connection: keep-alive\r\n"
			<< "Connection: " << connection << "\r\n"
			// << "Connection: close\r\n"
			// << "ETag: \"" << generateETag(body) << "\"\r\n"
			<< "ETag: " << generateETagv2(request.getUri()) << "\r\n"
			<< "Accept-Ranges: bytes\r\n"
			// << "Cache-Control: max-age=3600\r\n"  // Cache for 1 hour
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
			// << "Last-Modified: " << getFileModifigit push --set-upstream origin merged-postandmaincationTime(path) << "\r\n"
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

#include "Utils/Utils.h"

const std::string ResponseGenerator::handleGetRequest(const Client& client, const HttpRequest& request)
{
	Utils::Timer timer;
	LOG_INFO("Handling GET request");


	// very temp CGI
	// std::string uir = request.getUri().string();
	// if (uir.find("/cgi-bin/") != std::string::npos)
	// {
	// 	LOG_DEBUG("Requested path is a CGI script");

	// 	std::filesystem::path fileName = request.getUri().filename();
	// 	HttpRequest updatedRequest = request;
	// 	std::filesystem::path cgiPath = getenv("CGI_ROOT_DIR");
	// 	updatedRequest.setUri(cgiPath / fileName);
	// 	LOG_INFO("CGI path: {}", updatedRequest.getUri().string());
	// 	Cgi::executeCGI(client, config, updatedRequest);
	// 	return "";
	// }

	//? Validate the requested path
	if (std::filesystem::exists(request.getUri()))
	{
		//? Check if the requested path is a directory or a file
		if (std::filesystem::is_directory(request.getUri()))
		{
			LOG_DEBUG("Requested path is a directory");

			//TODO: see if config maps to the given uri, if so see if index is overwriten esle use default
			HttpRequest updatedRequest = request;
			// std::filesystem::path index = client.GetConfig().GetIndex(updatedRequest.getUri());
			LOG_INFO("Index: {}", client.GetConfig()->GetIndex(request.getOriginalUri()).string());
			std::filesystem::path index = request.getUri() / client.GetConfig()->GetIndex(request.getOriginalUri());
			LOG_INFO("Index: {}", index.string());
			LOG_INFO("Index: {}", request.getUri().string());
			// updatedRequest.setUri(updatedRequest.getUri() / "index.html");

			// const std::filesystem::path& uri = request.getUri();
			updatedRequest.setUri(updatedRequest.getUri() / index);
			if (!std::filesystem::exists(updatedRequest.getUri()))
			{
				//? if autoindex is enabled, generate a list of files in the directory

				// char *dirListing = "/home/imisumi/Desktop/Webserv/root/webserv/cgi-bin/directory_listing.py";
				// char *argv[] = { dirListing, (char*)updatedRequest.getUri().c_str(), NULL };


				// const char *dirListing = "/home/imisumi/Desktop/Webserv/root/webserv/cgi-bin/directory_listing.py";
				// char *argv[] = { (char*)dirListing, (char*)request.getUri().c_str(), NULL };
		
				const char *scriptPath = "/home/kaltevog/Desktop/Webserv/root/webserv/cgi-bin/databasedelete.py";
				std::string arg = request.getUri().string();
				const char *arg1 = arg.c_str();
				// Prepare the argv array. First element is the script itself.
				char *argv[] = { (char*)scriptPath, (char*)arg1, NULL };

				// Cgi::RunCgi(client, argv);
				return Cgi::RunCgi(client, argv);


				LOG_DEBUG("index.html does not exist");
				//? send 404 response
				return generateNotFoundResponse();
			}

			LOG_DEBUG("index.html exists");
			//? Check source has been modified
			if (updatedRequest.getHeader("If-None-Match") == generateETagv2(updatedRequest.getUri()) && updatedRequest.getHeader("If-Modified-Since") == getFileModificationTime(updatedRequest.getUri()))
			{
				return generateNotModifiedResponse();
			}
			return generateOKResponse(updatedRequest);
		}
		else if (std::filesystem::is_regular_file(request.getUri()))
		{
			//TODO: check if file is a CGI script




			


			LOG_DEBUG("Requested path is a file");

			if (request.getHeader("If-None-Match") == generateETagv2(request.getUri()) && request.getHeader("If-Modified-Since") == getFileModificationTime(request.getUri()))
			{
				return generateNotModifiedResponse();
			}

			return generateFileResponse(request);
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


#include <regex>

bool isBoundaryString(const std::string &value) {
    return value.find("-----------------------------") != std::string::npos;
}

std::string extractBoundary(const std::string &contentType) {
    std::regex boundaryRegex("boundary=(.*)");
    std::smatch match;
    if (std::regex_search(contentType, match, boundaryRegex)) {
        return "--" + match[1].str();
    }
    return "";
}

std::string parseMultipartData(const std::string &body, const std::string &boundary, const std::string &fieldName) {
    std::regex fieldRegex(boundary + R"([\r\n]+Content-Disposition: form-data; name=\")" + fieldName + R"(\"[\r\n]+[\r\n]+([^\r\n]*)[\r\n]+)");
    std::smatch match;
    if (std::regex_search(body, match, fieldRegex)) {
        return match[1].str();
    }
    return "";
}

bool saveFormDataToFile(const std::string &firstName, std::string lastName, std::string email) {
    if (isBoundaryString(lastName)) {
        lastName = "EMPTY";
    }
    if (isBoundaryString(email)) {
        email = "EMPTY";
    }

    std::ofstream file("/home/kaltevog/Desktop/Webserv/database/text.txt", std::ios_base::app);
    if (!file.is_open()) {
        std::cerr << "[ERROR] Failed to open file: /home/kaltevog/Desktop/Webserv/database/text.txt" << std::endl;
        return false;
    }

    file << "First Name: " << firstName << "\n";
    file << "Last Name: " << (lastName.empty() ? "EMPTY" : lastName) << "\n";
    file << "Email: " << (email.empty() ? "EMPTY" : email) << "\n\n";

    file.close();
    return !file.fail();
}

bool saveUploadedFile(const std::string &body, const std::string &boundary, const std::string &fieldName, const std::string &uploadDir) {
    std::regex fileRegex(boundary + R"([\r\n]+Content-Disposition: form-data; name=\")" + fieldName + R"(\"; filename=\"([^\"]+)\")" +
                         R"([\r\n]+Content-Type: ([^\r\n]+)[\r\n]+[\r\n]+([\s\S]+?)[\r\n]+)" + boundary);
    std::smatch match;
    if (std::regex_search(body, match, fileRegex)) {
        std::string fileName = match[1].str();
        std::string fileContent = match[3].str();

        std::string filePath = uploadDir + "/" + fileName;
        if (fileName.empty()) {
            std::cerr << "[INFO] No file uploaded." << std::endl;
            return true;
        }

        std::ofstream file(filePath, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "[ERROR] Failed to open file: " << filePath << std::endl;
            return false;
        }

        file.write(fileContent.c_str(), fileContent.size());
        file.close();
        return !file.fail();
    }

    return true;
}

const std::string ResponseGenerator::handlePostRequest(const Client& client, const HttpRequest& request) {
    Utils::Timer timer;
    LOG_INFO("Handling POST request");

    std::string contentType = request.getHeader("Content-Type");
    if (contentType.empty()) {
        LOG_ERROR("Missing Content-Type header");
        return generateBadRequestResponse();
    }

    std::string boundary = extractBoundary(contentType);
    if (boundary.empty()) {
        LOG_ERROR("Boundary missing in Content-Type header");
        return generateBadRequestResponse();
    }

    std::string body = request.getBody();
    if (body.empty()) {
        LOG_ERROR("Request body is empty");
        return generateBadRequestResponse();
    }

    std::string firstName = parseMultipartData(body, boundary, "firstname");
    if (firstName.empty()) {
        LOG_ERROR("First name is missing in the form data");
        return generateBadRequestResponse();
    }

    std::string lastName = parseMultipartData(body, boundary, "lastname");
    std::string email = parseMultipartData(body, boundary, "email");

    if (!saveFormDataToFile(firstName, lastName, email)) {
        LOG_ERROR("Failed to save form data to file");
        return generateInternalServerErrorResponse();
    }

	//hardcoded
    if (!saveUploadedFile(body, boundary, "file", "/home/kaltevog/Desktop/Webserv/database")) {
        LOG_ERROR("Failed to save uploaded file");
        return generateInternalServerErrorResponse();
    }

    LOG_INFO("Successfully handled POST request, saved form data, and saved file");
    return generateOKResponse(request);
}

const std::string ResponseGenerator::handleDeleteRequest(const Client& client, const HttpRequest& request) 
{
    Utils::Timer timer;
    LOG_INFO("Handling DELETE request");

    // Get the requested URI
    std::filesystem::path uri = request.getUri();

    // Build the full path to the /database directory
    std::filesystem::path basePath = "/home/kaltevog/Desktop/Webserv/database";
    std::filesystem::path fullPath = basePath / uri.filename(); // Only delete file in /database, using the filename part of the URI

    LOG_INFO("Attempting to delete file: {}", fullPath.string());

    // Check if the requested file exists
    if (std::filesystem::exists(fullPath) && std::filesystem::is_regular_file(fullPath))
    {
        // Try deleting the file
        try {
            std::filesystem::remove(fullPath);
            LOG_INFO("File deleted successfully: {}", fullPath.string());

            // Return success response
            return buildHttpResponse(ContentType::TEXT, "File deleted successfully", HTTPStatusCode::OK);
        } catch (const std::filesystem::filesystem_error& e) {
            LOG_ERROR("Failed to delete file: {}", e.what());
            return generateInternalServerErrorResponse();
        }
    }
    else
    {
        LOG_ERROR("Requested file does not exist or is not a regular file: {}", fullPath.string());
        return generateNotFoundResponse();
    }
}



//TODO: instead of sending path maybe update the path in the HrrpRequest
// std::string ResponseGenerator::generateOKResponse(const std::filesystem::path& path, const HttpRequest& request)
std::string ResponseGenerator::generateOKResponse(const HttpRequest& request)
{
	LOG_INFO("Generating 200 OK response");

	auto fileContents = readFileContents(request.getUri());
	if (fileContents == std::nullopt)
	{
		LOG_CRITICAL("Failed to read file contents: {}", request.getUri().string());
		return generateForbiddenResponse();
	}
	// return buildHttpResponse(request.getUri(), *fileContents, HTTPStatusCode::OK, request);
	return buildHttpResponse(*fileContents, HTTPStatusCode::OK, request);

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

	auto fileContents = readFileContents(std::filesystem::path(value / "403-Forbidden.html"));
	if (fileContents == std::nullopt)
	{
		LOG_CRITICAL("Failed to read file contents: {}",  "403-Forbidden.html");
		return std::string(s_ForbiddenResponse);
	}
	return buildHttpResponse(ContentType::HTML, *fileContents, HTTPStatusCode::Forbidden);
}

std::string ResponseGenerator::generateFileResponse(const HttpRequest& request)
{
	LOG_TRACE("Generating file response");
	LOG_TRACE("Request URI: {}", request.getUri().string());
	std::string fileContents = ReadImageFile(request.getUri());
	if (fileContents.empty())
	{
		LOG_CRITICAL("Failed to read file contents: {}", request.getUri().string());
		// return generateForbiddenResponse(path);
		return generateForbiddenResponse();
	}
	// return buildHttpResponse(request.getUri(), fileContents, HTTPStatusCode::OK, request);
	return buildHttpResponse(fileContents, HTTPStatusCode::OK, request);
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

	auto fileContents = readFileContents(std::filesystem::path(value / "404-NotFound.html"));
	if (fileContents == std::nullopt)
	{
		LOG_CRITICAL("Failed to read file contents: {}",  "404-NotFound.html");
		return std::string(s_NotFoundResponse);
	}
	return buildHttpResponse(ContentType::HTML, *fileContents, HTTPStatusCode::NotFound);
}


std::string ResponseGenerator::generateNotModifiedResponse()
{
	LOG_INFO("Generating 304 Not Modified response");

	std::ostringstream res;

	res << "HTTP/1.1 304 Not Modified\r\n";
	res << "Server: Webserv/1.0\r\n";
	res << "Date: " << getCurrentDateAndTime() << "\r\n";
	res << "Connection: keep-alive\r\n";
	res << "\r\n";  // End of headers

	return res.str();
}

std::string ResponseGenerator::generateNotImplementedResponse()
{
	LOG_INFO("Generating 404 Not Found response");

	const char* root = std::getenv("HTML_ROOT_DIR");
	if (root == nullptr)
	{
		WEB_ASSERT(false, "HTML_ROOT_DIR environment variable not set!");
		return nullptr;
	}

	std::filesystem::path value(root);

	auto fileContents = readFileContents(std::filesystem::path(value / "501-NotImplemented.html"));
	if (fileContents == std::nullopt)
	{
		LOG_CRITICAL("Failed to read file contents: {}",  "501-NotImplemented.html");
		return std::string(s_NotFoundResponse);
	}
	return buildHttpResponse(ContentType::HTML, *fileContents, HTTPStatusCode::NotFound);
}

std::string ResponseGenerator::generateBadRequestResponse()
{
	LOG_INFO("Generating 400 Bad Request response");

	const char* root = std::getenv("HTML_ROOT_DIR");
	if (root == nullptr)
	{
		WEB_ASSERT(false, "HTML_ROOT_DIR environment variable not set!");
		return nullptr;
	}

	std::filesystem::path value(root);

	auto fileContents = readFileContents(std::filesystem::path(value / "400-BadRequest.html"));
	if (fileContents == std::nullopt)
	{
		LOG_CRITICAL("Failed to read file contents: {}",  "400-BadRequest.html");
		return std::string(s_NotFoundResponse);
	}
	return buildHttpResponse(ContentType::HTML, *fileContents, HTTPStatusCode::NotFound);
}

std::string ResponseGenerator::generateInternalServerErrorResponse()
{
	LOG_INFO("Generating 500 Internal Server Error response");

	const char* root = std::getenv("HTML_ROOT_DIR");
	if (root == nullptr)
	{
		WEB_ASSERT(false, "HTML_ROOT_DIR environment variable not set!");
		return nullptr;
	}

	std::filesystem::path value(root);

	auto fileContents = readFileContents(std::filesystem::path(value / "500-InternalServerError.html"));
	if (fileContents == std::nullopt)
	{
		LOG_CRITICAL("Failed to read file contents: {}",  "500-InternalServerError.html");
		return std::string(s_NotFoundResponse);
	}
	return buildHttpResponse(ContentType::HTML, *fileContents, HTTPStatusCode::NotFound);
}













const std::string ResponseGenerator::InternalServerError(const Config& config)
{
	static std::string response = s_InternalServerErrorResponse;

	return response;
}

const std::string ResponseGenerator::InternalServerError()
{
	static std::string response = s_InternalServerErrorResponse;

	return response;
}

const std::string ResponseGenerator::MethodNotAllowed()
{
	static std::string response = s_MethodNotAllowedResponse;

	return response;
}