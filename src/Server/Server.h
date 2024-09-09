#pragma once

#include <sys/socket.h>
#include <netinet/in.h>

#include <fcntl.h>
#include <sys/epoll.h>

#include <memory>
#include <vector>

#include <unordered_map>

#include "RequestHandler.h"
#include "ResponseSender.h"
#include "Config/ConfigParser.h"
#include "Client.h"

#define MAX_EVENTS 4096
#define BUFFER_SIZE 4096 * 2

class Server
{
public:
	struct EpollData
	{
		int fd = -1;
		int cgi_fd = -1;
		uint32_t type = 0;
	};

	struct SocketSettings
	{
		int domain = 0;
		int type = 0;
		int protocol = 0;
	};

	struct SocketOptions
	{
		int level = 0;
		int optname = 0;
		const void* optval = nullptr;
		socklen_t optlen = 0;
	};

	struct SocketAddressConfig
	{
		int family = 0;
		int port = 0;
		uint32_t address = 0;
	};

public:
	static Server& Get();
	static void Init(const Config& config);
	static void Shutdown();
	static void Run();
	static void Stop();
	static bool IsRunning();

	Server(const Server&) = delete;
	Server& operator=(const Server&) = delete;


	int GetEpollInstance() const
	{
		return m_EpollInstance;
	}

	void SetCgiToClientMap(int cgi_fd, int client_fd)
	{
		m_CgiToClientMap[cgi_fd] = client_fd;
	}

	int GetClientFromCgi(int cgi_fd)
	{
		return m_CgiToClientMap[cgi_fd];
	}


	// struct EpolLData GetEpollEventFD(int fd)
	// {
	// 	return m_FdEventMap[fd];
	// }

	struct EpollData GetEpollData(int fd)
	{
		return m_FdEventMap[fd];
	}

	// static int AddEpollEventStatic(int fd, int event);

	int AddEpollEvent(int epollFD, int fd, int event, uint32_t type);
	int RemoveEpollEvent(int epollFD, int fd);
	int ModifyEpollEvent(int epollFD, int fd, int event, uint32_t type);
private:
	Server(const Config& config);
	~Server();

	int isServerSocket(int fd);

	// int CreateServerSocket();
	// void BindServerSocket();
	// void ListenServerSocket();
	Client AcceptConnection(int socket_fd);
	// void HandleInputEvent(int fd);
	void HandleInputEvent(const Client& client);
	void HandleOutputEvent(int fd);

	int CreateEpoll();

	int CreateSocket(const SocketSettings& settings);
	int SetSocketOptions(int fd, const SocketOptions& options);
	int BindSocket(int fd, const SocketAddressConfig& config);
	int ListenOnSocket(int socket_fd, int backlog);


	void RemoveClient(int socket_fd)
	{
		auto it = m_Clients.find(socket_fd);
		if (it != m_Clients.end())
		{
			m_Clients.erase(it);
		}
	}

	uint32_t GetClientCount() const
	{
		return m_Clients.size();
	}

private:
	const Config& m_Config;
	// Config m_Config;

	bool m_Running = true;

	// int m_ServerSocket = -1;
	std::vector<int> m_ServerPorts;

	// int port, int socket fd
	// std::unordered_map<int, int> m_ServerSockets;
	std::unordered_map<uint64_t, int> m_ServerSockets64;
	int m_EpollInstance = -1;



	struct sockaddr_in m_SockAddress;
	RequestHandler m_RequestHandler;
	ResponseSender m_ResponseSender;

	std::unordered_map<int, std::string> m_ClientResponses; // Maps client socket to response data

	// std::unordered_map<int, struct epoll_event> m_FdEventMap;
	std::unordered_map<int, struct EpollData> m_FdEventMap;
	std::unordered_map<int, int> m_CgiToClientMap;


	std::unordered_map<int, Client> m_Clients;
};