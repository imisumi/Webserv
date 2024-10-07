#include "ConnectionManager.h"

#include "Server.h"

#include "Utils/Utils.h"

// #include <
#include <fcntl.h>

#include <unistd.h>
#include "Core/Core.h"

#include <sys/socket.h>

#include "Constants.h"

static ConnectionManager* s_Instance = nullptr;

ConnectionManager::ConnectionManager() {}

ConnectionManager::~ConnectionManager() {}

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
		return Client();
	}

	client.Initialize(client_addr, socket_fd);

	LOG_INFO("New connection from: {}, client socket: {}, client port: {}, server port: {}, server address: {}",
			 client.GetClientAddress(), (int)client, client.GetClientPort(), client.GetServerPort(),
			 client.GetServerAddress());

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

	Server::EpollData data{
		.fd = static_cast<uint16_t>(client), .cgi_fd = std::numeric_limits<uint16_t>::max(), .type = EPOLL_TYPE_SOCKET};

	if (Server::AddEpollEvent((int)client, EPOLLIN | EPOLLET, data) == -1)
	{
		LOG_ERROR("Failed to add client socket to epoll!");
		// m_Clients.erase(client);
		close((int)client);
		return -1;
	}

	client.SetEpollInstance(Server::Get().GetEpollInstance());

	// uint64_t packedIpPort = PACK_U64(client.GetClientAddress(), client.GetServerPort());
	uint64_t packedIpPort = Utils::packIpAndPort(client.GetServerAddress(), client.GetServerPort());

	std::cout << std::endl;

	ServerSettings* settings = Server::GetServerSettings(packedIpPort);
	LOG_INFO("Server name: {}", settings->GetServerName());
	std::filesystem::path path("/");
	const std::vector<std::string>& indexes = settings->GetIndexList(path);
	for (const auto& index : indexes)
	{
		LOG_INFO("Index: {}", index);
	}
	client.SetServerConfig(settings);

	return client;
}

void ConnectionManager::RegisterClient(int fd, Client& client)
{
	WEB_ASSERT(s_Instance, "ConnectionManager does not exist!");

	LOG_DEBUG("Registering client: {}", fd);

	s_Instance->m_ConnectedClients[fd] = client;
}

void ConnectionManager::UnregisterClient(int fd)
{
	WEB_ASSERT(s_Instance, "ConnectionManager does not exist!");

	LOG_DEBUG("Unregistering client: {}", fd);
	s_Instance->m_ConnectedClients.erase(fd);
}

const Client& ConnectionManager::GetClient(int fd)
{
	WEB_ASSERT(s_Instance, "ConnectionManager does not exist!");

	return s_Instance->m_ConnectedClients[fd];
}

Client& ConnectionManager::GetClientRef(int fd)
{
	WEB_ASSERT(s_Instance, "ConnectionManager does not exist!");

	return s_Instance->m_ConnectedClients[fd];
}

uint32_t ConnectionManager::GetConnectedClients()
{
	WEB_ASSERT(s_Instance, "ConnectionManager does not exist!");

	return s_Instance->m_ConnectedClients.size();
}