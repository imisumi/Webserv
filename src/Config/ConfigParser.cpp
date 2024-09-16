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

Config	ConfigParser:: createDefaultConfig()
{
	Config	config = createConfigFromFile(DEFAULT_PATH);

	return config;
}

Config	ConfigParser:: createConfigFromFile(const std::filesystem::path& path)    
{
	Config			config;
	TokenVector		tokens;
	TokenMap		tokenMap;
	std::string		buffer;

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
	for (Servers::const_iterator it = config.m_Servers.begin(); it != config.m_Servers.end(); it++)
	{
		for (std::vector<uint64_t>::const_iterator itPort = it->m_Ports.begin(); itPort != it->m_Ports.end(); itPort++)
		{
			config.m_ServerMap[*itPort];
		}
	}
	for (Servers::iterator it = config.m_Servers.begin(); it != config.m_Servers.end(); it++)
	{
		for (std::vector<uint64_t>::const_iterator itPort = it->m_Ports.begin(); itPort != it->m_Ports.end(); itPort++)
		{
			config.m_ServerMap[*itPort].push_back(&(*it));
		}
	}
	return config;
}

void	ConfigParser::tokenMapToServerSettings(
	const TokenMap& tokenMap,
	Servers& servers)
{
	for (TokenMap::const_iterator it = tokenMap.begin(); it != tokenMap.end(); it++)
	{
		if (it->first != SERVER)
			throw std::runtime_error("expected server directive: found \"" + it->second + '\"');
		servers.push_back(createServerSettings(tokenMap.end(), it));
	}
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
		throw std::invalid_argument("expected " + identifierToString(expected) + ": found \"" + it->second + '\"');
}

static inline bool	containsDigitsExclusively(const std::string& s)
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
		if (shift < 0)
			throw std::invalid_argument("error in ip formatting");
		if (!containsDigitsExclusively(segment) || segment.length() > 3)
			throw std::invalid_argument("error in ip formatting");
		uint32_t	segmentValue = static_cast<uint32_t>(std::stoul(segment));
		if (segmentValue > 255)
			throw std::out_of_range("error in ip formatting");
		result |= (segmentValue << shift);
		shift -= 8;
	}
	return result;
}

static uint16_t	stringToUInt16(const std::string& s)
{
	std::size_t	pos;
	int			port;
	
	port = std::stoi(s, &pos);
	if (pos != s.length())
		throw std::invalid_argument("invalid uint16_t format: " + s);
	if (port < 0 || port > std::numeric_limits<uint16_t>::max())
		throw std::out_of_range("uint16_t out of range: " + s);
	return static_cast<uint16_t>(port);
}

void	ConfigParser:: handlePort(
	std::vector<uint64_t>& ports,
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
		port = stringToUInt16(hostPort[0]);
	}
	else
	{
		host = ipv4LiteralToUint32(hostPort[0]);
		port = stringToUInt16(hostPort[1]);
	}
	if (port == 0)
		throw std::invalid_argument("port cannot be 0");
	result += (static_cast<uint64_t>(host) << 32) + static_cast<uint64_t>(port);
	ports.emplace_back(result);
}

void	ConfigParser:: handleRoot(
	std::filesystem::path& root,
	const TokenMap::const_iterator& end,
	TokenMap::const_iterator& it)
{
	if (it->second[0] == '/')
	{
		root = it->second;
	}
	else
	{
		root = std::filesystem::current_path() / it->second;
	}
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
	uint8_t	methods = 0;

	for (; it != end; it++)
	{
		if (it->first == BRACKET_OPEN)
			break ;
		if (it->second == "GET")
			methods |= 1;
		else if (it->second == "POST")
			methods |= (1 << 1);
		else if (it->second == "DELETE")
			methods |= (1 << 2);
		else if (it->second == "PATCH")
			methods |= (1 << 3);
		else if (it->second == "PUT")
			methods |= (1 << 4);
		else
			throw std::invalid_argument("invalid http method: " + it->second);
	}
	httpMethods = methods;
	expectNextToken(end, it, HTTP_METHOD_DENY);
	expectNextToken(end, it, ARGUMENT);
	if (it->second != "all")
		throw std::invalid_argument("invalid deny argument: " + it->second);
	expectNextToken(end, it, DIRECTIVE_END);
	expectNextToken(end, it, BRACKET_CLOSE);
}

void	ConfigParser:: handleIndex(
	std::vector<std::string>& indexFiles,
	const TokenMap::const_iterator& end,
	TokenMap::const_iterator& it)
{
	for (; it != end; it++)
	{
		if (it->first == DIRECTIVE_END)
		{
			it--;
			break ;
		}
		if (it->first != ARGUMENT)
			throw std::invalid_argument("invalid index argument: " + it->second);
		indexFiles.push_back(it->second);
	}
	if (it == end)
		throw std::invalid_argument("invalid index format");
}

void	ConfigParser:: handleCgi(
	std::vector<std::string>& cgi,
	const TokenMap::const_iterator& end,
	TokenMap::const_iterator& it)
{
	for (; it != end; it++)
	{
		if (it->first == DIRECTIVE_END)
		{
			it--;
			break ;
		}
		if (it->first != ARGUMENT)
			throw std::invalid_argument("invalid cgi argument: " + it->second);
		cgi.push_back(it->second);
	}
	if (it == end)
		throw std::invalid_argument("invalid cgi format");
}

void	ConfigParser:: handleRedirect(
	uint16_t& redirect,
	const TokenMap::const_iterator& end,
	TokenMap::const_iterator& it)
{
	redirect = stringToUInt16(it->second);
	if (redirect < 100 || redirect > 599)
		throw std::invalid_argument("invalid redirect code");
}

void	ConfigParser:: handleErrorPage(
	std::unordered_map<uint16_t, std::filesystem::path>& errorPageMap,
	const TokenMap::const_iterator& end,
	TokenMap::const_iterator& it)
{
	std::vector<uint16_t>	returnCodes;

	if (!containsDigitsExclusively(it->second))
		throw std::invalid_argument("invalid error page format");
	for (; it != end; it++)
	{
		if (it->first != ARGUMENT)
			throw std::invalid_argument("invalid error page argument: " + it->second);
		if (containsDigitsExclusively(it->second))
		{
			const uint16_t currentReturnCode = stringToUInt16(it->second);
			if (currentReturnCode < 100 || currentReturnCode > 599)
				throw std::invalid_argument("invalid error page redirect code");
			auto result = errorPageMap.insert({currentReturnCode, std::filesystem::path()});
			if (result.second)
				returnCodes.push_back(currentReturnCode);
		}
		else
		{
			break ;
		}
	}
	if (it == end)
		throw std::invalid_argument("invalid error page format");
	if (it->first != ARGUMENT)
		throw std::invalid_argument("invalid error page argument: " + it->second);
	for (const uint16_t& code : returnCodes)
	{
		errorPageMap[code] = it->second;
	}
}

void	ConfigParser:: handleMaxBodySize(
	uint64_t& maxBodySize,
	const TokenMap::const_iterator& end,
	TokenMap::const_iterator& it)
{
	uint64_t	initialSize;
	std::size_t	pos;

	initialSize = static_cast<uint64_t>(std::stoull(it->second, &pos));
	const std::string unit = it->second.substr(pos);
	if (unit.empty())
	{
		maxBodySize = initialSize;
	}
	else if (unit == "B")
	{
		maxBodySize = initialSize;
	}
	else if (unit == "KB")
	{
		maxBodySize = initialSize * 1024;
		if (maxBodySize != 0 && maxBodySize / 1024 != initialSize)
			throw std::out_of_range("max body size overflow");
	}
	else if (unit == "MB")
	{
		maxBodySize = initialSize * 1024 * 1024;
		if (maxBodySize != 0 && maxBodySize / 1024 / 1024 != initialSize)
			throw std::out_of_range("max body size overflow");
	}
	else if (unit == "GB")
	{
		maxBodySize = initialSize * 1024 * 1024 * 1024;
		if (maxBodySize != 0 && maxBodySize / 1024 / 1024 / 1024 != initialSize)
			throw std::out_of_range("max body size overflow");
	}
	else
	{
		throw std::invalid_argument("invalid unit type: " + it->second);
	}
}

ServerSettings	ConfigParser:: createServerSettings(
	const TokenMap::const_iterator& end,
	TokenMap::const_iterator& it)
{
	ServerSettings	server;
	bool			expectDirective = true;

	expectNextToken(end, it, BRACKET_OPEN);
	it++;
	if (it == end)
		throw std::runtime_error("invalid format");
	for (; it != end; it++)
	{
		if (it->first == LOCATION)
		{
			expectNextToken(end, it, ARGUMENT);
			const std::filesystem::path location = it->second;
			server.m_Locations.emplace(location, createLocationSettings(end, it));
			continue ;
		}
		else if (it->first == LIMIT_EXCEPT)
		{
			expectNextToken(end, it, ARGUMENT);
			handleLimitExcept(server.m_GlobalSettings.httpMethods, end, it);
			continue ;
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
			handleRoot(server.m_GlobalSettings.root, end, it);
			expectNextToken(end, it, DIRECTIVE_END);
		}
		else if (it->first == INDEX)
		{
			expectNextToken(end, it, ARGUMENT);
			handleIndex(server.m_GlobalSettings.index, end, it);
			expectNextToken(end, it, DIRECTIVE_END);
		}
		else if (it->first == CGI)
		{
			expectNextToken(end, it, ARGUMENT);
			handleCgi(server.m_GlobalSettings.cgi, end, it);
			expectNextToken(end, it, DIRECTIVE_END);
		}
		else if (it->first == REDIRECT)
		{
			expectNextToken(end, it, ARGUMENT);
			handleRedirect(server.m_GlobalSettings.returnCode, end, it);
			expectNextToken(end, it, DIRECTIVE_END);
		}
		else if (it->first == ERROR_PAGE)
		{
			expectNextToken(end, it, ARGUMENT);
			handleErrorPage(server.m_GlobalSettings.errorPageMap, end, it);
			expectNextToken(end, it, DIRECTIVE_END);
		}
		else if (it->first == MAX_BODY_SIZE)
		{
			expectNextToken(end, it, ARGUMENT);
			handleMaxBodySize(server.m_GlobalSettings.maxBodySize, end, it);
			expectNextToken(end, it, DIRECTIVE_END);
		}
		else if (it->first == AUTOINDEX)
		{
			expectNextToken(end, it, ARGUMENT);
			handleAutoIndex(server.m_GlobalSettings.autoindex, end, it);
			expectNextToken(end, it, DIRECTIVE_END);
		}
		else if (it->first == BRACKET_CLOSE)
		{
			break ;
		}

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
	ServerSettings::LocationSettings	location;
	bool								expectDirective = true;

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
			continue ;
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
			handleRoot(location.root, end, it);
			expectNextToken(end, it, DIRECTIVE_END);
		}
		else if (it->first == INDEX)
		{
			expectNextToken(end, it, ARGUMENT);
			handleIndex(location.index, end, it);
			expectNextToken(end, it, DIRECTIVE_END);
		}
		else if (it->first == CGI)
		{
			expectNextToken(end, it, ARGUMENT);
			handleCgi(location.cgi, end, it);
			expectNextToken(end, it, DIRECTIVE_END);
		}
		else if (it->first == REDIRECT)
		{
			expectNextToken(end, it, ARGUMENT);
			handleRedirect(location.returnCode, end, it);
			expectNextToken(end, it, DIRECTIVE_END);
		}
		else if (it->first == ERROR_PAGE)
		{
			expectNextToken(end, it, ARGUMENT);
			handleErrorPage(location.errorPageMap, end, it);
			expectNextToken(end, it, DIRECTIVE_END);
		}
		else if (it->first == MAX_BODY_SIZE)
		{
			expectNextToken(end, it, ARGUMENT);
			handleMaxBodySize(location.maxBodySize, end, it);
			expectNextToken(end, it, DIRECTIVE_END);
		}
		else if (it->first == AUTOINDEX)
		{
			expectNextToken(end, it, ARGUMENT);
			handleAutoIndex(location.autoindex, end, it);
			expectNextToken(end, it, DIRECTIVE_END);
		}
		else if (it->first == BRACKET_CLOSE)
		{
			break ;
		}

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
		{"cgi", CGI},
		{"autoindex", AUTOINDEX},
		{"return", REDIRECT},
		{"location", LOCATION},
		{"limit_except", LIMIT_EXCEPT},
		{"error_page", ERROR_PAGE},
		{"client_max_body_size", MAX_BODY_SIZE},
		{"deny", HTTP_METHOD_DENY},
	};
	const std::unordered_map<std::string, TokenIdentifier> NonDirectiveMap = {
		{"{", BRACKET_OPEN},
		{"}", BRACKET_CLOSE},
		{";", DIRECTIVE_END},
	};
	std::unordered_map<std::string, TokenIdentifier>::const_iterator it;

	it = NonDirectiveMap.find(input);
	if (it != NonDirectiveMap.end())
		return it->second;
	if (expectDirective)
	{
		it = TokenIdentifierMap.find(input);
		if (it != TokenIdentifierMap.end())
			return it->second;
	}
	return ARGUMENT;
}

std::string	ConfigParser:: identifierToString(TokenIdentifier id)
{
	const std::unordered_map<TokenIdentifier, std::string> idMap = {
		{SERVER, "server"},
		{PORT, "port"},
		{SERVER_NAME, "server name"},
		{ROOT, "root"},
		{INDEX, "index"},
		{CGI, "cgi"},
		{AUTOINDEX, "autoindex"},
		{REDIRECT, "redirect"},
		{LOCATION, "location"},
		{LIMIT_EXCEPT, "http_method"},
		{ERROR_PAGE, "error_page"},
		{MAX_BODY_SIZE, "client_max_body_size"},
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
