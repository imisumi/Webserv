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
	public:
		std::vector<std::string>	tokens;

		//? Maybe just stack allocate this
		static ConfigParser CreateDefaultConfig();
		static ConfigParser CreateConfigFromFile(const std::filesystem::path& path);
	private:
		ConfigParser(const std::filesystem::path& path);

		// Deleted copy constructor and assignment operator and move constructor and assignment operator
		ConfigParser(const ConfigParser&) = delete;
		ConfigParser& operator=(const ConfigParser&) = delete;
		ConfigParser(ConfigParser&&) = delete;
		ConfigParser& operator=(ConfigParser&&) = delete;

	private:
		std::vector<ServerSettings>	serverSettings;
		std::shared_ptr<ServerSettings> h = std::make_shared<ServerSettings>();
		std::shared_ptr<ServerSettings> h = std::shared_ptr(new ServerSettings);
		std::map<uint16_t, std::shared_ptr<ServerSettings>>	Servers;
	public:
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
		// std::vector<uint16_t> m_Ports;
		std::string m_ServerName;

		LocationSettings globalSettings;
		std::map<std::filesystem::path, LocationSettings> m_Locations;

	public:
		LocationSettings&		operator[](std::filesystem::path path)
		{
			std::map<std::filesystem::path, LocationSettings>::iterator	it = this->m_Locations.find(path);

			if (it != this->m_Locations.end())
				return it->second;
			return this->globalSettings;
		};
		const LocationSettings&	operator[](std::filesystem::path path) const
		{
			std::map<std::filesystem::path, LocationSettings>::const_iterator	it = this->m_Locations.find(path);

			if (it != this->m_Locations.end())
				return it->second;
			return this->globalSettings;
		};
		const std::string& getServerName() const { return m_ServerName; };
};

class Config
{
	private:
		std::map<uint16_t, ServerSettings>	Servers;
		friend class ConfigParser;
	public:
		static Config createConfig() { return ConfigParser::CreateConfigFromFile(); };
};

Config a = Config::createConfig();

// class ConfigProccessor
// {
// public:
// 	struct LocationSettings
// 	{
// 		std::filesystem::path path;
// 		std::filesystem::path root;
// 		std::string index;
// 		bool autoindex;
// 		std::string cgi;
// 		std::string returnCode;
// 	};
// public:
// 	static ConfigParser ParseConfig(const std::filesystem::path& path)
// 	{
// 		ConfigParser config;
// 		return config;
// 	}

// };

// class ConfigParser
// {
// 	public:
// 	//TODO: find better solution
// 	#define BIT(n) (1 << (n))

// 	#define GET	   BIT(0)
// 	#define POST   BIT(1)
// 	#define DELETE BIT(2)


// 	struct ServerSettings
// 	{
// 		//? Server settings
// 		std::string serverName;
// 		uint16_t port;
// 		std::filesystem::path root;
// 		std::map<uint16_t, std::filesystem::path> errorPages;


// 		//? Shared settings
// 		bool autoindex = true;
// 		uint32_t clientMaxBodySize = 0;
// 		std::vector<std::string> index = {"index.html"};
// 		int limitExcept = GET;
		
// 		//TODO: add cgi
// 	};

// 	struct LocationSettings : ServerSettings
// 	{
// 		LocationSettings(ServerSettings settings) : ServerSettings(settings) {}

// 		std::filesystem::path path;
// 		std::filesystem::path locationRoot = root;
// 		uint16_t returnCode = 200;
// 	};


// private:
// 	struct ServerSettings m_ServerSettings;
// 	std::map<std::filesystem::path, ConfigParser::LocationSettings> m_LocationMap;
// };
