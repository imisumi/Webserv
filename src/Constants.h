#pragma once

#define BIT(n) (1u << (n))

#define EPOLL_TYPE_SOCKET   BIT(0)
#define EPOLL_TYPE_CGI      BIT(1)
#define EPOLL_TYPE_STDIN    BIT(2)
#define EPOLL_TYPE_STDOUT   BIT(3)

constexpr int CHILD_PROCESS = 0;
constexpr int READ_END = 0;
constexpr int WRITE_END = 1;

constexpr char SERVER_SOFTWARE[] = "Webserv/1.0";
constexpr char SERVER_PROTOCOL[] = "HTTP/1.1";

constexpr int MAX_EVENTS = 4096;
// #define BUFFER_SIZE 12000 * 2
// #define BUFFER_SIZE 4096
// #define BUFFER_SIZE (64 * 1024)
constexpr size_t BUFFER_SIZE = static_cast<size_t>(64 * 1024);
// constexpr size_t BUFFER_SIZE = static_cast<size_t>(2);
static_assert(BUFFER_SIZE > 1, "BUFFER_SIZE must be greater than 1");


constexpr int EPOLL_TIMEOUT = 1000;
constexpr int CLIENT_TIMEOUT = 5000;