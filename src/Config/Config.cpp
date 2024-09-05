/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   Config.cpp                                         :+:    :+:            */
/*                                                     +:+                    */
/*   By: kwchu <kwchu@student.codam.nl>               +#+                     */
/*                                                   +#+                      */
/*   Created: 2024/09/05 13:09:31 by kwchu         #+#    #+#                 */
/*   Updated: 2024/09/05 14:13:09 by kwchu         ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include "Config.h"

Config:: Config()
{
	#ifdef DEBUG
		std::cout << "Config constructor called.\n";
	#endif
	this->m_Servers = ConfigParser::createDefaultConfig();
};
		
Config:: Config(const std::filesystem::path& path)
{
	#ifdef DEBUG
		std::cout << "Config path constructor called.\n";
	#endif
	this->m_Servers = ConfigParser::createConfigFromFile(path);
};

Config::~Config()
{
	#ifdef DEBUG
		std::cout << "Config destructor called.\n";
	#endif
};
