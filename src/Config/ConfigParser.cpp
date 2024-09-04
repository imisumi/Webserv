#include "ConfigParser.h"

#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <unordered_map>
#include <iomanip>

ConfigParser	ConfigParser::CreateDefaultConfig()
{
	//TODO: have defualt config
	//? Can't use make_shared because constructor is private
	return ConfigParser("");
}

Config	ConfigParser::CreateConfigFromFile(const std::filesystem::path& path)    
{
	//? Can't use make_shared because constructor is private
	return ConfigParser(path);
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

static inline bool	stringEndsWith(const std::string& s, const std::string& end)
{
	if (s.length() >= end.length())
		return s.compare(s.length() - end.length(), end.length(), end) == 0;
	return false;
}

enum TokenIdentifier
{
	SERVER,
	PORT,
	SERVER_NAME,
	ROOT,
	INDEX,
	AUTOINDEX,
	REDIRECT,
	LOCATION,
	HTTP_METHOD,
	HTTP_METHOD_DENY,
	BRACKET_OPEN,
	BRACKET_CLOSE,
	DIRECTIVE_END,
	ARGUMENT,
};

static enum TokenIdentifier	getIdentifier(const std::string& input)
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

static std::string	escapeIdentifier(TokenIdentifier id)
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

static std::vector<std::string>	tokenizeString(const std::string& s, const std::string& delimiters)
{
	std::vector<std::string>	tokens;
	std::string					tokenBuffer;
	std::istringstream			tokenStream(s);
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

static bool	validBracketCount(std::vector<std::string>& tokens)
{
	size_t	openCount = 0;
	size_t	closedCount = 0;
	TokenIdentifier	id;

	for (std::vector<std::string>::iterator it = tokens.begin(); it != tokens.end(); it++)
	{
		id = getIdentifier(*it);
		if (id == BRACKET_OPEN)
			openCount++;
		else if (id == BRACKET_CLOSE)
			closedCount++;
	}
	return openCount == closedCount;
}

typedef std::vector<std::pair<TokenIdentifier, std::string>> TokenMap;

static TokenMap	createTokenIdMap(
	const std::vector<std::string>& tokens)
{
	std::vector<std::pair<TokenIdentifier, std::string>>	tokenIdMap;

	for (std::vector<std::string>::const_iterator it = tokens.begin(); it != tokens.end(); it++)
	{
		tokenIdMap.emplace_back(getIdentifier(*it), *it);
	}
	return tokenIdMap;
}

ConfigParser::ConfigParser(const std::filesystem::path& path)
{
	if (path.extension() != ".conf")
		throw std::runtime_error(path.string() + ": invalid extension");

	const std::string buffer = readFileIntoBuffer(path);
	if (buffer.empty())
		throw std::runtime_error("file content is empty");

	std::vector<std::string>	tokens = tokenizeString(buffer, " \t\r\v\f\n");

	if (tokens.empty())
		throw std::runtime_error("no tokens found");
	std::cout << "================================\n";
	std::cout << buffer << "\n";
	std::cout << "================================\n";

	for (std::vector<std::string>::iterator it = tokens.begin(); it != tokens.end(); it++)
		std::cout << *it << '\n';
	std::cout << "================================\n";
	
	if (!validBracketCount(tokens))
		throw std::runtime_error("error with bracket format");
	
	for (std::vector<std::string>::iterator it = tokens.begin(); it != tokens.end(); it++)
	{
		std::cout << std::setw(16) << escapeIdentifier(getIdentifier(*it)) << std::setw(0) << " : " + *it << '\n';
	}
	std::cout << "================================\n";
	TokenMap	tokenMap = createTokenIdMap(tokens);

	
	this->tokens = tokens;
}
