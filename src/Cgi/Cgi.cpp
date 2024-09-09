#include "Cgi.h"


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

	// printf("Child process (PID: %d) timed out and will terminate.\n", getpid());
	std::cerr << "Child process (PID: " << getpid() << ") timed out and will terminate." << std::endl;
	// exit(EXIT_FAILURE); // Terminate the child process
	// exit(EXIT_SUCCESS); // Terminate the child process
}

// Custom SIGCHLD handler to reap the specific child that has exited
void sigchld_handler(int signo)
{
	int status;
	pid_t pid;

	// Use WNOHANG to only reap child processes that have exited
	while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
		if (WIFEXITED(status)) {
			std::cout << "Child process " << pid << " exited with status: " << WEXITSTATUS(status) << std::endl;
		} else if (WIFSIGNALED(status)) {
			std::cout << "Child process " << pid << " was terminated by signal: " << WTERMSIG(status) << std::endl;
		}

		// Remove the child from the list of running processes
		// childProcesses.erase(pid);
	}
}

#include <fcntl.h> // For fcntl
#include "Server/Server.h"
#define PACK_U64(fd, metadata)   (((uint64_t)(fd) << 32) | (metadata))
#define EPOLL_TYPE_SOCKET   BIT(0)
#define EPOLL_TYPE_CGI      BIT(1)
#define EPOLL_TYPE_STDIN    BIT(2)
#define EPOLL_TYPE_STDOUT   BIT(3)

std::string Cgi::executeCGI(const Client& client, const Config& config, const HttpRequest& request)
{
	std::string path = request.getUri().string();
	int pipefd[2];
		
	if (pipe(pipefd) == -1)
	{
		perror("pipe");
		return ResponseGenerator::InternalServerError(config);
	}

	int flags = fcntl(pipefd[READ_END], F_GETFL);
	fcntl(pipefd[READ_END], F_SETFL, flags | O_NONBLOCK);

	int flags2 = fcntl(pipefd[WRITE_END], F_GETFL);
	fcntl(pipefd[WRITE_END], F_SETFL, flags2 | O_NONBLOCK);


	Server& server = Server::Get();

	// struct epoll_event ev;
	// ev.events = EPOLLIN | EPOLLET; // Monitor for readability
	// // ev.data.fd = pipefd[READ_END];
	// ev.data.u64 = PACK_U64(pipefd[READ_END], EPOLL_TYPE_CGI);

	// struct epoll_event ev = server.GetEpollEventFD(client);
	// LOG_INFO("Forwarding client FD: {} to CGI handler", (int)client);
	Server::EpollData data = server.GetEpollData(client);
	LOG_INFO("Forwarding client FD: {} to CGI handler", data.fd);
	server.SetCgiToClientMap(pipefd[READ_END], data.fd);

	server.AddEpollEvent(server.GetEpollInstance(), pipefd[READ_END], EPOLLIN | EPOLLET, EPOLL_TYPE_CGI);

	struct sigaction sa;
	sa.sa_handler = sigchld_handler;  // Assign the custom handler
	sigemptyset(&sa.sa_mask);         // Clear the signal mask (no blocked signals)
	sa.sa_flags = SA_RESTART;         // Restart interrupted system calls

	// Register the handler for SIGCHLD
	if (sigaction(SIGCHLD, &sa, NULL) == -1)
	{
		perror("sigaction");
		exit(1);
	}


	pid_t pid = fork();
	if (pid == -1)
	{
		perror("fork");
		close(pipefd[READ_END]);
		close(pipefd[WRITE_END]);
		return ResponseGenerator::InternalServerError(config);
	}

	if (pid == CHILD_PROCESS)
	{ // Child process
		handleChildProcess(config, request, pipefd);
	}
	return handleParentProcess(config, pipefd, pid);
}


void Cgi::handleChildProcess(const Config& config, const HttpRequest& request, int pipefd[])
{
	std::string path = request.getUri().string();

	// alarm(3); // Set the alarm for CGI timeout

	close(pipefd[READ_END]); // Close the read end of the pipe
	dup2(pipefd[WRITE_END], STDOUT_FILENO); // Redirect stdout to the pipe
	close(pipefd[WRITE_END]); // Close the original write end of the pipe

	char* argv[] = { const_cast<char*>(path.c_str()), NULL };
	const char* envp[] = {
		"SERVER_SOFTWARE=Webserv/1.0",
		"QUERY_STRING=", // Add query string if available
		"REQUEST_METHOD=GET", // Add request method (GET/POST etc.)
		"REQUEST_URI=", // Add request URI
		nullptr
	};

	execve(argv[0], argv, const_cast<char *const *>(envp));
	perror("execve");
	exit(EXIT_FAILURE);
}

const std::string Cgi::handleParentProcess(const Config& config, int pipefd[], pid_t pid)
{
	close(pipefd[WRITE_END]); // Close the write end of the pipe

	int status;
	// if (waitpid(pid, &status, 0) == -1)
	// {
	// 	perror("waitpid");
	// 	close(pipefd[READ_END]);
	// 	return ResponseGenerator::InternalServerError(config);
	// }


	if (waitpid(pid, &status, WNOHANG) == -1)
	{
		perror("waitpid");
		close(pipefd[READ_END]);
		return ResponseGenerator::InternalServerError(config);
	}


	if (WIFEXITED(status) && WEXITSTATUS(status) == EXIT_SUCCESS)
	{
		LOG_INFO("Successfully executed CGI script");
		char buffer[1024];
		std::string output;
		ssize_t count;
		while ((count = read(pipefd[READ_END], buffer, sizeof(buffer))) > 0)
		{
			output.append(buffer, count);
		}
		close(pipefd[READ_END]);

		size_t header_end = output.find("\r\n\r\n");
		if (header_end != std::string::npos)
		{
			size_t bodyLength = output.size() - header_end - 4;
			std::string httpResponse = "HTTP/1.1 200 OK\r\n";
			httpResponse += "Content-Length: " + std::to_string(bodyLength) + "\r\n";
			httpResponse += "Connection: close\r\n";
			httpResponse += "\r\n"; // End of headers
			httpResponse += output.substr(header_end + 4); // Body


			LOG_INFO("Successfully executed CGI script");
			return httpResponse;
		}
		else
		{
			LOG_ERROR("CGI script did not produce valid headers");
			return ResponseGenerator::InternalServerError(config);
		}
	}
	LOG_ERROR("CGI script failed with status: " + std::to_string(WEXITSTATUS(status)));
	// close(pipefd[READ_END]);
	return ResponseGenerator::InternalServerError(config);
}
