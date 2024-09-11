/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   test.cpp                                           :+:    :+:            */
/*                                                     +:+                    */
/*   By: kwchu <kwchu@student.codam.nl>               +#+                     */
/*                                                   +#+                      */
/*   Created: 2024/08/30 13:57:51 by kwchu         #+#    #+#                 */
/*   Updated: 2024/09/11 16:21:08 by kwchu         ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include "ConfigParser.h"
#include "Config.h"
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
		Config config = ConfigParser::createConfigFromFile(infile);

		std::vector<ServerSettings*> serverVector = config[83];
		std::vector<ServerSettings*>::iterator it = serverVector.begin();
		ServerSettings*	server = *it;
		std::cout << server->getServerName() << '\n';
	}
	catch (const std::exception& e)
	{
		std::cerr << "caught exception: " << e.what() << std::endl;
	}
	return 0;
}
