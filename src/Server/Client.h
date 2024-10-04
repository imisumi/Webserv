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


	// delete copy constructor and assignment operator

	bool Initialize(const struct sockaddr_in& clientAddress, int socket_fd);


	const char* GetClientAddress() const { return m_ClientAddress; }
	uint16_t GetClientPort() const { return m_ClientPort; }

	const char* GetPeerAddress() const { return m_PeerAddress; }
	uint16_t GetPeerPort() const { return m_PeerPort; }

	uint16_t GetServerPort() const { return m_ServerPort; }
	const char* GetServerAddress() const { return m_ServerAddress; }

	int GetEpollInstance() const { return m_EpollInstance; }
	void SetEpollInstance(int epollInstance) { m_EpollInstance = epollInstance; }


	void SetServerConfig(ServerSettings* config) { m_ServerConfig = config; }
	ServerSettings* GetServerConfig() const { return m_ServerConfig; }

	void SetNewRequest(const NewHttpRequest& request) { m_NewRequest = request; }
	const NewHttpRequest& GetNewRequest() const { return m_NewRequest; }
	NewHttpRequest& GetNewRequest() { return m_NewRequest; }


	HttpState parseRequest(const std::string& requestBuffer)
	{
		return m_NewRequest.parseStream(requestBuffer);
	}


	void reset()
	{
		m_NewRequest = NewHttpRequest();
		m_Response.clear();
	}


	void SetLocationSettings(const ServerSettings::LocationSettings& locationSettings) { m_LocationSettings = locationSettings; }
	const ServerSettings::LocationSettings& GetLocationSettings() const { return m_LocationSettings; }

	void SetResponse(const std::string& response) { m_Response = response; }
	const std::string& GetResponse() const { return m_Response; }


private:
	int m_Socket = -1;

	uint16_t m_ClientPort = -1;
	char m_ClientAddress[INET_ADDRSTRLEN] = { 0 };

	uint16_t m_ServerPort = -1;
	char m_ServerAddress[INET_ADDRSTRLEN] = { 0 };

	uint16_t m_PeerPort = -1;
	char m_PeerAddress[INET_ADDRSTRLEN] = { 0 };

	std::string m_Response;


	int m_EpollInstance = -1;


	NewHttpRequest m_NewRequest;


	ServerSettings* m_ServerConfig;
	ServerSettings::LocationSettings m_LocationSettings;
};