#include "Cgi.h"
#include "Server/ConnectionManager.h"
#include "Server/Server.h"

#include <sys/wait.h>
#include <unistd.h>
#include "Core/Log.h"

#include "Server/Response/ResponseGenerator.h"

#include "Constants.h"

#include <fcntl.h>	// For fcntl

#include <chrono>

void handle_alarm(int signal)
{
	// This signal handler does nothing but allows us to handle timeouts
}

static const char s_TimeoutErrorResponse[] =
	"HTTP/1.1 504 Gateway Timeout\r\n"
	"Content-Length: 41\r\n"  // Length of the body below
	"Connection: close\r\n"
	"Content-Type: text/plain\r\n"
	"\r\n"
	"504 Gateway Timeout: The server timed out";  // Body of the response

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

				Server::EpollData data{.fd = static_cast<uint16_t>(client_fd),
									   .cgi_fd = std::numeric_limits<uint16_t>::max(),
									   .type = EPOLL_TYPE_SOCKET};

				Server::ModifyEpollEvent(client_fd, EPOLLOUT | EPOLLET, data);

				// server.m_ClientResponses[client_fd] = ResponseGenerator::InternalServerError();
				client.SetResponse(ResponseGenerator::InternalServerError());
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
				Server::EpollData data{.fd = static_cast<uint16_t>(client_fd),
									   .cgi_fd = std::numeric_limits<uint16_t>::max(),
									   .type = EPOLL_TYPE_SOCKET};
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

std::string Cgi::executeCGI(const Client& client, const HttpRequest& request)
{
	std::string path = client.GetRequest().mappedPath.string();
	Log::info("Executing CGI script: {}", path);

	int inPipe[2];	 // Pipe to send data (POST body) to CGI
	int outPipe[2];	 // Pipe to receive data (CGI output) from CGI

	if (pipe(inPipe) == -1 || pipe(outPipe) == -1)
	{
		perror("pipe");
		return ResponseGenerator::GenerateErrorResponse(HTTPStatusCode::InternalServerError, client);
	}

	// Set the output pipe to non-blocking
	fcntl(outPipe[READ_END], F_SETFL, O_NONBLOCK);

	struct sigaction sa;
	sa.sa_handler = sigchld_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;

	// Register signal handler for SIGCHLD
	if (sigaction(SIGCHLD, &sa, NULL) == -1)
	{
		perror("sigaction");
		return ResponseGenerator::GenerateErrorResponse(HTTPStatusCode::InternalServerError, client);
	}

	pid_t pid = fork();
	if (pid == -1)
	{
		perror("fork");
		close(inPipe[READ_END]);
		close(inPipe[WRITE_END]);
		close(outPipe[READ_END]);
		close(outPipe[WRITE_END]);
		return ResponseGenerator::GenerateErrorResponse(HTTPStatusCode::InternalServerError, client);
	}

	if (pid == CHILD_PROCESS)
	{
		handleChildProcess(client, inPipe, outPipe);
	}

	Server::RegisterCgiProcess(pid, static_cast<int>(client));
	Log::info("Child process (PID: {}) created for client FD: {}", pid, static_cast<int>(client));

	// Parent process: handle sending the POST body (if POST method) to the CGI script
	if (request.method == "POST")
	{
		ssize_t bytes_written =
			write(inPipe[WRITE_END], client.GetRequest().body.data(), client.GetRequest().body.size());
		if (bytes_written == -1)
		{
			perror("write");
		}
		Log::info("Written {} bytes to the pipe", bytes_written);
	}

	close(inPipe[WRITE_END]);  // Close the write end of the input pipe after sending the POST body

	// Register the output pipe with epoll for reading the CGI response
	Server::EpollData data{.fd = static_cast<uint16_t>(client),
						   .cgi_fd = static_cast<uint16_t>(outPipe[READ_END]),
						   .type = EPOLL_TYPE_CGI};

	Server::AddEpollEvent(outPipe[READ_END], EPOLLIN | EPOLLET, data);	// Register output pipe with epoll

	return "";
}

void Cgi::handleChildProcess(const Client& client, int inPipe[], int outPipe[])
{
	const HttpRequest& request = client.GetRequest();

	std::array<std::string, 23> envpArray;
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
	envpArray[21] = "PATH_INFO=" + request.pathInfo.string();
	envpArray[22] = "CONTENT_TYPE=" + request.getHeaderValue("content-type");

	alarm(3);  // Set an alarm for CGI timeout

	std::string path = request.mappedPath.string();

	// Redirect stdin to the input pipe and stdout to the output pipe
	if (dup2(inPipe[READ_END], STDIN_FILENO) == -1 || dup2(outPipe[WRITE_END], STDOUT_FILENO) == -1)
	{
		perror("dup2");
		exit(EXIT_FAILURE);
	}

	// Close unused pipe ends in the child process
	close(inPipe[READ_END]);
	close(inPipe[WRITE_END]);
	close(outPipe[READ_END]);
	close(outPipe[WRITE_END]);

	char* argv[] = {const_cast<char*>(path.c_str()), NULL};
	char* envp[] = {envpArray[0].data(),  envpArray[1].data(),	envpArray[2].data(),  envpArray[3].data(),
					envpArray[4].data(),  envpArray[5].data(),	envpArray[6].data(),  envpArray[7].data(),
					envpArray[8].data(),  envpArray[9].data(),	envpArray[10].data(), envpArray[11].data(),
					envpArray[12].data(), envpArray[13].data(), envpArray[14].data(), envpArray[15].data(),
					envpArray[16].data(), envpArray[17].data(), envpArray[18].data(), envpArray[19].data(),
					envpArray[20].data(), envpArray[21].data(), envpArray[22].data(), nullptr};

	execve(argv[0], argv, const_cast<char* const*>(envp));
	perror("execve");
	exit(EXIT_FAILURE);
}
