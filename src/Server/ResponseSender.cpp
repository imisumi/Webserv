#include "ResponseSender.h"

#include <sys/socket.h>

#include "Core/Log.h"

#include <cstring>

ssize_t ResponseSender::sendResponse(const std::string& response, int epollFd)
{
	size_t bytesToSend = response.size();
	size_t byteOffset = 0;

	Log::debug("Sending response of size: {}", bytesToSend);

	if (bytesToSend > 0)
	{
		size_t bytesSent = send(epollFd, response.c_str() + byteOffset, bytesToSend, 0);

		if (bytesSent < bytesToSend)
		{
			return bytesSent;
		}
		if (bytesSent == -1)
		{
			Log::error("send: {}", strerror(errno));
			return -1;
		}

		bytesToSend -= bytesSent;
		byteOffset += bytesSent;
		Log::debug("Sent {} bytes, remaining: {}", bytesSent, bytesToSend);
	}
	Log::debug("Total bytes sent: {}", byteOffset);
	return byteOffset;
}

ssize_t ResponseSender::sendResponse(Client& client)
{
	const std::string& response = client.GetResponse();
	ssize_t bytesToSend = response.size() - client.GetBytesSent();
	ssize_t byteOffset = client.GetBytesSent();

	Log::debug("Sending response of size: {}", bytesToSend);

	ssize_t bytesSent = send((int)client, response.c_str() + byteOffset, bytesToSend, 0);
	if (bytesSent == -1)
	{
		Log::error("send: {}", strerror(errno));
		return -1;
	}
	return bytesSent;

	if (bytesSent < bytesToSend)
	{
		return bytesSent;
	}

	bytesToSend -= bytesSent;
	byteOffset += bytesSent;
	Log::debug("Sent {} bytes, remaining: {}", bytesSent, bytesToSend);

	Log::debug("Total bytes sent: {}", byteOffset);
	return byteOffset;
}