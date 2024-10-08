#pragma once

#include <string>

#include "Client.h"

// TODO: if this is all and no further functionality is added, this class can be removed and the function can be moved
// to Server.cpp
class ResponseSender
{
public:
	ResponseSender() {};
	~ResponseSender() {};

	static ssize_t sendResponse(const std::string& response, int epollFd);
	static ssize_t sendResponse(Client& client);

private:
};