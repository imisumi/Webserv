#pragma once



#include <string>
#include <filesystem>
#include <vector>
#include <unordered_map>
#include <iostream>


// #define BIT(x) (1u << x)

#define HTTP_GET BIT(0)
#define HTTP_POST BIT(1)
#define HTTP_DELETE BIT(4)

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

	std::string GetServerName() const { return m_ServerName; }

	uint8_t GetAllowedMethods(std::filesystem::path path) const
	{
		auto it = m_Locations.find(path);
		if (it != m_Locations.end())
		{
			return it->second.httpMethods;
		}
		return m_GlobalSettings.httpMethods;
	}

	std::filesystem::path GetIndex(std::filesystem::path path) const
	{
		auto it = m_Locations.find(path);
		if (it != m_Locations.end())
		{
			return it->second.index;
		}
		return m_GlobalSettings.index;
	}


	LocationSettings GetLocationSettings(std::filesystem::path path) const
	{
		auto it = m_Locations.find(path);
		if (it != m_Locations.end())
		{
			return it->second;
		}
		return m_GlobalSettings;
	}

	LocationSettings operator[](std::filesystem::path path) const
	{
		return GetLocationSettings(path);
	}

	// LocationSettings
private:
	// std::filesystem::path	m_Root;
	std::string				m_ServerName = "localhost";
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


	auto begin() const { return m_ServerMap.begin(); }
	auto end() const { return m_ServerMap.end(); }

	// Const indexing operator (read-only access)
	const std::vector<ServerSettings>& operator[](uint64_t key) const
	{
		std::cout << "key: " << key << std::endl;
		auto it = m_ServerMap.find(key);
		if (it == m_ServerMap.end())
		{
			// throw std::out_of_range("Key not found in the server map");
			// uint32_t port = static_cast<uint32_t>(key & 0xFFFFFFFF);
			uint16_t port = key;
			std::cout << "port: " << port << std::endl;
			it = m_ServerMap.find(port);
			if (it == m_ServerMap.end())
			{
				throw std::out_of_range("Key not found in the server map");
			}
		}
		return it->second;
	}

private:
	Config();
	std::unordered_map<uint64_t, std::vector<ServerSettings>> m_ServerMap;
};