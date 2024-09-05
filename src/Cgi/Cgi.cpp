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


// 	//TODO: 404 if file does not exist, 403 if no permission, 500 if execve fails or cgi script is invalid

#include <chrono>

void handle_alarm(int signal) {
	// This signal handler does nothing but allows us to handle timeouts
}

std::string Cgi::executeCGI(const Config& config, const HttpRequest& request) {
	std::string path = request.getUri().string();
	int pipefd[2];
		
	if (pipe(pipefd) == -1)
	{
		perror("pipe");
		return ResponseGenerator::InternalServerError(config);
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

	struct sigaction sa;
	sa.sa_handler = handle_alarm;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	if (sigaction(SIGALRM, &sa, NULL) == -1)
	{
		perror("sigaction");
		close(pipefd[READ_END]);
		close(pipefd[WRITE_END]);
		exit(EXIT_FAILURE);
	}

	alarm(5); // Set the alarm for CGI timeout


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
	if (waitpid(pid, &status, 0) == -1)
	{
		perror("waitpid");
		close(pipefd[READ_END]);
		return ResponseGenerator::InternalServerError(config);
	}

	if (WIFSIGNALED(status) && WTERMSIG(status) == SIGALRM)
	{
		LOG_ERROR("CGI script timed out");
		close(pipefd[READ_END]);
		kill(pid, SIGKILL); // Ensure the child process is terminated
		return ResponseGenerator::InternalServerError(config);
	}

	if (WIFEXITED(status) && WEXITSTATUS(status) == EXIT_SUCCESS)
	{
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
	close(pipefd[READ_END]);
	return ResponseGenerator::InternalServerError(config);
}