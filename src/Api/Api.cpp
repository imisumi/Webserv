#include "Api.h"
#include "Core/Log.h"

static const std::unordered_set<std::string> supportedImageExtensions = {
	".jpg",
	".jpeg",
	".png",
	".gif",
	".bmp",
	".tiff",
	".webp",
	".ico"
};

static const std::unordered_set<std::string> supportedFileExtensions = {
	".txt",
	".pdf"
};

/*
	Returns all images in the directory in json format
*/
std::string Api::getImages(const std::filesystem::path& path)
{
	LOG_ERROR("API images request");
	std::string httpResponse;
	std::string json = "{\n";
	
	// Add status
	json += "\"status\": \"success\",\n";
	json += "\"images\": [\n";

	bool firstImage = true; // Use this flag to handle commas correctly
	for (const auto& entry : std::filesystem::directory_iterator(path))
	{
		if (entry.is_regular_file())
		{
			std::string extension = entry.path().extension().string();
			if (supportedImageExtensions.find(extension) != supportedImageExtensions.end())
			{
				// If this is not the first image, add a comma
				if (!firstImage)
				{
					json += ",\n"; // Add comma before subsequent images
				}
				firstImage = false;

				// Add image details
				json += "{\n";
				json += "\"filename\": \"" + entry.path().filename().string() + "\",\n";
				json += "\"url\": \"/images/" + entry.path().filename().string() + "\",\n"; // Adjust URL as needed
				json += "\"size\": " + std::to_string(std::filesystem::file_size(entry.path())) + "\n";
				json += "}";
			}
		}
	}

	json += "\n]}\n"; // Close the images array and the JSON object

	// Prepare HTTP response
	httpResponse += "HTTP/1.1 200 OK\r\n";
	httpResponse += "Content-Type: application/json\r\n";
	httpResponse += "Content-Length: " + std::to_string(json.size()) + "\r\n";
	httpResponse += "\r\n";
	httpResponse += json;

	return httpResponse;
}

std::string Api::getFiles(const std::filesystem::path& path)
{
	LOG_ERROR("API files request");
	std::string httpResponse;
	std::string json = "{\n";
	
	// Add status
	json += "\"status\": \"success\",\n";
	json += "\"files\": [\n";

	bool firstImage = true; // Use this flag to handle commas correctly
	for (const auto& entry : std::filesystem::directory_iterator(path))
	{
		if (entry.is_regular_file())
		{
			std::string extension = entry.path().extension().string();
			if (supportedFileExtensions.find(extension) != supportedFileExtensions.end())
			{
				// If this is not the first image, add a comma
				if (!firstImage)
				{
					json += ",\n"; // Add comma before subsequent images
				}
				firstImage = false;

				// Add image details
				json += "{\n";
				json += "\"filename\": \"" + entry.path().filename().string() + "\",\n";
				json += "\"url\": \"/files/" + entry.path().filename().string() + "\",\n"; // Adjust URL as needed
				json += "\"size\": " + std::to_string(std::filesystem::file_size(entry.path())) + "\n";
				json += "}";
			}
		}
	}

	json += "\n]}\n"; // Close the images array and the JSON object

	// Prepare HTTP response
	httpResponse += "HTTP/1.1 200 OK\r\n";
	httpResponse += "Content-Type: application/json\r\n";
	httpResponse += "Content-Length: " + std::to_string(json.size()) + "\r\n";
	httpResponse += "\r\n";
	httpResponse += json;

	return httpResponse;
}