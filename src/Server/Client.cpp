#include "Client.h"
#include "Server.h"
#include "Utils/Utils.h"

#include <cstring>

bool Client::Initialize(const struct sockaddr_in& clientAddress, int socket_fd)
{
	// Convert and store the client address and port
	if (inet_ntop(AF_INET, &clientAddress.sin_addr, m_ClientAddress.data(), INET_ADDRSTRLEN) == nullptr)
	{
		return false;
	}
	m_ClientPort = ntohs(clientAddress.sin_port);

	// Retrieve and store the peer (remote end) address and port
	struct sockaddr_in peerAddress = {};
	socklen_t peerAddressLength = sizeof(peerAddress);
	if (getpeername(m_Socket, (struct sockaddr*)&peerAddress, &peerAddressLength) == -1)
	{
		return false;
	}
	if (inet_ntop(AF_INET, &peerAddress.sin_addr, m_PeerAddress.data(), INET_ADDRSTRLEN) == nullptr)
	{
		return false;
	}
	m_PeerPort = ntohs(peerAddress.sin_port);

	// Retrieve and store the local (server) address and port
	struct sockaddr_in localAddress = {};
	socklen_t localAddressLength = sizeof(localAddress);
	if (getsockname(m_Socket, (struct sockaddr*)&localAddress, &localAddressLength) == -1)
	{
		return false;
	}
	m_ServerPort = ntohs(localAddress.sin_port);
	for (const auto& [packedIpPort, socket] : Server::Get().GetServerSockets())
	{
		if (socket == socket_fd)
		{
			auto [ip, port] = Utils::unpackIpAndPort(packedIpPort);
			strncpy(m_ServerAddress.data(), ip.c_str(), INET_ADDRSTRLEN - 1);
			break;
		}
	}

	return true;
}