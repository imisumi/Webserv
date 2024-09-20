// #include "pch.h"

#include "Server.h"
#include "Core/Core.h"

#include "ResponseGenerator.h"
#include "ConnectionManager.h"




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
#define EPOLL_TYPE_SOCKET   BIT(0)
#define EPOLL_TYPE_CGI      BIT(1)
#define EPOLL_TYPE_STDIN    BIT(2)
#define EPOLL_TYPE_STDOUT   BIT(3)


static Server* s_Instance = nullptr;


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

int Server::AddEpollEvent(int epollFD, int fd, int event, struct EpollData* data)
{
	WEB_ASSERT(epollFD, "Invalid epoll file descriptor!");
	WEB_ASSERT(fd, "Invalid file descriptor!");
	WEB_ASSERT(data, "Invalid data!");
	WEB_ASSERT(event, "Invalid event!");

	struct epoll_event ev;
	ev.events = event;
	ev.data.ptr = data;

	return epoll_ctl(epollFD, EPOLL_CTL_ADD, fd, &ev);
}


int Server::RemoveEpollEvent(int epollFD, int fd)
{
	s_Instance->m_FdEventMap.erase(fd);

	return epoll_ctl(epollFD, EPOLL_CTL_DEL, fd, nullptr);
}


int Server::ModifyEpollEvent(int epollFD, int fd, int event, struct EpollData* data)
{
	WEB_ASSERT(epollFD, "Invalid epoll file descriptor!");
	WEB_ASSERT(fd, "Invalid file descriptor!");
	WEB_ASSERT(data, "Invalid data!");
	WEB_ASSERT(event, "Invalid event!");

	struct epoll_event ev;
	ev.events = event;
	ev.data.ptr = data;

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

uint64_t packIpAndPort(const std::string& ipStr, uint16_t port)
{
	struct in_addr ipAddr;

	// Convert the IP string to a 32-bit integer (network byte order)
	if (inet_pton(AF_INET, ipStr.c_str(), &ipAddr) != 1)
	{
		std::cerr << "Invalid IP address format" << std::endl;
		return 0;
	}

	// Convert to host byte order (so we can work with it directly)
	uint32_t ip = ntohl(ipAddr.s_addr);

	// Combine IP (shifted by 32 bits) and the port into a uint64_t
	uint64_t result = static_cast<uint64_t>(ip) << 32;	// Put IP in the upper 32 bits
	result |= static_cast<uint64_t>(port);				// Put port in the lower 16 bits

	return result;
}

std::pair<std::string, uint16_t> unpackIpAndPort(uint64_t packedValue)
{
	// Extract the port (lower 16 bits)
	uint16_t port = packedValue & 0xFFFF;

	// Extract the IP address (upper 32 bits)
	uint32_t ip = (packedValue >> 32) & 0xFFFFFFFF;

	// Convert the 32-bit IP back to string format
	struct in_addr ipAddr;
	ipAddr.s_addr = htonl(ip);  // Convert back to network byte order

	char ipStr[INET_ADDRSTRLEN];
	if (inet_ntop(AF_INET, &ipAddr, ipStr, INET_ADDRSTRLEN) == NULL) {
		std::cerr << "Failed to convert IP to string format" << std::endl;
		return {"", 0};
	}

	return {std::string(ipStr), port};
}

/**
 * @brief Redirects CGI output to a specified file descriptor.
 *
 * @param cgi_fd The file descriptor for the CGI process. This is the descriptor from which
 *               data will be read.
 * @param redir_fd The file descriptor where the CGI output should be redirected. This is the
 *                 destination where the data from `cgi_fd` will be written.
 *
 * @return An integer value indicating the success or failure of adding the epoll event. Typically,
 *         this will be 0 on success and a negative error code on failure.
 */
int Server::CgiRedirect(int cgi_fd, int redir_fd)
{
	struct Server::EpollData *ev_data = new Server::EpollData();
	ev_data->fd = redir_fd;
	ev_data->cgi_fd = cgi_fd;
	ev_data->type = EPOLL_TYPE_CGI;

	return AddEpollEvent(s_Instance->m_EpollInstance, cgi_fd, EPOLLIN | EPOLLET, ev_data);
}

void Server::Init(const Config& config)
{
	WEB_ASSERT(!s_Instance, "Server already exists!");
	LOG_INFO("Server is starting up!");

	s_Instance = new Server(config);

	ConnectionManager::Init();

	//? EPOLL_CLOEXEC: automatically close the file descriptor when calling exec
	if ((s_Instance->m_EpollInstance = s_Instance->CreateEpoll()) == -1)
	{
		LOG_ERROR("Failed to create epoll file descriptor!");
		s_Instance->m_Running = false;
		return;
	}

	//TODO: replace int with server settings, also this is handled in the config
	//? assuming that the config parser properly deals with duplicates

	for (auto& [packedIPPort, serverSettings] : config)
	{
		ServerSettings* settings = serverSettings[0];
		const uint32_t ip = static_cast<uint32_t>(packedIPPort >> 32);
		const uint16_t port = static_cast<uint16_t>(packedIPPort & 0xFFFF);

		char ipStr[INET_ADDRSTRLEN];
		struct in_addr ipAddr;
		ipAddr.s_addr = htonl(ip);
		if (inet_ntop(AF_INET, &ipAddr, ipStr, INET_ADDRSTRLEN) == NULL)
		{
			LOG_ERROR("Failed to convert IP to string format");
			s_Instance->m_Running = false;
			return;
		}
		LOG_INFO("Creating server socket on IP: {}, port: {}", ipStr, port);

		// auto it = s_Instance->m_ServerSockets64.find(packedIPPort);
		// if (it != s_Instance->m_ServerSockets64.end())
		// {
		// 	LOG_ERROR("Server socket already exists!");
		// 	s_Instance->m_Running = false;
		// 	return;
		// }

		//? Logic
		int socketFD = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
		if (socketFD == -1)
		{
			LOG_ERROR("Failed to create server socket!");
			s_Instance->m_Running = false;
			return;
		}
		s_Instance->m_ServerSockets64[packedIPPort] = socketFD;

		int reuse = 1;
		//? SO_REUSEADDR: allows other sockets to bind to an address even if it is already in use
		//? SO_REUSEPORT: allows multiple sockets to bind to the same port and ip
		if (setsockopt(socketFD, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1)
		{
			LOG_ERROR("Failed to set SO_REUSEADDR!");
			s_Instance->m_Running = false;
			return;
		}
		reuse = 1;
		if (setsockopt(socketFD, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) == -1)
		{
			LOG_ERROR("Failed to set SO_REUSEPORT!");
			s_Instance->m_Running = false;
			return;
		}

		struct sockaddr_in socketAddress = {};
		socketAddress.sin_family = AF_INET;
		socketAddress.sin_port = htons(port);
		socketAddress.sin_addr.s_addr = htonl(ip);
		// socketAddress.sin_addr.s_addr = inet_addr("127.0.0.5");

		if (bind(socketFD, (struct sockaddr*)&socketAddress, sizeof(socketAddress)) == -1)
		{
			LOG_ERROR("Failed to bind server socket!");
			s_Instance->m_Running = false;
			return;
		}

		if (listen(socketFD, SOMAXCONN) == -1)
		{
			LOG_ERROR("Failed to listen on server socket!");
			s_Instance->m_Running = false;
			return;
		}

		struct EpollData* data = new EpollData();
		data->fd = socketFD;
		data->type = EPOLL_TYPE_SOCKET;
		data->cgi_fd = -1;

		if (s_Instance->AddEpollEvent(s_Instance->m_EpollInstance, socketFD, EPOLLIN | EPOLLET, data) == -1)
		{
			LOG_ERROR("Failed to add server socket to epoll!");
			s_Instance->m_Running = false;
			return;
		}
	}
	return ;
}

void Server::Shutdown()
{
	WEB_ASSERT(s_Instance, "Server does not exist!");
	LOG_INFO("Server is shutting down!");

	// close(s_Instance->m_ServerSocket);

	// for (auto& [port, socket] : s_Instance->m_ServerSockets)
	// {
	// 	close(socket);
	// }

	close(s_Instance->m_EpollInstance);

	delete s_Instance;
	s_Instance = nullptr;
}


int Server::isServerSocket(int fd)
{
	for (auto& [packedIpPort, socket] : s_Instance->m_ServerSockets64)
	{
		if (fd == socket)
		{
			// return ;
			return static_cast<uint32_t>(packedIpPort >> 32);
		}
	}

	return -1;
}

void Server::Run()
{
	WEB_ASSERT(s_Instance, "Server does not exist!");

	LOG_INFO("Server is running!");

	struct epoll_event events[MAX_EVENTS];

	std::string clientIP[MAX_EVENTS];
	// int tempClient = -1;

	
	while (s_Instance->m_Running)
	{
		LOG_INFO("------------------------------------------------------------------------------------------");
		LOG_INFO("Waiting for events...");
		LOG_INFO("------------------------------------------------------------------------------------------\n\n");

		//? EPOLL_WAIT: wait for events on the epoll instance
		int eventCount = epoll_wait(s_Instance->m_EpollInstance, events, MAX_EVENTS, -1);
		if (eventCount == -1)
		{
			if (errno == EINTR)
			{
				// Interrupted by a signal, such as SIGCHLD, retry epoll_wait
				std::cout << "epoll_wait interrupted by signal, retrying..." << std::endl;
				continue;
			}
			LOG_ERROR("epoll_wait: {}", strerror(errno));
			return;
		}
		for (int i = 0; i < eventCount; i++)
		{
			const struct EpollData* epollData = (struct EpollData*)events[i].data.ptr;
			const uint32_t epoll_fd = epollData->fd;
			const uint32_t epoll_type = epollData->type;
			const uint32_t cgi_fd = epollData->cgi_fd;

			if (events[i].events & EPOLLIN && s_Instance->isServerSocket(epoll_fd) != -1)
			{
				// Handle incoming connections
				while (true)
				{
					Client newClient = ConnectionManager::AcceptConnection(epoll_fd);
					if ((int)newClient == -1)
					{
						if (errno == EAGAIN || errno == EWOULDBLOCK)
						{
							// No more clients to accept
							break;
						}
						// LOG_CRITICAL("Failed to accept connection!");
						LOG_ERROR("Failed to accept connection! errno: {}, error: {}", errno, strerror(errno));

						break;
					}
					ConnectionManager::RegisterClient((int)newClient, newClient);
				}
				continue;
			}
			Client client = ConnectionManager::GetClient(epoll_fd);
			LOG_INFO("epoll fd: {}, client socket: {}", epoll_fd, (int)client);
			if (events[i].events & EPOLLIN)
			{
				LOG_DEBUG("Handling input event...");
				if (epoll_type == EPOLL_TYPE_SOCKET)
				{
					s_Instance->HandleSocketInputEvent(client);
				}
				else if (epoll_type == EPOLL_TYPE_CGI)
				{
					s_Instance->HandleCgiInputEvent(cgi_fd, epoll_fd);
				}
			}
			else if (events[i].events & EPOLLOUT)
			{
				LOG_DEBUG("Handling output event...");

				if (epoll_type == EPOLL_TYPE_SOCKET)
					s_Instance->HandleOutputEvent(epoll_fd);
			}
			else if (events[i].events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
			{
				// LOG_INFO("Handling error event...");
				// LOG_INFO("Event type: {}", (int)events[i].events);
				LOG_CRITICAL("Handling error event...");

				// close(epoll_fd);
				// continue;
			}
			else
			{
				LOG_CRITICAL("Unhandled event type");
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

void Server::HandleSocketInputEvent(Client& client)
{
	LOG_INFO("Handling socket input event...");

	//TODO: what if the buffer is too small?
	char buffer[BUFFER_SIZE];

	//TODO: put in a loop till all data is read
	ssize_t n = recv(client, buffer, sizeof(buffer) - 1, 0);
	if (n > 0)
	{
		buffer[n] = '\0';
		const std::string bufferStr = std::string(buffer, n);
		LOG_TRACE("Received data:\n{}", bufferStr);

		//TODO: get the correct sevrer config and pass it to the request handler
		Config config = ConfigParser::createDefaultConfig();
		// const std::string response = s_Instance->m_RequestHandler.handleRequest(client, bufferStr);
		const std::string response = s_Instance->m_RequestHandler.HandleRequest(client, bufferStr);
		if (response.empty())
		{
			LOG_INFO("Request is a CGI request, forwarding to CGI handler...");
			return;
		}


		// LOG_DEBUG("Response:\n{}", response);
		m_ClientResponses[client] = response;

		struct EpollData* data = new EpollData();
		data->fd = client;
		data->type = EPOLL_TYPE_SOCKET;
		data->cgi_fd = -1;

		// if (s_Instance->ModifyEpollEvent(s_Instance->m_EpollInstance, client, EPOLLOUT | EPOLLET, EPOLL_TYPE_SOCKET) == -1)
		if (ModifyEpollEvent(s_Instance->m_EpollInstance, client, EPOLLOUT | EPOLLET, data) == -1)
		{
			LOG_ERROR("Failed to modify client socket in epoll!");
			s_Instance->m_Running = false;
			return;
		}
	}
	else if (n == 0)
	{
		LOG_DEBUG("Client closed connection.");

		if (RemoveEpollEvent(s_Instance->m_EpollInstance, client) == -1)
		{
			LOG_ERROR("Failed to remove client socket from epoll!");
			s_Instance->m_Running = false;
		}

		//TODO: also when removing from epoll?

		close(client);
		// s_Instance->RemoveClient(client);
		// LOG_INFO("Total Clients: {}", s_Instance->GetClientCount());
		ConnectionManager::UnregisterClient(client);
		LOG_INFO("Total Clients: {}", ConnectionManager::GetConnectedClients());

		
	}
	else
	{
		close(client);
		LOG_ERROR("HandleInputEvent read: {}", strerror(errno));
		// s_Instance->RemoveClient(client);
		// LOG_INFO("Total Clients: {}", s_Instance->GetClientCount());
		ConnectionManager::UnregisterClient(client);
		LOG_INFO("Total Clients: {}", ConnectionManager::GetConnectedClients());
	}
}

void Server::HandleCgiInputEvent(int cgi_fd, int client_fd)
{
	LOG_DEBUG("Handling CGI input event...");
	char buffer[BUFFER_SIZE];
	// ssize_t n = read(epoll_fd, buffer, sizeof(buffer) - 1);
	ssize_t n = read(cgi_fd, buffer, sizeof(buffer) - 1);
	if (n > 0)
	{
		buffer[n] = '\0';
		const std::string output = std::string(buffer, n);

		size_t header_end = output.find("\r\n\r\n");
		if (header_end != std::string::npos)
		{
			size_t bodyLength = output.size() - header_end - 4;
			std::string httpResponse = "HTTP/1.1 200 OK\r\n";
			httpResponse += "Content-Length: " + std::to_string(bodyLength) + "\r\n";
			httpResponse += "Connection: close\r\n";
			httpResponse += output;

			LOG_DEBUG("Response:\n{}", httpResponse);

			s_Instance->m_ClientResponses[client_fd] = httpResponse;
		}
		else
		{
			LOG_ERROR("CGI script did not produce valid headers");
			s_Instance->m_ClientResponses[client_fd] = ResponseGenerator::InternalServerError(s_Instance->m_Config);
		}

		// struct EpollData* tempData = (struct EpollData*)events[i].data.ptr;

		struct EpollData* data = new EpollData();
		data->fd = client_fd;
		data->type = EPOLL_TYPE_SOCKET;
		data->cgi_fd = -1;

		// if (s_Instance->ModifyEpollEvent(s_Instance->m_EpollInstance, tempClient, EPOLLOUT | EPOLLET, EPOLL_TYPE_SOCKET) == -1)
		if (s_Instance->ModifyEpollEvent(s_Instance->m_EpollInstance, client_fd, EPOLLOUT | EPOLLET, data) == -1)
		{
			LOG_ERROR("Failed to modify client socket in epoll!");
			s_Instance->m_Running = false;
		}


		RemoveEpollEvent(s_Instance->m_EpollInstance, cgi_fd);
		close(cgi_fd);
	}
	else if (n == 0)
	{
		LOG_DEBUG("Client closed connection.");

		if (RemoveEpollEvent(s_Instance->m_EpollInstance, client_fd) == -1)
		{
			LOG_ERROR("Failed to remove client socket from epoll!");
			s_Instance->m_Running = false;
		}
		close(client_fd);

	}
	else
	{
		close(client_fd);
		LOG_ERROR("HandleInputEvent read: {}", strerror(errno));
		// s_Instance->RemoveClient(client_fd);
		// close(cgi_fd);
		// LOG_INFO("Total Clients: {}", s_Instance->GetClientCount());
		ConnectionManager::UnregisterClient(client_fd);
		LOG_INFO("Total Clients: {}", ConnectionManager::GetConnectedClients());
		close(cgi_fd);
	}
}

void Server::HandleOutputEvent(int epoll_fd)
{
	// Attempt to find the client response for the given file descriptor
	if (auto it = m_ClientResponses.find(epoll_fd); it != m_ClientResponses.end())
	{
		const std::string& response = it->second;


		if (response.find("image/x-icon") != std::string::npos)
		{
			// LOG_DEBUG("Sending favicon.ico to client: {}", epoll_fd);
			LOG_TRACE("Sending favicon.ico to client: {}", epoll_fd);
		}
		else
		{
			LOG_TRACE("Sending response to client:\n{}", response);
		}

		// LOG_TRACE("Sending response to client:\n{}", response);
		// LOG_INFO("Response:\n{}", response);

		//TODO: bit in a loop till all data is sent
		ssize_t bytes = send(epoll_fd, response.c_str(), response.size(), 0);
		if (bytes < response.size())
		{
			LOG_ERROR("Failed to send entire response to client: {}", epoll_fd);
		}
		if (bytes == -1)
		{
			LOG_ERROR("send: {}", strerror(errno));
			if (RemoveEpollEvent(s_Instance->m_EpollInstance, epoll_fd) == -1)
			{
				LOG_ERROR("Failed to remove client socket from epoll!");
				s_Instance->m_Running = false;
			}
			close(epoll_fd);
		}
		else
		{
			LOG_DEBUG("Sent response to client: {}", epoll_fd);

			// Remove the entry from the map

			struct EpollData* data = new EpollData();
			data->fd = epoll_fd;
			data->type = EPOLL_TYPE_SOCKET;
			data->cgi_fd = -1;


			//TODO: find a better way of doing this
			if (response.find("Connection: close") != std::string::npos)
			{
				LOG_DEBUG("Closing connection for client socket: {}", epoll_fd);
				if (RemoveEpollEvent(s_Instance->m_EpollInstance, epoll_fd) == -1)
				{
					LOG_ERROR("Failed to remove client socket from epoll!");
					s_Instance->m_Running = false;
				}
				close(epoll_fd);
			}
			else
			{
				if (ModifyEpollEvent(s_Instance->m_EpollInstance, epoll_fd, EPOLLIN | EPOLLET, data) == -1)
				{
					LOG_ERROR("Failed to modify client socket in epoll!");
					s_Instance->m_Running = false;
				}
			}
			m_ClientResponses.erase(it);

			// if (s_Instance->ModifyEpollEvent(s_Instance->m_EpollInstance, epoll_fd, EPOLLIN | EPOLLET, EPOLL_TYPE_SOCKET) == -1)
		}
	}
	else
	{
		LOG_ERROR("No response found for client socket {}.", epoll_fd);
		exit(1);
		// close(epoll_fd);
	}
}