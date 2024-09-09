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
#define EPOLL_TYPE_SOCKET   BIT(0)
#define EPOLL_TYPE_CGI      BIT(1)
#define EPOLL_TYPE_STDIN    BIT(2)
#define EPOLL_TYPE_STDOUT   BIT(3)


static Server* s_Instance = nullptr;


//? temp

// int Server::AddEpollEventStatic(int fd, int event)
// {
// 	return s_Instance->AddEpollEvent(s_Instance->m_EpollInstance, fd, event);
// }

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

// int Server::AddEpollEvent(int epollFD, int fd, int event, uint32_t type)
// {
// 	epoll_event ev;
// 	ev.events = event;
// 	// ev.data.fd = fd;
// 	ev.data.u64 = PACK_U64(fd, type);

// 	// m_FdEventMap[fd] = ev;
// 	EpollData data;
// 	data.fd = fd;
// 	data.type = type;
// 	data.cgi_fd = -1;
// 	m_FdEventMap[fd] = data;

// 	return epoll_ctl(epollFD, EPOLL_CTL_ADD, fd, &ev);
// }

int Server::AddEpollEvent(int epollFD, int fd, int event, struct EpollData* data)
{
	epoll_event ev;
	ev.events = event;
	// ev.data.fd = fd;
	// ev.data.u64 = PACK_U64(fd, type);
	ev.data.ptr = data;

	// m_FdEventMap[fd] = ev;

	// m_FdEventMap[fd] = data;

	return epoll_ctl(epollFD, EPOLL_CTL_ADD, fd, &ev);
}

int Server::AddEpollEventStatic(int epollFD, int fd, int event, struct EpollData* data)
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
	m_FdEventMap.erase(fd);

	return epoll_ctl(epollFD, EPOLL_CTL_DEL, fd, nullptr);
}

// int Server::ModifyEpollEvent(int epollFD, int fd, int event, uint32_t type)
// {
// 	// EPOLLRDHUP | EPOLLHUP | EPOLLERR;
// 	epoll_event ev;
// 	ev.events = event;
// 	// ev.data.fd = fd;
// 	ev.data.u64 = PACK_U64(fd, type);


// 	//TODO: use this do differentiate between event data type


// 	return epoll_ctl(epollFD, EPOLL_CTL_MOD, fd, &ev);
// }

int Server::ModifyEpollEventStatic(int epollFD, int fd, int event, struct EpollData* data)
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

int Server::ModifyEpollEvent(int epollFD, int fd, int event, struct EpollData* data)
{
	// EPOLLRDHUP | EPOLLHUP | EPOLLERR;
	epoll_event ev;
	ev.events = event;
	// ev.data.fd = fd;
	// ev.data.u64 = PACK_U64(fd, type);
	ev.data.ptr = data;


	//TODO: use this do differentiate between event data type


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
	if (inet_pton(AF_INET, ipStr.c_str(), &ipAddr) != 1) {
		std::cerr << "Invalid IP address format" << std::endl;
		return 0;
	}

	// Convert to host byte order (so we can work with it directly)
	uint32_t ip = ntohl(ipAddr.s_addr);

	// Combine IP (shifted by 32 bits) and the port into a uint64_t
	uint64_t result = static_cast<uint64_t>(ip) << 32;  // Put IP in the upper 32 bits
	result |= static_cast<uint64_t>(port);			  // Put port in the lower 16 bits

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

void Server::Init(const Config& config)
{
	WEB_ASSERT(!s_Instance, "Server already exists!");
	LOG_INFO("Server is starting up!");

	s_Instance = new Server(config);

	//? EPOLL_CLOEXEC: automatically close the file descriptor when calling exec
	if ((s_Instance->m_EpollInstance = s_Instance->CreateEpoll()) == -1)
	{
		LOG_ERROR("Failed to create epoll file descriptor!");
		s_Instance->m_Running = false;
		return;
	}

	//TODO: replace int with server settings, also this is handled in the config
	std::unordered_map<uint64_t, int> serverHosts;
	//? assuming that the config parser properly deals with duplicates

	uint64_t newIP;
	newIP = packIpAndPort("127.0.0.5", 8080);
	serverHosts[newIP] = 0;
	newIP = packIpAndPort("127.0.0.4", 8080);
	serverHosts[newIP] = 0;
	newIP = packIpAndPort("127.0.0.4", 8081);
	serverHosts[newIP] = 0;
	newIP = packIpAndPort("0.0.0.0", 8080);
	serverHosts[newIP] = 0;

	// loop through the server hosts
	for (auto& [packedIPPort, serverID] : serverHosts)
	{
		int ip = (packedIPPort >> 32) & 0xFFFFFFFF;
		char ipStr[INET_ADDRSTRLEN];
		struct in_addr ipAddr;
		ipAddr.s_addr = htonl(ip);
		if (inet_ntop(AF_INET, &ipAddr, ipStr, INET_ADDRSTRLEN) == NULL)
		{
			LOG_ERROR("Failed to convert IP to string format");
			s_Instance->m_Running = false;
			return;
		}
		int port = packedIPPort & 0xFFFF;
		LOG_INFO("Creating server socket on IP: {}, port: {}", ipStr, port);

		auto it = s_Instance->m_ServerSockets64.find(packedIPPort);
		if (it != s_Instance->m_ServerSockets64.end())
		{
			LOG_ERROR("Server socket already exists!");
			s_Instance->m_Running = false;
			return;
		}

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
		if (setsockopt(socketFD, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT , &reuse, sizeof(reuse)) == -1)
		{
			LOG_ERROR("Failed to set socket options!");
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

		if (listen(socketFD, 3) == -1)
		{
			LOG_ERROR("Failed to listen on server socket!");
			s_Instance->m_Running = false;
			return;
		}

		// if (s_Instance->AddEpollEvent(s_Instance->m_EpollInstance, socketFD, EPOLLIN | EPOLLET) == -1)
		// {
		// 	LOG_ERROR("Failed to add server socket to epoll!");
		// 	s_Instance->m_Running = false;
		// 	return;
		// }


		// LOG_INFO("Socket fd: {}", socketFD);
		epoll_event ev;
		ev.events = EPOLLIN | EPOLLET;
		ev.data.u64 = (static_cast<uint64_t>(socketFD) << 32) | EPOLL_TYPE_SOCKET;

		// if (epoll_ctl(s_Instance->m_EpollInstance, EPOLL_CTL_ADD, socketFD, &ev) == -1)
		// {
		// 	LOG_ERROR("Failed to add server socket to epoll!");
		// 	s_Instance->m_Running = false;
		// 	return;
		// }

		struct EpollData* data = new EpollData();
		data->fd = socketFD;
		data->type = EPOLL_TYPE_SOCKET;
		data->cgi_fd = -1;

		// if (s_Instance->AddEpollEvent(s_Instance->m_EpollInstance, socketFD, EPOLLIN | EPOLLET, PACK_U64(socketFD, EPOLL_TYPE_SOCKET)) == -1)
		if (s_Instance->AddEpollEvent(s_Instance->m_EpollInstance, socketFD, EPOLLIN | EPOLLET, data) == -1)
		{
			LOG_ERROR("Failed to add server socket to epoll!");
			s_Instance->m_Running = false;
			return;
		}

		


		// int sock = static_cast<uint32_t>(ev.data.u64 >> 32);
		// uint32_t metadata = static_cast<uint32_t>(ev.data.u64 & 0xFFFFFFFF);
		// LOG_INFO("Socket fd: {}, {}", sock, metadata);
		// exit(1);
	}
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
		LOG_INFO("Waiting for events...");

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
			// uint32_t epoll_fd = static_cast<uint32_t>(events[i].data.u64 >> 32);
			// uint32_t epoll_type = static_cast<uint32_t>(events[i].data.u64 & 0xFFFFFFFF);

			struct EpollData* epollData = (struct EpollData*)events[i].data.ptr;
			uint32_t epoll_fd = epollData->fd;
			uint32_t epoll_type = epollData->type;
			uint32_t cgi_fd = epollData->cgi_fd;

			LOG_DEBUG("Handling event...");
			int incomingPort = s_Instance->isServerSocket(epoll_fd);
			if (incomingPort != -1)
			{
				Client newClient = s_Instance->AcceptConnection(epoll_fd);
				if (newClient == -1)
				{
					LOG_ERROR("Failed to accept connection!");
				}
				s_Instance->m_Clients[newClient] = newClient;
				// tempClient = newClient;
				continue;
			}

			Client client = s_Instance->m_Clients[epoll_fd];
			LOG_INFO("epoll fd: {}, client socket: {}", epoll_fd, (int)client);

			if (events[i].events & EPOLLIN)
			{
				LOG_DEBUG("Handling input event...");
				if (epoll_type == EPOLL_TYPE_SOCKET)
					s_Instance->HandleInputEvent(client);
				else if (epoll_type == EPOLL_TYPE_CGI)
				{
					LOG_DEBUG("Handling CGI input event...");
					char buffer[BUFFER_SIZE];
					// ssize_t n = read(epoll_fd, buffer, sizeof(buffer) - 1);
					ssize_t n = read(cgi_fd, buffer, sizeof(buffer) - 1);
					if (n > 0)
					{
						buffer[n] = '\0';
						const std::string output = std::string(buffer, n);


						int tempClient = s_Instance->GetClientFromCgi(epoll_fd);
						LOG_INFO("Temp client: {}", tempClient);

						size_t header_end = output.find("\r\n\r\n");
						if (header_end != std::string::npos)
						{
							size_t bodyLength = output.size() - header_end - 4;
							std::string httpResponse = "HTTP/1.1 200 OK\r\n";
							httpResponse += "Content-Length: " + std::to_string(bodyLength) + "\r\n";
							httpResponse += "Connection: close\r\n";
							// httpResponse += output.substr(0, header_end + 2); // Body
							// httpResponse += "\r\n"; // End of headers
							// httpResponse += output.substr(header_end + 4); // Body
							httpResponse += output;


							LOG_DEBUG("Response:\n{}", httpResponse);

							s_Instance->m_ClientResponses[epoll_fd] = httpResponse;
						}
						else
						{
							LOG_ERROR("CGI script did not produce valid headers");
							// s_Instance->m_ClientResponses[tempClient] = ResponseGenerator::InternalServerError(s_Instance->m_Config);
						}

						// struct EpollData* tempData = (struct EpollData*)events[i].data.ptr;

						struct EpollData* data = new EpollData();
						data->fd = epoll_fd;
						data->type = EPOLL_TYPE_SOCKET;
						data->cgi_fd = -1;

						// if (s_Instance->ModifyEpollEvent(s_Instance->m_EpollInstance, tempClient, EPOLLOUT | EPOLLET, EPOLL_TYPE_SOCKET) == -1)
						if (s_Instance->ModifyEpollEvent(s_Instance->m_EpollInstance, epoll_fd, EPOLLOUT | EPOLLET, data) == -1)
						{
							LOG_ERROR("Failed to modify client socket in epoll!");
							s_Instance->m_Running = false;
						}


						s_Instance->RemoveEpollEvent(s_Instance->m_EpollInstance, cgi_fd);
						close(cgi_fd);
					}
					else if (n == 0)
					{
						LOG_DEBUG("Client closed connection.");

						if (s_Instance->RemoveEpollEvent(s_Instance->m_EpollInstance, epoll_fd) == -1)
						{
							LOG_ERROR("Failed to remove client socket from epoll!");
							s_Instance->m_Running = false;
						}

					}
					else
					{
						close(epoll_fd);
						LOG_ERROR("HandleInputEvent read: {}", strerror(errno));
						s_Instance->RemoveClient(epoll_fd);
						LOG_INFO("Total Clients: {}", s_Instance->GetClientCount());
					}
				}
			}
			else if (events[i].events & EPOLLOUT)
			{
				LOG_DEBUG("Handling output event...");

				if (epoll_type == EPOLL_TYPE_SOCKET)
					s_Instance->HandleOutputEvent(epoll_fd);
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
	// m_Clients[client] = client;

	LOG_INFO("New connection from: {}, client socket: {}, client port: {}, server port: {}", 
		client.GetClientAddress(), (int)client, client.GetClientPort(), client.GetServerPort());


	struct EpollData *data = new EpollData();
	data->fd = client;
	data->type = EPOLL_TYPE_SOCKET;
	data->cgi_fd = -1;

	// if (s_Instance->AddEpollEvent(s_Instance->m_EpollInstance, client, EPOLLIN | EPOLLET, EPOLL_TYPE_SOCKET) == -1)
	if (s_Instance->AddEpollEvent(s_Instance->m_EpollInstance, client, EPOLLIN | EPOLLET, data) == -1)
	{
		LOG_ERROR("Failed to add client socket to epoll!");
		// m_Clients.erase(client);
		close(client);
		return -1;
	}

	client.SetEpollInstance(s_Instance->m_EpollInstance);

	// LOG_DEBUG("New connection from: {}, client socket: {}", inet_ntoa(clientAddress.sin_addr), client);

	return client;
}

void Server::HandleInputEvent(const Client& client)
{
	//TODO: what if the buffer is too small?
	char buffer[BUFFER_SIZE];

	//TODO: MSG_DONTWAIT or 0?
	ssize_t n = recv(client, buffer, sizeof(buffer) - 1, 0);
	if (n > 0)
	{
		buffer[n] = '\0';
		const std::string bufferStr = std::string(buffer, n);
		LOG_INFO("Received data:\n{}", bufferStr);

		//TODO: get the correct sevrer config and pass it to the request handler
		Config config = Config::CreateDefaultConfig();
		const std::string response = s_Instance->m_RequestHandler.handleRequest(client, config, bufferStr);

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
		if (s_Instance->ModifyEpollEvent(s_Instance->m_EpollInstance, client, EPOLLOUT | EPOLLET, data) == -1)
		{
			LOG_ERROR("Failed to modify client socket in epoll!");
			s_Instance->m_Running = false;
			return;
		}
	}
	else if (n == 0)
	{
		LOG_DEBUG("Client closed connection.");

		if (s_Instance->RemoveEpollEvent(s_Instance->m_EpollInstance, client) == -1)
		{
			LOG_ERROR("Failed to remove client socket from epoll!");
			s_Instance->m_Running = false;
		}

		//TODO: also when removing from epoll?

		close(client);
		s_Instance->RemoveClient(client);
		LOG_INFO("Total Clients: {}", s_Instance->GetClientCount());
		
	}
	else
	{
		close(client);
		LOG_ERROR("HandleInputEvent read: {}", strerror(errno));
		s_Instance->RemoveClient(client);
		LOG_INFO("Total Clients: {}", s_Instance->GetClientCount());
	}
}

void Server::HandleOutputEvent(int epoll_fd)
{
	// Attempt to find the client response for the given file descriptor
	if (auto it = m_ClientResponses.find(epoll_fd); it != m_ClientResponses.end())
	{
		const std::string& response = it->second;

		LOG_INFO("Sending response to client...");
		LOG_INFO("Response:\n{}", response);

		ssize_t bytes = send(epoll_fd, response.c_str(), response.size(), 0);
		if (bytes == -1)
		{
			LOG_ERROR("send: {}", strerror(errno));
			close(epoll_fd);
		}
		else
		{
			LOG_DEBUG("Sent response to client.");

			// Remove the entry from the map
			m_ClientResponses.erase(it);

			struct EpollData* data = new EpollData();
			data->fd = epoll_fd;
			data->type = EPOLL_TYPE_SOCKET;
			data->cgi_fd = -1;

			// if (s_Instance->ModifyEpollEvent(s_Instance->m_EpollInstance, epoll_fd, EPOLLIN | EPOLLET, EPOLL_TYPE_SOCKET) == -1)
			if (s_Instance->ModifyEpollEvent(s_Instance->m_EpollInstance, epoll_fd, EPOLLIN | EPOLLET, data) == -1)
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