// #include "pch.h"

#include "Server.h"
#include "Core/Core.h"

#include "ResponseGenerator.h"
#include "ConnectionManager.h"

#include "Utils/Utils.h"
#include "Constants.h"


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

int Server::CreateEpollInstance()
{
	WEB_ASSERT(s_Instance, "Server does not exist!");

	s_Instance->m_EpollInstance = epoll_create1(EPOLL_CLOEXEC);
	return s_Instance->m_EpollInstance;
}

int Server::AddEpollEvent(int fd, int event, EpollData data)
{
	struct epoll_event ev;
	ev.events = event;
	ev.data.u64 = data;

	return epoll_ctl(Get().GetEpollInstance(), EPOLL_CTL_ADD, fd, &ev);
}


int Server::RemoveEpollEvent(int epollFD, int fd)
{
	s_Instance->m_FdEventMap.erase(fd);

	return epoll_ctl(epollFD, EPOLL_CTL_DEL, fd, nullptr);
}


int Server::ModifyEpollEvent(int epollFD, int fd, int event, EpollData data)
{
	WEB_ASSERT(epollFD, "Invalid epoll file descriptor!");
	WEB_ASSERT(fd, "Invalid file descriptor!");
	// WEB_ASSERT(data, "Invalid data!");
	WEB_ASSERT(event, "Invalid event!");

	struct epoll_event ev;
	ev.events = event;
	ev.data.u64 = data;

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
	EpollData ev_data{
		.fd = static_cast<uint16_t>(redir_fd),
		.cgi_fd = static_cast<uint16_t>(cgi_fd),
		.type = EPOLL_TYPE_CGI
	};

	return AddEpollEvent(cgi_fd, EPOLLIN | EPOLLET, ev_data);
}

int Server::EstablishServerSocket(uint32_t ip, uint16_t port)
{
	char ipStr[INET_ADDRSTRLEN];
	struct in_addr ipAddr;
	ipAddr.s_addr = htonl(ip);
	if (inet_ntop(AF_INET, &ipAddr, ipStr, INET_ADDRSTRLEN) == NULL)
	{
		LOG_ERROR("Failed to convert IP to string format");
		return -1;
	}
	LOG_INFO("Creating server socket on IP: {}, port: {}", ipStr, port);

	//? Logic
	int socketFD = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (socketFD == -1)
	{
		LOG_ERROR("Failed to create server socket!");
		return -1;
	}

	//? SO_REUSEADDR: allows other sockets to bind to an address even if it is already in use
	//? SO_REUSEPORT: allows multiple sockets to bind to the same port and ip
	{
		int reuse = 1;
		if (setsockopt(socketFD, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1)
		{
			LOG_ERROR("Failed to set SO_REUSEADDR!");
			return -1;
		}
	}
	{
		int reuse = 1;
		if (setsockopt(socketFD, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) == -1)
		{
			LOG_ERROR("Failed to set SO_REUSEPORT!");
			return -1;
		}
	}

	struct sockaddr_in socketAddress = {};
	socketAddress.sin_family = AF_INET;
	socketAddress.sin_port = htons(port);
	socketAddress.sin_addr.s_addr = htonl(ip);

	if (bind(socketFD, (struct sockaddr*)&socketAddress, sizeof(socketAddress)) == -1)
	{
		LOG_ERROR("Failed to bind server socket!");
		return -1;
	}

	if (listen(socketFD, SOMAXCONN) == -1)
	{
		LOG_ERROR("Failed to listen on server socket!");
		return -1;
	}

	return socketFD;
}

void Server::Init(const Config& config)
{
	WEB_ASSERT(!s_Instance, "Server already exists!");
	LOG_INFO("Server is starting up!");

	s_Instance = new Server(config);

	ConnectionManager::Init();

	//? EPOLL_CLOEXEC: automatically close the file descriptor when calling exec
	if (CreateEpollInstance() == -1)
	{
		LOG_ERROR("Failed to create epoll file descriptor!");
		Stop();
		return;
	}

	//? assuming that the config parser properly deals with duplicates
	for (const auto& [packedIPPort, serverSettings] : config)
	{
		const uint32_t ip = static_cast<uint32_t>(packedIPPort >> 32);
		const uint16_t port = static_cast<uint16_t>(packedIPPort & 0xFFFF);

		int socket_fd = EstablishServerSocket(ip, port);
		if (socket_fd == -1)
		{
			LOG_ERROR("Failed to establish server socket!");
			Stop();
			return;
		}
		LOG_ERROR("Packed IP: {}", packedIPPort);
		s_Instance->m_ServerSockets64[packedIPPort] = socket_fd;

		EpollData data{
			.fd = static_cast<uint16_t>(socket_fd),
			.cgi_fd = std::numeric_limits<uint16_t>::max(),
			.type = EPOLL_TYPE_SOCKET
		};

		if (AddEpollEvent(socket_fd, EPOLLIN | EPOLLET, data) == -1)
		{
			LOG_ERROR("Failed to add server socket to epoll!");
			Stop();
			return;
		}
	}
	return ;
}

void Server::Shutdown()
{
	WEB_ASSERT(s_Instance, "Server does not exist!");
	LOG_INFO("Server is shutting down!");

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
			EpollData epollData;
			epollData = events[i].data.u64;
			const uint16_t epoll_fd = epollData.fd;
			const uint16_t cgi_fd = epollData.cgi_fd;
			const int epoll_type = epollData.type;

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

	std::string vBuffer;
	vBuffer.reserve(BUFFER_SIZE);
	LOG_ERROR("Capacity: {}", vBuffer.capacity());
	int i = 0;
	uint32_t totalBytes = 0;
	while (true)
	{
		ssize_t n = recv(client, vBuffer.data(), vBuffer.capacity(), 0);

		LOG_INFO("Received bytes: {},\titteration: {},\twith read size of: {}", n, i++, vBuffer.capacity());
		totalBytes += n;
		if (n == -1)
		{
			// if (errno == EAGAIN || errno == EWOULDBLOCK)
			// {
			// 	LOG_INFO("No data available at the moment (non-blocking behavior)");
			// 	// No data available at the moment (non-blocking behavior)
			// 	break;  // Exit loop and wait for epoll to notify again
			// }
			close(client);
			LOG_ERROR("HandleInputEvent read: {}", strerror(errno));
			ConnectionManager::UnregisterClient(client);
			return;
		}
		else if (n == 0)
		{
			LOG_DEBUG("Client closed connection.");
			if (RemoveEpollEvent(s_Instance->m_EpollInstance, client) == -1)
			{
				LOG_ERROR("Failed to remove client socket from epoll!");
				s_Instance->m_Running = false;
			}
			close(client);
			ConnectionManager::UnregisterClient(client);
			return;
		}

		//TODO: find way to avoid this copy
		HttpState state = client.parseRequest(std::string(vBuffer.data(), n));

		// LOG_INFO("State: {}", (int)state);
		if (state == HttpState::Error)
		{
			close(client);
			LOG_ERROR("HandleInputEvent read: {}", strerror(errno));
			ConnectionManager::UnregisterClient(client);
			return;
		}
		else if (state >= HttpState::BodyBegin)
		{
			if (client.GetNewRequest().method == "GET" || client.GetNewRequest().method == "DELETE")
			{
				LOG_INFO("GET/DELETE request received, no body expected");
				break;
			}
			else if (client.GetNewRequest().method == "POST")
			{
				if (client.GetNewRequest().getHeaderValue("content-length").empty())
				{
					LOG_ERROR("Content-Length header is missing");
					close(client);
					ConnectionManager::UnregisterClient(client);
					return;
				}
				ssize_t contentLength = std::stoll(client.GetNewRequest().getHeaderValue("content-length"));
				if (contentLength == client.GetNewRequest().body.size())
				{
					LOG_INFO("Full http contents have been read, no need to recv again");
					break ;
				}
				else
				{
					client.GetNewRequest().body.reserve(contentLength);
					vBuffer.reserve(contentLength - client.GetNewRequest().body.size());
					continue;
				}
			}
		}
	}
	LOG_DEBUG("Total bytes received: {}", totalBytes);

	//TODO: get the correct sevrer config and pass it to the request handler
	Config config = ConfigParser::createDefaultConfig();
	const std::string response = s_Instance->m_RequestHandler.HandleRequest(client);
	// const std::string response = ResponseGenerator::OkResponse();
	if (response.empty())
	{
		LOG_INFO("Request is a CGI request, forwarding to CGI handler...");
		return;
	}


	// LOG_DEBUG("Response:\n{}", response);
	m_ClientResponses[client] = response;
	EpollData data{
		.fd = static_cast<uint16_t>(client),
		.cgi_fd = std::numeric_limits<uint16_t>::max(),
		.type = EPOLL_TYPE_SOCKET
	};

	if (ModifyEpollEvent(s_Instance->m_EpollInstance, client, EPOLLOUT | EPOLLET, data) == -1)
	{
		LOG_ERROR("Failed to modify client socket in epoll!");
		s_Instance->m_Running = false;
		return;
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

		EpollData data{
			.fd = static_cast<uint16_t>(client_fd),
			.cgi_fd = std::numeric_limits<uint16_t>::max(),
			.type = EPOLL_TYPE_SOCKET
		};

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

			EpollData data{
				.fd = static_cast<uint16_t>(epoll_fd),
				.cgi_fd = std::numeric_limits<uint16_t>::max(),
				.type = EPOLL_TYPE_SOCKET
			};


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