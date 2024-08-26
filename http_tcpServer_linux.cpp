#include <http_tcpServer_linux.h>

#include <iostream>
#include <sstream>
#include <unistd.h>

namespace
{
    const int BUFFER_SIZE = 30720;

    void log(const std::string &message)
    {
        std::cout << message << std::endl;
    }

    void exitWithError(const std::string &errorMessage)
    {
        log("ERROR: " + errorMessage);
        exit(1);
    }
}

#include <csignal>
#include <atomic>
#include <sys/epoll.h>

namespace http
{

    TcpServer::TcpServer(std::string ip_address, int port) : m_ip_address(ip_address), m_port(port), m_socket(), m_new_socket(),
                                                             m_incomingMessage(),
                                                             m_socketAddress(), m_socketAddress_len(sizeof(m_socketAddress)),
                                                             m_serverMessage(buildResponse())
    {
        m_socketAddress.sin_family = AF_INET;
        m_socketAddress.sin_port = htons(m_port);
        m_socketAddress.sin_addr.s_addr = inet_addr(m_ip_address.c_str());

        if (startServer() != 0)
        {
            std::ostringstream ss;
            ss << "Failed to start server with PORT: " << ntohs(m_socketAddress.sin_port);
            log(ss.str());
        }
    }

    TcpServer::~TcpServer()
    {
		std::cout << "Destructor" << std::endl;
        closeServer();
    }

    int TcpServer::startServer()
    {
        m_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (m_socket < 0)
        {
            exitWithError("Cannot create socket");
            return 1;
        }

        if (bind(m_socket, (sockaddr *)&m_socketAddress, m_socketAddress_len) < 0)
        {
            exitWithError("Cannot connect socket to address");
            return 1;
        }

        return 0;
    }

    void TcpServer::closeServer()
    {
        close(m_socket);
        close(m_new_socket);
        exit(0);
    }
	// #include <csignal>
	// #include <atomic>

	// bool g_isRunning = true;
	std::atomic<bool> g_isRunning(true);
	void customSignalHandler(int signum)
	{
		std::cout << "Interrupt signal (" << signum << ") received. Shutting down..." << std::endl;
		g_isRunning = false;
	}

    void TcpServer::startListen()
    {
        if (listen(m_socket, 20) < 0)
        {
            exitWithError("Socket listen failed");
        }

        std::ostringstream ss;
        ss << "\n*** Listening on ADDRESS: " << inet_ntoa(m_socketAddress.sin_addr) << " PORT: " << ntohs(m_socketAddress.sin_port) << " ***\n\n";
        log(ss.str());

        int bytesReceived;


		std::signal(SIGINT, customSignalHandler);

        while (g_isRunning)
        {
            log("====== Waiting for a new connection ======\n\n\n");
            acceptConnection(m_new_socket);

            char buffer[BUFFER_SIZE] = {0};
            bytesReceived = read(m_new_socket, buffer, BUFFER_SIZE);
            if (bytesReceived < 0)
            {
                exitWithError("Failed to read bytes from client socket connection");
            }

            std::ostringstream ss;
            ss << "------ Received Request from client ------\n\n";
            log(ss.str());

			// std::cout << buffer << std::endl;

            sendResponse(buffer);

            close(m_new_socket);
        }
    }

    void TcpServer::acceptConnection(int &new_socket)
    {
        new_socket = accept(m_socket, (sockaddr *)&m_socketAddress, &m_socketAddress_len);
        if (new_socket < 0)
        {
            std::ostringstream ss;
            ss << "Server failed to accept incoming connection from ADDRESS: " << inet_ntoa(m_socketAddress.sin_addr) << "; PORT: " << ntohs(m_socketAddress.sin_port);
            exitWithError(ss.str());
        }
    }

    std::string TcpServer::buildResponse()
    {
		std::string htmlFile = "<!DOCTYPE html>"
                       "<html lang=\"en\">"
                       "<head>"
                       "<meta charset=\"UTF-8\">"
                       "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
                       "<title>Simple Website</title>"
                       "</head>"
                       "<body>"
                       "<h1> HOME </h1>"
					   "<form action=\"http://localhost:8080\" method=\"POST\">"
						"<label for=\"name\">Name:</label><br>"
						"<input type=\"text\" id=\"name\" name=\"name\"><br>"
						"<input type=\"submit\" value=\"Submit\">"
						"</form>"
                       "<p> Hello from your Server :) </p>"
                       "</body>"
                       "</html>";


		std::ostringstream ss;
        ss << "HTTP/1.1 200 OK\nContent-Type: text/html\nContent-Length: " << htmlFile.size() << "\n\n"
           << htmlFile;

        return ss.str();
    }

	std::string buildResponse2(int i)
    {
        // std::string htmlFile = "<!DOCTYPE html><html lang=\"en\"><body><h1> HOME </h1><p> Hello from your Server :) </p></body></html>";
        std::string htmlFile = "<!DOCTYPE html>"
                       "<html lang=\"en\">"
                       "<head>"
                       "<meta charset=\"UTF-8\">"
                       "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
                       "<title>Simple Website</title>"
                       "</head>"
                       "<body>"
                       "<h1> HOME </h1>"
                       "<p> Hello :)" + std::to_string(i) + "</p>"
                       "</body>"
                       "</html>";
		std::ostringstream ss;
        ss << "HTTP/1.1 200 OK\nContent-Type: text/html\nContent-Length: " << htmlFile.size() << "\n\n"
           << htmlFile;

        return ss.str();
    }

	#define POST 1
	#define GET 2
	#include <assert.h>

    void TcpServer::sendResponse(const std::string &response)
    {
        long bytesSent;

		// static int i = 0;
		// m_serverMessage = buildResponse2(i++);


		// "GET" "POST"

		int method;;
		//TODO: fix
		if (response.find("POST") != std::string::npos)
			method = POST;
		else if (response.find("GET") != std::string::npos)
			method = GET;
		else
			assert(false);

		if (method == POST)
		{
			bytesSent = write(m_new_socket, m_serverMessage.c_str(), m_serverMessage.size());
			std::cout << response << std::endl;

		}
		else if (method == GET)
		{
			bytesSent = write(m_new_socket, m_serverMessage.c_str(), m_serverMessage.size());
			std::cout << response << std::endl;
			// if (bytesSent == m_serverMessage.size())
			// {
			// 	log("------ Server Response sent to client ------\n\n");
			// 	// static int i = 0;
			// 	// m_serverMessage = buildResponse2(i++);
			// }
			// else
			// {
			// 	log("Error sending response to client");
			// }
		}


        // bytesSent = write(m_new_socket, m_serverMessage.c_str(), m_serverMessage.size());

        // if (bytesSent == m_serverMessage.size())
        // {
        //     log("------ Server Response sent to client ------\n\n");
		// 	// static int i = 0;
		// 	// m_serverMessage = buildResponse2(i++);
        // }
        // else
        // {
        //     log("Error sending response to client");
        // }
    }

} // namespace http