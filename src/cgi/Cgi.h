#pragma once


#include "Config/ConfigParser.h"


class Cgi
{
public:

	static bool isValidCGI(const Config& config, const std::filesystem::path& path);
private:

};