#include "RequestHandler.h"

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


int RequestHandler::handleRequest(const std::string& request, int epollFd)
{
	m_EpollFd = epollFd;
	parseRequest(request);

	switch (m_RequestType)
	{
	case RequestType::GET:			return handleGetRequest();		// break;
	case RequestType::POST:			handlePostRequest();	break;
	case RequestType::PUT:			handlePutRequest();		break;
	case RequestType::DELETE:		handleDeleteRequest();	break;
	case RequestType::HEAD:			handleHeadRequest();	break;
	case RequestType::OPTIONS:		handleOptionsRequest();	break;
	default:												break;
	}
	return 0;
}

static RequestType GetRequestType(const std::string& request)
{
	if (request.find("GET") != std::string::npos)
		return RequestType::GET;
	else if (request.find("POST") != std::string::npos)
		return RequestType::POST;
	else if (request.find("PUT") != std::string::npos)
		return RequestType::PUT;
	else if (request.find("PATCH") != std::string::npos)
		return RequestType::PATCH;
	else if (request.find("DELETE") != std::string::npos)
		return RequestType::DELETE;
	else if (request.find("HEAD") != std::string::npos)
		return RequestType::HEAD;
	else if (request.find("OPTIONS") != std::string::npos)
		return RequestType::OPTIONS;

	return RequestType::UNKNOWN;
}

static std::string RequestTypeToString(RequestType type)
{
	switch (type)
	{
	case RequestType::GET:			return "GET";
	case RequestType::POST:			return "POST";
	case RequestType::PUT:			return "PUT";
	case RequestType::PATCH:		return "PATCH";
	case RequestType::DELETE:		return "DELETE";
	case RequestType::HEAD:			return "HEAD";
	case RequestType::OPTIONS:		return "OPTIONS";
	default:						return "UNKNOWN";
	}
}

void RequestHandler::parseRequest(const std::string& request)
{
	//TODO: fix
	size_t methodEnd = request.find(' ');
	if (methodEnd != std::string::npos)
	{
		m_RequestType = GetRequestType(request.substr(0, methodEnd));
		LOG_TRACE("Request type: {}", RequestTypeToString(m_RequestType));
		size_t pathEnd = request.find(' ', methodEnd + 1);
		if (pathEnd != std::string::npos)
		{
			m_RequestPath = request.substr(methodEnd + 1, pathEnd - methodEnd - 1);
			LOG_TRACE("Request path: {}", m_RequestPath);

			size_t protocalEnd = request.find('\r', pathEnd + 1);
			if (protocalEnd != std::string::npos)
			{
				m_ProtocalVersion = request.substr(pathEnd + 1, protocalEnd - pathEnd - 1);
				LOG_TRACE("Protocal version: {}", m_ProtocalVersion);
			}
		}
	}
}

static std::string readFileIntoBuffer(const std::filesystem::path& path)
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


int RequestHandler::handleGetRequest()
{
	LOG_DEBUG("Handling GET request");

	std::filesystem::path path("root/html/index.html");
	std::string fileContents = readFileIntoBuffer(path);

	std::string response = buildHttpResponse(path, fileContents);

	//TODO: move to ResponseSender
	return send(m_EpollFd, response.c_str(), response.size(), 0);
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
