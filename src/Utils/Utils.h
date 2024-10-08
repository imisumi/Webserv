#pragma once

#include <chrono>
#include <iostream>
#include <arpa/inet.h>
#include <array>

namespace Utils
{
class Timer
{
public:
	Timer() { reset(); }
	~Timer() { std::cout << "Elapsed time: " << elapsedMillis() << "ms\n"; }
	void reset() { start = std::chrono::high_resolution_clock::now(); }

	// elapsed time in microseconds
	float elapsed() const
	{
		return std::chrono::duration<float, std::micro>(std::chrono::high_resolution_clock::now() - start).count();
	}
	// elapsed time in milliseconds
	auto elapsedMillis() const -> float
	{
		return std::chrono::duration<float, std::milli>(std::chrono::high_resolution_clock::now() - start).count();
	}
	// elapsed time in seconds
	float elapsedSeconds() const
	{
		return std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - start).count();
	}

private:
	std::chrono::time_point<std::chrono::high_resolution_clock> start;
};

/*
 * Pack an IP address and port into a single 64-bit integer
 *
 * @param ipStr The IP address as a string (e.g. "127.0.0.1")
 * @param port The port number
 * @return A 64-bit integer with the IP address in the upper 32 bits and the port in the lower 32 bits
 */
static uint64_t packIpAndPort(const std::string& ipStr, uint16_t port)
{
	struct in_addr ipAddr = {};

	// Convert the IP string to a 32-bit integer (network byte order)
	if (inet_pton(AF_INET, ipStr.c_str(), &ipAddr) != 1)
	{
		std::cerr << "Invalid IP address format" << std::endl;
		return 0;
	}

	// Convert to host byte order (so we can work with it directly)
	uint32_t ip = ntohl(ipAddr.s_addr);

	// Combine IP (shifted by 32 bits) and the port into a uint64_t
	return static_cast<uint64_t>(ip) << 32 | static_cast<uint64_t>(port);
}

/*
 * Unpack an IP address and port from a single 64-bit integer
 *
 * @param packedValue The 64-bit integer containing the IP address and port
 * @return A pair containing the IP address as a string and the port number
 */
static auto unpackIpAndPort(uint64_t packedValue) -> std::pair<std::string, uint16_t>
{
	// Extract the port (lower 16 bits)
	uint16_t port = packedValue & 0xFFFF;

	// Extract the IP address (upper 32 bits)
	// uint32_t ipAddress = (packedValue >> 32) & 0xFFFFFFFF;
	uint32_t ipAddress = packedValue >> 32;

	// Convert the 32-bit IP back to string format
	struct in_addr ipAddr = {};
	ipAddr.s_addr = htonl(ipAddress);  // Convert back to network byte order

	// Use std::array for IP string representation
	std::array<char, INET_ADDRSTRLEN> ipStr{};

	if (inet_ntop(AF_INET, &ipAddr, ipStr.data(), INET_ADDRSTRLEN) == nullptr)
	{
		std::cerr << "Failed to convert IP to string format" << std::endl;
		return {"", 0};
	}

	return {std::string(ipStr.data()), port};  // Use ipStr.data() to construct std::string
}
}  // namespace Utils