#include "ResponseSender.h"

#include <sys/socket.h>

#include "Core/Log.h"

ssize_t ResponseSender::sendResponse(const std::string& response, int epollFd)
{
	ssize_t bytesToSend = response.size();
	ssize_t byteOffset = 0;

	LOG_DEBUG("Sending response of size: {}", bytesToSend);

	if (bytesToSend > 0)
	{
		ssize_t bytesSent = send(epollFd, response.c_str() + byteOffset, bytesToSend, 0);

		if (bytesSent < bytesToSend)
		{
			return bytesSent;
		}
		if (bytesSent == -1)
		{
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

ssize_t ResponseSender::sendResponse(Client& client)
{
	const std::string& response = client.GetResponse();
	ssize_t bytesToSend = response.size() - client.GetBytesSent();
	ssize_t byteOffset = client.GetBytesSent();

	LOG_DEBUG("Sending response of size: {}", bytesToSend);

	ssize_t bytesSent = send((int)client, response.c_str() + byteOffset, bytesToSend, 0);
	if (bytesSent == -1)
	{
		LOG_ERROR("send: {}", strerror(errno));
		return -1;
	}
	return bytesSent;

	if (bytesSent < bytesToSend)
	{
		return bytesSent;
	}

	bytesToSend -= bytesSent;
	byteOffset += bytesSent;
	LOG_DEBUG("Sent {} bytes, remaining: {}", bytesSent, bytesToSend);

	LOG_DEBUG("Total bytes sent: {}", byteOffset);
	return byteOffset;
}