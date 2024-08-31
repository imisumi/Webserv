#include "RequestHandler.h"

#include "ResponseGenerator.h"

#include "Core/Log.h"

#include <filesystem>
#include <fstream>


//TODO: move to ResponseSender
#include <sys/socket.h>


#include <functional>  // For std::hash

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


const std::string RequestHandler::handleRequest(const std::string& request, int epollFd)
{
	m_EpollFd = epollFd;
	parseRequest(request);

	Config config = Config::CreateDefaultConfig();
	// return "";

	return ResponseGenerator::generateResponse(config, m_RequestParser.getRequest());

	switch (m_RequestMethod)
	{
	case RequestMethod::GET:			return handleGetRequest();		// break;
	case RequestMethod::POST:			handlePostRequest();	break;
	case RequestMethod::PUT:			handlePutRequest();		break;
	case RequestMethod::DELETE:			handleDeleteRequest();	break;
	case RequestMethod::HEAD:			handleHeadRequest();	break;
	case RequestMethod::OPTIONS:		handleOptionsRequest();	break;
	default:												break;
	}
	return "";
}

static RequestMethod GetRequestMethod(const std::string& request)
{
	if (request.find("GET") != std::string::npos)
		return RequestMethod::GET;
	else if (request.find("POST") != std::string::npos)
		return RequestMethod::POST;
	else if (request.find("PUT") != std::string::npos)
		return RequestMethod::PUT;
	else if (request.find("PATCH") != std::string::npos)
		return RequestMethod::PATCH;
	else if (request.find("DELETE") != std::string::npos)
		return RequestMethod::DELETE;
	else if (request.find("HEAD") != std::string::npos)
		return RequestMethod::HEAD;
	else if (request.find("OPTIONS") != std::string::npos)
		return RequestMethod::OPTIONS;

	return RequestMethod::UNKNOWN;
}

#define GREEN "\033[32m"
#define WHITE "\033[37m"
#define RESET "\033[0m"


static std::string RequestMethodToString(RequestMethod type)
{
	switch (type)
	{
	case RequestMethod::GET:			return "GET";
	case RequestMethod::POST:			return "POST";
	case RequestMethod::PUT:			return "PUT";
	case RequestMethod::PATCH:		return "PATCH";
	case RequestMethod::DELETE:		return "DELETE";
	case RequestMethod::HEAD:			return "HEAD";
	case RequestMethod::OPTIONS:		return "OPTIONS";
	default:						return "UNKNOWN";
	}
}

//temp
const char* RequestMethodToCString(RequestMethod type)
{
    switch (type)
    {
    case RequestMethod::GET:        return "GET";
    case RequestMethod::POST:       return "POST";
    case RequestMethod::PUT:        return "PUT";
    case RequestMethod::PATCH:      return "PATCH";
    case RequestMethod::DELETE:     return "DELETE";
    case RequestMethod::HEAD:       return "HEAD";
    case RequestMethod::OPTIONS:    return "OPTIONS";
    default:                        return "UNKNOWN";
    }
}

void RequestHandler::parseRequest(const std::string& request)
{
	//TODO: fix
	// size_t methodEnd = request.find(' ');
	// if (methodEnd != std::string::npos)
	// {
	// 	m_RequestMethod = GetRequestMethod(request.substr(0, methodEnd));
	// 	LOG_TRACE("Request type: {}", RequestMethodToString(m_RequestMethod));
	// 	size_t pathEnd = request.find(' ', methodEnd + 1);
	// 	if (pathEnd != std::string::npos)
	// 	{
	// 		m_RequestPath = request.substr(methodEnd + 1, pathEnd - methodEnd - 1);
	// 		LOG_TRACE("Request path: {}", m_RequestPath);

	// 		size_t protocalEnd = request.find('\r', pathEnd + 1);
	// 		if (protocalEnd != std::string::npos)
	// 		{
	// 			m_ProtocalVersion = request.substr(pathEnd + 1, protocalEnd - pathEnd - 1);
	// 			LOG_TRACE("Protocal version: {}", m_ProtocalVersion);
	// 		}
	// 	}
	// }
	// HttpRequestParser m_RequestParser;
	m_RequestParser.reset();
	if (m_RequestParser.parse(request))
	{
		const HttpRequest& req = m_RequestParser.getRequest();
		// std::cout << GREEN << "Method: " << WHITE << req.method << RESET << std::endl;
		std::cout << GREEN << "Method: " << WHITE << RequestMethodToCString(req.method) << RESET << std::endl;
		// std::cout << GREEN << "URI: " << WHITE << req.uri << RESET << std::endl;
		std::cout << GREEN << "URI: " << WHITE << req.uri.string() << RESET << std::endl;
		std::cout << GREEN << "Version: " << WHITE << req.version << RESET << std::endl;
		for (const auto& header : req.headers) {
			std::cout << GREEN << header.first << ": " << WHITE << header.second << RESET << std::endl;
		}
		std::cout << GREEN << "Body: " << WHITE << req.body << RESET << std::endl;

		// m_RequestMethod = req.method == "GET" ? RequestMethod::GET : RequestMethod::UNKNOWN;
		m_RequestMethod = req.method;
		m_RequestPath = req.uri;
		m_ProtocalVersion = req.version;
	}
	else
	{
		std::cerr << "Failed to parse request" << std::endl;
	}


}

static std::string readFileContents(const std::filesystem::path& path)
{
	std::ifstream file(path);
	if (!file.is_open()) {
		throw std::runtime_error("Could not open file: " + path.string());
	}

	std::stringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}

// Function to format the date-time string with time zone information
std::string getCurrentDateAndTime()
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

std::string getFileModificationTime(const std::filesystem::path& filePath)
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

std::string generateETag(const std::string& content)
{
	// Use std::hash to create a hash value for the content
	std::hash<std::string> hasher;
	size_t hash = hasher(content);

	// Convert the hash to a hexadecimal string
	std::ostringstream ss;
	ss << std::hex << hash;
	return ss.str();
}

std::string buildHttpResponse(const std::filesystem::path& path, const std::string& body)
{
	std::ostringstream response;

	response << "HTTP/1.1 200 OK\r\n"
			 << "Server: Webserv/1.0\r\n"
			 << "Date: " << getCurrentDateAndTime() << "\r\n"
			 << "Content-Type: text/html\r\n"
			 << "Content-Length: " << body.size() << "\r\n"
			 << "Last-Modified: " << getFileModificationTime(path) << "\r\n"
			 //TODO: hard coded values should check this
			 << "Connection: keep-alive\r\n"
			 << "ETag: \"" << generateETag(body) << "\"\r\n"
			 << "Accept-Ranges: bytes\r\n"
			 // Disable caching
			 << "Cache-Control: no-store, no-cache, must-revalidate, proxy-revalidate, max-age=0\r\n"
			 << "Pragma: no-cache\r\n"
			 << "Expires: 0\r\n"
			 << "\r\n";  // End of headers

	response << body;

	return response.str();
}

#include <sys/socket.h>


const std::string RequestHandler::handleGetRequest()
{
	LOG_DEBUG("Handling GET request");


	Config config = Config::CreateDefaultConfig();
	return ResponseGenerator::generateResponse(config, m_RequestParser.getRequest());


	// const HttpRequest& req = m_RequestParser.getRequest();
	// LOG_DEBUG("Requested URI: {}", req.uri.string());
	// const std::filesystem::path root("/home/imisumi-wsl/dev/Webserv/root/html");
	// std::filesystem::path uri = root / std::filesystem::relative(req.uri, "/");


	// LOG_DEBUG("Requested path: {}", uri.string());

	// if (std::filesystem::exists(uri))
	// {
	// 	if (std::filesystem::is_directory(uri))
	// 	{
	// 		LOG_DEBUG("Requested path is a directory");
			

	// 		//? check if index.html exists
	// 		//TODO: could be overwriten by the user in the config file
	// 		std::filesystem::path indexPath = uri / "index.html";
	// 		if (std::filesystem::exists(indexPath))
	// 		{
	// 			LOG_DEBUG("index.html exists");
	// 			uri = indexPath;

	// 			// return ResponseGenerator::generateResponse(ResponseType::OK, readFileContents(uri));
	// 			std::string fileContents = readFileContents(uri);

	// 			std::string response = buildHttpResponse(uri, fileContents);
	// 			return response;
	// 		}
	// 		else
	// 		{
	// 			LOG_DEBUG("index.html does not exist");
	// 			//? send 404 response
	// 			return ResponseGenerator::generateResponse(ResponseType::NotFound);
	// 		}
	// 	}
	// 	else if (std::filesystem::is_regular_file(uri))
	// 	{
	// 		LOG_DEBUG("Requested path is a file");
	// 	}
	// 	else
	// 	{
	// 		LOG_ERROR("Requested path is not a file or directory");
	// 	}
	// }
	// else
	// {
	// 	LOG_DEBUG("Requested path does not exist");
	// 	//? send 404 response
	// 	return ResponseGenerator::generateResponse(ResponseType::NotFound);
	// }





	// std::filesystem::path path("root/html/index.html");
	// std::string fileContents = readFileContents(path);

	// std::string response = buildHttpResponse(path, fileContents);

	// //TODO: move to ResponseSender
	// // return send(m_EpollFd, response.c_str(), response.size(), 0);
	// // return response;
	// return "HTTP/1.1 404 Not Found\r\n"
	// 			"Content-Type: text/plain\r\n"
	// 			"Content-Length: 0\r\n"
	// 			"\r\n";
}

void RequestHandler::handlePostRequest()
{
	//TODO: implement
}

//? Not needed according to the subject
void RequestHandler::handlePutRequest()
{
	constexpr const char* not_found_response = 
		"HTTP/1.1 403 Forbidden\r\n"
		"Content-Length: 0\r\n"
		"Connection: close\r\n"
		"\r\n";

	constexpr std::size_t response_length = []() {
		std::size_t length = 0;
		while (not_found_response[length] != '\0') ++length;
		return length;
	}();

	send(m_EpollFd, not_found_response, response_length, 0);
}

void RequestHandler::handleDeleteRequest()
{
	//TODO: implement
}

//? Not needed according to the subject
void RequestHandler::handleHeadRequest()
{
	const char not_found_response[] = 
		"HTTP/1.1 403 Forbidden\r\n"
		"Content-Length: 0\r\n"
		"Connection: close\r\n"
		"\r\n";

	send(m_EpollFd, not_found_response, sizeof(not_found_response) - 1, 0);
}

//? Not needed according to the subject
void RequestHandler::handleOptionsRequest()
{
	const char not_found_response[] = 
		"HTTP/1.1 403 Forbidden\r\n"
		"Content-Length: 0\r\n"
		"Connection: close\r\n"
		"\r\n";

	send(m_EpollFd, not_found_response, sizeof(not_found_response) - 1, 0);
}
