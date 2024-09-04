#pragma once

#include <string>


//TODO: if this is all and no further functionality is added, this class can be removed and the function can be moved to Server.cpp
class ResponseSender
{
public:
	ResponseSender() {};
	~ResponseSender() {};

	ssize_t sendResponse(const std::string& response, int epollFd);

private:
};