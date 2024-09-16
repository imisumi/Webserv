#include "Cgi.h"
#include "Server/Server.h"
#include "Server/ConnectionManager.h"


bool Cgi::isValidCGI(const Config& config, const std::filesystem::path& path)
{
	if (path.extension() == ".cgi" || path.extension() == ".py")
	{
		std::filesystem::perms perms = std::filesystem::status(path).permissions();
		if ((perms & std::filesystem::perms::owner_exec) != std::filesystem::perms::none ||
			(perms & std::filesystem::perms::group_exec) != std::filesystem::perms::none ||
			(perms & std::filesystem::perms::others_exec) != std::filesystem::perms::none)
		{
			return true;
		}
	}
	return false;
}


#include "Core/Log.h"
#include <unistd.h>
#include <sys/wait.h>

#include "Server/ResponseGenerator.h"

#define CHILD_PROCESS 0
#define READ_END 0
#define WRITE_END 1


// // 	//TODO: 404 if file does not exist, 403 if no permission, 500 if execve fails or cgi script is invalid

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


void sigalarm_handler(int signo) {
	// (void) signo; // Unused parameter
	// int status;
	// while (waitpid(-1, &status, WNOHANG) > 0) {
	// 	if (WIFEXITED(status)) {
	// 		printf("Child process exited with status %d\n", WEXITSTATUS(status));
	// 	} else if (WIFSIGNALED(status)) {
	// 		printf("Child process killed by signal %d\n", WTERMSIG(status));
	// 	}
	// }

	// std::cout << s_TimeoutErrorResponse;

	// printf("Child process (PID: %d) timed out and will terminate.\n", getpid());
	// std::cerr << "Child process (PID: " << getpid() << ") timed out and will terminate." << std::endl;
	// exit(EXIT_FAILURE); // Terminate the child process
	// exit(EXIT_SUCCESS); // Terminate the child process
	_exit(EXIT_FAILURE); // Terminate the child process
}

#define EPOLL_TYPE_SOCKET   BIT(0)

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
				Client client = ConnectionManager::GetClient(client_fd);
				LOG_INFO("Child process: {}, Client FD: {}", pid, client_fd);

				// struct Server::EpollData *ev_data = server.GetEpollData(client_fd);
				struct Server::EpollData *ev_data = new Server::EpollData();
				LOG_INFO("Client FD: {}", client_fd);
				ev_data->fd = client_fd;
				ev_data->cgi_fd = -1;
				ev_data->type = EPOLL_TYPE_SOCKET;

				// Server::AddEpollEventStatic(client.GetEpollInstance(), client_fd, EPOLLOUT | EPOLLET, ev_data);
				Server::ModifyEpollEvent(client.GetEpollInstance(), client_fd, EPOLLOUT | EPOLLET, ev_data);

				std::string response = s_TimeoutErrorResponse;
				server.m_ClientResponses[client_fd] = response;

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

#include <fcntl.h> // For fcntl
// #include "Server/Server.h"
#define PACK_U64(fd, metadata)   (((uint64_t)(fd) << 32) | (metadata))
#define EPOLL_TYPE_SOCKET   BIT(0)
#define EPOLL_TYPE_CGI      BIT(1)
#define EPOLL_TYPE_STDIN    BIT(2)
#define EPOLL_TYPE_STDOUT   BIT(3)

std::string Cgi::RunCgi(const Client& client, char* argv[])
{
	int pipefd[2];
		
	if (pipe(pipefd) == -1)
	{
		perror("pipe");
		return ResponseGenerator::InternalServerError();
	}

	int flags = fcntl(pipefd[READ_END], F_GETFL);
	fcntl(pipefd[READ_END], F_SETFL, flags | O_NONBLOCK);

	int flags2 = fcntl(pipefd[WRITE_END], F_GETFL);
	fcntl(pipefd[WRITE_END], F_SETFL, flags2 | O_NONBLOCK);


	Server::CgiRedirect(pipefd[READ_END], (int)client);

	struct sigaction sa;
	sa.sa_handler = sigchld_handler;  // Assign the custom handler
	sigemptyset(&sa.sa_mask);         // Clear the signal mask (no blocked signals)
	sa.sa_flags = SA_RESTART;         // Restart interrupted system calls

	// Register the handler for SIGCHLD
	if (sigaction(SIGCHLD, &sa, NULL) == -1)
	{
		perror("sigaction");
		// exit(1);
		return ResponseGenerator::InternalServerError();
	}


	pid_t pid = fork();
	if (pid == -1)
	{
		perror("fork");
		close(pipefd[READ_END]);
		close(pipefd[WRITE_END]);
		return ResponseGenerator::InternalServerError();
	}

	if (pid == CHILD_PROCESS)
	{ // Child process
		alarm(3); // Set the alarm for CGI timeout

		// std::string path = request.getUri().string();

		// alarm(3); // Set the alarm for CGI timeout

		close(pipefd[READ_END]); // Close the read end of the pipe
		dup2(pipefd[WRITE_END], STDOUT_FILENO); // Redirect stdout to the pipe
		close(pipefd[WRITE_END]); // Close the original write end of the pipe

		// char* argv[] = { const_cast<char*>(path.c_str()), NULL };
		const char* envp[] = {
			"SERVER_SOFTWARE=Webserv/1.0",
			"QUERY_STRING=", // Add query string if available
			"REQUEST_METHOD=GET", // Add request method (GET/POST etc.)
			"REQUEST_URI=", // Add request URI
			nullptr
		};

		execve(argv[0], argv, const_cast<char *const *>(envp));
		// perror("execve");
		exit(EXIT_FAILURE);
	}
	// Server& server = Server::Get();
	// server.childProcesses[pid] = (int)client;
	// Server::RegisterCgiProcess(pid, (int)client);
	Server& server = Server::Get();
	server.childProcesses[pid] = (int)client;
	LOG_INFO("Child process: {} registered with client: {}", pid, (int)client);
	// return handleParentProcess(config, pipefd, pid);
	close(pipefd[WRITE_END]); // Close the write end of the pipe

	return "";

	return "";
}

std::string Cgi::executeCGI(const Client& client, const HttpRequest& request)
{
	std::string path = request.getUri().string();
	int pipefd[2];
		
	if (pipe(pipefd) == -1)
	{
		perror("pipe");
		return ResponseGenerator::InternalServerError();
	}

	int flags = fcntl(pipefd[READ_END], F_GETFL);
	fcntl(pipefd[READ_END], F_SETFL, flags | O_NONBLOCK);

	int flags2 = fcntl(pipefd[WRITE_END], F_GETFL);
	fcntl(pipefd[WRITE_END], F_SETFL, flags2 | O_NONBLOCK);


	// Server::CgiRedirect(pipefd[READ_END], (int)client);
// #if 0
	struct Server::EpollData *ev_data = new Server::EpollData();
	ev_data->fd = (int)client;
	ev_data->cgi_fd = pipefd[READ_END];
	ev_data->type = EPOLL_TYPE_CGI;

	LOG_INFO("Forwarding client FD: {} to CGI handler", (int)client);

	Server::AddEpollEvent(client.GetEpollInstance(), pipefd[READ_END], EPOLLIN | EPOLLET, ev_data);

	struct sigaction sa;
	sa.sa_handler = sigchld_handler;  // Assign the custom handler
	sigemptyset(&sa.sa_mask);         // Clear the signal mask (no blocked signals)
	sa.sa_flags = SA_RESTART;         // Restart interrupted system calls

	// Register the handler for SIGCHLD
	if (sigaction(SIGCHLD, &sa, NULL) == -1)
	{
		perror("sigaction");
		// exit(1);
		return ResponseGenerator::InternalServerError();
	}


	pid_t pid = fork();
	if (pid == -1)
	{
		perror("fork");
		close(pipefd[READ_END]);
		close(pipefd[WRITE_END]);
		return ResponseGenerator::InternalServerError();
	}

	if (pid == CHILD_PROCESS)
	{ // Child process
		handleChildProcess(request, pipefd);
	}
	Server::RegisterCgiProcess(pid, (int)client);
	LOG_INFO("Child process (PID: {}) created for client FD: {}", pid, (int)client);
	close(pipefd[WRITE_END]); // Close the write end of the pipe

	return "";
}


void Cgi::handleChildProcess(const HttpRequest& request, int pipefd[])
{

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




	std::string path = request.getUri().string();

	// alarm(3); // Set the alarm for CGI timeout

	close(pipefd[READ_END]); // Close the read end of the pipe
	dup2(pipefd[WRITE_END], STDOUT_FILENO); // Redirect stdout to the pipe
	close(pipefd[WRITE_END]); // Close the original write end of the pipe

	char* argv[] = { const_cast<char*>(path.c_str()), NULL };
	const char* envp[] = {
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

	execve(argv[0], argv, const_cast<char *const *>(envp));
	perror("execve");
	exit(EXIT_FAILURE);
}

const std::string Cgi::handleParentProcess(int pipefd[], pid_t pid)
{
	close(pipefd[WRITE_END]); // Close the write end of the pipe

	return "";
}
