#include "ResponseSender.h"




#include <sys/socket.h>

#include "Core/Log.h"


ssize_t ResponseSender::sendResponse(const std::string& response, int epollFd)
{
	ssize_t bytesToSend = response.size();
	ssize_t byteOffset = 0;

	LOG_DEBUG("Sending response of size: {}", bytesToSend);

	while (bytesToSend > 0)
	{
		ssize_t bytesSent = send(epollFd, response.c_str() + byteOffset, bytesToSend, 0);
		
		if (bytesSent < bytesToSend)
		{
			return bytesSent;
		}
		if (bytesSent == -1)
		{
			//TODO: remove
			if (errno == EAGAIN || errno == EWOULDBLOCK)
			{
				LOG_DEBUG("send: EAGAIN");
				return byteOffset;
			}
			LOG_ERROR("send: {}", strerror(errno));
			return -1;
		}

		bytesToSend -= bytesSent;
		byteOffset += bytesSent;
		LOG_DEBUG("Sent {} bytes, remaining: {}", bytesSent, bytesToSend);
	}
	LOG_DEBUG("Total bytes sent: {}", byteOffset);
	return byteOffset;
}


// if (bytesSent < bytesToSend)
// 		{
// 			return bytesSent;
// 		}