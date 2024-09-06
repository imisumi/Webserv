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

// Macros to pack and unpack the u64 field
#define PACK_U64(fd, metadata)   (((uint64_t)(fd) << 32) | (metadata))
#define UNPACK_FD(u64)           ((int)((u64) >> 32))          // Extracts the upper 32 bits (fd)
#define UNPACK_METADATA(u64)     ((uint32_t)((u64) & 0xFFFFFFFF)) // Extracts the lower 32 bits (metadata)

// Define BIT(n) macro to get the value with the nth bit set
#define BIT(n) (1U << (n))
#define EPOLL_TYPE_SOCKET   BIT(0)
#define EPOLL_TYPE_CGI      BIT(1)
#define EPOLL_TYPE_STDIN    BIT(2)
#define EPOLL_TYPE_STDOUT   BIT(3)


static Server* s_Instance = nullptr;


//? temp

int Server::AddEpollEventStatic(int fd, int event)
{
	return s_Instance->AddEpollEvent(s_Instance->m_EpollFD, fd, event);
}

Server::Server(const Config& config)
	: m_Config(config)
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
	// EPOLLRDHUP | EPOLLHUP | EPOLLERR;
	epoll_event ev;
	ev.events = event;
	ev.data.fd = fd;


	//TODO: use this do differentiate between event data type
	// ev.data.u32 = 0;
	// ev.data.u32 = 10;

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

void Server::Init(const Config& config)
{
	WEB_ASSERT(!s_Instance, "Server already exists!");
	LOG_INFO("Server is starting up!");

	s_Instance = new Server(config);

	//? EPOLL_CLOEXEC: automatically close the file descriptor when calling exec
	if ((s_Instance->m_EpollFD = s_Instance->CreateEpoll()) == -1)
	{
		LOG_ERROR("Failed to create epoll file descriptor!");
		s_Instance->m_Running = false;
		return;
	}


	//? assuming that the config parser properly deals with duplicates
	s_Instance->m_ServerPorts.push_back(80);
	// s_Instance->m_ServerPorts.push_back(8080);
	// s_Instance->m_ServerPorts.push_back(8080);
	// s_Instance->m_ServerPorts.push_back(8002);

	int i = 0;
	for (int port : s_Instance->m_ServerPorts)
	{
		LOG_INFO("Creating server socket on port: {}", port);

		const SocketSettings settings = {
			.domain = AF_INET,
			.type = SOCK_STREAM | SOCK_NONBLOCK,
			.protocol = 0
		};

		auto it = s_Instance->m_ServerSockets.find(port);
		if (it != s_Instance->m_ServerSockets.end())
		{
			LOG_ERROR("Server socket already exists!");
			s_Instance->m_Running = false;
			return;
		}
		if ((s_Instance->m_ServerSockets[port] = s_Instance->CreateSocket(settings)) == -1)
		{
			LOG_ERROR("Failed to create server socket!");
			s_Instance->m_Running = false;
			return;
		}


		// frist 32 bits ip, 32-48 port
		// std::unordered_map<uint64_t, ServerSettings> m_ServerSockets;

		int reuse = 1;
		const SocketOptions options = {
			.level = SOL_SOCKET,
			.optname = SO_REUSEADDR | SO_REUSEPORT,
			// .optval = &s_Instance->m_ServerSockets[port],
			.optval = &reuse,
			// .optlen = sizeof(s_Instance->m_ServerSockets[port])
			.optlen = sizeof(reuse)
		};

		if (s_Instance->SetSocketOptions(s_Instance->m_ServerSockets[port], options) == -1)
		{
			LOG_ERROR("Failed to set socket options!");
			s_Instance->m_Running = false;
			return;
		}


		// int addr = INADDR_ANY;
		// if (i == 1)
		int	addr = 127 << 24 | 0 << 16 | 0 << 8 | 2;
		const SocketAddressConfig config = {
			.family = AF_INET, //? address type
			//TODO: extract from config
			.port = port, //? convert to network byte order
			// .address = addr
			.address = INADDR_ANY
		};

		if (s_Instance->BindSocket(s_Instance->m_ServerSockets[port], config) == -1)
		{
			LOG_ERROR("Failed to bind server socket!");
			s_Instance->m_Running = false;
			return;
		}


		if (s_Instance->ListenOnSocket(s_Instance->m_ServerSockets[port], 3) == -1)
		{
			LOG_ERROR("Failed to listen on server socket!");
			s_Instance->m_Running = false;
			return;
		}



		if (s_Instance->AddEpollEvent(s_Instance->m_EpollFD, s_Instance->m_ServerSockets[port], EPOLLIN | EPOLLET))
		{
			LOG_ERROR("Failed to add server socket to epoll!");
			s_Instance->m_Running = false;
			return;
		}
		i++;
	}

}

void Server::Shutdown()
{
	WEB_ASSERT(s_Instance, "Server does not exist!");
	LOG_INFO("Server is shutting down!");

	// close(s_Instance->m_ServerSocket);

	for (auto& [port, socket] : s_Instance->m_ServerSockets)
	{
		close(socket);
	}

	close(s_Instance->m_EpollFD);

	delete s_Instance;
	s_Instance = nullptr;
}


int Server::isServerSocket(int fd)
{
	for (auto& [port, socket] : s_Instance->m_ServerSockets)
	{
		if (fd == socket)
		{
			return port;
		}
	}

	return -1;
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
			int incomingPort = s_Instance->isServerSocket(events[i].data.fd);
			if (incomingPort != -1)
			{
				Client client = s_Instance->AcceptConnection(events[i].data.fd);
				if (client == -1)
				{
					LOG_ERROR("Failed to accept connection!");
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

				int fd = UNPACK_FD(events[i].data.u64);
				int metadata = UNPACK_METADATA(events[i].data.u64);

				s_Instance->HandleOutputEvent(events[i].data.fd);
			}
			else
			{
				LOG_DEBUG("Unhandled event type");
			}
		}
	}
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


Client Server::AcceptConnection(int socket_fd)
{
	struct sockaddr_in clientAddress;
	socklen_t clientAddressLength = sizeof(clientAddress);

	// Handle new incoming connection
	Client client = accept(socket_fd, (struct sockaddr*)&clientAddress, &clientAddressLength);
	if (client == -1)
	{
		LOG_ERROR("accept: {}", strerror(errno));
		return -1;
	}

	client.Initialize(clientAddress);
	m_Clients[client] = client;

	LOG_INFO("New connection from: {}, client socket: {}, client port: {}, server port: {}", 
		client.GetClientAddress(), (int)client, client.GetClientPort(), client.GetServerPort());


	if (s_Instance->AddEpollEvent(s_Instance->m_EpollFD, client, EPOLLIN | EPOLLET) == -1)
	{
		LOG_ERROR("Failed to add client socket to epoll!");
		m_Clients.erase(client);
		close(client);
		return -1;
	}

	// LOG_DEBUG("New connection from: {}, client socket: {}", inet_ntoa(clientAddress.sin_addr), client);

	return client;
}

void Server::HandleInputEvent(int epoll_fd)
{
	//TODO: what if the buffer is too small?
	char buffer[BUFFER_SIZE];

	//TODO: MSG_DONTWAIT or 0?
	ssize_t n = recv(epoll_fd, buffer, sizeof(buffer) - 1, 0);
	if (n > 0)
	{
		buffer[n] = '\0';
		const std::string bufferStr = std::string(buffer, n);
		LOG_INFO("Received data:\n{}", bufferStr);

		//TODO: get the correct sevrer config and pass it to the request handler
		Config config = Config::CreateDefaultConfig();
		const std::string response = s_Instance->m_RequestHandler.handleRequest(config, bufferStr);


		// LOG_DEBUG("Response:\n{}", response);
		m_ClientResponses[epoll_fd] = response;

		if (s_Instance->ModifyEpollEvent(s_Instance->m_EpollFD, epoll_fd, EPOLLOUT) == -1)
		{
			LOG_ERROR("Failed to modify client socket in epoll!");
			s_Instance->m_Running = false;
			return;
		}
	}
	else if (n == 0)
	{
		LOG_DEBUG("Client closed connection.");

		if (s_Instance->RemoveEpollEvent(s_Instance->m_EpollFD, epoll_fd) == -1)
		{
			LOG_ERROR("Failed to remove client socket from epoll!");
			s_Instance->m_Running = false;
		}

		//TODO: also when removing from epoll?

		close(epoll_fd);
		s_Instance->RemoveClient(epoll_fd);
		LOG_INFO("Total Clients: {}", s_Instance->GetClientCount());
		
	}
	else
	{
		close(epoll_fd);
		LOG_ERROR("HandleInputEvent read: {}", strerror(errno));
		s_Instance->RemoveClient(epoll_fd);
		LOG_INFO("Total Clients: {}", s_Instance->GetClientCount());
	}
}

void Server::HandleOutputEvent(int epoll_fd)
{
	// Attempt to find the client response for the given file descriptor
	if (auto it = m_ClientResponses.find(epoll_fd); it != m_ClientResponses.end())
	{
		const std::string& response = it->second;
		//TODO: use response sender
		if (s_Instance->m_ResponseSender.sendResponse(response, epoll_fd) == -1)
		{
			LOG_ERROR("send: {}", strerror(errno));
			close(epoll_fd);
		}
		else
		{
			LOG_DEBUG("Sent response to client.");

			// Remove the entry from the map
			m_ClientResponses.erase(it);

			if (s_Instance->ModifyEpollEvent(s_Instance->m_EpollFD, epoll_fd, EPOLLIN) == -1)
			{
				LOG_ERROR("Failed to modify client socket in epoll!");
				s_Instance->m_Running = false;
			}
		}
	}
	else
	{
		LOG_ERROR("No response found for client socket {}.", epoll_fd);
		exit(1);
		// close(epoll_fd);
	}
}