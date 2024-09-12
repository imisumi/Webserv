/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   Config.h                                           :+:    :+:            */
/*                                                     +:+                    */
/*   By: kwchu <kwchu@student.codam.nl>               +#+                     */
/*                                                   +#+                      */
/*   Created: 2024/09/05 13:09:45 by kwchu         #+#    #+#                 */
/*   Updated: 2024/09/12 14:47:21 by kwchu         ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "ConfigParser.h"
#include "ServerSettings.h"
#include <filesystem>
#include <unordered_map>
#include <vector>

class Config
{
	private:
		std::unordered_map<uint64_t, std::vector<ServerSettings*>>	m_ServerMap;
		std::vector<ServerSettings>									m_Servers;
		friend class ConfigParser;
	public:
		std::vector<ServerSettings*>	operator[](const uint64_t key)
		{
			const std::unordered_map<uint64_t, std::vector<ServerSettings*>>::iterator it = this->m_ServerMap.find(key);
			if (it != this->m_ServerMap.end())
				return it->second;
			throw std::out_of_range("key not found");
		};
		const std::vector<ServerSettings*>	operator[](const uint64_t key) const
		{
			const std::unordered_map<uint64_t, std::vector<ServerSettings*>>::const_iterator it = this->m_ServerMap.find(key);
			if (it != this->m_ServerMap.end())
				return it->second;
			throw std::out_of_range("key not found");
		};
		void	print();
		Config();
		Config(const Config& copy);
		Config& operator=(const Config& other);
		Config(Config&& copy);
		Config& operator=(Config&& other);
};
