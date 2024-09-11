/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   ServerSettings.h                                   :+:    :+:            */
/*                                                     +:+                    */
/*   By: kwchu <kwchu@student.codam.nl>               +#+                     */
/*                                                   +#+                      */
/*   Created: 2024/09/05 16:25:36 by kwchu         #+#    #+#                 */
/*   Updated: 2024/09/11 14:06:39 by kwchu         ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <filesystem>
#include <string>
#include <vector>
#include <map>

class ServerSettings
{
	public:
		struct LocationSettings
		{
			std::filesystem::path	root;
			std::string				index;
			bool					autoindex;
			std::string				cgi;
			std::string				returnCode;
			std::string				errorPages;
			uint8_t					httpMethods;
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
		const std::string& getServerName() const { return m_ServerName; };
		std::vector<uint64_t> getPorts() const { return m_Ports; };
};