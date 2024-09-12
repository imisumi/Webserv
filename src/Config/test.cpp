// /* ************************************************************************** */
// /*                                                                            */
// /*                                                        ::::::::            */
// /*   test.cpp                                           :+:    :+:            */
// /*                                                     +:+                    */
// /*   By: kwchu <kwchu@student.codam.nl>               +#+                     */
// /*                                                   +#+                      */
// /*   Created: 2024/08/30 13:57:51 by kwchu         #+#    #+#                 */
// /*   Updated: 2024/09/12 14:25:38 by kwchu         ########   odam.nl         */
// /*                                                                            */
// /* ************************************************************************** */

// #include "ConfigParser.h"
// #include "Config.h"
// #include <iostream>
// #include <vector>
// #include <algorithm>
// #include <exception>
// #include <iomanip>
// #include <stdexcept>
// #include <unistd.h>
// #include "colours.hpp"

// int main(int argc, char **argv)
// {	
// 	if (argc != 2)
// 	{
// 		std::cout << "no file provided\n";
// 		return 0;
// 	}

// 	std::filesystem::path	infile(argv[1]);
// 	try
// 	{
// 		Config config = ConfigParser::createConfigFromFile(infile);

// 		std::string line;
// 		config.print();
// 		while (std::getline(std::cin, line))
// 		{
// 			try
// 			{
// 				uint64_t port = static_cast<uint64_t>(std::stoul(line));
// 				std::vector<ServerSettings*> serverVector = config[port];
// 				std::vector<ServerSettings*>::iterator it = serverVector.begin();
// 				ServerSettings*	server = *it;
// 				std::cout << server->GetServerName() << '\n';
// 			}
// 			catch (const std::exception& e)
// 			{
// 				std::cerr << "caught exception: " << e.what() << std::endl;
// 			}
// 		}
// 	}
// 	catch (const std::exception& e)
// 	{
// 		std::cerr << "caught exception: " << e.what() << std::endl;
// 	}
// 	return 0;
// }
