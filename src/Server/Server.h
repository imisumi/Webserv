#pragma once

#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#include <memory>
#include <unordered_map>
#include <vector>

#include "RequestHandler.h"
#include "ResponseSender.h"
// #include "Config/ConfigParser.h"
#include <assert.h>

#include "Client.h"
#include "Config/Config.h"
#include "Core/Core.h"
// #define MAX_EVENTS 4096
constexpr int MAX_EVENTS = 4096;
// #define BUFFER_SIZE 12000 * 2
// #define BUFFER_SIZE 4096
// #define BUFFER_SIZE (64 * 1024)
// constexpr size_t BUFFER_SIZE = static_cast<size_t>(64 * 1024);
constexpr size_t BUFFER_SIZE = static_cast<size_t>(2);
static_assert(BUFFER_SIZE > 1, "BUFFER_SIZE must be greater than 1");

class Server
{
public:
	// struct EpollData
	// {
	// 	int fd = -1;
	// 	int cgi_fd = -1;
	// 	uint32_t type = 0;
	// };

	class EpollData
	{
	public:
		uint16_t fd = std::numeric_limits<uint16_t>::max();
		uint16_t cgi_fd = std::numeric_limits<uint16_t>::max();
		int type = -1;
		EpollData& operator=(uint64_t packed)
		{
			this->fd = packed >> 48;
			this->cgi_fd = packed >> 32;
			this->type = packed & 0xFFFF;
			return *this;
		}
		operator uint64_t() const
		{
			WEB_ASSERT(type >= 0, "type is not set");
			return (static_cast<uint64_t>(fd) << 48) | (static_cast<uint64_t>(cgi_fd) << 32) |
				   static_cast<uint64_t>(type);
		}
	} __attribute__((packed));

	static_assert(sizeof(EpollData) == 8);

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
		uint32_t address = 0;
	};

	static Server& Get();
	static void Init(const Config& config);
	static void Shutdown();
	static void Run();
	static void Stop();
	static bool IsRunning();

	Server(const Server&) = delete;
	Server& operator=(const Server&) = delete;

	int GetEpollInstance() const { return m_EpollInstance; }

	void SetCgiToClientMap(int cgi_fd, int client_fd) { m_CgiToClientMap[cgi_fd] = client_fd; }

	int GetClientFromCgi(int cgi_fd) { return m_CgiToClientMap[cgi_fd]; }

	static int AddEpollEvent(int fd, int event, EpollData data);
	static int ModifyEpollEvent(int epollFD, int fd, int event, EpollData data);
	static int RemoveEpollEvent(int epollFD, int fd);

	std::unordered_map<pid_t, int> childProcesses;
	// std::unordered_map<int, std::string> m_ClientResponses; // Maps client socket to response data

	static int CgiRedirect(int cgi_fd, int redir_fd);
	static void RegisterCgiProcess(pid_t pid, int client_fd) { Get().childProcesses[pid] = client_fd; }
	static void DeregisterCgiProcess(pid_t pid) { Get().childProcesses.erase(pid); }

	static ServerSettings* GetServerSettings(uint64_t packedIpPort) { return Get().m_Config[packedIpPort][0]; }

	const std::unordered_map<uint64_t, int>& GetServerSockets() const { return m_ServerSockets64; }

private:
	Server(const Config& config);
	~Server();

	static int CreateEpollInstance();
	static int EstablishServerSocket(uint32_t ip, uint16_t port);

	int isServerSocket(int fd);

	void HandleSocketInputEvent(Client& client);
	void HandleOutputEvent(Client& client, int fd);

	void HandleCgiInputEvent(int cgi_fd, int client_fd, Client& client);

	int CreateSocket(const SocketSettings& settings);
	int SetSocketOptions(int fd, const SocketOptions& options);
	int BindSocket(int fd, const SocketAddressConfig& config);
	int ListenOnSocket(int socket_fd, int backlog);

	static constexpr int m_MaxPollEvents = 1024;

	const Config& m_Config;
	// Config m_Config;

	bool m_Running = true;

	// int m_ServerSocket = -1;
	std::vector<int> m_ServerPorts;

	// int port, int socket fd
	std::unordered_map<uint64_t, int> m_ServerSockets64;
	int m_EpollInstance = -1;

	struct sockaddr_in m_SockAddress;
	RequestHandler m_RequestHandler;
	ResponseSender m_ResponseSender;

	std::unordered_map<int, EpollData> m_FdEventMap;
	std::unordered_map<int, int> m_CgiToClientMap;

	std::array<struct epoll_event, MAX_EVENTS> m_Events;
};