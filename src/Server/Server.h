#pragma once

#include <sys/socket.h>
#include <netinet/in.h>

#include <fcntl.h>
#include <sys/epoll.h>

#include <memory>

#include "RequestHandler.h"
#include "ResponseSender.h"

#define MAX_EVENTS 4096
#define BUFFER_SIZE 4096

class Server
{
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
private:
	bool m_Running = true;

	int m_ServerSocket = -1;
	int m_EpollFD = -1;

	int m_Connections = 0;

	struct epoll_event m_EpollEvent;

	struct sockaddr_in m_SockAddress;

	//TODO: maybe stack-allocate this?
	std::shared_ptr<RequestHandler> m_RequestHandler;
	// std::shared_ptr<ResponseSender> m_ResponseSender;
};