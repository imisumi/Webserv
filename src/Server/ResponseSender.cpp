#include "ResponseSender.h"




#include <sys/socket.h>

#include "Core/Log.h"


ssize_t ResponseSender::sendResponse(const std::string& response, int epollFd)
{
	ssize_t bytes = send(epollFd, response.c_str(), response.size(), 0);
	if (bytes == -1)
	{
		LOG_ERROR("send: {}", strerror(errno));
	}
	return bytes;
}