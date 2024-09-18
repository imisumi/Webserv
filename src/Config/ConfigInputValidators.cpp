/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   ConfigInputValidators.cpp                          :+:    :+:            */
/*                                                     +:+                    */
/*   By: kwchu <kwchu@student.codam.nl>               +#+                     */
/*                                                   +#+                      */
/*   Created: 2024/09/18 12:41:20 by kwchu         #+#    #+#                 */
/*   Updated: 2024/09/18 13:13:00 by kwchu         ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include <string>
#include <cctype>

bool	stringContainsDigitsExclusively(const std::string& s)
{
	if (s.empty())
		return false;
	for (const char& c : s)
	{
		if (!isdigit(c))
			return false;
	}
	return true;
}

bool	isDelimiter(char& c, const std::string& delimiters)
{
	return delimiters.find(c) != std::string::npos;
}

bool	isSpecialCharacter(char& c)
{
	return c == '{' || c == '}' || c == ';';
}
