#pragma once

#include <string>

class ResponseSender
{
public:
	ResponseSender() {};
	~ResponseSender() {};

	// void sendResponse(const std::string& response);
	void sendResponse(const std::string& response, int epollFd);
	// void sendResponse(const std::string& response, int epollFd, int fd);
	// void sendResponse(const std::string& response, int epollFd, int fd, int status);
	// void sendResponse(const std::string& response, int epollFd, int fd, int status, const std::string& contentType);
	// void sendResponse(const std::string& response, int epollFd, int fd, int status, const std::string& contentType, const std::string& contentLength);
	// void sendResponse(const std::string& response, int epollFd, int fd, int status, const std::string& contentType, const std::string& contentLength, const std::string& connection);
	// void sendResponse(const std::string& response, int epollFd, int fd, int status, const std::string& contentType, const std::string& contentLength, const std::string& connection, const std::string& location);
	// void sendResponse(const std::string& response, int epollFd, int fd, int status, const std::string& contentType, const std::string& contentLength, const std::string& connection, const std::string& location, const std::string& server);
	// void sendResponse(const std::string& response, int epollFd, int fd, int status, const std::string& contentType, const std::string& contentLength, const std::string& connection, const std::string& location, const std::string& server, const std::string& date);
	// void sendResponse(const std::string& response, int epollFd, int fd, int status, const std::string& contentType, const std::string& contentLength, const std::string& connection, const std::string& location, const std::string& server, const std::string& date, const std::string& lastModified);
	// void sendResponse(const std::string& response, int epollFd, int fd, int status, const std::string& contentType, const std::string& contentLength, const std::string& connection, const std::string& location, const std::string& server, const std::string&

private:
};