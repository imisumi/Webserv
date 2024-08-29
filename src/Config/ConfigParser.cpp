#include "ConfigParser.h"

#include <iostream>


// int main()
// {
// 	ConfigParser config;
// 	ConfigParser::ServerSettings serverSettings;
// 	serverSettings.serverName = "WebServer";



// 	ConfigParser::ServerSettings serverSettings;



// 	ConfigParser::LocationSettings locationSettings(serverSettings);

// 	std::cout << "Server name: " << serverSettings.serverName << std::endl;
// 	std::cout << "Location server name: " << locationSettings.serverName << std::endl;

// 	locationSettings.serverName = "LocationServer";
// 	std::cout << "Server name: " << serverSettings.serverName << std::endl;
// 	std::cout << "Location server name: " << locationSettings.serverName << std::endl;


// 	// serverSettings.root = "/var/www/html";


// 	// const struct Server serv = ConfigProcessor::ParseConfig("config.json");





	

// 	return 0;
// }



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


Config::Config(const std::filesystem::path& path)
	: m_Path(path)
{
	//TODO: validate path
	//TODO: parse config
	//TODO: validate config

	// LocationSettings locationSettings = Config["/"];
}
