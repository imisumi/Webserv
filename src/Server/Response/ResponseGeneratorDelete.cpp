#include "ResponseGenerator.h"

const std::string ResponseGenerator::handleDeleteRequest(const Client& Client)
{
	const HttpRequest& request = Client.GetRequest();

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
			return InternalServerError();
		}
	}
	else if (!std::filesystem::exists(request.mappedPath))
	{
		Log::error("file does not exist");
		return GenerateErrorResponse(HTTPStatusCode::NotFound, Client);
	}
	else
	{
		Log::error("bad request");
		return GenerateErrorResponse(HTTPStatusCode::BadRequest, Client);
	}
}
