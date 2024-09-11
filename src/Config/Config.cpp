#include "Config.h"



Config::Config()
{
	{
		std::filesystem::path	root;
		std::string				index;
		bool					autoindex;
		std::string				cgi;
		std::string				returnCode;
		std::string				errorPages;
		uint8_t					httpMethods;
	};
}

Config Config::CreateDefaultConfig()
{
	Config config;

	ServerSettings::LocationSettings globalSettings;
	globalSettings.root = "root/html";
	globalSettings.index = "index.html";
	globalSettings.autoindex = true;
	globalSettings.cgi = "cgi-bin";

	ServerSettings::LocationSettings defaultLocation; //?    "/"
	defaultLocation.root = "root/html";
	defaultLocation.index = "index.html";
	defaultLocation.autoindex = true;
	defaultLocation.cgi = "cgi-bin";


	// server for port 8080 on localhost
	ServerSettings serverSettings;
	serverSettings.SetGlobalSettings(globalSettings);
	serverSettings.InsertLocation("/", defaultLocation);

	std::vector<ServerSettings> settings;
	settings.push_back(serverSettings);

	// uint32_t ip = 127 << 24 | 0 << 16 | 0 << 8 | 1;
	// uint16_t port = 8080;
	// uint64_t ipAndPort = (uint64_t)ip << 16 | port;

	uint32_t ip = 127 << 24 | 0 << 16 | 0 << 8 | 1;
	uint16_t port = 8080;
	uint64_t ipAndPort = (uint64_t)ip << 32 | port;

	config.InsertServer(ipAndPort, settings);


	return config;
}

#include <iostream>
#include <cstdint>
#include <arpa/inet.h>  // For `inet_ntoa` and `in_addr`

std::pair<std::string, uint32_t> ExtractIpAndPort(uint64_t input)
{
	uint32_t ip = static_cast<uint32_t>(input >> 32);

	uint32_t port = static_cast<uint32_t>(input & 0xFFFFFFFF);

	struct in_addr ip_addr;
	ip_addr.s_addr = htonl(ip);  // Convert to network byte order
	std::string ip_str = inet_ntoa(ip_addr);

	return std::make_pair(ip_str, port);
}

void Config::Print() const
{
	for (const auto& [key, value] : m_ServerMap)
	{
		// std::cout << "Server: " << key << std::endl;
		auto [ip, port] = ExtractIpAndPort(key);
		std::cout << "Server\n{\n";
		std::cout << "\tListening on: " << ip << ":" << port << std::endl;
		for (const auto& server : value)
		{
			std::cout << "\tRoot: " << server.GetGlobalSettings().root << std::endl;
			std::cout << "\tIndex: " << server.GetGlobalSettings().index << std::endl;
			std::cout << "\tAutoindex: " << std::boolalpha << server.GetGlobalSettings().autoindex << std::endl;
			std::cout << "\tCgi: " << server.GetGlobalSettings().cgi << std::endl;
			std::cout << std::endl;
			for (const auto& [path, location] : server.GetLocations())
			{
				std::cout << "\tLocation: " << path << std::endl;
				std::cout << "\t{\n";
				std::cout << "\t\tRoot: " << location.root << std::endl;
				std::cout << "\t\tIndex: " << location.index << std::endl;
				std::cout << "\t\tAutoindex: " << std::boolalpha << location.autoindex << std::endl;
				std::cout << "\t\tCgi: " << location.cgi << std::endl;
				std::cout << "\t}\n";
			}
		}
		std::cout << "}\n";
	}
}