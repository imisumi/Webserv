#include "ConfigParser.h"

#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <unordered_map>

std::shared_ptr<Config> Config::CreateDefaultConfig()
{
	//TODO: have defualt config
	//? Can't use make_shared because constructor is private
	return std::shared_ptr<Config>(new Config(std::filesystem::path{}));
}

std::shared_ptr<Config> Config::CreateConfigFromFile(const std::filesystem::path& path)    
{
	//? Can't use make_shared because constructor is private
	return std::shared_ptr<Config>(new Config(path));
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

enum ConfigIdentifier
{
	SERVER,
	PORT,
	SERVER_NAME,
	ROOT,
	INDEX,
	AUTOINDEX,
	REDIRECT,
	LOCATION,
	BRACKET_OPEN,
	BRACKET_CLOSE,
	ARGUMENTS,
};

static enum ConfigIdentifier	getIdentifier(const std::string& input)
{
	const std::unordered_map<std::string, ConfigIdentifier> idMap = {
		{"server", SERVER},
		{"listen", PORT},
		{"server_name", SERVER_NAME},
		{"root", ROOT},
		{"index", INDEX},
		{"autoindex", AUTOINDEX},
		{"return", REDIRECT},
		{"location", LOCATION},
		{"{", BRACKET_OPEN},
		{"}", BRACKET_CLOSE},
	};
	std::unordered_map<std::string, ConfigIdentifier>::const_iterator it;

	it = idMap.find(input);
	if (it != idMap.end())
		return it->second;
	return ARGUMENTS;
}

static std::string	escapeIdentifier(ConfigIdentifier id)
{
	const std::unordered_map<ConfigIdentifier, std::string> idMap = {
		{SERVER, "server"},
		{PORT, "port"},
		{SERVER_NAME, "server name"},
		{ROOT, "root"},
		{INDEX, "index"},
		{AUTOINDEX, "autoindex"},
		{REDIRECT, "redirect"},
		{LOCATION, "location"},
		{BRACKET_OPEN, "bracket open"},
		{BRACKET_CLOSE, "bracket close"},
	};
	std::unordered_map<ConfigIdentifier, std::string>::const_iterator it;

	it = idMap.find(id);
	if (it != idMap.end())
		return it->second;
	return "arguments";
}


static inline bool	isDelimiter(char c, const std::string& delimiters)
{
	return delimiters.find(c) != std::string::npos;
}

static std::vector<std::string>	tokenizeString(const std::string& s, const std::string& delimiters)
{
	std::vector<std::string>	tokens;
	std::string					parsedToken;
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
		{
			if (!parsedToken.empty())
			{
				tokens.push_back(parsedToken);
				parsedToken.clear();
			}
		}
		else if (!isComment)
			parsedToken += c;
		if (c == '\n')
			isComment = false;
	}
	if (!parsedToken.empty())
		tokens.push_back(parsedToken);
	return tokens;
}

static bool	validBracketCount(std::vector<std::string>& tokens)
{
	size_t	openCount = 0;
	size_t	closedCount = 0;
	ConfigIdentifier	id;

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

Config::Config(const std::filesystem::path& path)
	: m_Path(path)
{
	std::string	buffer;

	if (!stringEndsWith(path, ".conf"))
		throw std::runtime_error(path.string() + ": invalid extension");

	buffer = readFileIntoBuffer(path);
	if (buffer.empty())
		throw std::runtime_error("file content is empty");

	std::vector<std::string>	tokens = tokenizeString(buffer, " \t\r\v\f\n");

	std::cout << "================================\n";
	std::cout << buffer << "\n";
	std::cout << "================================\n";

	for (std::vector<std::string>::iterator it = tokens.begin(); it != tokens.end(); it++)
		std::cout << *it << '\n';
	std::cout << "================================\n";
	
	if (!validBracketCount(tokens))
		throw std::runtime_error("error with bracket format");
	std::cout << '\n';
	for (std::vector<std::string>::iterator it = tokens.begin(); it != tokens.end(); it++)
	{
		std::cout << std::setw(15) << escapeIdentifier(getIdentifier(*it)) << std::setw(0) << "\t: " + *it << '\n';
	}
}
