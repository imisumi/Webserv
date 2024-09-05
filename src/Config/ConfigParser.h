#pragma once

#include <string>
#include <cstdint>
#include <filesystem>
#include <vector>
#include <map>
#include <bitset>
#include <memory>

#define DEFAULT_PORT 8080

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
			DIRECTIVE,
			DIRECTIVE_END,
			ARGUMENT,
			UNRECOGNISED,
		};

		typedef std::vector<std::pair<TokenIdentifier, std::string>> TokenMap;
		typedef std::vector<std::string> TokenVector;
	public:
		static std::vector<ServerSettings>	createDefaultConfig();
		static std::vector<ServerSettings>	createConfigFromFile(const std::filesystem::path& path);

		static TokenVector		tokenize(const std::string& input);
		static TokenMap			assignTokenType(const TokenVector& tokens);
		static TokenIdentifier	getIdentifier(const std::string& token);
		static std::string		escapeIdentifier(TokenIdentifier id);
};

class ServerSettings
{
	public:
		struct LocationSettings
		{
			std::filesystem::path root;
			std::string index;
			bool autoindex;
			std::string cgi;
			std::string returnCode;
		};

	private:
		std::vector<uint16_t> m_Ports;
		std::string m_ServerName;
		LocationSettings m_GlobalSettings;
		std::map<std::filesystem::path, LocationSettings> m_Locations;

	public:
		LocationSettings&		operator[](const std::filesystem::path& path)
		{
			std::map<std::filesystem::path, LocationSettings>::iterator	it = this->m_Locations.find(path);

			if (it != this->m_Locations.end())
				return it->second;
			return this->m_GlobalSettings;
		};
		const LocationSettings&	operator[](const std::filesystem::path& path) const
		{
			std::map<std::filesystem::path, LocationSettings>::const_iterator	it = this->m_Locations.find(path);

			if (it != this->m_Locations.end())
				return it->second;
			return this->m_GlobalSettings;
		};
		const std::string& getServerName() const { return m_ServerName; };
		std::vector<uint16_t> getPorts() const { return m_Ports; };
};
