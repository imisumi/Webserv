#pragma once

#define BIT(n) (1u << (n))

#define EPOLL_TYPE_SOCKET   BIT(0)
#define EPOLL_TYPE_CGI      BIT(1)
#define EPOLL_TYPE_STDIN    BIT(2)
#define EPOLL_TYPE_STDOUT   BIT(3)

#define CHILD_PROCESS 0
#define READ_END 0
#define WRITE_END 1

constexpr char SERVER_SOFTWARE[] = "Webserv/1.0";
constexpr char SERVER_PROTOCOL[] = "HTTP/1.1";
