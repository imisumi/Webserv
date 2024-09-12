/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   Config.cpp                                         :+:    :+:            */
/*                                                     +:+                    */
/*   By: kwchu <kwchu@student.codam.nl>               +#+                     */
/*                                                   +#+                      */
/*   Created: 2024/09/05 13:09:31 by kwchu         #+#    #+#                 */
/*   Updated: 2024/09/12 15:05:52 by kwchu         ########   odam.nl         */
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
        std::cout << "\tServer name: " << value[0]->GetServerName() << std::endl;
        std::cout << "\tListening on: " << ip << ":" << port << std::endl;
        for (const auto& server : value)
        {
            std::cout << "\tRoot: " << server->GetGlobalSettings().root << std::endl;
			for (auto& i : server->GetGlobalSettings().index)
				std::cout << "\tIndex: " << i << std::endl;
            std::cout << "\tAutoindex: " << std::boolalpha << server->GetGlobalSettings().autoindex << std::endl;
			for (auto& i : server->GetGlobalSettings().cgi)
				std::cout << "\tCgi: " << i << std::endl;
            std::cout << "\tMethods: " << MethodToString(server->GetGlobalSettings().httpMethods) << std::endl;
            std::cout << std::endl;
            for (const auto& [path, location] : server->GetLocations())
            {
                std::cout << "\tLocation: " << path << std::endl;
                std::cout << "\t{\n";
                std::cout << "\t\tRoot: " << location.root << std::endl;
				for (auto& i : location.index)
					std::cout << "\t\tIndex: " << i << std::endl;
                std::cout << "\t\tAutoindex: " << std::boolalpha << location.autoindex << std::endl;
				for (auto& i : location.cgi)
					std::cout << "\t\tCgi: " << i << std::endl;
                std::cout << "\t\tMethods: " << MethodToString(location.httpMethods) << std::endl;
                std::cout << "\t}\n";
            }
        }
        std::cout << "}\n";
    }
}