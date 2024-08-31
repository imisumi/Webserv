#pragma once

#include <sys/socket.h>
#include <netinet/in.h>

#include <fcntl.h>
#include <sys/epoll.h>

#include <memory>

#include <unordered_map>

#include "RequestHandler.h"
#include "ResponseSender.h"

#define MAX_EVENTS 4096
#define BUFFER_SIZE 4096 * 2

class Server
{
public:
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
		int address = 0;
	};

public:
	static Server& Get();
	static void Init();
	static void Shutdown();
	static void Run();
	static void Stop();
	static bool IsRunning();

	Server(const Server&) = delete;
	Server& operator=(const Server&) = delete;
private:
	Server();
	~Server();

	// int CreateServerSocket();
	// void BindServerSocket();
	// void ListenServerSocket();
	int AcceptConnection();
	void HandleInputEvent(int fd);
	void HandleOutputEvent(int fd);

	int CreateEpoll();
	int AddEpollEvent(int epollFD, int fd, int event);
	int RemoveEpollEvent(int epollFD, int fd);
	int ModifyEpollEvent(int epollFD, int fd, int event);

	int CreateSocket(const SocketSettings& settings);
	int SetSocketOptions(int fd, const SocketOptions& options);
	int BindSocket(int fd, const SocketAddressConfig& config);
	int ListenOnSocket(int socket_fd, int backlog);

private:
	// Config m_Config;
	std::string bufferStr;
	bool m_Running = true;

	int m_ServerSocket = -1;
	int m_EpollFD = -1;

	int m_Connections = 0;

	// struct epoll_event m_EpollEvent;

	struct sockaddr_in m_SockAddress;

	//TODO: maybe stack-allocate this?
	std::shared_ptr<RequestHandler> m_RequestHandler;
	// std::shared_ptr<ResponseSender> m_ResponseSender;

	std::unordered_map<int, std::string> m_ClientResponses; // Maps client socket to response data
};