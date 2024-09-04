#include "Cgi.h"


bool Cgi::isValidCGI(const Config& config, const std::filesystem::path& path)
{
	if (path.extension() == ".cgi" || path.extension() == ".py")
	{
		std::filesystem::perms perms = std::filesystem::status(path).permissions();
		if ((perms & std::filesystem::perms::owner_exec) != std::filesystem::perms::none ||
			(perms & std::filesystem::perms::group_exec) != std::filesystem::perms::none ||
			(perms & std::filesystem::perms::others_exec) != std::filesystem::perms::none)
		{
			return true;
		}
	}
	return false;
}
