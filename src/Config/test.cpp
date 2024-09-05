/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   test.cpp                                           :+:    :+:            */
/*                                                     +:+                    */
/*   By: kwchu <kwchu@student.codam.nl>               +#+                     */
/*                                                   +#+                      */
/*   Created: 2024/08/30 13:57:51 by kwchu         #+#    #+#                 */
/*   Updated: 2024/09/05 16:19:00 by kwchu         ########   odam.nl         */
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

int main(int argc, char **argv)
{	
	if (argc != 2)
	{
		std::cout << "no file provided\n";
		return 0;
	}

	std::filesystem::path	infile(argv[1]);
	try
	{
		ConfigParser::createConfigFromFile(infile);
	}
	catch (const std::exception& e)
	{
		std::cerr << "caught exception: " << e.what() << std::endl;
	}
	return 0;
}
