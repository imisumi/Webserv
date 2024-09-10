/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   Config.cpp                                         :+:    :+:            */
/*                                                     +:+                    */
/*   By: kwchu <kwchu@student.codam.nl>               +#+                     */
/*                                                   +#+                      */
/*   Created: 2024/09/05 13:09:31 by kwchu         #+#    #+#                 */
/*   Updated: 2024/09/10 15:40:07 by kwchu         ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include "Config.h"
#include "ServerSettings.h"

Config:: Config()
{

};

Config:: Config(Config& copy)
{
	this->m_ServerMap = copy.m_ServerMap;
	this->m_Servers = copy.m_Servers;
};

Config&	Config:: operator=(Config& other)
{
	if (this != &other)
	{
		this->m_ServerMap = other.m_ServerMap;
		this->m_Servers = other.m_Servers;
	}
	return *this;
};

Config:: Config(Config&& copy)
{
	this->m_ServerMap = copy.m_ServerMap;
	this->m_Servers = copy.m_Servers;
};

Config&	Config:: operator=(Config&& other)
{
	if (this != &other)
	{
		this->m_ServerMap = other.m_ServerMap;
		this->m_Servers = other.m_Servers;
	}
	return *this;
};
