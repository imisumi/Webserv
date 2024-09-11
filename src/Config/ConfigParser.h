// #pragma once

// #include <string>
// #include <cstdint>

// // #include 

// #include <filesystem>
// #include <vector>
// #include <map>

// #include <bitset>

// #include <memory>


// class Config
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


// 	struct ServerConfig
// 	{
// 		LocationSettings location;
// 	};



// 	uint16_t getPort() const { return m_Port; }
// 	const std::filesystem::path& getRoot() const { return m_Root; }
// 	const std::string& getServerName() const { return m_ServerName; }


// 	//? Maybe just stack allocate this
// 	static Config CreateDefaultConfig();
// 	static Config CreateConfigFromFile(const std::filesystem::path& path);


// 	//TODO: add overload for operator[] to get location settings

// private:
// 	Config(const std::filesystem::path& path);

// 	//temp
// 	Config();

// 	// Deleted copy constructor and assignment operator and move constructor and assignment operator
// 	Config(const Config&) = delete;
// 	Config& operator=(const Config&) = delete;
// 	Config(Config&&) = delete;
// 	Config& operator=(Config&&) = delete;

// private:
// 	const std::filesystem::path m_Path;
// 	std::filesystem::path m_Root;

// 	uint16_t m_Port;
// 	std::string m_ServerName;

// 	std::map<std::filesystem::path, LocationSettings> m_Locations;
// };

// // class ConfigProccessor
// // {
// // public:
// // 	struct LocationSettings
// // 	{
// // 		std::filesystem::path path;
// // 		std::filesystem::path root;
// // 		std::string index;
// // 		bool autoindex;
// // 		std::string cgi;
// // 		std::string returnCode;
// // 	};
// // public:
// // 	static Config ParseConfig(const std::filesystem::path& path)
// // 	{
// // 		Config config;
// // 		return config;
// // 	}

// // };

// // class ConfigParser
// // {
// // 	public:
// // 	//TODO: find better solution
// // 	#define BIT(n) (1 << (n))

// // 	#define GET	   BIT(0)
// // 	#define POST   BIT(1)
// // 	#define DELETE BIT(2)


// // 	struct ServerSettings
// // 	{
// // 		//? Server settings
// // 		std::string serverName;
// // 		uint16_t port;
// // 		std::filesystem::path root;
// // 		std::map<uint16_t, std::filesystem::path> errorPages;


// // 		//? Shared settings
// // 		bool autoindex = true;
// // 		uint32_t clientMaxBodySize = 0;
// // 		std::vector<std::string> index = {"index.html"};
// // 		int limitExcept = GET;
		
// // 		//TODO: add cgi
// // 	};

// // 	struct LocationSettings : ServerSettings
// // 	{
// // 		LocationSettings(ServerSettings settings) : ServerSettings(settings) {}

// // 		std::filesystem::path path;
// // 		std::filesystem::path locationRoot = root;
// // 		uint16_t returnCode = 200;
// // 	};


// // private:
// // 	struct ServerSettings m_ServerSettings;
// // 	std::map<std::filesystem::path, ConfigParser::LocationSettings> m_LocationMap;
// // };
