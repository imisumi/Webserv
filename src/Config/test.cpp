/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   test.cpp                                           :+:    :+:            */
/*                                                     +:+                    */
/*   By: kwchu <kwchu@student.codam.nl>               +#+                     */
/*                                                   +#+                      */
/*   Created: 2024/08/30 13:57:51 by kwchu         #+#    #+#                 */
/*   Updated: 2024/09/02 16:26:17 by kwchu         ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include "ConfigParser.h"
#include <iostream>

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
		Config::CreateConfigFromFile(infile);
	}
	catch (const std::exception& e)
	{
		std::cerr << "caught exception: " << e.what() << std::endl;
	}
	return 0;
}
