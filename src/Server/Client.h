#pragma once

#include <cstdint>

#include <arpa/inet.h>

class Client
{
public:
	Client(int socket = -1) : m_Socket(socket) {}

	Client& operator=(int socket)
	{
		m_Socket = socket;
		return *this;
	}
	operator int() const { return m_Socket; }


	bool Initialize(const struct sockaddr_in& clientAddress)
	{
		// Convert and store the client address and port
		if (inet_ntop(AF_INET, &clientAddress.sin_addr, m_ClientAddress, INET_ADDRSTRLEN) == nullptr)
		{
			// Handle inet_ntop error
			return false;
		}
		m_ClientPort = ntohs(clientAddress.sin_port);

		// Retrieve and store the peer (remote end) address and port
		struct sockaddr_in peerAddress;
		socklen_t peerAddressLength = sizeof(peerAddress);
		if (getpeername(m_Socket, (struct sockaddr*)&peerAddress, &peerAddressLength) == -1)
		{
			// Handle getpeername error
			return false;
		}
		if (inet_ntop(AF_INET, &peerAddress.sin_addr, m_PeerAddress, INET_ADDRSTRLEN) == nullptr)
		{
			// Handle inet_ntop error
			return false;
		}
		m_PeerPort = ntohs(peerAddress.sin_port);

		// Retrieve and store the local (server) address and port
		struct sockaddr_in localAddress;
		socklen_t localAddressLength = sizeof(localAddress);
		if (getsockname(m_Socket, (struct sockaddr*)&localAddress, &localAddressLength) == -1)
		{
			// Handle getsockname error
			return false;
		}
		m_ServerPort = ntohs(localAddress.sin_port);

		return true;
	}


	const char* GetClientAddress() const { return m_ClientAddress; }
	uint16_t GetClientPort() const { return m_ClientPort; }

	const char* GetPeerAddress() const { return m_PeerAddress; }
	uint16_t GetPeerPort() const { return m_PeerPort; }

	uint16_t GetServerPort() const { return m_ServerPort; }

	
private:
	int m_Socket = -1;

	uint16_t m_ClientPort = -1;
	char m_ClientAddress[INET_ADDRSTRLEN] = { 0 };

	uint16_t m_ServerPort = -1;


	uint16_t m_PeerPort = -1;
	char m_PeerAddress[INET_ADDRSTRLEN] = { 0 };


	int m_EpollInstance = -1;
};