#pragma once

#include "Client.h"

#include <unordered_map>


class ConnectionManager
{
public:
	static ConnectionManager& Get();
	static void Init();
	static void Shutdown();

	static Client AcceptConnection(int socket_fd);

	static void RegisterClient(int fd, Client& client);
	static void UnregisterClient(int fd);

	static const Client& GetClient(int fd);
	static Client& GetClientRef(int fd);

	static std::size_t GetConnectedClients();

	// static

private:
	ConnectionManager();
	~ConnectionManager();

private:
	std::unordered_map<int, Client> m_ConnectedClients;
};