/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   ConfigTokenizer.cpp                                :+:    :+:            */
/*                                                     +:+                    */
/*   By: kwchu <kwchu@student.codam.nl>               +#+                     */
/*                                                   +#+                      */
/*   Created: 2024/09/18 12:52:50 by kwchu         #+#    #+#                 */
/*   Updated: 2024/09/18 13:06:48 by kwchu         ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include "ConfigParser.h"

#include <string>
#include <vector>
#include <sstream>
#include <unordered_map>

static inline void	addToken(
	std::vector<std::string>& tokens, 
	std::string& tokenBuffer)
{
	if (!tokenBuffer.empty())
	{
		tokens.push_back(tokenBuffer);
		tokenBuffer.clear();
	}
}

ConfigParser::TokenVector	ConfigParser:: tokenize(const std::string& input, const std::string& delimiters)
{
	ConfigParser::TokenVector	tokens;
	std::string					tokenBuffer;
	std::istringstream			tokenStream(input);
	bool						isComment = false;
	char						c;

	if (delimiters.empty())
	{
		throw std::runtime_error("empty delimiters");
	}
	while (tokenStream.get(c))
	{
		if (c == '#')
			isComment = true;
		else if (isDelimiter(c, delimiters))
			addToken(tokens, tokenBuffer);
		else if (!isComment)
		{
			if (isSpecialCharacter(c))
			{
				addToken(tokens, tokenBuffer);
				tokenBuffer += c;
				addToken(tokens, tokenBuffer);
			}
			else
				tokenBuffer += c;
		}
		if (c == '\n')
			isComment = false;
	}
	addToken(tokens, tokenBuffer);
	return tokens;
}

ConfigParser::TokenMap	ConfigParser:: assignTokenType(
	const ConfigParser::TokenVector& tokens)
{
	ConfigParser::TokenMap	tokenIdMap;
	bool					expectDirective = true;

	for (std::vector<std::string>::const_iterator it = tokens.begin(); it != tokens.end(); it++)
	{
		TokenIdentifier	id = getIdentifier(*it, expectDirective);

		tokenIdMap.emplace_back(id, *it);
		if (id == BRACKET_OPEN
			|| id == BRACKET_CLOSE
			|| id == DIRECTIVE_END)
			expectDirective = true;
		else
			expectDirective = false;
	}
	return tokenIdMap;
}

ConfigParser::TokenIdentifier	ConfigParser:: getIdentifier(
	const std::string& input, bool expectDirective)
{
	const std::unordered_map<std::string, TokenIdentifier> TokenIdentifierMap = {
		{"server", SERVER},
		{"listen", PORT},
		{"server_name", SERVER_NAME},
		{"root", ROOT},
		{"index", INDEX},
		{"cgi", CGI},
		{"autoindex", AUTOINDEX},
		{"return", REDIRECT},
		{"location", LOCATION},
		{"limit_except", LIMIT_EXCEPT},
		{"error_page", ERROR_PAGE},
		{"client_max_body_size", MAX_BODY_SIZE},
		{"deny", HTTP_METHOD_DENY},
	};
	const std::unordered_map<std::string, TokenIdentifier> NonDirectiveMap = {
		{"{", BRACKET_OPEN},
		{"}", BRACKET_CLOSE},
		{";", DIRECTIVE_END},
	};
	std::unordered_map<std::string, TokenIdentifier>::const_iterator it;

	it = NonDirectiveMap.find(input);
	if (it != NonDirectiveMap.end())
		return it->second;
	if (expectDirective)
	{
		it = TokenIdentifierMap.find(input);
		if (it != TokenIdentifierMap.end())
			return it->second;
	}
	return ARGUMENT;
}

std::string	ConfigParser:: identifierToString(TokenIdentifier id)
{
	const std::unordered_map<TokenIdentifier, std::string> idMap = {
		{SERVER, "server"},
		{PORT, "port"},
		{SERVER_NAME, "server name"},
		{ROOT, "root"},
		{INDEX, "index"},
		{CGI, "cgi"},
		{AUTOINDEX, "autoindex"},
		{REDIRECT, "redirect"},
		{LOCATION, "location"},
		{LIMIT_EXCEPT, "http_method"},
		{ERROR_PAGE, "error_page"},
		{MAX_BODY_SIZE, "client_max_body_size"},
		{HTTP_METHOD_DENY, "http_method_deny"},
		{BRACKET_OPEN, "bracket open"},
		{BRACKET_CLOSE, "bracket close"},
		{DIRECTIVE_END, "directive end"},
		{ARGUMENT, "argument"},
	};
	std::unordered_map<TokenIdentifier, std::string>::const_iterator it;

	it = idMap.find(id);
	if (it != idMap.end())
		return it->second;
	return "unrecognised";
}