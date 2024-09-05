#include "ConfigParser.h"

#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <unordered_map>
#include <iomanip>

std::vector<ServerSettings>	ConfigParser::createDefaultConfig()
{
	std::vector<ServerSettings>	servers;

	return servers;
}

std::vector<ServerSettings>	ConfigParser::createConfigFromFile(const std::filesystem::path& path)    
{
	std::vector<ServerSettings>	servers;

	if (path.extension() != ".conf")
		throw std::runtime_error(path.string() + ": invalid extension");

	const std::string buffer = readFileIntoBuffer(path);
	if (buffer.empty())
		throw std::runtime_error("file content is empty");

	std::vector<std::string>	tokens = tokenize(buffer);

	if (tokens.empty())
		throw std::runtime_error("no tokens found");
	std::cout << "================================\n";
	std::cout << buffer << "\n";
	std::cout << "================================\n";

	for (std::vector<std::string>::iterator it = tokens.begin(); it != tokens.end(); it++)
		std::cout << *it << '\n';
	std::cout << "================================\n";
	
	for (std::vector<std::string>::iterator it = tokens.begin(); it != tokens.end(); it++)
	{
		std::cout << std::setw(16) << escapeIdentifier(getIdentifier(*it)) << std::setw(0) << " : " + *it << '\n';
	}
	std::cout << "================================\n";
	TokenMap	tokenMap = assignTokenType(tokens);

	return servers;
}

static std::string readFileIntoBuffer(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("could not open file: " + path.string());
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
	file.close();
    return buffer.str();
}


ConfigParser::TokenIdentifier	ConfigParser:: getIdentifier(
	const std::string& input)
{
	const std::unordered_map<std::string, TokenIdentifier> idMap = {
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
		{"{", BRACKET_OPEN},
		{"}", BRACKET_CLOSE},
		{";", DIRECTIVE_END},
	};
	std::unordered_map<std::string, TokenIdentifier>::const_iterator it;

	it = idMap.find(input);
	if (it != idMap.end())
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
		{HTTP_METHOD, "http_method"},
		{HTTP_METHOD_DENY, "http_method_deny"},
		{BRACKET_OPEN, "bracket open"},
		{BRACKET_CLOSE, "bracket close"},
		{DIRECTIVE_END, "directive end"},
	};
	std::unordered_map<TokenIdentifier, std::string>::const_iterator it;

	it = idMap.find(id);
	if (it != idMap.end())
		return it->second;
	return "argument";
}


static inline bool	isDelimiter(char& c, const std::string& delimiters)
{
	return delimiters.find(c) != std::string::npos;
}

static inline bool	isSpecialCharacter(char& c)
{
	return c == '{' || c == '}' || c == ';';
}

static inline void	addToken(std::vector<std::string>& tokens, std::string& tokenBuffer)
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

	for (std::vector<std::string>::const_iterator it = tokens.begin(); it != tokens.end(); it++)
	{
		tokenIdMap.emplace_back(getIdentifier(*it), *it);
	}
	return tokenIdMap;
}
