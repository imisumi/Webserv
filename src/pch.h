#pragma once

#include <filesystem>
#include <string>
#include <vector>
#include <map>

#include <chrono>
#include <fstream>


// include socket headers
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

// include epoll headers
#include <sys/epoll.h>

#include <cerrno>