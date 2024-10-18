#include "Cgi.h"
#include "Server/Server.h"
#include "Server/ConnectionManager.h"

#include "Core/Log.h"
#include <unistd.h>
#include <sys/wait.h>

#include "Server/Response/ResponseGenerator.h"


#include "Constants.h"

#include <fcntl.h> // For fcntl

#include <chrono>

void handle_alarm(int signal) {
	// This signal handler does nothing but allows us to handle timeouts
}

static const char s_TimeoutErrorResponse[] =
    "HTTP/1.1 504 Gateway Timeout\r\n"
    "Content-Length: 41\r\n" // Length of the body below
    "Connection: close\r\n"
    "Content-Type: text/plain\r\n"
    "\r\n"
    "504 Gateway Timeout: The server timed out"; // Body of the response


void sigchld_handler(int signo)
{
	int status;
	pid_t pid;

	// Use WNOHANG to only reap child processes that have exited
	while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
	{
		// Server::DeregisterCgiProcess(pid);
		if (WIFEXITED(status))
		{
			int exit_code = WEXITSTATUS(status);
			std::cout << "Child process: " << pid << " exited with status: " << exit_code << std::endl;
			
			if (exit_code == EXIT_FAILURE)
			{
				std::cerr << "Child process: " << pid << " failed with exit code: " << exit_code << std::endl;

				Server& server = Server::Get();
				int client_fd = server.childProcesses[pid];
				Client& client = ConnectionManager::GetClientRef(client_fd);
				Log::info("Child process: {}, Client FD: {}", pid, client_fd);

				Server::EpollData data{
					.fd = static_cast<uint16_t>(client_fd),
					.cgi_fd = std::numeric_limits<uint16_t>::max(),
					.type = EPOLL_TYPE_SOCKET
				};

				Server::ModifyEpollEvent(client_fd, EPOLLOUT | EPOLLET, data);

				// server.m_ClientResponses[client_fd] = ResponseGenerator::InternalServerError();
				client.SetResponse(ResponseGenerator::GenerateErrorResponse(HTTPStatusCode::InternalServerError, client));
				// client.SetResponse(ResponseGenerator::InternalServerError());
			}
		}
		else if (WIFSIGNALED(status))
		{
			int signal_num = WTERMSIG(status);

			if (signal_num == SIGALRM)
			{
				std::cerr << "Child process " << pid << " timed out with signal: " << signal_num << std::endl;
				Server& server = Server::Get();

				int client_fd = server.childProcesses[pid];
				// Client client = server.GetClient(client_fd);
				Client& client = ConnectionManager::GetClientRef(client_fd);
				Log::info("Child process: {}, Client FD: {}", pid, client_fd);

				// struct Server::EpollData *ev_data = server.GetEpollData(client_fd);
				Log::info("Client FD: {}", client_fd);
				Server::EpollData data{
					.fd = static_cast<uint16_t>(client_fd),
					.cgi_fd = std::numeric_limits<uint16_t>::max(),
					.type = EPOLL_TYPE_SOCKET
				};
				Server::ModifyEpollEvent(client_fd, EPOLLOUT | EPOLLET, data);

				std::string response = s_TimeoutErrorResponse;
				// server.m_ClientResponses[client_fd] = response;
				client.SetResponse(response);

				// Handle CGI timeout here
				// Send timeout error page to client
			}
			else
			{
				std::cout << "Child process " << pid << " was terminated by signal: " << signal_num << std::endl;
			}
		}
		Server::DeregisterCgiProcess(pid);
	}
}


// Custom SIGCHLD handler to reap the specific child that has exited

// void sigchld_handler(int signo)
// {
// 	int status;
// 	pid_t pid;

// 	// Use WNOHANG to only reap child processes that have exited
// 	while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
// 		if (WIFEXITED(status)) {
// 			std::cout << "Child process " << pid << " exited with status: " << WEXITSTATUS(status) << std::endl;
// 		} else if (WIFSIGNALED(status)) {
// 			std::cout << "Child process " << pid << " was terminated by signal: " << WTERMSIG(status) << std::endl;
// 		}

// 		// Remove the child from the list of running processes
// 		// childProcesses.erase(pid);
// 	}
// }



std::string Cgi::executeCGI(const Client& client, const HttpRequest& request)
{
	// std::string path = request.getUri().string();
	// std::string path = request.path.string();
	// std::string path = request.mappedPath.string();
	std::string path = client.GetRequest().mappedPath.string();
	Log::info("Executing CGI script: {}", path);
	int pipefd[2];
		
	if (pipe(pipefd) == -1)
	{
		perror("pipe");
		return ResponseGenerator::GenerateErrorResponse(HTTPStatusCode::InternalServerError, client);
	}

	int flags = fcntl(pipefd[READ_END], F_GETFL);
	fcntl(pipefd[READ_END], F_SETFL, flags | O_NONBLOCK);

	int flags2 = fcntl(pipefd[WRITE_END], F_GETFL);
	fcntl(pipefd[WRITE_END], F_SETFL, flags2 | O_NONBLOCK);


	// Server::CgiRedirect(pipefd[READ_END], (int)client);
// #if 0
	Server::EpollData data{
			.fd = static_cast<uint16_t>(client),
			.cgi_fd = static_cast<uint16_t>(pipefd[READ_END]),
			.type = EPOLL_TYPE_CGI
	};

	Log::info("Forwarding client FD: {} to CGI handler", (int)client);

	Server::AddEpollEvent(pipefd[READ_END], EPOLLIN | EPOLLET, data);

	struct sigaction sa;
	sa.sa_handler = sigchld_handler;  // Assign the custom handler
	sigemptyset(&sa.sa_mask);         // Clear the signal mask (no blocked signals)
	sa.sa_flags = SA_RESTART;         // Restart interrupted system calls

	// Register the handler for SIGCHLD
	if (sigaction(SIGCHLD, &sa, NULL) == -1)
	{
		perror("sigaction");
		// exit(1);
		return ResponseGenerator::GenerateErrorResponse(HTTPStatusCode::InternalServerError, client);
	}


	pid_t pid = fork();
	if (pid == -1)
	{
		perror("fork");
		close(pipefd[READ_END]);
		close(pipefd[WRITE_END]);
		return ResponseGenerator::GenerateErrorResponse(HTTPStatusCode::InternalServerError, client);
	}

	if (pid == CHILD_PROCESS)
	{ // Child process
		handleChildProcess(client, pipefd);
	}
	Server::RegisterCgiProcess(pid, (int)client);
	Log::info("Child process (PID: {}) created for client FD: {}", pid, (int)client);
	close(pipefd[WRITE_END]); // Close the write end of the pipe

	return "";
}

constexpr std::array<const char*, 22> headers = {
	"CONTENT_LENGTH=",
	"QUERY_STRING=", // Add query string if available
	"REQUESTED_URI=", // Add request URI
	"REDIRECT_STATUS=200",
	"SCRIPT_NAME=", // Add script name
	"SCRIPT_FILENAME=", // Add script filename
	"DOCUMENT_ROOT=", // Add document root
	"REQUEST_METHOD=GET", // Add request method (GET/POST etc.)
	"SERVER_PROTOCOL=HTTP/1.1",
	"SERVER_SOFTWARE=Webserv/1.0",
	"SERVER_PORT=8080",
	"SERVER_ADDR=",
	"SERVER_NAME=",
	"REMOTE_ADDR=", // Add remote address
	"REMOTE_PORT=", // Add remote port
	"HTTP_HOST=", // Add host
	"HTTP_USER_AGENT=", // Add user agent
	"HTTP_ACCEPT=", // Add accept
	"HTTP_ACCEPT_LANGUAGE=", // Add accept language
	"HTTP_ACCEPT_ENCODING=", // Add accept encoding
	"HTTP_CONNECTION=", // Add connection
	nullptr
};


void Cgi::handleChildProcess(const Client& client, int pipefd[])
{
	const HttpRequest& request = client.GetRequest();
	std::array<std::string, 21> envpArray;
	envpArray[0] = "CONTENT_LENGTH=" + std::to_string(request.body.size());
	envpArray[1] = "QUERY_STRING=" + request.query;
	envpArray[2] = "REQUESTED_URI=" + request.path.string();
	envpArray[3] = "REDIRECT_STATUS=";
	envpArray[4] = "SCRIPT_NAME=" + request.path.string();
	envpArray[5] = "SCRIPT_FILENAME=" + request.mappedPath.string();
	envpArray[6] = "DOCUMENT_ROOT=" + client.GetLocationSettings().root.string();
	envpArray[7] = "REQUEST_METHOD=" + request.method;
	envpArray[8] = "SERVER_PROTOCOL=" + std::string(SERVER_PROTOCOL);
	envpArray[9] = "SERVER_SOFTWARE=" + std::string(SERVER_SOFTWARE);
	envpArray[10] = "SERVER_PORT=" + std::to_string(client.GetServerPort());
	envpArray[11] = "SERVER_ADDR=" + std::string(client.GetServerAddress());
	envpArray[12] = "SERVER_NAME=" + std::string(client.GetServerConfig()->GetServerName());
	envpArray[13] = "REMOTE_ADDR=" + std::string(client.GetClientAddress());
	envpArray[14] = "REMOTE_PORT=" + std::to_string(client.GetClientPort());
	envpArray[15] = "HTTP_HOST=" + request.getHeaderValue("host");
	envpArray[16] = "HTTP_USER_AGENT=" + request.getHeaderValue("user-agent");
	envpArray[17] = "HTTP_ACCEPT=" + request.getHeaderValue("accept");
	envpArray[18] = "HTTP_ACCEPT_LANGUAGE=" + request.getHeaderValue("accept-language");
	envpArray[19] = "HTTP_ACCEPT_ENCODING=" + request.getHeaderValue("accept-encoding");
	envpArray[20] = "HTTP_CONNECTION=" + request.getHeaderValue("connection");




	// struct sigaction sa;
	// sa.sa_handler = sigalarm_handler;  // Assign the custom handler
	// sigemptyset(&sa.sa_mask);         // Clear the signal mask (no blocked signals)
	// sa.sa_flags = 0;         // Restart interrupted system calls

	// // Register the handler for SIGALRM
	// if (sigaction(SIGALRM, &sa, NULL) == -1)
	// {
	// 	perror("sigaction");
	// 	exit(1);
	// }
	alarm(3); // Set the alarm for CGI timeout

	// std::string path = request.getUri().string();
	// std::string path = request.path.string();
	std::string path = request.mappedPath.string();

	// alarm(3); // Set the alarm for CGI timeout

	close(pipefd[READ_END]); // Close the read end of the pipe
	dup2(pipefd[WRITE_END], STDOUT_FILENO); // Redirect stdout to the pipe
	close(pipefd[WRITE_END]); // Close the original write end of the pipe

	char* argv[] = { const_cast<char*>(path.c_str()), NULL };
	char* envp[] = {
		envpArray[0].data(),
		envpArray[1].data(),
		envpArray[2].data(),
		envpArray[3].data(),
		envpArray[4].data(),
		envpArray[5].data(),
		envpArray[6].data(),
		envpArray[7].data(),
		envpArray[8].data(),
		envpArray[9].data(),
		envpArray[10].data(),
		envpArray[11].data(),
		envpArray[12].data(),
		envpArray[13].data(),
		envpArray[14].data(),
		envpArray[15].data(),
		envpArray[16].data(),
		envpArray[17].data(),
		envpArray[18].data(),
		envpArray[19].data(),
		envpArray[20].data(),
		nullptr
	};
	// const char* envp[] = {
	// 	"CONTENT_LENGTH=",
	// 	"QUERY_STRING=", // Add query string if available
	// 	"REQUESTED_URI=", // Add request URI
	// 	"REDIRECT_STATUS=200",
	// 	"SCRIPT_NAME=", // Add script name
	// 	"SCRIPT_FILENAME=", // Add script filename
	// 	"DOCUMENT_ROOT=", // Add document root
	// 	"REQUEST_METHOD=GET", // Add request method (GET/POST etc.)
	// 	"SERVER_PROTOCOL=HTTP/1.1",
	// 	"SERVER_SOFTWARE=Webserv/1.0",
	// 	"SERVER_PORT=8080",
	// 	"SERVER_ADDR=",
	// 	"SERVER_NAME=",
	// 	"REMOTE_ADDR=", // Add remote address
	// 	"REMOTE_PORT=", // Add remote port
	// 	"HTTP_HOST=", // Add host
	// 	"HTTP_USER_AGENT=", // Add user agent
	// 	"HTTP_ACCEPT=", // Add accept
	// 	"HTTP_ACCEPT_LANGUAGE=", // Add accept language
	// 	"HTTP_ACCEPT_ENCODING=", // Add accept encoding
	// 	"HTTP_CONNECTION=", // Add connection
	// 	nullptr
	// };

	execve(argv[0], argv, const_cast<char *const *>(envp));
	perror("execve");
	exit(EXIT_FAILURE);
}

const std::string Cgi::handleParentProcess(int pipefd[], pid_t pid)
{
	close(pipefd[WRITE_END]); // Close the write end of the pipe

	return "";
}
