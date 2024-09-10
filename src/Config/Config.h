/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   Config.h                                           :+:    :+:            */
/*                                                     +:+                    */
/*   By: kwchu <kwchu@student.codam.nl>               +#+                     */
/*                                                   +#+                      */
/*   Created: 2024/09/05 13:09:45 by kwchu         #+#    #+#                 */
/*   Updated: 2024/09/10 15:40:13 by kwchu         ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "ConfigParser.h"
#include "ServerSettings.h"
#include <filesystem>
#include <vector>

class Config
{
	private:
		std::map<uint64_t, std::vector<ServerSettings*>>	m_ServerMap;
		std::vector<ServerSettings>							m_Servers;
		friend class ConfigParser;

		Config(const Config&) = delete;
		Config& operator=(const Config&) = delete;
	public:
		std::vector<ServerSettings*>	operator[](const uint64_t key)
		{
			const std::map<uint64_t, std::vector<ServerSettings*>>::iterator it = this->m_ServerMap.find(key);
			if (it != this->m_ServerMap.end())
				return it->second;
			throw std::out_of_range("key not found");
		};
		const std::vector<ServerSettings*>	operator[](const uint64_t key) const
		{
			const std::map<uint64_t, std::vector<ServerSettings*>>::const_iterator it = this->m_ServerMap.find(key);
			if (it != this->m_ServerMap.end())
				return it->second;
			throw std::out_of_range("key not found");
		};
		Config();
		Config(Config& copy);
		Config& operator=(Config& other);
		Config(Config&& copy);
		Config& operator=(Config&& other);
};
