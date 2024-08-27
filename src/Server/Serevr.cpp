#include "Server.h"
#include "Core/Core.h"

#include <chrono>
#include <thread>
#include <filesystem>
#include <fstream>

// include socket headers
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

// include epoll headers
#include <sys/epoll.h>

#include <cerrno>


static Server* s_Instance = nullptr;

Server::Server()
{
}

Server::~Server()
{
}

Server& Server::Get()
{
	WEB_ASSERT(s_Instance, "Server does not exist!");

	return *s_Instance;
}

void Server::Init()
{
	WEB_ASSERT(!s_Instance, "Server already exists!");
	LOG_INFO("Server is starting up!");

	s_Instance = new Server();


	//? EPOLL_CLOEXEC: automatically close the file descriptor when calling exec
	s_Instance->m_EpollFD = epoll_create1(EPOLL_CLOEXEC);
	WEB_ASSERT(s_Instance->m_EpollFD, "Failed to create epoll file descriptor!");

	// SOCK_STREAM for TCP && SOCK_NONBLOCK for non-blocking (alternative to fcntl)
	// s_Instance->m_ServerSocket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	s_Instance->m_ServerSocket = socket(AF_INET, SOCK_STREAM, 0);
	WEB_ASSERT(s_Instance->m_ServerSocket, "Failed to create server socket!");

	if (setsockopt(
		s_Instance->m_ServerSocket, 
		SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, 
		&s_Instance->m_ServerSocket, 
		sizeof(s_Instance->m_ServerSocket)))
	{
		LOG_ERROR("Failed to set socket options!");
		s_Instance->m_Running = false;
		return;
	}

	s_Instance->m_SockAddress.sin_family = AF_INET; //? address type
	s_Instance->m_SockAddress.sin_port = htons(8080); //? convert to network byte order
	s_Instance->m_SockAddress.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(s_Instance->m_ServerSocket, (struct sockaddr*)&s_Instance->m_SockAddress, sizeof(s_Instance->m_SockAddress)) < 0)
	{
		LOG_ERROR("Failed to bind server socket!");
		s_Instance->m_Running = false;
		return;
	}

	if (listen(s_Instance->m_ServerSocket, 3) < 0)
	{
		LOG_ERROR("Failed to listen on server socket!");
		s_Instance->m_Running = false;
		return;
	}


	s_Instance->m_EpollEvent.events = EPOLLIN | EPOLLET; //? EPOLLIN for read events && EPOLLET for edge-triggered mode
	s_Instance->m_EpollEvent.data.fd = s_Instance->m_ServerSocket;

	//? EPOLL_CTL_ADD: add a file descriptor to the epoll instance
	if (epoll_ctl(s_Instance->m_EpollFD, EPOLL_CTL_ADD, s_Instance->m_ServerSocket, &s_Instance->m_EpollEvent) < 0)
	{
		LOG_ERROR("Failed to add server socket to epoll!");
		s_Instance->m_Running = false;
		return;
	}
}

void Server::Shutdown()
{
	WEB_ASSERT(s_Instance, "Server does not exist!");
	LOG_INFO("Server is shutting down!");

	close(s_Instance->m_ServerSocket);

	delete s_Instance;
	s_Instance = nullptr;
}


void SendFavIcon(int client_socket)
{
	const char* RESPONSE_TEMPLATE =
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: image/x-icon\r\n"
		"Content-Length: %d\r\n"
		"Cache-Control: max-age=86400\r\n" // Cache for 1 day
		"Connection: close\r\n"
		"\r\n";
	std::filesystem::path favicon_path("resources/web.png");
	LOG_INFO("Favicon path: {}", favicon_path.string());

	if (std::filesystem::exists(favicon_path) && std::filesystem::is_regular_file(favicon_path))
	{
		// Get the size of the file
		std::streamsize size = std::filesystem::file_size(favicon_path);

		// Prepare the response header
		char header[256];
		int header_length = snprintf(header, sizeof(header), RESPONSE_TEMPLATE, static_cast<int>(size));

		// Send the header
		send(client_socket, header, header_length, 0);

		// Read the file and send its contents
		std::ifstream icon_file(favicon_path, std::ios::in | std::ios::binary);
		if (icon_file) {
			char* buffer = new char[size];
			if (icon_file.read(buffer, size)) {
				send(client_socket, buffer, size, 0);
			}
			delete[] buffer;
			icon_file.close();
		}
		else
		{
			LOG_ERROR("Failed to read favicon!");
		}
	}
	else //? control f5 to ignore cache
	{
		LOG_ERROR("Failed to find favicon!");
		const char* not_found_response = 
			"HTTP/1.1 404 Not Found\r\n"
			"Content-Length: 0\r\n"
			"Connection: close\r\n"
			"\r\n";
		send(client_socket, not_found_response, strlen(not_found_response), 0);
	}
}

void Server::Run()
{
	WEB_ASSERT(s_Instance, "Server does not exist!");

	LOG_INFO("Server is running!");

	struct epoll_event events[MAX_EVENTS];
	while (s_Instance->m_Running)
	{
		LOG_TRACE("Server is running...");

		// std::this_thread::sleep_for(std::chrono::seconds(1));

		//? EPOLL_WAIT: wait for events on the epoll instance
		int eventCount = epoll_wait(s_Instance->m_EpollFD, events, MAX_EVENTS, -1);
		if (eventCount == -1)
		{
			LOG_ERROR("epoll_wait: {}", strerror(errno));
			return;
		}
		for (int i = 0; i < eventCount; ++i)
		{
			if (events[i].data.fd == s_Instance->m_ServerSocket)
			{
				LOG_INFO("New connection!");
				struct sockaddr_in clientAddress;
				socklen_t clientAddressLength = sizeof(clientAddress);

				// Handle new incoming connection
				int client_socket = accept(s_Instance->m_ServerSocket, (struct sockaddr*)&clientAddress, &clientAddressLength);
				if (client_socket == -1)
				{
					LOG_ERROR("accept: {}", strerror(errno));
					continue;
				}

				LOG_INFO("Connection accepted from: {}", inet_ntoa(clientAddress.sin_addr));

				// Add client socket to epoll
				s_Instance->m_EpollEvent.events = EPOLLIN | EPOLLOUT;
				s_Instance->m_EpollEvent.data.fd = client_socket;
				if (epoll_ctl(s_Instance->m_EpollFD, EPOLL_CTL_ADD, client_socket, &s_Instance->m_EpollEvent) == -1)
				{
					LOG_ERROR("epoll_ctl: {}", strerror(errno));
					close(client_socket);
				}
			}
			else
			{
				// Handle I/O events on client socket
				char buffer[BUFFER_SIZE];
				ssize_t n = read(events[i].data.fd, buffer, sizeof(buffer));
				if (n > 0)
				{
					LOG_INFO("Received data:\n{}", buffer);
					const std::string bufferStr(buffer);

					// Process data and prepare a response
					if (bufferStr.find("GET /favicon.ico") != std::string::npos)
					{
						//TODO: get stuck on page reload
						SendFavIcon(events[i].data.fd);
					}
					else if (bufferStr.find("GET") != std::string::npos)
					{
						std::string respone = "HTTP/1.1 200 OK\nContent-Type: text/plain\nContent-Length: 12\n\nHello World!";
						send(events[i].data.fd, respone.c_str(), respone.size(), 0);
					}
				}
				else if (n == 0)
				{
					close(events[i].data.fd);
				}
				else
				{
					LOG_ERROR("read: {}", strerror(errno));
				}
			}
		}
	}
	close(s_Instance->m_ServerSocket);
	close(s_Instance->m_EpollFD);
}

void Server::Stop()
{
	WEB_ASSERT(s_Instance, "Server does not exist!");

	s_Instance->m_Running = false;
}

bool Server::IsRunning()
{
	WEB_ASSERT(s_Instance, "Server does not exist!");

	return s_Instance->m_Running;
}