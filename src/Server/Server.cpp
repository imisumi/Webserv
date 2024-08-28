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

	s_Instance->m_RequestHandler = std::make_shared<RequestHandler>();
	// s_Instance->m_ResponseSender = std::make_shared<ResponseSender>();


	//? EPOLL_CLOEXEC: automatically close the file descriptor when calling exec
	s_Instance->m_EpollFD = epoll_create1(EPOLL_CLOEXEC);
	WEB_ASSERT(s_Instance->m_EpollFD, "Failed to create epoll file descriptor!");


	// SOCK_STREAM for TCP && SOCK_NONBLOCK for non-blocking (alternative to fcntl)
	s_Instance->m_ServerSocket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	// s_Instance->m_ServerSocket = socket(AF_INET, SOCK_STREAM, 0);
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

static std::string readFileContents(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + path.string());
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
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


	std::string clientIP[MAX_EVENTS];
	while (s_Instance->m_Running)
	{
		// LOG_TRACE("Server is running...");

		std::this_thread::sleep_for(std::chrono::milliseconds(500));

		//? EPOLL_WAIT: wait for events on the epoll instance
		int eventCount = epoll_wait(s_Instance->m_EpollFD, events, MAX_EVENTS, -1);
		if (eventCount == -1)
		{
			LOG_ERROR("epoll_wait: {}", strerror(errno));
			return;
		}
		// LOG_DEBUG("Event count: {}", eventCount);
		// LOG_INFO("Event count: {}", eventCount);
		// for (int i = 0; i < eventCount; i++)
		for (int i = 0; i < eventCount; ++i)
		{
			if (events[i].data.fd == s_Instance->m_ServerSocket)
			{
				LOG_DEBUG("New connection!");
				struct sockaddr_in clientAddress;
				socklen_t clientAddressLength = sizeof(clientAddress);

				// Handle new incoming connection
				int client_socket = accept(s_Instance->m_ServerSocket, (struct sockaddr*)&clientAddress, &clientAddressLength);
				if (client_socket == -1)
				{
					LOG_ERROR("accept: {}", strerror(errno));
					continue;
				}

				int flags = fcntl(client_socket, F_GETFL, 0);
				fcntl(client_socket, F_SETFL, flags | O_NONBLOCK);

				LOG_DEBUG("Connection accepted from: {}", inet_ntoa(clientAddress.sin_addr));
				clientIP[i] = inet_ntoa(clientAddress.sin_addr);
				s_Instance->m_Connections++;
				LOG_DEBUG("Connected clients: {}", s_Instance->m_Connections);

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
					LOG_DEBUG("Received data:\n{}", buffer);
					const std::string bufferStr(buffer);

					std::string bufferFirstLine = bufferStr.substr(0, bufferStr.find("\n"));


					// LOG_INFO("webserv     |  {} - - {} \"{}\"", clientIP[i], getFormattedUTCTime(), bufferFirstLine);

					s_Instance->m_RequestHandler->handleRequest(bufferStr, events[i].data.fd);
				// else if (n == 0 || (events[i].events & (EPOLLRDHUP | EPOLLHUP))) 
				}
				else if (n == 0)
				{
					close(events[i].data.fd);
					// epoll_ctl(s_Instance->m_EpollFD, EPOLL_CTL_DEL, events[i].data.fd, NULL);
					// if (epoll_ctl(s_Instance->m_EpollFD, EPOLL_CTL_DEL, events[i].data.fd, NULL) == -1) {
					// 	LOG_ERROR("Failed to remove socket from epoll instance: {}", strerror(errno));
					// }
					s_Instance->m_Connections--;
					LOG_DEBUG("Client disconnected. Connected clients: {}", s_Instance->m_Connections);
				}
				else if ((errno == EAGAIN || errno == EWOULDBLOCK)) //TODOL not allowed I think
				{
					continue;
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