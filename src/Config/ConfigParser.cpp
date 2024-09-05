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

	tokens = tokenize(buffer);
	if (tokens.empty())
		throw std::runtime_error("no tokens found");

	tokenMap = assignTokenType(tokens);
	
	for (TokenMap::iterator it = tokenMap.begin(); it != tokenMap.end(); it++)
	{
		std::cout << std::setw(16) << escapeIdentifier(it->first) << std::setw(0) << " : " + it->second << '\n';
	}
	std::cout << "================================\n";

	return servers;
}

ConfigParser::Servers	ConfigParser::tokenMapToServerSettings(
	const TokenMap& tokenMap)
{
	Servers	servers;

	for (TokenMap::const_iterator it = tokenMap.begin(); it != tokenMap.end(); it++)
	{
		if (it->first == SERVER)
		{	
			ServerSettings	server = createServerSettings(tokenMap, it++);
			servers.emplace_back(server);
		}
	}
	return servers;
}

ServerSettings	ConfigParser:: createServerSettings(
	const TokenMap& tokenMap,
	TokenMap::const_iterator it)
{
	ServerSettings	server;

	if (it->first != BRACKET_OPEN)
		throw std::runtime_error("no bracket opening found");
	it++;
	for (; it != tokenMap.end(); it++)
	{

	}
	return server;
}

ServerSettings::LocationSettings	ConfigParser:: createLocationSettings(
	const TokenMap& tokenMap,
	TokenMap::const_iterator it)
{
	ServerSettings::LocationSettings location;

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
	const std::string& input, bool isDirective)
{
	const std::unordered_map<std::string, TokenIdentifier> DirectiveMap = {
		{"server", SERVER},
		{"listen", PORT},
		{"server_name", SERVER_NAME},
		{"root", ROOT},
		{"index", INDEX},
		{"autoindex", AUTOINDEX},
		{"return", REDIRECT},
		{"location", LOCATION},
		{"limit_except", HTTP_METHOD},
		{"deny", HTTP_METHOD_DENY},
	};
	const std::unordered_map<std::string, TokenIdentifier> OthersMap = {
		{"{", BRACKET_OPEN},
		{"}", BRACKET_CLOSE},
		{";", DIRECTIVE_END},
	};
	std::unordered_map<std::string, TokenIdentifier>::const_iterator it;

	if (isDirective)
	{
		it = DirectiveMap.find(input);
		if (it != DirectiveMap.end())
			return it->second;
		else
			return UNRECOGNISED;
	}
	else
	{
		it = OthersMap.find(input);
		if (it != OthersMap.end())
			return it->second;
		else
			return ARGUMENT;
	}
	return UNRECOGNISED;
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
		{HTTP_METHOD, "http_method"},
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

ConfigParser::TokenVector	ConfigParser:: tokenize(const std::string& input)
{
	ConfigParser::TokenVector	tokens;
	std::string					tokenBuffer;
	std::istringstream			tokenStream(input);
	bool						isComment = false;
	char						c;
	const std::string			delimiters = " \t\r\v\f\n";

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
	bool	expectDirective = true;

	for (std::vector<std::string>::const_iterator it = tokens.begin(); it != tokens.end(); it++)
	{
		TokenIdentifier	id = getIdentifier(*it, expectDirective);

		if (id == UNRECOGNISED)
			throw std::runtime_error("unrecognised directive");
		else if (id == BRACKET_OPEN
			|| id == BRACKET_CLOSE
			|| id == DIRECTIVE_END)
			expectDirective = true;
		else
			expectDirective = false;
		std::cout << (expectDirective == true ? "true" : "false") << '\n';
		std::cout << *it << '\n';
		std::cout << GREEN << escapeIdentifier(id) << '\n' << RESET;
		tokenIdMap.emplace_back(id, *it);
	}
	return tokenIdMap;
}
