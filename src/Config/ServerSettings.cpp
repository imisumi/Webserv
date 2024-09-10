/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   ServerSettings.cpp                                 :+:    :+:            */
/*                                                     +:+                    */
/*   By: kwchu <kwchu@student.codam.nl>               +#+                     */
/*                                                   +#+                      */
/*   Created: 2024/09/10 15:16:05 by kwchu         #+#    #+#                 */
/*   Updated: 2024/09/10 15:29:30 by kwchu         ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include "ServerSettings.h"

ServerSettings:: ServerSettings()
{

};

ServerSettings:: ServerSettings(ServerSettings&& copy)
{
	this->m_GlobalSettings = copy.m_GlobalSettings;
	this->m_Locations = copy.m_Locations;
	this->m_Ports = copy.m_Ports;
	this->m_ServerName = copy.m_ServerName;
};

ServerSettings&	ServerSettings:: operator=(ServerSettings&& other)
{
	if (this != &other)
	{
		this->m_GlobalSettings = other.m_GlobalSettings;
		this->m_Locations = other.m_Locations;
		this->m_Ports = other.m_Ports;
		this->m_ServerName = other.m_ServerName;
	}
	return *this;
};

ServerSettings:: ServerSettings(ServerSettings& copy)
{
	this->m_GlobalSettings = copy.m_GlobalSettings;
	this->m_Locations = copy.m_Locations;
	this->m_Ports = copy.m_Ports;
	this->m_ServerName = copy.m_ServerName;
};

ServerSettings&	ServerSettings:: operator=(ServerSettings& other)
{
	if (this != &other)
	{
		this->m_GlobalSettings = other.m_GlobalSettings;
		this->m_Locations = other.m_Locations;
		this->m_Ports = other.m_Ports;
		this->m_ServerName = other.m_ServerName;
	}
	return *this;
};
