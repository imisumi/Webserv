/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Config.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: imisumi <imisumi@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/09/05 13:09:45 by kwchu             #+#    #+#             */
/*   Updated: 2024/10/08 14:19:32 by imisumi          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "ConfigParser.h"
#include "ServerSettings.h"

#include <filesystem>
#include <unordered_map>
#include <vector>
#include <iostream>

class Config
{
	private:
		std::unordered_map<uint64_t, std::vector<ServerSettings*>>	m_ServerMap; // posible use index-based array
		std::vector<ServerSettings>									m_Servers;
		friend class ConfigParser;
	public:
		std::vector<ServerSettings*>	operator[](const uint64_t key)
		{
			// std::cout << "key: " << key << std::endl;
			auto it = m_ServerMap.find(key);
			if (it == m_ServerMap.end())
			{
				// throw std::out_of_range("Key not found in the server map");
				// uint32_t port = static_cast<uint32_t>(key & 0xFFFFFFFF);
				uint16_t port = key;
				// std::cout << "port: " << port << std::endl;
				it = m_ServerMap.find(port);
				if (it == m_ServerMap.end())
				{
					throw std::out_of_range("Key not found in the server map");
				}
			}
			return it->second;
		};
		const std::vector<ServerSettings*> operator[](const uint64_t key) const
		{
			// std::cout << "key: " << key << std::endl;
			auto it = m_ServerMap.find(key);
			if (it == m_ServerMap.end())
			{
				// throw std::out_of_range("Key not found in the server map");
				// uint32_t port = static_cast<uint32_t>(key & 0xFFFFFFFF);
				uint16_t port = key;
				// std::cout << "port: " << port << std::endl;
				it = m_ServerMap.find(port);
				if (it == m_ServerMap.end())
				{
					throw std::out_of_range("Key not found in the server map");
				}
			}
			return it->second;
		}

		auto begin() const { return m_ServerMap.begin(); }
		auto end() const { return m_ServerMap.end(); }

		void	print();
		Config() = default;
};
