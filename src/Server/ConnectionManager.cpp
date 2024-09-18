#include "ConnectionManager.h"

#include "Server.h"

// #include <

#include "Core/Core.h"

#include <sys/socket.h>

#define EPOLL_TYPE_SOCKET   BIT(0)
#define EPOLL_TYPE_CGI      BIT(1)
#define EPOLL_TYPE_STDIN    BIT(2)
#define EPOLL_TYPE_STDOUT   BIT(3)

#define PACK_U64(fd, metadata)   (((uint64_t)(fd) << 32) | (metadata))


static ConnectionManager* s_Instance = nullptr;


ConnectionManager::ConnectionManager()
{
}

ConnectionManager::~ConnectionManager()
{
}

ConnectionManager& ConnectionManager::Get()
{
	WEB_ASSERT(s_Instance, "ConnectionManager does not exist!");

	return *s_Instance;
}

void ConnectionManager::Init()
{
	WEB_ASSERT(!s_Instance, "ConnectionManager already exists!");

	s_Instance = new ConnectionManager();
}

void ConnectionManager::Shutdown()
{
	WEB_ASSERT(s_Instance, "ConnectionManager does not exist!");

	delete s_Instance;
	s_Instance = nullptr;
}

Client ConnectionManager::AcceptConnection(int socket_fd)
{
	WEB_ASSERT(s_Instance, "ConnectionManager does not exist!");

	struct sockaddr_in client_addr;
	socklen_t client_addr_len = sizeof(client_addr);

	Client client = accept(socket_fd, (struct sockaddr*)&client_addr, &client_addr_len);
	if ((int)client == -1)
	{
		LOG_ERROR("Failed to accept connection!");
		return Client();
	}

	client.Initialize(client_addr);
	LOG_INFO("New connection from: {}, client socket: {}, client port: {}, server port: {}", 
		client.GetClientAddress(), (int)client, client.GetClientPort(), client.GetServerPort());


	// make the socket non-blocking
	int flags = fcntl((int)client, F_GETFL, 0);
	if (flags == -1)
	{
		LOG_ERROR("Failed to get socket flags!");
		close((int)client);
		return -1;
	}

	if (fcntl((int)client, F_SETFL, flags | O_NONBLOCK) == -1)
	{
		LOG_ERROR("Failed to set socket flags!");
		close((int)client);
		return -1;
	}




	struct Server::EpollData *data = new Server::EpollData();
	data->fd = (int)client;
	data->type = EPOLL_TYPE_SOCKET;
	data->cgi_fd = -1;

	// if (s_Instance->AddEpollEvent(s_Instance->m_EpollInstance, client, EPOLLIN | EPOLLET, EPOLL_TYPE_SOCKET) == -1)
	if (Server::AddEpollEvent(Server::Get().GetEpollInstance(), (int)client, EPOLLIN | EPOLLET, data) == -1)
	{
		LOG_ERROR("Failed to add client socket to epoll!");
		// m_Clients.erase(client);
		close((int)client);
		return -1;
	}

	client.SetEpollInstance(Server::Get().GetEpollInstance());


	uint64_t packedIpPort = PACK_U64(client.GetClientAddress(), client.GetServerPort());

	ServerSettings* settings = Server::GetServerSettings(packedIpPort);
	client.SetConfig(settings);

	return client;
}

void ConnectionManager::RegisterClient(int fd, Client client)
{
	WEB_ASSERT(s_Instance, "ConnectionManager does not exist!");

	s_Instance->m_ConnectedClients[fd] = client;
}

void ConnectionManager::UnregisterClient(int fd)
{
	WEB_ASSERT(s_Instance, "ConnectionManager does not exist!");

	s_Instance->m_ConnectedClients.erase(fd);
}

Client ConnectionManager::GetClient(int fd)
{
	WEB_ASSERT(s_Instance, "ConnectionManager does not exist!");

	return s_Instance->m_ConnectedClients[fd];
}

uint32_t ConnectionManager::GetConnectedClients()
{
	WEB_ASSERT(s_Instance, "ConnectionManager does not exist!");

	return s_Instance->m_ConnectedClients.size();
}