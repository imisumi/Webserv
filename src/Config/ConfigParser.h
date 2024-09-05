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
			HTTP_METHOD,
			HTTP_METHOD_DENY,
			BRACKET_OPEN,
			BRACKET_CLOSE,
			DIRECTIVE_END,
			ARGUMENT,
			UNRECOGNISED,
		};

		typedef std::vector<std::pair<TokenIdentifier, std::string>> TokenMap;
		typedef std::vector<std::string> TokenVector;
		typedef std::vector<ServerSettings>	Servers;

		static TokenVector		tokenize(const std::string& input);
		static TokenMap			assignTokenType(const TokenVector& tokens);
		static TokenIdentifier	getIdentifier(const std::string& token, bool expectDirective);
		static std::string		escapeIdentifier(TokenIdentifier id);

		static Servers							tokenMapToServerSettings(const TokenMap& tokenMap);
		static ServerSettings					createServerSettings(const TokenMap& tokenMap, TokenMap::const_iterator it);
		static ServerSettings::LocationSettings	createLocationSettings(const TokenMap& tokenMap, TokenMap::const_iterator it);
		static std::string						readFileIntoBuffer(const std::filesystem::path& path);
	public:
		static Servers	createDefaultConfig();
		static Servers	createConfigFromFile(const std::filesystem::path& path);

};
