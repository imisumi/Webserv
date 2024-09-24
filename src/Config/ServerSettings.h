/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   ServerSettings.h                                   :+:    :+:            */
/*                                                     +:+                    */
/*   By: imisumi-wsl <imisumi-wsl@student.42.fr>      +#+                     */
/*                                                   +#+                      */
/*   Created: 2024/09/05 16:25:36 by kwchu         #+#    #+#                 */
/*   Updated: 2024/09/24 16:15:27 by kwchu         ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <filesystem>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <iostream>

class ServerSettings
{
	public:
		struct LocationSettings
		{
			std::filesystem::path								root;
			std::vector<std::string>							index = {"index.html", "index.htm", "index.php"};
			bool												autoindex = false;
			std::vector<std::string>							cgi;
			std::pair<uint16_t, std::filesystem::path>			redirect;
			std::unordered_map<uint16_t, std::filesystem::path>	errorPageMap;
			uint64_t											maxBodySize = 1024;
			uint8_t												httpMethods = 1;
		};

	private:
		friend class ConfigParser;
		std::vector<uint64_t>	m_Ports;
		std::string				m_ServerName;
		LocationSettings		m_GlobalSettings;
		std::map<std::filesystem::path, LocationSettings> m_Locations;

	public:
		ServerSettings();
		ServerSettings(const ServerSettings& copy);
		ServerSettings&	operator=(const ServerSettings& other);
		ServerSettings(ServerSettings&& copy);
		ServerSettings&	operator=(ServerSettings&& other);
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
		const std::string& GetServerName() const { return m_ServerName; };
		const LocationSettings&	GetGlobalSettings() const { return m_GlobalSettings; };
		const std::map<std::filesystem::path, LocationSettings>&	GetLocations() const { return m_Locations; };
		std::vector<uint64_t> getPorts() const { return m_Ports; };



		uint8_t	GetAllowedMethods(const std::filesystem::path& path) const
		{
			std::map<std::filesystem::path, LocationSettings>::const_iterator	it = this->m_Locations.find(path);

			if (it != this->m_Locations.end())
				return it->second.httpMethods;
			return this->m_GlobalSettings.httpMethods;
		}

		std::filesystem::path GetIndex(const std::filesystem::path& path) const
		{
			std::map<std::filesystem::path, LocationSettings>::const_iterator	it = this->m_Locations.find(path);

			if (it != this->m_Locations.end())
				return it->second.index[0]; //TODO: fix
			return this->m_GlobalSettings.index[0]; //TODO: fix
		}

		const std::vector<std::string>& GetIndexList(const std::filesystem::path& path) const
		{
			std::map<std::filesystem::path, LocationSettings>::const_iterator	it = this->m_Locations.find(path);

			if (it != m_Locations.end())
				return it->second.index;
			return m_GlobalSettings.index;
		}


		const LocationSettings& GetLocationSettings(std::filesystem::path path) const
		{
			auto it = m_Locations.find(path);
			if (it != m_Locations.end())
			{
				return it->second;
			}
			return m_GlobalSettings;
		}

};
