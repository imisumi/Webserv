/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   ConfigInputConverters.cpp                          :+:    :+:            */
/*                                                     +:+                    */
/*   By: kwchu <kwchu@student.codam.nl>               +#+                     */
/*                                                   +#+                      */
/*   Created: 2024/09/18 12:41:20 by kwchu         #+#    #+#                 */
/*   Updated: 2024/09/18 13:02:26 by kwchu         ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include "ConfigParser.h"

#include <string>
#include <cstdint>
#include <sstream>
#include <limits>
#include <fstream>

uint32_t	ipv4LiteralToUint32(const std::string& s)
{
	std::istringstream	isstream(s);
	std::string			octet;
	int					shift = 24;
	uint32_t			result = 0;

	while (std::getline(isstream, octet, '.'))
	{
		if (shift < 0)
			throw std::invalid_argument("error in ip formatting");
		if (!stringContainsDigitsExclusively(octet) || octet.length() > 3)
			throw std::invalid_argument("error in ip formatting");
		uint32_t	segmentValue = static_cast<uint32_t>(std::stoul(octet));
		if (segmentValue > 255)
			throw std::out_of_range("error in ip formatting");
		result |= (segmentValue << shift);
		shift -= 8;
	}
	return result;
}

uint16_t	stringToUInt16(const std::string& s)
{
	std::size_t	pos;
	int			port;
	
	port = std::stoi(s, &pos);
	if (pos != s.length())
		throw std::invalid_argument("invalid uint16_t format: " + s);
	if (port < 0 || port > std::numeric_limits<uint16_t>::max())
		throw std::out_of_range("uint16_t out of range: " + s);
	return static_cast<uint16_t>(port);
}

std::string ConfigParser:: readFileIntoBuffer(const std::filesystem::path& path)
{
	std::ifstream file(path);
	if (!file.is_open())
	{
		throw std::runtime_error("could not open file: " + path.string());
	}

	std::stringstream buffer;
	buffer << file.rdbuf();
	file.close();
	return buffer.str();
}