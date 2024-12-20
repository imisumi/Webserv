/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerSettings.h                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: imisumi <imisumi@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/09/05 16:25:36 by kwchu             #+#    #+#             */
/*   Updated: 2024/10/29 15:36:22 by imisumi          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <filesystem>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <iostream>

#include "Core/Log.h"

class ServerSettings
{
	public:
		struct LocationSettings
		{
			std::filesystem::path								root;
			std::vector<std::string>							index = {"index.html", "index.htm", "index.php"};
			bool												autoindex = false;
			std::vector<std::string>							cgi;
			std::pair<uint16_t, std::string>					redirect = {0, ""};
			std::unordered_map<uint16_t, std::filesystem::path>	errorPageMap;
			uint64_t											maxBodySize = 1024;
			uint8_t												httpMethods = 1;
		};

	private:
		friend class ConfigParser;
		std::vector<uint64_t>	m_Ports;
		std::string				m_ServerName = "";
		LocationSettings		m_GlobalSettings;
		std::map<std::filesystem::path, LocationSettings> m_Locations;

	public:
		ServerSettings() = default;

		LocationSettings&		operator[](const std::filesystem::path& path)
		{
			std::map<std::filesystem::path, LocationSettings>::iterator	it = this->m_Locations.find(path);

			if (it != this->m_Locations.end())
			{
				Log::info("Location found: {}", path.string());
				return it->second;
			}
			Log::info("Location not found, returning global settings");
			return this->m_GlobalSettings;
		};
		const LocationSettings&	operator[](const std::filesystem::path& path) const
		{
			// std::map<std::filesystem::path, LocationSettings>::const_iterator	it = this->m_Locations.find(path);
			auto it = this->m_Locations.find(path);

			if (it != this->m_Locations.end())
				return it->second;
			return this->m_GlobalSettings;
		};
		const std::string& GetServerName() const { return m_ServerName; };
		const LocationSettings&	GetGlobalSettings() const { return m_GlobalSettings; };
		const std::map<std::filesystem::path, LocationSettings>&	GetLocations() const { return m_Locations; };
		std::vector<uint64_t> getPorts() const { return m_Ports; };


		bool hasLocationSettings(const std::filesystem::path& path) const
		{
			return m_Locations.find(path) != m_Locations.end();
		}



		uint8_t	GetAllowedMethods(const std::filesystem::path& path) const
		{
			std::map<std::filesystem::path, LocationSettings>::const_iterator	it = this->m_Locations.find(path);

			if (it != this->m_Locations.end())
				return it->second.httpMethods;
			return this->m_GlobalSettings.httpMethods;
		}

		const std::vector<std::string>& GetIndexList(const std::filesystem::path& _path) const
		{
			for (const auto & [path, settings] : m_Locations)
			{
				Log::info("Looking for index: {}", path.string());
				Log::info("Looking for index: {}", _path.string());
				if (path == _path)
				{
					return settings.index;
				}
			}
			return m_GlobalSettings.index;
		}


		const LocationSettings& GetLocationSettings(const std::filesystem::path& path) const
		{
			// Log::debug("Looking for location: {}", path.string());

			// for (const auto& [location, settings] : m_Locations)
			// {
			// 	if (location == path)
			// 	{
			// 		Log::debug("Location found: {}", location.string());
			// 		return settings;
			// 	}
			// }
			// Log::debug("Location not found, returning global settings");
			// return m_GlobalSettings;


			auto it = m_Locations.find(path);
			if (it != m_Locations.end())
			{
				Log::debug("Location found: {}", path.string());
				return it->second;
			}
			Log::debug("Location not found, returning global settings");
			return m_GlobalSettings;
		}

};
