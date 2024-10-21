#include "Api.h"
#include <filesystem>
#include <string>
#include <unordered_set>
#include "Core/Log.h"

static const std::unordered_set<std::string> supportedImageExtensions = {".jpg", ".jpeg", ".png",  ".gif",
																		 ".bmp", ".tiff", ".webp", ".ico"};

std::string Api::getImages(const std::filesystem::path& path)
{
	Log::info("API images request");
	std::string httpResponse;
	std::string json = "{\n";
	json += "\"status\": \"success\",\n";
	json += "\"images\": [\n";
	bool firstImage = true;

	// TODO: return custom json
	if (!std::filesystem::is_directory(path))
		return "";
	for (const auto& entry : std::filesystem::directory_iterator(path))
	{
		if (entry.is_regular_file())
		{
			std::string extension = entry.path().extension().string();
			if (supportedImageExtensions.find(extension) != supportedImageExtensions.end())
			{
				if (!firstImage)
				{
					json += ",\n";
				}
				firstImage = false;
				json += "{\n";
				json += "\"filename\": \"" + entry.path().filename().string() + "\",\n";
				json += "\"url\": \"/images/" + entry.path().filename().string() + "\",\n";
				json += "\"size\": " + std::to_string(std::filesystem::file_size(entry.path())) + "\n";
				json += "}";
			}
		}
	}
	json += "\n]}\n";
	httpResponse += "HTTP/1.1 200 OK\r\n";
	httpResponse += "Content-Type: application/json\r\n";
	httpResponse += "Content-Length: " + std::to_string(json.size()) + "\r\n";
	httpResponse += "\r\n";
	httpResponse += json;
	return httpResponse;
}

std::string Api::getFiles(const std::filesystem::path& path)
{
	Log::info("API files request");
	std::string httpResponse;
	std::string json = "{\n";
	json += "\"status\": \"success\",\n";
	json += "\"files\": [\n";
	bool firstFile = true;

	if (!std::filesystem::is_directory(path))
		return "";
	for (const auto& entry : std::filesystem::directory_iterator(path))
	{
		if (entry.is_regular_file())
		{
			if (!firstFile)
			{
				json += ",\n";
			}
			firstFile = false;
			json += "{\n";
			json += "\"filename\": \"" + entry.path().filename().string() + "\",\n";
			json += "\"url\": \"/files/" + entry.path().filename().string() + "\",\n";
			json += "\"size\": " + std::to_string(std::filesystem::file_size(entry.path())) + "\n";
			json += "}";
		}
	}
	json += "\n]}\n";
	httpResponse += "HTTP/1.1 200 OK\r\n";
	httpResponse += "Content-Type: application/json\r\n";
	httpResponse += "Content-Length: " + std::to_string(json.size()) + "\r\n";
	httpResponse += "\r\n";
	httpResponse += json;
	return httpResponse;
}
