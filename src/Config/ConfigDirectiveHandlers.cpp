/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigDirectiveHandlers.cpp                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: imisumi-wsl <imisumi-wsl@student.42.fr>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/09/18 12:20:38 by kwchu             #+#    #+#             */
/*   Updated: 2024/09/27 03:31:51 by imisumi-wsl      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ConfigParser.h"
#include "TemplateFunctions.h"
#include "Config.h"

#include <filesystem>
#include <unordered_map>
#include <string>

void	ConfigParser:: handlePort(
	std::vector<uint64_t>& ports,
	const TokenMap::const_iterator& end,
	TokenMap::const_iterator& it)
{
	TokenVector	hostPort = tokenize(it->second, ":");
	uint32_t	host;
	uint16_t 	port;

	if (hostPort.size() > 2)
		throw std::invalid_argument("invalid host:port format: " + it->second);
	if (hostPort.size() == 1)
	{
		host = 0;
		port = stringToUInt16(hostPort[0]);
	}
	else
	{
		host = ipv4LiteralToUint32(hostPort[0]);
		port = stringToUInt16(hostPort[1]);
	}
	if (port == 0)
		throw std::invalid_argument("port cannot be 0");
	uint64_t result = (static_cast<uint64_t>(host) << 32) | static_cast<uint64_t>(port);
	ports.emplace_back(result);
}

void	ConfigParser:: handleRoot(
	std::filesystem::path& root,
	const TokenMap::const_iterator& end,
	TokenMap::const_iterator& it)
{
	if (it->second[0] == '/')
		root = it->second;
	else
		root = std::filesystem::current_path() / it->second;
}

void	ConfigParser:: handleAutoIndex(
	bool& autoIndex,
	const TokenMap::const_iterator& end,
	TokenMap::const_iterator& it)
{
	if (it->second == "on")
		autoIndex = true;
	else if (it->second == "off")
		autoIndex = false;
	else
		throw std::invalid_argument("invalid autoindex argument: " + it->second);
}

void	ConfigParser:: handleLimitExcept(
	uint8_t& httpMethods,
	const TokenMap::const_iterator& end,
	TokenMap::const_iterator& it)
{
	uint8_t	methods = 0;

	for (; it != end; it++)
	{
		if (it->first == BRACKET_OPEN)
			break ;
		if (it->second == "GET")
			methods |= 1;
		else if (it->second == "POST")
			methods |= (1 << 1);
		else if (it->second == "DELETE")
			methods |= (1 << 2);
		else if (it->second == "PATCH")
			methods |= (1 << 3);
		else if (it->second == "PUT")
			methods |= (1 << 4);
		else
			throw std::invalid_argument("invalid http method: " + it->second);
	}
	httpMethods = methods;
	expectNextToken(end, it, HTTP_METHOD_DENY);
	expectNextToken(end, it, ARGUMENT);
	if (it->second != "all")
		throw std::invalid_argument("invalid deny argument: " + it->second);
	expectNextToken(end, it, DIRECTIVE_END);
	expectNextToken(end, it, BRACKET_CLOSE);
}


void	ConfigParser:: handleIndex(
	std::vector<std::string>& indexFiles,
	const TokenMap::const_iterator& end,
	TokenMap::const_iterator& it)
{
	indexFiles.clear();
	for (; it != end; it++)
	{
		if (it->first == DIRECTIVE_END)
		{
			it--;
			break ;
		}
		if (it->first != ARGUMENT)
			throw std::invalid_argument("invalid index argument: " + it->second);
		if (elementIsUniqueInVector(indexFiles, it->second))
			indexFiles.push_back(it->second);
	}
	if (it == end)
		throw std::invalid_argument("invalid index format");
}

void	ConfigParser:: handleCgi(
	std::vector<std::string>& cgi,
	const TokenMap::const_iterator& end,
	TokenMap::const_iterator& it)
{
	for (; it != end; it++)
	{
		if (it->first == DIRECTIVE_END)
		{
			it--;
			break ;
		}
		if (it->first != ARGUMENT)
			throw std::invalid_argument("invalid cgi argument: " + it->second);
		cgi.push_back(it->second);
	}
	if (it == end)
		throw std::invalid_argument("invalid cgi format");
}

void	ConfigParser:: handleRedirect(
	std::pair<uint16_t, std::filesystem::path>& redirect,
	const TokenMap::const_iterator& end,
	TokenMap::const_iterator& it)
{
	redirect.first = stringToUInt16(it->second);
	if (redirect.first < 300 || redirect.first > 308)
		throw std::invalid_argument("invalid redirect code");
	try
	{
		expectNextToken(end, it, ARGUMENT);
		redirect.second = it->second;
	}
	catch (const std::exception& e)
	{
		it--;
	}
}

void	ConfigParser:: handleErrorPage(
	std::unordered_map<uint16_t, std::filesystem::path>& errorPageMap,
	const TokenMap::const_iterator& end,
	TokenMap::const_iterator& it)
{
	std::vector<uint16_t>	returnCodes;
	std::vector<uint16_t>	replaceReturnCodes;
	bool					replace = false;

	if (!errorPageMap.empty())
		replace = true;
	if (!stringContainsDigitsExclusively(it->second))
		throw std::invalid_argument("invalid error page format");
	for (; it != end; it++)
	{
		if (it->first != ARGUMENT)
			throw std::invalid_argument("invalid error page argument: " + it->second);
		if (stringContainsDigitsExclusively(it->second))
		{
			const uint16_t currentReturnCode = stringToUInt16(it->second);
			if (currentReturnCode < 100 || currentReturnCode > 399)
				throw std::invalid_argument("invalid error page redirect code");
			if (replace && errorPageMap.find(currentReturnCode) != errorPageMap.end())
				replaceReturnCodes.push_back(currentReturnCode);
			auto result = errorPageMap.insert({currentReturnCode, std::filesystem::path()});
			if (result.second)
				returnCodes.push_back(currentReturnCode);
		}
		else
		{
			break ;
		}
	}
	if (it == end)
		throw std::invalid_argument("invalid error page format");
	if (it->first != ARGUMENT)
		throw std::invalid_argument("invalid error page argument: " + it->second);
	for (const uint16_t& code : replaceReturnCodes)
	{
		errorPageMap[code] = it->second;
	}
	for (const uint16_t& code : returnCodes)
	{
		errorPageMap[code] = it->second;
	}
}

void	ConfigParser:: handleMaxBodySize(
	uint64_t& maxBodySize,
	const TokenMap::const_iterator& end,
	TokenMap::const_iterator& it)
{
	uint64_t	initialSize;
	std::size_t	pos;

	initialSize = static_cast<uint64_t>(std::stoull(it->second, &pos));
	const std::string unit = it->second.substr(pos);
	if (unit.empty())
	{
		maxBodySize = initialSize;
	}
	else if (unit == "B")
	{
		maxBodySize = initialSize;
	}
	else if (unit == "KB")
	{
		maxBodySize = initialSize * 1024;
		if (maxBodySize / 1024 != initialSize)
			throw std::out_of_range("max body size overflow");
	}
	else if (unit == "MB")
	{
		maxBodySize = initialSize * 1024 * 1024;
		if (maxBodySize / 1024 / 1024 != initialSize)
			throw std::out_of_range("max body size overflow");
	}
	else if (unit == "GB")
	{
		maxBodySize = initialSize * 1024 * 1024 * 1024;
		if (maxBodySize / 1024 / 1024 / 1024 != initialSize)
			throw std::out_of_range("max body size overflow");
	}
	else
	{
		throw std::invalid_argument("invalid unit type: " + it->second);
	}
}
