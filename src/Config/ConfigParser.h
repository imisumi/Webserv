#pragma once

#include <string>
#include <cstdint>
#include <filesystem>
#include <vector>
#include <map>
#include <bitset>
#include <memory>
#include "ServerSettings.h"

#define DEFAULT_PORT 8080
#define DEFAULT_PATH "conf/default.conf"

class Config;

class ConfigParser
{
	private:
		ConfigParser(const ConfigParser&) = delete;
		ConfigParser& operator=(const ConfigParser&) = delete;
		ConfigParser(ConfigParser&&) = delete;
		ConfigParser& operator=(ConfigParser&&) = delete;

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
			LIMIT_EXCEPT,
			ERROR_PAGE,
			MAX_BODY_SIZE,
			CGI,
			HTTP_METHOD_DENY,
			BRACKET_OPEN,
			BRACKET_CLOSE,
			DIRECTIVE_END,
			ARGUMENT,
			UNRECOGNISED,
		};

		typedef std::vector<std::pair<TokenIdentifier, std::string>>	TokenMap;
		typedef std::vector<std::string>								TokenVector;
		typedef std::map<uint64_t, ServerSettings>						ServerMap;
		typedef	std::vector<ServerSettings>								Servers;

		static TokenVector		tokenize(const std::string& input, const std::string& delimiters);
		static TokenMap			assignTokenType(const TokenVector& tokens);
		static TokenIdentifier	getIdentifier(const std::string& token, bool expectDirective);
		static std::string		identifierToString(TokenIdentifier id);

		static void								tokenMapToServerSettings(const TokenMap& tokenMap, Servers& servers);
		static ServerSettings					createServerSettings(const TokenMap::const_iterator& end, TokenMap::const_iterator& it);
		static ServerSettings::LocationSettings	createLocationSettings(const TokenMap::const_iterator& end, TokenMap::const_iterator& it);

		static void	expectNextToken(const TokenMap::const_iterator& end, TokenMap::const_iterator& it, TokenIdentifier expected);
		static void	handlePort(std::vector<uint64_t>& ports, const TokenMap::const_iterator& end, TokenMap::const_iterator& it);
		static void	handleIndex(std::vector<std::string>& indexFiles, const TokenMap::const_iterator& end, TokenMap::const_iterator& it);
		static void	handleCgi(std::vector<std::string>& cgi, const TokenMap::const_iterator& end, TokenMap::const_iterator& it);
		static void	handleRedirect(uint16_t& redirect, const TokenMap::const_iterator& end, TokenMap::const_iterator& it);
		static void	handleErrorPage(std::unordered_map<uint16_t, std::filesystem::path>& errorPageMap, const TokenMap::const_iterator& end, TokenMap::const_iterator& it);
		static void	handleMaxBodySize(uint64_t& maxBodySize, const TokenMap::const_iterator& end, TokenMap::const_iterator& it);
		static void	handleAutoIndex(bool& autoIndex, const TokenMap::const_iterator& end, TokenMap::const_iterator& it);
		static void	handleLimitExcept(uint8_t& httpMethods, const TokenMap::const_iterator& end, TokenMap::const_iterator& it);

		static std::string	readFileIntoBuffer(const std::filesystem::path& path);
	public:
		static Config	createDefaultConfig();
		static Config	createConfigFromFile(const std::filesystem::path& path);

};
