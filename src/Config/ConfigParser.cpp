#include "ConfigParser.h"

#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

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
	INVALID,
};

enum ConfigIdentifier	getIdentifier(const std::string& input)
{
	const std::string stringIdTable[] = {
		"server",
		"listen",
		"server_name",
		"root",
		"index",
		"autoindex",
		"return",
		"location",
	};

	enum ConfigIdentifier	idTable[] = {
		SERVER,
		PORT,
		SERVER_NAME,
		ROOT,
		INDEX,
		AUTOINDEX,
		REDIRECT,
		LOCATION,
		INVALID,
	};

	int	i = 0;

	for (const std::string& stringId : stringIdTable)
	{
		if (input == stringId)
			break ;
		i++;
	}
	return idTable[i];
}

static inline bool	isDelimiter(char c, const std::string& delimiters)
{
	return delimiters.find(c) != std::string::npos;
}

std::vector<std::string>	tokenizeString(const std::string& s, const std::string& delimiters)
{
	std::vector<std::string>	tokens;
	std::string					parsedToken;
	std::istringstream			tokenStream(s);
	char						c;

	if (delimiters.empty())
	{
		throw std::runtime_error("empty delimiters");
	}
	while (tokenStream.get(c))
	{
		if (isDelimiter(c, delimiters))
		{
			if (!parsedToken.empty())
			{
				tokens.push_back(parsedToken);
				parsedToken.clear();
			}
		}
		else
			parsedToken += c;
	}
	return tokens;
}

static std::string escapeString(const std::string& input) {
	std::string output;
	for (char c : input) {
		switch (c) {
			case '\n': output += "\\n"; break;
			case '\t': output += "\\t"; break;
			case '\r': output += "\\r"; break;
			case '\b': output += "\\b"; break;
			case '\f': output += "\\f"; break;
			case '\v': output += "\\v"; break;
			case '\\': output += "\\\\"; break;
			case '\"': output += "\\\""; break;
			case '\'': output += "\\\'"; break;
			default: output += c; break;
		}
	}
	return output;
}

static inline bool containsSetExclusively(const std::string& s, const std::string& set)
{
	for (const char& c : s)
	{
		if (set.find(c) == std::string::npos)
			return false;
	}
	return true;
}

static void	removeEmptyTokensSet(std::vector<std::string>& tokens, const std::string& set)
{
	for (std::vector<std::string>::iterator it = tokens.begin(); it != tokens.end(); it++)
	{
		if (containsSetExclusively(*it, set))
			tokens.erase(it);
	}
}

static void	removeCommentBodies(std::vector<std::string>& tokens)
{
	std::size_t	commentStart = std::string::npos;
	std::size_t	commentEnd = std::string::npos;
	bool		isComment = false;

	for (std::vector<std::string>::iterator it = tokens.begin(); it != tokens.end(); it++)
	{
		std::cout << escapeString(*it) << '\n';
		
	}
}

Config::Config(const std::filesystem::path& path)
	: m_Path(path)
{
	std::string	buffer;

	if (!stringEndsWith(path, ".conf"))
		throw std::runtime_error("invalid extension");

	buffer = readFileIntoBuffer(path);
	if (buffer.empty())
		throw std::runtime_error("file content is empty");
	std::vector<std::string>	tokens = tokenizeString(buffer, " \t\r\v\f");

	// std::cout << buffer;
	removeCommentBodies(tokens);
	removeEmptyTokensSet(tokens, "\n");
	removeEmptyTokensSet(tokens, "#");
	// for (std::vector<std::string>::iterator it = tokens.begin(); it != tokens.end(); it++)
	// 	std::cout << escapeString(*it) << '\n';
	
	// LocationSettings locationSettings = Config["/"];
}
