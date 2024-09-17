/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   Config.cpp                                         :+:    :+:            */
/*                                                     +:+                    */
/*   By: kwchu <kwchu@student.codam.nl>               +#+                     */
/*                                                   +#+                      */
/*   Created: 2024/09/05 13:09:31 by kwchu         #+#    #+#                 */
/*   Updated: 2024/09/16 21:13:05 by kwchu         ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include "Config.h"
#include "ServerSettings.h"
#include <iostream>
#include <arpa/inet.h>

Config:: Config()
{

};

Config:: Config(const Config& copy)
{
	this->m_ServerMap = copy.m_ServerMap;
	this->m_Servers = copy.m_Servers;
};

Config&	Config:: operator=(const Config& other)
{
	if (this != &other)
	{
		this->m_ServerMap = other.m_ServerMap;
		this->m_Servers = other.m_Servers;
	}
	return *this;
};

Config:: Config(Config&& copy)
{
	this->m_ServerMap = copy.m_ServerMap;
	this->m_Servers = copy.m_Servers;
};

Config&	Config:: operator=(Config&& other)
{
	if (this != &other)
	{
		this->m_ServerMap = other.m_ServerMap;
		this->m_Servers = other.m_Servers;
	}
	return *this;
};

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
    if (methods & (1 << 0))
    {
        result += "GET ";
    }
    if (methods & (1 << 1))
    {
        result += "POST ";
    }
    if (methods & (1 << 2))
    {
        result += "DELETE ";
    }
    return result;
}

void	Config:: print()
{
    for (const auto& [key, value] : m_ServerMap)
    {
        // std::cout << "Server: " << key << std::endl;
        auto [ip, port] = ExtractIpAndPort(key);
        std::cout << "Server\n{\n";
        std::cout << "\tServer name: " << value[0]->GetServerName() << '\n';
        std::cout << "\tListening on: " << ip << ":" << port << '\n';
        for (const auto& server : value)
        {
            std::cout << "\tRoot: " << server->GetGlobalSettings().root << '\n';
			for (auto& i : server->GetGlobalSettings().index)
				std::cout << "\tIndex: " << i << '\n';
            std::cout << "\tAutoindex: " << std::boolalpha << server->GetGlobalSettings().autoindex << '\n';
			for (auto& i : server->GetGlobalSettings().cgi)
				std::cout << "\tCgi: " << i << '\n';
            std::cout << "\tMethods: " << MethodToString(server->GetGlobalSettings().httpMethods) << '\n';
			std::cout << "\tRedirect: " << server->GetGlobalSettings().returnCode << '\n';
			std::cout << "\tMax Client Body Size: " << server->GetGlobalSettings().maxBodySize << "B\n";
			for (const auto& [key, value] : server->GetGlobalSettings().errorPageMap)
				std::cout << "\tError Page: " << key << " - " << value << '\n';
            std::cout << '\n';
            for (const auto& [path, location] : server->GetLocations())
            {
                std::cout << "\tLocation: " << path << '\n';
                std::cout << "\t{\n";
                std::cout << "\t\tRoot: " << location.root << '\n';
				for (auto& i : location.index)
					std::cout << "\t\tIndex: " << i << '\n';
                std::cout << "\t\tAutoindex: " << std::boolalpha << location.autoindex << '\n';
				for (auto& i : location.cgi)
					std::cout << "\t\tCgi: " << i << '\n';
				std::cout << "\t\tRedirect: " << location.returnCode << '\n';
				std::cout << "\t\tMax Client Body Size: " << location.maxBodySize << "B\n";
				for (const auto& [key, value] : location.errorPageMap)
					std::cout << "\t\tError Page: " << key << " - " << value << '\n';
                std::cout << "\t\tMethods: " << MethodToString(location.httpMethods) << '\n';
                std::cout << "\t}\n";
            }
        }
        std::cout << "}\n";
    }
}