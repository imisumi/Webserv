/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   Config.h                                           :+:    :+:            */
/*                                                     +:+                    */
/*   By: kwchu <kwchu@student.codam.nl>               +#+                     */
/*                                                   +#+                      */
/*   Created: 2024/09/05 13:09:45 by kwchu         #+#    #+#                 */
/*   Updated: 2024/09/07 00:46:29 by kwchu         ########   odam.nl         */
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
		std::map<uint64_t, ServerSettings>	m_Servers;
		friend class ConfigParser;

		Config(const Config&) = delete;
		Config& operator=(const Config&) = delete;
		Config(Config&&) = delete;
		Config& operator=(Config&&) = delete;
	public:
		Config();
		Config(const std::filesystem::path& path);
		~Config();
};
