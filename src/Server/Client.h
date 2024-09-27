#pragma once

#include <cstdint>

#include <arpa/inet.h>

#include "Config/Config.h"
#include "HttpRequestParser.h"
#include "NewHttpParser.h"

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


	bool Initialize(const struct sockaddr_in& clientAddress, int socket_fd);


	const char* GetClientAddress() const { return m_ClientAddress; }
	uint16_t GetClientPort() const { return m_ClientPort; }

	const char* GetPeerAddress() const { return m_PeerAddress; }
	uint16_t GetPeerPort() const { return m_PeerPort; }

	uint16_t GetServerPort() const { return m_ServerPort; }
	const char* GetServerAddress() const { return m_ServerAddress; }

	int GetEpollInstance() const { return m_EpollInstance; }
	void SetEpollInstance(int epollInstance) { m_EpollInstance = epollInstance; }


	void SetConfig(ServerSettings* config) { m_Config = config; }
	ServerSettings* GetConfig() const { return m_Config; }

	void SetRequest(const HttpRequest& request) { m_Request = request; }
	const HttpRequest& GetRequest() const { return m_Request; }


	void SetNewRequest(const NewHttpRequest& request) { m_NewRequest = request; }
	const NewHttpRequest& GetNewRequest() const { return m_NewRequest; }


	void SetLocationSettings(const ServerSettings::LocationSettings& locationSettings) { m_LocationSettings = locationSettings; }
	const ServerSettings::LocationSettings& GetLocationSettings() const { return m_LocationSettings; }
private:
	int m_Socket = -1;

	uint16_t m_ClientPort = -1;
	char m_ClientAddress[INET_ADDRSTRLEN] = { 0 };

	uint16_t m_ServerPort = -1;
	char m_ServerAddress[INET_ADDRSTRLEN] = { 0 };

	uint16_t m_PeerPort = -1;
	char m_PeerAddress[INET_ADDRSTRLEN] = { 0 };


	int m_EpollInstance = -1;


	ServerSettings* m_Config;
	HttpRequest m_Request;

	NewHttpRequest m_NewRequest;

	ServerSettings::LocationSettings m_LocationSettings;
};