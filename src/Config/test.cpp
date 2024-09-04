/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   test.cpp                                           :+:    :+:            */
/*                                                     +:+                    */
/*   By: kwchu <kwchu@student.codam.nl>               +#+                     */
/*                                                   +#+                      */
/*   Created: 2024/08/30 13:57:51 by kwchu         #+#    #+#                 */
/*   Updated: 2024/09/04 16:23:02 by kwchu         ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include "ConfigParser.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <exception>
#include <iomanip>
#include <stdexcept>
#include <unistd.h>
#include "colours.hpp"

void	assertTokenOutput(
	const std::vector<std::string>& input,
	const std::vector<std::string>& expected)
{
	if (input.size() != expected.size())
		throw std::invalid_argument("amount of tokens do not match.");
	std::vector<std::string>::const_iterator inputIterator = input.begin(); 
	std::vector<std::string>::const_iterator expectedIterator = expected.begin();

	std::cout << CYAN << std::setw(23) << "input" << std::setw(0) << " | expected\n" << RESET; 
	for (; inputIterator != input.end() && expectedIterator != expected.end(); inputIterator++, expectedIterator++)
	{
		if (*inputIterator == *expectedIterator)
			std::cout << GREEN << std::setw(23) << *inputIterator << RESET;
		else
			std::cout << RED << *inputIterator << RESET;
		std::cout << std::setw(0) << " | " << *expectedIterator << '\n';
	}
}

void	runTests()
{
	std::filesystem::path	infile("parsertest.conf");

	try
	{
		std::vector<std::string>	expected = {
			"server",
			"{",
			"listen",
			"80",
			";",
			"server_name",
			"localhost",
			";",
			"root",
			"/usr/share/nginx/html",
			";",
			"asdasd",
			"location",
			"/",
			"{",
			"index",
			"index.html",
			";",
			"limit_except",
			"GET",
			"POST",
			"{",
			"deny",
			"all",
			";",
			"}",
			"blablabla",
			"autoindex",
			"on",
			";",
			"}",
			"location",
			"/test",
			"{",
			"root",
			"/usr/share/nginx/html/",
			";",
			"autoindex",
			"on",
			";",
			"index",
			"bla.html",
			";",
			"}",
			"}",
		};

		ConfigParser input = ConfigParser::CreateConfigFromFile(infile);
		assertTokenOutput(input.tokens, expected);
	}
	catch (const std::exception& e)
	{
		std::cerr << "caught exception: " << e.what() << std::endl;
	}
}

int main(int argc, char **argv)
{	
	if (argc != 2)
	{
		std::cout << "no file provided, running tests\n";
		sleep(1);
		runTests();
		return 0;
	}

	std::filesystem::path	infile(argv[1]);
	try
	{
		ConfigParser::CreateConfigFromFile(infile);
	}
	catch (const std::exception& e)
	{
		std::cerr << "caught exception: " << e.what() << std::endl;
	}
	return 0;
}
