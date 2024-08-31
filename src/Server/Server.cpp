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

int Server::CreateEpoll()
{
	//TODO: mabye pass flags as a parameter, or use epoll_create which takes no flags but is deprecated
	return epoll_create1(EPOLL_CLOEXEC);
}

int Server::AddEpollEvent(int epollFD, int fd, int event)
{
	epoll_event ev;
	ev.events = event;
	ev.data.fd = fd;

	return epoll_ctl(epollFD, EPOLL_CTL_ADD, fd, &ev);
}

int Server::RemoveEpollEvent(int epollFD, int fd)
{
	return epoll_ctl(epollFD, EPOLL_CTL_DEL, fd, nullptr);
}

int Server::ModifyEpollEvent(int epollFD, int fd, int event)
{
	epoll_event ev;
	ev.events = event;
	ev.data.fd = fd;

	return epoll_ctl(epollFD, EPOLL_CTL_MOD, fd, &ev);
}

// return socket file descriptor
int Server::CreateSocket(const SocketSettings& settings)
{
	return socket(settings.domain, settings.type, settings.protocol);
}

int Server::SetSocketOptions(int fd, const SocketOptions& options)
{
	return setsockopt(fd, options.level, options.optname, options.optval, options.optlen);
}

int Server::BindSocket(int fd, const SocketAddressConfig& config)
{
	struct sockaddr_in socketAddress = {};
	socketAddress.sin_family = config.family;
	socketAddress.sin_port = htons(config.port);
	socketAddress.sin_addr.s_addr = htonl(config.address);
	return bind(fd, (struct sockaddr*)&socketAddress, sizeof(socketAddress));
}

int Server::ListenOnSocket(int socket_fd, int backlog)
{
	return listen(socket_fd, backlog);
}

void Server::Init()
{
	WEB_ASSERT(!s_Instance, "Server already exists!");
	LOG_INFO("Server is starting up!");

	s_Instance = new Server();

	s_Instance->m_RequestHandler = std::make_shared<RequestHandler>();
	// s_Instance->m_ResponseSender = std::make_shared<ResponseSender>();


	//? EPOLL_CLOEXEC: automatically close the file descriptor when calling exec
	if ((s_Instance->m_EpollFD = s_Instance->CreateEpoll()) == -1)
	{
		LOG_ERROR("Failed to create epoll file descriptor!");
		s_Instance->m_Running = false;
		return;
	}


	// SOCK_STREAM for TCP && SOCK_NONBLOCK for non-blocking (alternative to fcntl)
	const SocketSettings settings = {
		.domain = AF_INET,
		.type = SOCK_STREAM | SOCK_NONBLOCK,
		.protocol = 0
	};

	if ((s_Instance->m_ServerSocket = s_Instance->CreateSocket(settings)) == -1)
	{
		LOG_ERROR("Failed to create server socket!");
		s_Instance->m_Running = false;
		return;
	}

	const SocketOptions options = {
		.level = SOL_SOCKET,
		.optname = SO_REUSEADDR | SO_REUSEPORT,
		.optval = &s_Instance->m_ServerSocket,
		.optlen = sizeof(s_Instance->m_ServerSocket)
	};

	if (s_Instance->SetSocketOptions(s_Instance->m_ServerSocket, options) == -1)
	{
		LOG_ERROR("Failed to set socket options!");
		s_Instance->m_Running = false;
		return;
	}

	const SocketAddressConfig config = {
		.family = AF_INET, //? address type
		//TODO: extract from config
		.port = 8080, //? convert to network byte order
		.address = INADDR_ANY
	};

	if (s_Instance->BindSocket(s_Instance->m_ServerSocket, config) == -1)
	{
		LOG_ERROR("Failed to bind server socket!");
		s_Instance->m_Running = false;
		return;
	}

	if (s_Instance->ListenOnSocket(s_Instance->m_ServerSocket, 3) == -1)
	{
		LOG_ERROR("Failed to listen on server socket!");
		s_Instance->m_Running = false;
		return;
	}


	// s_Instance->m_EpollEvent.events = EPOLLIN | EPOLLET; //? EPOLLIN for read events && EPOLLET for edge-triggered mode
	// s_Instance->m_EpollEvent.data.fd = s_Instance->m_ServerSocket;

	// //? EPOLL_CTL_ADD: add a file descriptor to the epoll instance

	if (s_Instance->AddEpollEvent(s_Instance->m_EpollFD, s_Instance->m_ServerSocket, EPOLLIN | EPOLLET))
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


// static std::string getFormattedTime() {
//     // Get the current time
//     std::time_t t = std::time(nullptr);
//     std::tm* tm = std::gmtime(&t); // Use gmtime for UTC time

//     // Create a string stream to format the date and time
//     std::ostringstream oss;

//     // Format the date and time as [DD/Mon/YYYY:HH:MM:SS +0000]
//     oss << '[' 
//         << std::setfill('0') << std::setw(2) << tm->tm_mday << '/'          // Day
//         << std::put_time(tm, "%b") << '/'                                   // Month abbreviation
//         << (tm->tm_year + 1900) << ':'                                      // Year
//         << std::setw(2) << tm->tm_hour << ':'                               // Hours
//         << std::setw(2) << tm->tm_min << ':'                                // Minutes
//         << std::setw(2) << tm->tm_sec << " +0000"                           // Seconds and timezone
//         << ']';

//     return oss.str();
// }

std::string getFormattedUTCTime() {
    // Get the current time
    std::time_t t = std::time(nullptr);

    // Convert to UTC time
    std::tm* tm_utc = std::gmtime(&t);

    // Create a string stream to format the date and time
    std::ostringstream oss;

    // Format the date and time as [DD/Mon/YYYY:HH:MM:SS +0000]
    oss << '[' 
        << std::setfill('0') << std::setw(2) << tm_utc->tm_mday << '/'     // Day
        << std::put_time(tm_utc, "%b") << '/'                              // Month abbreviation
        << (tm_utc->tm_year + 1900) << ':'                                 // Year
        << std::setw(2) << tm_utc->tm_hour << ':'                          // Hours
        << std::setw(2) << tm_utc->tm_min << ':'                           // Minutes
        << std::setw(2) << tm_utc->tm_sec << " +0000"                      // Seconds and UTC offset
        << ']';

    return oss.str();
}

void Server::Run()
{
	WEB_ASSERT(s_Instance, "Server does not exist!");

	LOG_INFO("Server is running!");

	struct epoll_event events[MAX_EVENTS];


	

	if (s_Instance->AddEpollEvent(s_Instance->m_EpollFD, STDIN_FILENO, EPOLLIN))
	{
		LOG_ERROR("Failed to add stdin to epoll!");
		s_Instance->m_Running = false;
		return;
	}
	


	std::string clientIP[MAX_EVENTS];
	while (s_Instance->m_Running)
	{
		// LOG_TRACE("Server is running...");

		// std::this_thread::sleep_for(std::chrono::milliseconds(500));

		LOG_INFO("Waiting for events...");

		//? EPOLL_WAIT: wait for events on the epoll instance
		int eventCount = epoll_wait(s_Instance->m_EpollFD, events, MAX_EVENTS, -1);
		if (eventCount == -1)
		{
			LOG_ERROR("epoll_wait: {}", strerror(errno));
			return;
		}
		LOG_DEBUG("Event count: {}", eventCount);
		for (int i = 0; i < eventCount; i++)
		{
			if (events[i].data.fd == s_Instance->m_ServerSocket)
			{
				if (s_Instance->AcceptConnection() == -1)
				{
					LOG_ERROR("Failed to accept connection!");
				}
			}
			else if (events[i].data.fd == STDIN_FILENO)
			{
				//TODO: maybe add some handy console commands?
				LOG_DEBUG("Handling stdin event...");

				char buffer[1024];
				ssize_t n = read(STDIN_FILENO, buffer, sizeof(buffer));
				if (n > 0)
				{
					buffer[n] = '\0';
					LOG_INFO("Received input: {}", buffer);
					if (strcmp(buffer, "exit\n") == 0)
					{
						LOG_INFO("Received exit command. Shutting down server...");
						s_Instance->m_Running = false;
					}
				}
				else if (n == 0)
				{
					LOG_DEBUG("EOF on stdin");
					s_Instance->m_Running = false;
				}
				else
				{
					LOG_ERROR("read: {}", strerror(errno));
					s_Instance->m_Running = false;
				}
			}
			else if (events[i].events & EPOLLIN)
			{
				LOG_DEBUG("Handling input event...");

				s_Instance->HandleInputEvent(events[i].data.fd);
			}
			else if (events[i].events & EPOLLOUT)
			{
				LOG_DEBUG("Handling output event...");

				s_Instance->HandleOutputEvent(events[i].data.fd);
			}
			else
			{
				LOG_DEBUG("Unhandled event type");
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



int Server::AcceptConnection()
{
	struct sockaddr_in clientAddress;
	socklen_t clientAddressLength = sizeof(clientAddress);

	// Handle new incoming connection
	int client_socket = accept(s_Instance->m_ServerSocket, (struct sockaddr*)&clientAddress, &clientAddressLength);
	if (client_socket == -1)
	{
		LOG_ERROR("accept: {}", strerror(errno));
		return -1;
	}


	int flags = fcntl(client_socket, F_GETFL, 0);
	fcntl(client_socket, F_SETFL, flags | O_NONBLOCK);

	if (s_Instance->AddEpollEvent(s_Instance->m_EpollFD, client_socket, EPOLLIN | EPOLLET))
	{
		LOG_ERROR("Failed to add client socket to epoll!");
		close(client_socket);
		return -1;
	}

	LOG_DEBUG("New connection from: {}, client socket: {}", inet_ntoa(clientAddress.sin_addr), client_socket);

	return client_socket;
}

void Server::HandleInputEvent(int fd)
{
	//TODO: what if the buffer is too small?
	char buffer[BUFFER_SIZE];

	//TODO: MSG_DONTWAIT or 0?
	ssize_t n = recv(fd, buffer, sizeof(buffer) - 1, 0);
	if (n > 0)
	{
		buffer[n] = '\0';
		const std::string bufferStr = std::string(buffer, n);
		// LOG_INFO("Received data:\n{}", bufferStr);

		const std::string response = s_Instance->m_RequestHandler->handleRequest(bufferStr, fd);

		m_ClientResponses[fd] = response;

		if (s_Instance->ModifyEpollEvent(s_Instance->m_EpollFD, fd, EPOLLOUT) == -1)
		{
			LOG_ERROR("Failed to modify client socket in epoll!");
			s_Instance->m_Running = false;
			return;
		}
	}
	else if (n == 0)
	{
		LOG_DEBUG("Client closed connection.");

		if (s_Instance->RemoveEpollEvent(s_Instance->m_EpollFD, fd) == -1)
		{
			LOG_ERROR("Failed to remove client socket from epoll!");
			s_Instance->m_Running = false;
		}

		//TODO: also when removing from epoll?
		close(fd);
	}
	else
	{
		close(fd);
		LOG_ERROR("read: {}", strerror(errno));
	}
}

void Server::HandleOutputEvent(int fd)
{
	//TODO: use response sender
	auto it = m_ClientResponses.find(fd);
	if (it != m_ClientResponses.end())
	{
		const std::string& response = it->second;
		ssize_t sentBytes = send(fd, response.c_str(), response.size(), 0);

		if (sentBytes == -1)
		{
			LOG_ERROR("send: {}", strerror(errno));
			close(fd);
		}
		else
		{
			LOG_DEBUG("Sent response to client.");

			// Remove the entry from the map
			m_ClientResponses.erase(it);

			if (s_Instance->ModifyEpollEvent(s_Instance->m_EpollFD, fd, EPOLLIN) == -1)
			{
				LOG_ERROR("Failed to modify client socket in epoll!");
				s_Instance->m_Running = false;
				return;
			}
		}
	}
	else
	{
		LOG_ERROR("No response found for client socket {}.", fd);
		close(fd);
	}
}