#pragma once



#include <string>
#include <filesystem>
#include <vector>
#include <unordered_map>


class ServerSettings
{
public:
struct LocationSettings
	{
		std::filesystem::path	root;
		// std::filesystem::file
		// std::string				index;
		std::filesystem::path	index;
		bool					autoindex;
		// std::string				cgi;
		std::filesystem::path	cgi;
		std::string				returnCode;
		std::string				errorPages;
		uint8_t					httpMethods;
	};

	void SetGlobalSettings(const LocationSettings& settings) { m_GlobalSettings = settings; }
	void InsertLocation(const std::filesystem::path& path, const LocationSettings& settings) { m_Locations[path] = settings; }

	LocationSettings GetGlobalSettings() const { return m_GlobalSettings; }
	auto GetLocations() const { return m_Locations; }
	// LocationSettings
private:
	// std::filesystem::path	m_Root;
	LocationSettings		m_GlobalSettings;
	std::unordered_map<std::filesystem::path, LocationSettings> m_Locations;
};

class Config
{
public:
	static Config CreateDefaultConfig();

	void InsertServer(uint64_t key, const std::vector<ServerSettings>& settings) { m_ServerMap[key] = settings; }

	std::filesystem::path getRoot() const { return ""; }


	void Print() const;
private:
	Config();
	std::unordered_map<uint64_t, std::vector<ServerSettings>> m_ServerMap;
};