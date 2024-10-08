/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   ConfigParser.cpp                                   :+:    :+:            */
/*                                                     +:+                    */
/*   By: imisumi-wsl <imisumi-wsl@student.42.fr>      +#+                     */
/*                                                   +#+                      */
/*   Created: 2024/09/18 12:53:07 by kwchu         #+#    #+#                 */
/*   Updated: 2024/10/08 15:53:43 by kwchu         ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include "ConfigParser.h"
#include "Config.h"
#include "Utils/Utils.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

Config ConfigParser::createDefaultConfig()
{
	Config config = createConfigFromFile(DEFAULT_PATH);

	return config;
}

Config ConfigParser::createConfigFromFile(const std::filesystem::path& path)
{
	Config config;
	TokenVector tokens;
	TokenMap tokenMap;
	std::string buffer;

	if (path.extension() != ".conf")
		throw std::runtime_error(path.string() + ": invalid extension");

	buffer = readFileIntoBuffer(path);
	if (buffer.empty())
		throw std::runtime_error("file content is empty");

	tokens = tokenize(buffer, " \t\r\v\f\n");
	if (tokens.empty())
		throw std::runtime_error("no tokens found");

	tokenMap = assignTokenType(tokens);
	tokenMapToServerSettings(tokenMap, config.m_Servers);
	assignPortToServerSettings(config.m_ServerMap, config.m_Servers);
	return config;
}

void ConfigParser::tokenMapToServerSettings(const TokenMap& tokenMap, Servers& servers)
{
	for (TokenMap::const_iterator it = tokenMap.begin(); it != tokenMap.end(); it++)
	{
		if (it->first != SERVER)
			throw std::runtime_error("expected server directive: found \"" + it->second + '\"');
		servers.push_back(createServerSettings(tokenMap.end(), it));
	}
}

void ConfigParser::assignPortToServerSettings(ServerMap& serverMap, Servers& servers)
{
	for (auto& server : servers)
	{
		for (const auto& port : server.m_Ports)
		{
			serverMap[port];
			if (serverMap[port].empty())
				serverMap[port].push_back(&server);
			else
			{
				for (const auto& serverSettings : serverMap[port])
				{
					if (serverSettings->m_ServerName != server.m_ServerName)
					{
						serverMap[port].push_back(&server);
					}
					else
					{
						auto [ip, port2] = Utils::unpackIpAndPort(port);
						LOG_WARN("Ignored server block on port {}:{} with same server_name {}", ip, port2, server.m_ServerName);
					}
				}
			}
		}
	}
}

inline void ConfigParser::expectNextToken(const TokenMap::const_iterator& end, TokenMap::const_iterator& it,
										  TokenIdentifier expected)
{
	it++;
	if (it == end)
		throw std::runtime_error("error with format");
	if (it->first != expected)
		throw std::invalid_argument("expected " + identifierToString(expected) + ": found \"" + it->second + '\"');
}

ServerSettings ConfigParser::createServerSettings(const TokenMap::const_iterator& end, TokenMap::const_iterator& it)
{
	ServerSettings server;
	bool expectDirective = true;
	uint16_t repeatDirective = 0;
	std::vector<TokenMap::const_iterator> locationStart;

	expectNextToken(end, it, BRACKET_OPEN);
	it++;
	if (it == end)
		throw std::runtime_error("invalid format");
	for (; it != end; it++)
	{
		if (it->first == LOCATION)
		{
			locationStart.push_back(it);
			ServerSettings::LocationSettings dummyLocation;
			expectNextToken(end, it, ARGUMENT);
			handleLocationSettings(dummyLocation, end, it);
			continue;
		}
		else if (it->first == LIMIT_EXCEPT && !(repeatDirective & (1 << LIMIT_EXCEPT)))
		{
			expectNextToken(end, it, ARGUMENT);
			handleLimitExcept(server.m_GlobalSettings.httpMethods, end, it);
			repeatDirective |= (1 << LIMIT_EXCEPT);
			continue;
		}

		if (expectDirective && it->first == ARGUMENT)
			throw std::invalid_argument("expected directive: found \"" + it->second + '\"');
		expectDirective = false;

		if (it->first == SERVER_NAME && !(repeatDirective & (1 << SERVER_NAME)))
		{
			expectNextToken(end, it, ARGUMENT);
			server.m_ServerName = it->second;
			expectNextToken(end, it, DIRECTIVE_END);
			repeatDirective |= (1 << SERVER_NAME);
		}
		else if (it->first == PORT)
		{
			expectNextToken(end, it, ARGUMENT);
			handlePort(server.m_Ports, end, it);
			expectNextToken(end, it, DIRECTIVE_END);
		}
		else if (it->first == ROOT && !(repeatDirective & (1 << ROOT)))
		{
			expectNextToken(end, it, ARGUMENT);
			handleRoot(server.m_GlobalSettings.root, end, it);
			expectNextToken(end, it, DIRECTIVE_END);
			repeatDirective |= (1 << ROOT);
		}
		else if (it->first == INDEX && !(repeatDirective & (1 << INDEX)))
		{
			expectNextToken(end, it, ARGUMENT);
			handleIndex(server.m_GlobalSettings.index, end, it);
			expectNextToken(end, it, DIRECTIVE_END);
			repeatDirective |= (1 << INDEX);
		}
		else if (it->first == CGI && !(repeatDirective & (1 << CGI)))
		{
			expectNextToken(end, it, ARGUMENT);
			handleCgi(server.m_GlobalSettings.cgi, end, it);
			expectNextToken(end, it, DIRECTIVE_END);
			repeatDirective |= (1 << CGI);
		}
		else if (it->first == REDIRECT && !(repeatDirective & (1 << REDIRECT)))
		{
			expectNextToken(end, it, ARGUMENT);
			handleRedirect(server.m_GlobalSettings.redirect, end, it);
			expectNextToken(end, it, DIRECTIVE_END);
			repeatDirective |= (1 << REDIRECT);
		}
		else if (it->first == ERROR_PAGE)
		{
			expectNextToken(end, it, ARGUMENT);
			handleErrorPage(server.m_GlobalSettings.errorPageMap, end, it);
			expectNextToken(end, it, DIRECTIVE_END);
		}
		else if (it->first == MAX_BODY_SIZE && !(repeatDirective & (1 << MAX_BODY_SIZE)))
		{
			expectNextToken(end, it, ARGUMENT);
			handleMaxBodySize(server.m_GlobalSettings.maxBodySize, end, it);
			expectNextToken(end, it, DIRECTIVE_END);
			repeatDirective |= (1 << MAX_BODY_SIZE);
		}
		else if (it->first == AUTOINDEX && !(repeatDirective & (1 << AUTOINDEX)))
		{
			expectNextToken(end, it, ARGUMENT);
			handleAutoIndex(server.m_GlobalSettings.autoindex, end, it);
			expectNextToken(end, it, DIRECTIVE_END);
			repeatDirective |= (1 << AUTOINDEX);
		}
		else if (it->first == BRACKET_CLOSE)
		{
			break;
		}

		if (it->first == DIRECTIVE_END)
			expectDirective = true;
	}
	for (TokenMap::const_iterator loc : locationStart)
	{
		expectNextToken(end, loc, ARGUMENT);
		ServerSettings::LocationSettings location = server.m_GlobalSettings;
		const std::filesystem::path locationPath = loc->second;
		handleLocationSettings(location, end, loc);
		server.m_Locations.emplace(locationPath, location);
	}
	if (server.m_GlobalSettings.root.empty())
		throw std::runtime_error("no root specified");
	if (server.m_Ports.empty())
		throw std::runtime_error("no port(s) specified");
	return server;
}

void ConfigParser::handleLocationSettings(ServerSettings::LocationSettings& location,
										  const TokenMap::const_iterator& end, TokenMap::const_iterator& it)
{
	bool expectDirective = true;
	uint16_t repeatDirective = 0;

	expectNextToken(end, it, BRACKET_OPEN);
	it++;
	if (it == end)
		throw std::runtime_error("invalid format");
	for (; it != end; it++)
	{
		if (it->first == LOCATION)
		{
			throw std::runtime_error("nested locations not allowed");
		}
		else if (it->first == LIMIT_EXCEPT && !(repeatDirective & (1 << LIMIT_EXCEPT)))
		{
			expectNextToken(end, it, ARGUMENT);
			handleLimitExcept(location.httpMethods, end, it);
			repeatDirective |= (1 << LIMIT_EXCEPT);
			continue;
		}

		if (expectDirective && it->first == ARGUMENT)
			throw std::invalid_argument("expected directive: found \"" + it->second + '\"');
		expectDirective = false;
		if (it->first == SERVER_NAME)
		{
			throw std::runtime_error("server name not allowed in location context");
		}
		else if (it->first == PORT)
		{
			throw std::runtime_error("port not allowed in location context");
		}
		else if (it->first == ROOT && !(repeatDirective & (1 << ROOT)))
		{
			expectNextToken(end, it, ARGUMENT);
			handleRoot(location.root, end, it);
			expectNextToken(end, it, DIRECTIVE_END);
			repeatDirective |= (1 << ROOT);
		}
		else if (it->first == INDEX && !(repeatDirective & (1 << INDEX)))
		{
			expectNextToken(end, it, ARGUMENT);
			handleIndex(location.index, end, it);
			expectNextToken(end, it, DIRECTIVE_END);
			repeatDirective |= (1 << INDEX);
		}
		else if (it->first == CGI && !(repeatDirective & (1 << CGI)))
		{
			expectNextToken(end, it, ARGUMENT);
			handleCgi(location.cgi, end, it);
			expectNextToken(end, it, DIRECTIVE_END);
			repeatDirective |= (1 << CGI);
		}
		else if (it->first == REDIRECT && !(repeatDirective & (1 << REDIRECT)))
		{
			expectNextToken(end, it, ARGUMENT);
			handleRedirect(location.redirect, end, it);
			expectNextToken(end, it, DIRECTIVE_END);
			repeatDirective |= (1 << REDIRECT);
		}
		else if (it->first == ERROR_PAGE)
		{
			expectNextToken(end, it, ARGUMENT);
			handleErrorPage(location.errorPageMap, end, it);
			expectNextToken(end, it, DIRECTIVE_END);
		}
		else if (it->first == MAX_BODY_SIZE && !(repeatDirective & (1 << MAX_BODY_SIZE)))
		{
			expectNextToken(end, it, ARGUMENT);
			handleMaxBodySize(location.maxBodySize, end, it);
			expectNextToken(end, it, DIRECTIVE_END);
			repeatDirective |= (1 << MAX_BODY_SIZE);
		}
		else if (it->first == AUTOINDEX && !(repeatDirective & (1 << AUTOINDEX)))
		{
			expectNextToken(end, it, ARGUMENT);
			handleAutoIndex(location.autoindex, end, it);
			expectNextToken(end, it, DIRECTIVE_END);
			repeatDirective |= (1 << AUTOINDEX);
		}
		else if (it->first == BRACKET_CLOSE)
		{
			break;
		}

		if (it->first == DIRECTIVE_END)
			expectDirective = true;
	}
}
