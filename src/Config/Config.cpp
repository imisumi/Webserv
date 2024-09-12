#include "Config.h"

#include <iostream>
#include <cstdint>
#include <arpa/inet.h>  // For `inet_ntoa` and `in_addr`

#define BIT(x) (1u << x)

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


ServerSettings::LocationSettings CreateNewLocation(
	std::filesystem::path root,
	std::filesystem::path index,
	bool autoindex,
	std::filesystem::path cgi,
	uint8_t httpMethods)
{
	ServerSettings::LocationSettings location;
	location.root = root;
	location.index = index;
	location.autoindex = autoindex;
	location.cgi = cgi;
	location.httpMethods = httpMethods;
	return location;
}

Config Config::CreateDefaultConfig()
{
	Config config;

	// server for port 8080 on localhost
	std::filesystem::path root = "/";
	ServerSettings serverSettings;
	serverSettings.SetGlobalSettings(CreateNewLocation("root/html/123", "index.html", true, "cgi-bin", HTTP_GET));
	serverSettings.InsertLocation(root, CreateNewLocation("root/html", "index.html", true, "cgi-bin", HTTP_GET));
	serverSettings.InsertLocation("/contact", CreateNewLocation("root/html", "contact.html", true, "cgi-bin", HTTP_GET));

	std::vector<ServerSettings> settings;
	settings.push_back(serverSettings);

	// uint32_t ip = 127 << 24 | 0 << 16 | 0 << 8 | 1;
	// uint16_t port = 8080;
	// uint64_t ipAndPort = (uint64_t)ip << 16 | port;

	// uint32_t ip = 127 << 24 | 0 << 16 | 0 << 8 | 1;
	uint32_t ip = 0 << 24 | 0 << 16 | 0 << 8 | 0;
	uint16_t port = 8080;
	uint64_t ipAndPort = (uint64_t)ip << 32 | port;

	// std::cout << "ipAndPort: " << ipAndPort << std::endl;
	std::cout << "ipAndPort: " << ipAndPort << std::endl;

	config.InsertServer(ipAndPort, settings);


	return config;
}

std::pair<std::string, uint32_t> ExtractIpAndPort(uint64_t input)
{
	uint32_t ip = static_cast<uint32_t>(input >> 32);

	uint32_t port = static_cast<uint32_t>(input & 0xFFFFFFFF);

	struct in_addr ip_addr;
	ip_addr.s_addr = htonl(ip);  // Convert to network byte order
	std::string ip_str = inet_ntoa(ip_addr);

	return std::make_pair(ip_str, port);
}

std::string MethodToString(uint8_t methods)
{
	std::string result;
	if (methods & HTTP_GET)
	{
		result += "GET ";
	}
	if (methods & HTTP_POST)
	{
		result += "POST ";
	}
	if (methods & HTTP_DELETE)
	{
		result += "DELETE ";
	}
	return result;
}

void Config::Print() const
{
	for (const auto& [key, value] : m_ServerMap)
	{
		// std::cout << "Server: " << key << std::endl;
		auto [ip, port] = ExtractIpAndPort(key);
		std::cout << "Server\n{\n";
		std::cout << "\tServer name: " << value[0].GetServerName() << std::endl;
		std::cout << "\tListening on: " << ip << ":" << port << std::endl;
		for (const auto& server : value)
		{
			std::cout << "\tRoot: " << server.GetGlobalSettings().root << std::endl;
			std::cout << "\tIndex: " << server.GetGlobalSettings().index << std::endl;
			std::cout << "\tAutoindex: " << std::boolalpha << server.GetGlobalSettings().autoindex << std::endl;
			std::cout << "\tCgi: " << server.GetGlobalSettings().cgi << std::endl;
			std::cout << "\tMethods: " << MethodToString(server.GetGlobalSettings().httpMethods) << std::endl;
			std::cout << std::endl;
			for (const auto& [path, location] : server.GetLocations())
			{
				std::cout << "\tLocation: " << path << std::endl;
				std::cout << "\t{\n";
				std::cout << "\t\tRoot: " << location.root << std::endl;
				std::cout << "\t\tIndex: " << location.index << std::endl;
				std::cout << "\t\tAutoindex: " << std::boolalpha << location.autoindex << std::endl;
				std::cout << "\t\tCgi: " << location.cgi << std::endl;
				std::cout << "\t\tMethods: " << MethodToString(location.httpMethods) << std::endl;
				std::cout << "\t}\n";
			}
		}
		std::cout << "}\n";
	}
}