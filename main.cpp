#include <iostream>
#include <csignal>
#include <thread>
#include <atomic>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <cstring>
#include <stdexcept>

std::atomic<bool> g_isRunning(true);

void customSignalHandler(int signum) {
    std::cout << "Interrupt signal (" << signum << ") received. Shutting down..." << std::endl;
    g_isRunning = false;
}

void log(const std::string& message) {
    std::cout << message << std::endl;
}

void exitWithError(const std::string& message) {
    std::cerr << message << std::endl;
    std::exit(EXIT_FAILURE);
}

int setupServerSocket() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        exitWithError("Socket failed");
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        exitWithError("Setsockopt failed");
    }

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        exitWithError("Bind failed");
    }

    if (listen(server_fd, 3) < 0) {
        exitWithError("Listen failed");
    }

    // Set the server socket to non-blocking mode
    int flags = fcntl(server_fd, F_GETFL, 0);
    if (flags < 0 || fcntl(server_fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        exitWithError("Failed to set non-blocking mode");
    }

    return server_fd;
}

int setupEpoll(int server_fd) {
    int epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
        exitWithError("Epoll create failed");
    }

    struct epoll_event event;
    event.events = EPOLLIN | EPOLLET; // Edge-triggered
    event.data.fd = server_fd;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) < 0) {
        exitWithError("Epoll ctl failed");
    }

    return epoll_fd;
}

void handleClient(int client_fd) {
    const int BUFFER_SIZE = 1024;
    char buffer[BUFFER_SIZE];
    
    ssize_t bytesReceived = read(client_fd, buffer, BUFFER_SIZE);
    if (bytesReceived > 0) {
        std::cout << "Received data: " << std::string(buffer, bytesReceived) << std::endl;
    } else if (bytesReceived == 0) {
        // Client closed the connection
        std::cout << "Client disconnected" << std::endl;
    } else {
        // Error occurred
        std::cerr << "Read error" << std::endl;
    }

    close(client_fd);
}

void serverLoop(int server_fd) {
    int epoll_fd = setupEpoll(server_fd);

    const int MAX_EVENTS = 10;
    struct epoll_event events[MAX_EVENTS];

    while (g_isRunning) {
        int event_count = epoll_wait(epoll_fd, events, MAX_EVENTS, 1000); // 1 second timeout
        if (event_count < 0) {
            if (errno == EINTR) {
                continue; // Interrupted by signal, retry
            }
            exitWithError("Epoll wait failed");
        }

        for (int i = 0; i < event_count; ++i) {
            if (events[i].data.fd == server_fd) {
                // Accept new connections
                while (true) {
                    int client_fd = accept(server_fd, nullptr, nullptr);
                    if (client_fd < 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            break; // No more connections to accept
                        }
                        exitWithError("Accept failed");
                    }

                    // Set the client socket to non-blocking mode
                    int flags = fcntl(client_fd, F_GETFL, 0);
                    if (flags < 0 || fcntl(client_fd, F_SETFL, flags | O_NONBLOCK) < 0) {
                        exitWithError("Failed to set non-blocking mode for client socket");
                    }

                    // Add client socket to epoll
                    struct epoll_event event;
                    event.events = EPOLLIN | EPOLLET; // Edge-triggered
                    event.data.fd = client_fd;

                    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) < 0) {
                        exitWithError("Epoll ctl failed for client socket");
                    }
                }
            } else {
                // Handle client data
                handleClient(events[i].data.fd);
            }
        }
    }

    close(epoll_fd);
    close(server_fd);
    std::cout << "Server has been shut down cleanly." << std::endl;
}

int main() {
    std::signal(SIGINT, customSignalHandler);  // Register the signal handler

    int server_fd = setupServerSocket();

    // Run the server loop
    serverLoop(server_fd);

    return 0;
}
