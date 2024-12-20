// #include "pch.h"

#include "Server.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <thread>

#include "ConnectionManager.h"
#include "Constants.h"
#include "Core/Core.h"
#include "Response/ResponseGenerator.h"
#include "ResponseSender.h"
#include "Utils/Utils.h"

// include socket headers
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

// include epoll headers
#include <sys/epoll.h>

#include <cerrno>
// #include <string.h>
#include <cstring>

static Server* s_Instance = nullptr;

Server::Server(const Config& config) : m_Config(config) {}

Server::~Server() {}

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

int Server::AddEpollEvent(int fd, uint32_t event, EpollData data)
{
	struct epoll_event ev = {};
	ev.events = event;
	ev.data.u64 = data;

	return epoll_ctl(s_Instance->m_EpollInstance, EPOLL_CTL_ADD, fd, &ev);
}

int Server::RemoveEpollEvent(int fd)
{
	s_Instance->m_FdEventMap.erase(fd);

	return epoll_ctl(s_Instance->m_EpollInstance, EPOLL_CTL_DEL, fd, nullptr);
}

int Server::ModifyEpollEvent(int fd, uint32_t event, EpollData data)
{
	WEB_ASSERT(s_Instance->m_EpollInstance, "Invalid epoll file descriptor!");
	WEB_ASSERT(fd, "Invalid file descriptor!");
	// WEB_ASSERT(data, "Invalid data!");
	WEB_ASSERT(event, "Invalid event!");

	struct epoll_event ev = {};
	ev.events = event;
	ev.data.u64 = data;

	return epoll_ctl(s_Instance->m_EpollInstance, EPOLL_CTL_MOD, fd, &ev);
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
 * @param cgi_fd The file descriptor for the CGI process. This is the descriptor
 * from which data will be read.
 * @param redir_fd The file descriptor where the CGI output should be
 * redirected. This is the destination where the data from `cgi_fd` will be
 * written.
 *
 * @return An integer value indicating the success or failure of adding the
 * epoll event. Typically, this will be 0 on success and a negative error code
 * on failure.
 */
int Server::CgiRedirect(int cgi_fd, int redir_fd)
{
	EpollData ev_data{
		.fd = static_cast<uint16_t>(redir_fd), .cgi_fd = static_cast<uint16_t>(cgi_fd), .type = EPOLL_TYPE_CGI};

	return AddEpollEvent(cgi_fd, EPOLLIN | EPOLLET, ev_data);
}

int Server::EstablishServerSocket(uint32_t ip, uint16_t port)
{
	// char ipStr[INET_ADDRSTRLEN];
	std::array<char, INET_ADDRSTRLEN> ipStr = {};
	struct in_addr ipAddr = {};
	ipAddr.s_addr = htonl(ip);
	// if (inet_ntop(AF_INET, &ipAddr, ipStr, INET_ADDRSTRLEN) == NULL)
	if (inet_ntop(AF_INET, &ipAddr, ipStr.data(), INET_ADDRSTRLEN) == nullptr)
	{
		Log::error("Failed to convert IP to string format");
		return -1;
	}
	// Log::info("Creating server socket on IP: {}, port: {}", ipStr, port);
	Log::info("Creating server socket on IP: {}, port: {}", ipStr.data(), port);

	//? Logic
	int socketFD = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (socketFD == -1)
	{
		Log::error("Failed to create server socket!");
		return -1;
	}

	//? SO_REUSEADDR: allows other sockets to bind to an address even if it is
	// already in use ? SO_REUSEPORT: allows multiple sockets to bind to the
	// same port and ip
	{
		int reuse = 1;
		if (setsockopt(socketFD, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1)
		{
			Log::error("Failed to set SO_REUSEADDR!");
			return -1;
		}
	}
	{
		int reuse = 1;
		if (setsockopt(socketFD, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) == -1)
		{
			Log::error("Failed to set SO_REUSEPORT!");
			return -1;
		}
	}

	struct sockaddr_in socketAddress = {};
	socketAddress.sin_family = AF_INET;
	socketAddress.sin_port = htons(port);
	socketAddress.sin_addr.s_addr = htonl(ip);

	if (bind(socketFD, reinterpret_cast<struct sockaddr*>(&socketAddress), sizeof(socketAddress)) == -1)
	{
		Log::error("Failed to bind server socket!");
		return -1;
	}

	if (listen(socketFD, SOMAXCONN) == -1)
	{
		Log::error("Failed to listen on server socket!");
		return -1;
	}

	return socketFD;
}

void Server::Init(const Config& config)
{
	WEB_ASSERT(!s_Instance, "Server already exists!");
	Log::info("Server is starting up!");

	s_Instance = new Server(config);

	ConnectionManager::Init();

	//? EPOLL_CLOEXEC: automatically close the file descriptor when calling exec
	if (CreateEpollInstance() == -1)
	{
		Log::error("Failed to create epoll file descriptor!");
		Stop();
		return;
	}

	//? assuming that the config parser properly deals with duplicates
	for (const auto& [packedIPPort, serverSettings] : config)
	{
		const uint32_t ipAddress = static_cast<uint32_t>(packedIPPort >> 32);
		const uint16_t port = static_cast<uint16_t>(packedIPPort & 0xFFFF);

		int socket_fd = EstablishServerSocket(ipAddress, port);
		if (socket_fd == -1)
		{
			Log::error("Failed to establish server socket!");
			Stop();
			return;
		}
		Log::error("Packed IP: {}", packedIPPort);
		s_Instance->m_ServerSockets64[packedIPPort] = socket_fd;

		EpollData data{.fd = static_cast<uint16_t>(socket_fd),
					   .cgi_fd = std::numeric_limits<uint16_t>::max(),
					   .type = EPOLL_TYPE_SOCKET};

		if (AddEpollEvent(socket_fd, EPOLLIN | EPOLLET, data) == -1)
		{
			Log::error("Failed to add server socket to epoll!");
			Stop();
			return;
		}
	}

	s_Instance->m_Api.addApiRoute("/api/v1/images",
								  []()
								  {
									  std::filesystem::path databaseImagePath =
										  std::filesystem::current_path() / "database" / "images";
									  return Api::getImages(databaseImagePath);	 // Return the string response
								  });
	s_Instance->m_Api.addApiRoute("/api/v1/files",
								  []()
								  {
									  std::filesystem::path databaseFilePath =
										  std::filesystem::current_path() / "database" / "files";
									  return Api::getFiles(databaseFilePath);  // Return the string response
								  });
}

void Server::Shutdown()
{
	WEB_ASSERT(s_Instance, "Server does not exist!");
	Log::info("Server is shutting down!");

	close(s_Instance->m_EpollInstance);

	delete s_Instance;
	s_Instance = nullptr;
}

int Server::isServerSocket(int fd)
{
	for (const auto& [packedIpPort, socket] : s_Instance->m_ServerSockets64)
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

	Log::info("Server is running!");

	struct epoll_event events[MAX_EVENTS];

	while (s_Instance->m_Running)
	{
		// Log::info(
		// 	"-----------------------------------------------------------------"
		// 	"-------------------------");
		// Log::info("Waiting for events...");
		// Log::info(
		// 	"-----------------------------------------------------------------"
		// 	"-------------------------\n\n");

		//? EPOLL_WAIT: wait for events on the epoll instance
		const int eventCount = epoll_wait(s_Instance->m_EpollInstance, events, MAX_EVENTS, EPOLL_TIMEOUT);
		if (eventCount == -1)
		{
			if (errno == EINTR)
			{
				// Interrupted by a signal, such as SIGCHLD, retry epoll_wait
				std::cout << "epoll_wait interrupted by signal, retrying..." << std::endl;
				continue;
			}
			Log::error("epoll_wait: {}", strerror(errno));
			return;
		}
		for (int i = 0; i < eventCount; i++)
		{
			EpollData epollData;
			epollData = events[i].data.u64;
			const uint16_t epoll_fd = epollData.fd;
			const uint16_t cgi_fd = epollData.cgi_fd;
			const int epoll_type = epollData.type;
			const uint32_t event = events[i].events;

			if (((event & EPOLLIN) != 0U) && (s_Instance->isServerSocket(epoll_fd) != -1))
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
						// Log::critical("Failed to accept connection!");
						Log::error("Failed to accept connection! errno: {}, error: {}", errno, strerror(errno));

						break;
					}
					ConnectionManager::RegisterClient((int)newClient, newClient);
				}
				continue;
			}
			Client& client = ConnectionManager::GetClientRef(epoll_fd);
			client.GetTimer().reset();
			Log::info("epoll fd: {}, client socket: {}", epoll_fd, (int)client);
			if ((event & EPOLLIN) != 0U)
			{
				Log::debug("Handling input event...");
				if (epoll_type == EPOLL_TYPE_SOCKET)
				{
					s_Instance->HandleSocketInputEvent(client);
				}
				else if (epoll_type == EPOLL_TYPE_CGI)
				{
					s_Instance->HandleCgiInputEvent(cgi_fd, epoll_fd, client);
				}
			}
			else if ((event & EPOLLOUT) != 0U)
			{
				Log::debug("Handling output event...");

				if (epoll_type == EPOLL_TYPE_SOCKET)
				{
					s_Instance->HandleOutputEvent(client, epoll_fd);
				}
			}
			else if ((event & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) != 0U)
			{
				// Log::info("Handling error event...");
				// Log::info("Event type: {}", (int)events[i].events);
				Log::critical("Handling error event...");

				// close(epoll_fd);
				// continue;
			}
			else
			{
				Log::critical("Unhandled event type");
			}
		}






		for (auto& [fd, client] : ConnectionManager::GetConnectedClientsMap())
		{

			if (client.GetRequest().GetState() == HttpState::Start)
			{
				continue;
			}
			Log::info("Client: {}, elapsed time: {}", (int)fd, client.GetTimer().elapsedMillis());

			if (client.GetTimer().elapsedMillis() > CLIENT_TIMEOUT)
			{
				Log::info("Client timeout, closing connection...");

				client.SetBytesSent(0);
				client.SetResponse(ResponseGenerator::Timeout());

				EpollData data{.fd = static_cast<uint16_t>(client),
							   .cgi_fd = std::numeric_limits<uint16_t>::max(),
							   .type = EPOLL_TYPE_SOCKET};
				if (ModifyEpollEvent(fd, EPOLLOUT | EPOLLET, data) == -1)
				{
					Log::error("Failed to modify client socket in epoll!");
					Stop();
				}
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
	Log::info("Handling socket input event...");

	std::array<char, BUFFER_SIZE> buffer = {};

	ssize_t n = recv((int)client, buffer.data(), BUFFER_SIZE, 0);
	if (n == -1)
	{
		close(client);
		Log::error("HandleInputEvent read: {}", strerror(errno));
		client.reset();
		ConnectionManager::UnregisterClient(client);
		return;
	}

	if (n == 0)
	{
		Log::debug("Client closed connection.");
		if (RemoveEpollEvent(client) == -1)
		{
			Log::error("Failed to remove client socket from epoll!");
			s_Instance->Stop();
		}
		close(client);
		client.reset();
		ConnectionManager::UnregisterClient(client);
		return;
	}

	// Log::info("Raw data\n {}", buffer.data());
	// for (char c : buffer)
	// {
	// 	if (c == '\r')
	// 	{
	// 		std::cout << "\\r";
	// 	}
	// 	else if (c == '\n')
	// 	{
	// 		std::cout << "\\n";
	// 		std::cout << c;
	// 	}
	// 	else
	// 	{
	// 		std::cout << c;
	// 	}
	// }

	// TODO: find way to avoid this copy
	const std::string bufferStr(buffer.data(), n);
	HttpState state = client.parseRequest(bufferStr);

	// HttpState state = client.parseRequest(buffer);
	if (state == HttpState::Error)
	{
		close(client);
		Log::error("HttpParser state Error");
		client.reset();
		ConnectionManager::UnregisterClient(client);
		return;
	}
	if (state < HttpState::Body)
	{
		Log::error("Failed to read entire request at once, reregister client with epoll");

		EpollData data{.fd = static_cast<uint16_t>(client),
					   .cgi_fd = std::numeric_limits<uint16_t>::max(),
					   .type = EPOLL_TYPE_SOCKET};

		if (ModifyEpollEvent(client, EPOLLIN | EPOLLET, data) == -1)
		{
			Log::error("Failed to modify client socket in epoll!");
			s_Instance->Stop();
		}
		return;
	}

	if (client.GetRequest().method == "GET" || client.GetRequest().method == "DELETE" ||
		client.GetRequest().method == "HEAD")
	{
		Log::info("GET/DELETE/HEAD request received, no body expected");
	}
	else if (client.GetRequest().method == "POST")
	{
		if (client.GetRequest().getHeaderValue("content-length").empty() &&
			client.GetRequest().transferEncoding == HttpRequest::TransferEncoding::NONE)
		{
			Log::error("Content-Length header is missing");
			close(client);
			client.reset();
			ConnectionManager::UnregisterClient(client);
			return;
		}
		if (client.GetRequest().transferEncoding == HttpRequest::TransferEncoding::CHUNKED)
		{
			Log::error("Failed to read entire request at once, reregister client with epoll");

			if (client.GetRequest().GetState() != HttpState::Done)
			{
				EpollData data{.fd = static_cast<uint16_t>(client),
							   .cgi_fd = std::numeric_limits<uint16_t>::max(),
							   .type = EPOLL_TYPE_SOCKET};

				if (ModifyEpollEvent(client, EPOLLIN | EPOLLET, data) == -1)
				{
					Log::error("Failed to modify client socket in epoll!");
					Stop();
				}
				return;
			}
		}
		else
		{
			ssize_t contentLength = std::stoll(client.GetRequest().getHeaderValue("content-length"));
			if (client.GetRequest().body.size() < static_cast<std::string::size_type>(contentLength))
			{
				Log::error("Failed to read entire request at once, reregister client with epoll");

				client.GetRequest().body.reserve(contentLength);

				EpollData data{.fd = static_cast<uint16_t>(client),
							   .cgi_fd = std::numeric_limits<uint16_t>::max(),
							   .type = EPOLL_TYPE_SOCKET};

				if (ModifyEpollEvent(client, EPOLLIN | EPOLLET, data) == -1)
				{
					Log::error("Failed to modify client socket in epoll!");
					Stop();
				}
				return;
			}
		}
	}
	else
	{
		Log::error("Unsupported HTTP method: {}", client.GetRequest().method);
		close(client);
		client.reset();
		ConnectionManager::UnregisterClient(client);
		return;
	}
	const std::string response = s_Instance->m_RequestHandler.HandleRequest(client);
	if (response.empty())
	{
		Log::info("Request is a CGI request, forwarding to CGI handler...");
		return;
	}

	client.SetBytesSent(0);
	client.SetResponse(response);
	EpollData data{
		.fd = static_cast<uint16_t>(client), .cgi_fd = std::numeric_limits<uint16_t>::max(), .type = EPOLL_TYPE_SOCKET};

	if (ModifyEpollEvent(client, EPOLLOUT | EPOLLET, data) == -1)
	{
		Log::error("Failed to modify client socket in epoll!");
		// s_Instance->m_Running = false;
		Stop();
		return;
	}
}

void Server::HandleCgiInputEvent(int cgi_fd, int client_fd, Client& client)
{
	Log::debug("Handling CGI input event...");

	std::array<char, BUFFER_SIZE> buffer = {};
	ssize_t n = read(cgi_fd, buffer.data(), buffer.size() - 1);
	if (n > 0)
	{
		const std::string output = std::string(buffer.data(), n);

		size_t header_end = output.find("\r\n\r\n");
		if (header_end != std::string::npos)
		{
			size_t bodyLength = output.size() - header_end - 4;
			std::string httpResponse = "HTTP/1.1 200 OK\r\n";
			httpResponse += "Content-Length: " + std::to_string(bodyLength) + "\r\n";
			httpResponse += "connection: " + client.GetRequest().getHeaderValue("connection") + "\r\n";
			httpResponse += output;

			Log::debug("Response:\n{}", httpResponse);

			client.SetResponse(httpResponse);
		}
		else
		{
			Log::error("CGI script did not produce valid headers");
			Log::error("Output:\n{}", output);
			client.SetResponse(ResponseGenerator::InternalServerError());
		}

		EpollData data{.fd = static_cast<uint16_t>(client_fd),
					   .cgi_fd = std::numeric_limits<uint16_t>::max(),
					   .type = EPOLL_TYPE_SOCKET};

		if (s_Instance->ModifyEpollEvent(client_fd, EPOLLOUT | EPOLLET, data) == -1)
		{
			Log::error("Failed to modify client socket in epoll!");
			s_Instance->Stop();
		}

		RemoveEpollEvent(cgi_fd);
		close(cgi_fd);
	}
	else if (n == 0)
	{
		Log::debug("Client closed connection.");

		if (RemoveEpollEvent(client_fd) == -1)
		{
			Log::error("Failed to remove client socket from epoll!");
			s_Instance->Stop();
		}
		close(client_fd);
	}
	else
	{
		close(client_fd);
		Log::error("HandleInputEvent read: {}", strerror(errno));
		// s_Instance->RemoveClient(client_fd);
		// close(cgi_fd);
		// Log::info("Total Clients: {}", s_Instance->GetClientCount());
		ConnectionManager::UnregisterClient(client_fd);
		Log::info("Total Clients: {}", ConnectionManager::GetConnectedClients());
		close(cgi_fd);
	}
}

void Server::HandleOutputEvent(Client& client, int epoll_fd)
{
	WEB_ASSERT(!client.GetResponse().empty(), "Response is empty!");

	const std::string& response = client.GetResponse();

	if (response.size() < BUFFER_SIZE)
		Log::info("response:\n{}", response);
	ssize_t bytes = ResponseSender::sendResponse(client);
	if (bytes == -1)
	{
		Log::error("send: {}", strerror(errno));
		if (RemoveEpollEvent(epoll_fd) == -1)
		{
			Log::error("Failed to remove client socket from epoll!");
			Stop();
		}
		close(epoll_fd);
		return;
	}
	const ssize_t clientSentBytes = client.GetBytesSent();
	client.SetBytesSent(clientSentBytes + bytes);
	if (client.GetBytesSent() < response.size())
	{
		Log::error("Failed to send entire response to client: {}", (int)client);

		//? regegister the client for EPOLLOUT events
		EpollData data{.fd = static_cast<uint16_t>(epoll_fd),
					   .cgi_fd = std::numeric_limits<uint16_t>::max(),
					   .type = EPOLL_TYPE_SOCKET};

		if (ModifyEpollEvent(epoll_fd, EPOLLOUT | EPOLLET, data) == -1)
		{
			Log::error("Failed to modify client socket in epoll!");
			Stop();
		}
		return;
	}
	const HttpRequest& request = client.GetRequest();
	Log::release("{} {} {} {}", request.method, request.uri, response.substr(0, response.find_first_of("\r\n")), request.getHeaderValue("host"));
	Log::debug("Sent response to client: {}", epoll_fd);

	EpollData data{.fd = static_cast<uint16_t>(epoll_fd),
				   .cgi_fd = std::numeric_limits<uint16_t>::max(),
				   .type = EPOLL_TYPE_SOCKET};

	// TODO: find a better way of doing this
	if (response.find("connection: close") != std::string::npos)
	{
		Log::debug("Closing connection for client socket: {}", epoll_fd);
		if (RemoveEpollEvent(epoll_fd) == -1)
		{
			Log::error("Failed to remove client socket from epoll!");
			s_Instance->m_Running = false;
		}
		close(epoll_fd);
	}
	else if (ModifyEpollEvent(epoll_fd, EPOLLIN | EPOLLET, data) == -1)
	{
		Log::error("Failed to modify client socket in epoll!");
		s_Instance->m_Running = false;
	}

	client.reset();
}
