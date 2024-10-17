#include "ResponseGenerator.h"

#include "Utils/Utils.h"
#include "Api/Api.h"
#include "Cgi/Cgi.h"

const std::string ResponseGenerator::handleGetRequest(const Client& client)
{
	Utils::Timer timer;
	Log::info("Handling GET request");

	if (client.GetLocationSettings().redirect.first != 0)
		return GenerateRedirectResponse(client.GetLocationSettings().redirect.first, client.GetLocationSettings().redirect.second);

	Api api;
	api.addApiRoute("/api/v1/images");
	api.addApiRoute("/api/v1/files");
	if (api.isApiRoute(client.GetRequest().path.string()))
	{
		if (client.GetRequest().path.string() == "/api/v1/images")
		{
			std::filesystem::path databaseImagePath = std::filesystem::current_path() / "database" / "images";
			return Api::getImages(databaseImagePath);
		}
		else if (client.GetRequest().path.string() == "/api/v1/files")
		{
			std::filesystem::path databaseFilePath = std::filesystem::current_path() / "database" / "files";
			return Api::getFiles(databaseFilePath);
		}
	}
	//? Validate the requested path

	const std::filesystem::path& path = client.GetRequest().mappedPath;
	if (std::filesystem::exists(path))
	{
		//? Check if the requested path is a directory or a file
		if (std::filesystem::is_directory(path))
		{
			Log::debug("Requested path is a directory");

			const std::vector<std::string>& indexes = client.GetLocationSettings().index;
			Log::info("----------- {}", client.GetLocationSettings().index[0]);
			Log::info("Index: {}", indexes[0]);
			Log::info("Server name: {}", client.GetServerConfig()->GetServerName());

			for (const auto& index : indexes)
			{
				Log::info("Looking for: {}", index);
				std::filesystem::path indexPath = path / index;
				if (std::filesystem::exists(indexPath))
				{
					Log::info("Index found: {}", indexPath.string());

					if (!isFileModified(client.GetRequest()))
					{
						return generateNotModifiedResponse();
					}

					// client.GetRequest().setUri(indexPath);
					// HttpRequest updatedRequest = client.GetRequest();
					// updatedRequest.setUri(indexPath);

					HttpRequest updatedRequest = client.GetRequest();
					updatedRequest.mappedPath = indexPath;
					return generateOKResponse(indexPath, updatedRequest);
				}
			}
			if (client.GetLocationSettings().autoindex)
			{
				return generateDirectoryListingResponse(path);
			}
			return generateNotFoundResponse();
		}
		else if (std::filesystem::is_regular_file(path))
		{
			Log::debug("Requested path is a file");
			const ServerSettings::LocationSettings& location = client.GetLocationSettings();

			//? check if the file is a CGI script
			if (location.cgi.size() > 0)
			{
				Log::info("Requested path is a CGI script");
				for (const auto& cgi : location.cgi)
				{
					Log::info("CGI: {}", cgi);
					if (path.extension() == cgi)
					{
						Log::info("CGI script found: {}", path.string());
						return Cgi::executeCGI(client, client.GetRequest());
					}
				}
				return ResponseGenerator::generateForbiddenResponse();
			}

			// if (!isFileModified(client.GetRequest()))
			// {
			// 	return generateNotModifiedResponse();
			// }

			return generateFileResponse(client.GetRequest());
		}
		else
		{
			//TODO: send 405 response???
			Log::error("Requested path is not a file or directory");

			return generateNotFoundResponse();
		}
	}
	Log::debug("Requested path does not exist");
	return generateNotFoundResponse();
}