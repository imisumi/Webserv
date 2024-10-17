#include "ResponseGenerator.h"

const std::string ResponseGenerator::handleDeleteRequest(const Client& Client)
{
	const HttpRequest& request = Client.GetRequest();

	// delete the file
	Log::info("hello");
	if (std::filesystem::exists(request.mappedPath) && std::filesystem::is_regular_file(request.mappedPath))
	{
		if (std::filesystem::remove(request.mappedPath))
		{
			Log::info("File deleted successfully: {}", request.mappedPath.string());
			return OkResponse();
		}
		else
		{
			Log::error("Failed to delete file: {}", request.mappedPath.string());
			return generateInternalServerErrorResponse();
		}
	}
	else if (!std::filesystem::exists(request.mappedPath))
	{
		return NotFound();
	}
	else
	{
		return BadRequest();
	}
}
