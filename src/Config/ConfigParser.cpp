#include "ConfigParser.h"

#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <unordered_map>
#include <iomanip>
#include "Config.h"
#include "colours.hpp"
#include <cstdint>
#include <string>
#include <limits>

ConfigParser::Servers ConfigParser::createDefaultConfig()
{
	Servers servers = createConfigFromFile(DEFAULT_PATH);

	return servers;
}

ConfigParser::Servers ConfigParser::createConfigFromFile(
	const std::filesystem::path& path)    
{
	Servers		servers;
	TokenVector	tokens;
	TokenMap	tokenMap;
	std::string	buffer;

	if (path.extension() != ".conf")
		throw std::runtime_error(path.string() + ": invalid extension");

	buffer = readFileIntoBuffer(path);
	if (buffer.empty())
		throw std::runtime_error("file content is empty");

	tokens = tokenize(buffer, " \t\r\v\f\n");
	if (tokens.empty())
		throw std::runtime_error("no tokens found");

	tokenMap = assignTokenType(tokens);
	
	for (TokenMap::iterator it = tokenMap.begin(); it != tokenMap.end(); it++)
	{
		std::cout << std::setw(16) << escapeIdentifier(it->first) << std::setw(0) << " : " + it->second << '\n';
	}
	std::cout << "================================\n";
	tokenMapToServerSettings(tokenMap, servers);
	return servers;
}


ConfigParser::Servers	ConfigParser::tokenMapToServerSettings(
	const TokenMap& tokenMap,
	Servers& servers)
{
	for (TokenMap::const_iterator it = tokenMap.begin(); it != tokenMap.end(); it++)
	{
		if (it->first != SERVER)
			throw std::runtime_error("expected server directive: found \"" + it->second + '\"');
		servers.emplace_back(createServerSettings(tokenMap.end(), it));
	}
	return servers;
}

inline void	ConfigParser:: expectNextToken(
	const TokenMap::const_iterator& end, 
	TokenMap::const_iterator& it, 
	TokenIdentifier expected)
{
	it++;
	if (it == end)
		throw std::runtime_error("error with format");
	if (it->first != expected)
		throw std::invalid_argument("expected " + escapeIdentifier(expected) + ": found \"" + it->second + '\"');
}

inline bool	containsDigitsExclusively(const std::string& s)
{
	if (s.empty())
		return false;
	for (const char& c : s)
	{
		if (!isdigit(c))
			return false;
	}
	return true;
}

static uint32_t	ipv4LiteralToUint32(const std::string& s)
{
	std::istringstream	isstream(s);
	std::string			segment;
	int					shift = 24;
	uint32_t			result = 0;

	while (std::getline(isstream, segment, '.'))
	{
		if (!containsDigitsExclusively(segment) || segment.length() > 3)
			throw std::invalid_argument("error in ip formatting");
		uint32_t	segmentValue = static_cast<uint32_t>(std::stoul(segment));
		if (segmentValue > 255)
			throw std::invalid_argument("error in ip formatting");
		result |= (segmentValue << shift);
		shift = shift - 8;
	}
	if (shift != 0)
		throw std::invalid_argument("invalid ip literal: " + s);
	return result;
}

static uint16_t	portLiteralToUint16(const std::string& s)
{
	std::size_t	pos;
	int			port;
	
	port = std::stoi(s, &pos);
	if (pos != s.length())
		throw std::invalid_argument("invalid port format: " + s);
	if (port < 1 || port > std::numeric_limits<uint16_t>::max())
		throw std::out_of_range("port out of range: " + s);
	return static_cast<uint16_t>(port);
}

void	ConfigParser:: handlePort(
	std::vector<uint64_t> ports,
	const TokenMap::const_iterator& end,
	TokenMap::const_iterator& it)
{
	TokenVector	hostPort = tokenize(it->second, ":");
	uint32_t	host;
	uint16_t 	port;
	uint64_t	result = 0;

	if (hostPort.size() > 2)
		throw std::invalid_argument("invalid host:port format: " + it->second);
	if (hostPort.size() == 1)
	{
		host = 0;
		port = portLiteralToUint16(hostPort[0]);
	}
	else
	{
		host = ipv4LiteralToUint32(hostPort[0]);
		port = portLiteralToUint16(hostPort[1]);
	}
	result += host;
	result = result << 48;
	result += port;
	ports.emplace_back(result);
}

void	ConfigParser:: handleAutoIndex(
	bool& autoIndex,
	const TokenMap::const_iterator& end,
	TokenMap::const_iterator& it)
{
	if (it->second == "on")
		autoIndex = true;
	else if (it->second == "off")
		autoIndex = false;
	else
		throw std::invalid_argument("invalid autoindex argument: " + it->second);
}

void	ConfigParser:: handleLimitExcept(
	uint8_t& httpMethods,
	const TokenMap::const_iterator& end,
	TokenMap::const_iterator& it)
{
	for (; it != end; it++)
	{
		if (it->first == BRACKET_OPEN)
			break ;
		if (it->second == "GET")
			httpMethods ^= 1;
		else if (it->second == "POST")
			httpMethods ^= (1 << 1);
		else if (it->second == "DELETE")
			httpMethods ^= (1 << 2);
		else if (it->second == "PATCH")
			httpMethods ^= (1 << 3);
		else if (it->second == "PUT")
			httpMethods ^= (1 << 4);
		else
			throw std::invalid_argument("invalid http method: " + it->second);
	}
	expectNextToken(end, it, HTTP_METHOD_DENY);
	expectNextToken(end, it, ARGUMENT);
	if (it->second != "all")
		throw std::invalid_argument("invalid deny argument: " + it->second);
}

ServerSettings	ConfigParser:: createServerSettings(
	const TokenMap::const_iterator& end,
	TokenMap::const_iterator& it)
{
	ServerSettings			server;
	bool					expectDirective = true;

	expectNextToken(end, it, BRACKET_OPEN);
	it++;
	if (it == end)
		throw std::runtime_error("invalid format");
	for (; it != end; it++)
	{
		if (it->first == LOCATION)
		{
			expectNextToken(end, it, ARGUMENT);
			server.m_Locations.emplace(createLocationSettings(end, it));
		}
		else if (it->first == LIMIT_EXCEPT)
		{
			expectNextToken(end, it, ARGUMENT);
			handleLimitExcept(server.m_GlobalSettings.httpMethods, end, it);
			expectNextToken(end, it, BRACKET_CLOSE);
		}

		if (expectDirective && it->first == ARGUMENT)
			throw std::invalid_argument("expected directive: found \"" + it->second + '\"');
		expectDirective = false;

		if (it->first == SERVER_NAME)
		{
			expectNextToken(end, it, ARGUMENT);
			server.m_ServerName = it->second;
			expectNextToken(end, it, DIRECTIVE_END);
		}
		else if (it->first == PORT)
		{
			expectNextToken(end, it, ARGUMENT);
			handlePort(server.m_Ports, end, it);
			expectNextToken(end, it, DIRECTIVE_END);
		}
		else if (it->first == ROOT)
		{
			expectNextToken(end, it, ARGUMENT);
			server.m_GlobalSettings.root = it->second;
			expectNextToken(end, it, DIRECTIVE_END);
		}
		else if (it->first == INDEX)
		{
			expectNextToken(end, it, ARGUMENT);
			server.m_GlobalSettings.index = it->second;
			expectNextToken(end, it, DIRECTIVE_END);
		}
		else if (it->first == AUTOINDEX)
		{
			expectNextToken(end, it, ARGUMENT);
			handleAutoIndex(server.m_GlobalSettings.autoindex, end, it);
			expectNextToken(end, it, DIRECTIVE_END);
		}
		else if (it->first == BRACKET_CLOSE)
			break ;

		if (it->first == DIRECTIVE_END)
			expectDirective = true;
	}
	if (server.m_GlobalSettings.root.empty())
		throw std::runtime_error("no root specified");
	if (server.m_Ports.empty())
		throw std::runtime_error("no port(s) specified");
	return server;
}

ServerSettings::LocationSettings	ConfigParser:: createLocationSettings(
	const TokenMap::const_iterator& end,
	TokenMap::const_iterator& it)
{
	ServerSettings::LocationSettings location;
	bool							expectDirective = true;

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
		else if (it->first == LIMIT_EXCEPT)
		{
			expectNextToken(end, it, ARGUMENT);
			handleLimitExcept(location.httpMethods, end, it);
			expectNextToken(end, it, BRACKET_CLOSE);
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
		else if (it->first == ROOT)
		{
			expectNextToken(end, it, ARGUMENT);
			location.root = it->second;
			expectNextToken(end, it, DIRECTIVE_END);
		}
		else if (it->first == INDEX)
		{
			expectNextToken(end, it, ARGUMENT);
			location.index = it->second;
			expectNextToken(end, it, DIRECTIVE_END);
		}
		else if (it->first == AUTOINDEX)
		{
			expectNextToken(end, it, ARGUMENT);
			handleAutoIndex(location.autoindex, end, it);
			expectNextToken(end, it, DIRECTIVE_END);
		}
		else if (it->first == BRACKET_CLOSE)
			break ;

		if (it->first == DIRECTIVE_END)
			expectDirective = true;
	}
	return location;
}

std::string ConfigParser:: readFileIntoBuffer(const std::filesystem::path& path)
{
	std::ifstream file(path);
	if (!file.is_open())
	{
		throw std::runtime_error("could not open file: " + path.string());
	}

	std::stringstream buffer;
	buffer << file.rdbuf();
	file.close();
	return buffer.str();
}


ConfigParser::TokenIdentifier	ConfigParser:: getIdentifier(
	const std::string& input, bool expectDirective)
{
	const std::unordered_map<std::string, TokenIdentifier> TokenIdentifierMap = {
		{"server", SERVER},
		{"listen", PORT},
		{"server_name", SERVER_NAME},
		{"root", ROOT},
		{"index", INDEX},
		{"autoindex", AUTOINDEX},
		{"return", REDIRECT},
		{"location", LOCATION},
		{"limit_except", LIMIT_EXCEPT},
		{"deny", HTTP_METHOD_DENY},
		{"{", BRACKET_OPEN},
		{"}", BRACKET_CLOSE},
		{";", DIRECTIVE_END},
	};
	std::unordered_map<std::string, TokenIdentifier>::const_iterator it;

	it = TokenIdentifierMap.find(input);
	if (expectDirective && it != TokenIdentifierMap.end())
		return it->second;
	return ARGUMENT;
}

std::string	ConfigParser:: escapeIdentifier(TokenIdentifier id)
{
	const std::unordered_map<TokenIdentifier, std::string> idMap = {
		{SERVER, "server"},
		{PORT, "port"},
		{SERVER_NAME, "server name"},
		{ROOT, "root"},
		{INDEX, "index"},
		{AUTOINDEX, "autoindex"},
		{REDIRECT, "redirect"},
		{LOCATION, "location"},
		{LIMIT_EXCEPT, "http_method"},
		{HTTP_METHOD_DENY, "http_method_deny"},
		{BRACKET_OPEN, "bracket open"},
		{BRACKET_CLOSE, "bracket close"},
		{DIRECTIVE_END, "directive end"},
		{ARGUMENT, "argument"},
	};
	std::unordered_map<TokenIdentifier, std::string>::const_iterator it;

	it = idMap.find(id);
	if (it != idMap.end())
		return it->second;
	return "unrecognised";
}

static inline bool	isDelimiter(char& c, const std::string& delimiters)
{
	return delimiters.find(c) != std::string::npos;
}

static inline bool	isSpecialCharacter(char& c)
{
	return c == '{' || c == '}' || c == ';';
}

static inline void	addToken(
	std::vector<std::string>& tokens, 
	std::string& tokenBuffer)
{
	if (!tokenBuffer.empty())
	{
		tokens.push_back(tokenBuffer);
		tokenBuffer.clear();
	}
}

ConfigParser::TokenVector	ConfigParser:: tokenize(const std::string& input, const std::string& delimiters)
{
	ConfigParser::TokenVector	tokens;
	std::string					tokenBuffer;
	std::istringstream			tokenStream(input);
	bool						isComment = false;
	char						c;

	if (delimiters.empty())
	{
		throw std::runtime_error("empty delimiters");
	}
	while (tokenStream.get(c))
	{
		if (c == '#')
			isComment = true;
		else if (isDelimiter(c, delimiters))
			addToken(tokens, tokenBuffer);
		else if (!isComment)
		{
			if (isSpecialCharacter(c))
			{
				addToken(tokens, tokenBuffer);
				tokenBuffer += c;
				addToken(tokens, tokenBuffer);
			}
			else
				tokenBuffer += c;
		}
		if (c == '\n')
			isComment = false;
	}
	addToken(tokens, tokenBuffer);
	return tokens;
}

ConfigParser::TokenMap	ConfigParser:: assignTokenType(
	const ConfigParser::TokenVector& tokens)
{
	ConfigParser::TokenMap	tokenIdMap;
	bool					expectDirective = true;

	for (std::vector<std::string>::const_iterator it = tokens.begin(); it != tokens.end(); it++)
	{
		TokenIdentifier	id = getIdentifier(*it, expectDirective);

		tokenIdMap.emplace_back(id, *it);
		if (id == BRACKET_OPEN
			|| id == BRACKET_CLOSE
			|| id == DIRECTIVE_END)
			expectDirective = true;
		else
			expectDirective = false;
	}
	return tokenIdMap;
}
