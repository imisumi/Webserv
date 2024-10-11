#include "ResponseGenerator.h"

#include "Core/Core.h"

#include "Utils/Utils.h"

#include <regex>
#include <algorithm>
#include <cctype>

std::string getProjectRootDir()
{
	return std::filesystem::current_path();
	const char* rootDir = std::getenv("WEBSERV_ROOT");
	if (!rootDir)
	{
		Log::error("WEBSERV_ROOT environment variable not set!");
		throw std::runtime_error("WEBSERV_ROOT environment variable is not set.");
	}
	return std::string(rootDir);
}

bool isBoundaryString(const std::string& value)
{
	return value.find("-----------------------------") != std::string::npos;
}

std::string extractBoundary(const std::string& contentType)
{
	std::regex boundaryRegex("boundary=(.*)");
	std::smatch match;
	if (std::regex_search(contentType, match, boundaryRegex))
	{
		return "--" + match[1].str();
	}
	return "";
}

std::string parseMultipartData(const std::string& body, const std::string& boundary, const std::string& fieldName)
{
	std::regex fieldRegex(boundary + R"([\r\n]+Content-Disposition: form-data; name=\")" + fieldName +
						  R"(\"[\r\n]+[\r\n]+([^\r\n]*)[\r\n]+)");
	std::smatch match;
	if (std::regex_search(body, match, fieldRegex))
	{
		return match[1].str();
	}
	return "";
}

bool saveFormDataToFile(std::string& firstName, std::string lastName, std::string email)
{
	if (isBoundaryString(firstName))
	{
		firstName = "EMPTY";
	}
	if (isBoundaryString(lastName))
	{
		lastName = "EMPTY";
	}
	if (isBoundaryString(email))
	{
		email = "EMPTY";
	}

	std::string filePath = getProjectRootDir() + "/database/text.txt";
	std::ofstream file(filePath, std::ios_base::app);
	if (!file.is_open())
	{
		std::cerr << "[ERROR] Failed to open file: " << filePath << std::endl;
		return false;
	}
	file << "Last Name: " << (firstName.empty() ? "EMPTY" : firstName) << "\n";
	file << "Last Name: " << (lastName.empty() ? "EMPTY" : lastName) << "\n";
	file << "Email: " << (email.empty() ? "EMPTY" : email) << "\n\n";

	file.close();
	return !file.fail();
}
bool saveUploadedFile(const std::string& body, const std::string& boundary, const std::string& fieldName,
					  const std::string& uploadDir)
{
	std::regex fileRegex(boundary + R"([\r\n]+Content-Disposition: form-data; name=\")" + fieldName +
						 R"(\"; filename=\"([^\"]+)\")" + R"([\r\n]+Content-Type: ([^\r\n]+)[\r\n]+[\r\n]+)");
	std::smatch match;
	if (std::regex_search(body, match, fileRegex))
	{
		std::string fileName = match[1].str();
		std::string contentType = match[2].str();

		// Vind het begin van het bestand na de headers
		size_t fileStartPos = body.find(match[0]) + match[0].length();
		size_t fileEndPos = body.find(boundary, fileStartPos) - 4;	// -4 to remove preceding \r\n--

		std::string fileContent = body.substr(fileStartPos, fileEndPos - fileStartPos);

		if (fileName.empty())
		{
			std::cerr << "[INFO] No file uploaded." << std::endl;
			return true;
		}

		std::string filePath = uploadDir + "/" + fileName;
		std::ofstream file(filePath, std::ios::binary);
		if (!file.is_open())
		{
			std::cerr << "[ERROR] Failed to open file: " << filePath << std::endl;
			return false;
		}

		file.write(fileContent.c_str(), fileContent.size());
		file.close();
		if (file.fail())
		{
			std::cerr << "[ERROR] Failed to write to file: " << filePath << std::endl;
			return false;
		}
		return true;
	}

	std::cerr << "[ERROR] Regex failed to match for file field: " << fieldName << std::endl;
	return false;
}

static const std::unordered_map<std::string, std::string> supportedFileTypes = {
	{".html", "text/html"}, {".css", "text/css"},	 {".ico", "image/x-icon"},
	{".jpg", "image/jpeg"}, {".jpeg", "image/jpeg"}, {".png", "image/png"}};



// Function to trim whitespace from both ends of a string
std::string trim(const std::string& str)
{
	size_t first = str.find_first_not_of(' ');
	size_t last = str.find_last_not_of(' ');
	return (first == std::string::npos) ? "" : str.substr(first, last - first + 1);
}

// Function to convert string to lowercase
std::string toLowerCase(const std::string& str)
{
	std::string lowerStr = str;
	std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), ::tolower);
	return lowerStr;
}

bool isSupportedFileType(const std::string& contentType)
{
	std::string trimmedContentType = trim(contentType);	 // Trim whitespace
	std::string lowerContentType =
		toLowerCase(trimmedContentType);  // Convert to lowercase for case-insensitive comparison

	for (const auto& [extension, mimeType] : supportedFileTypes)
	{
		if (lowerContentType == mimeType)
		{
			return true;
		}
	}
	return false;
}

std::string ResponseGenerator::parseMultipartContentType(const std::string& body, const std::string& boundary,
														 const std::string& fieldName)
{
	size_t pos = body.find("Content-Disposition: form-data; name=\"" + fieldName + "\"");
	if (pos == std::string::npos)
		return "";

	pos = body.find("Content-Type:", pos);
	if (pos == std::string::npos)
		return "";

	size_t endPos = body.find("\r\n", pos);
	std::string contentType = body.substr(pos + 13, endPos - (pos + 13));  // Extract Content-Type value
	return contentType;
}

const std::string ResponseGenerator::handlePostRequest(const Client& client)
{
	Utils::Timer timer;
	Log::info("Handling POST request");

	if (client.GetLocationSettings().redirect.first != 0)
		return GenerateRedirectResponse(client.GetLocationSettings().redirect.first,
										client.GetLocationSettings().redirect.second);

	std::string contentType = client.GetRequest().getHeaderValue("content-type");
	if (contentType.empty())
	{
		Log::error("Missing Content-Type header");
		return generateBadRequestResponse();
	}

	std::string boundary = extractBoundary(contentType);
	if (boundary.empty())
	{
		Log::error("Boundary missing in Content-Type header");
		return generateBadRequestResponse();
	}

	std::string body = client.GetRequest().body;
	if (body.empty())
	{
		Log::error("Request body is empty");
		return generateBadRequestResponse();
	}

	// Parse form data fields
	std::string firstName = parseMultipartData(body, boundary, "firstname");
	std::string lastName = parseMultipartData(body, boundary, "lastname");
	std::string email = parseMultipartData(body, boundary, "email");

	if (!saveFormDataToFile(firstName, lastName, email))
	{
		Log::error("Failed to save form data to file");
		return generateInternalServerErrorResponse();
	}

	// Parse file data and content type
	std::string fileContentType = parseMultipartContentType(body, boundary, "file");
	if (fileContentType.empty())
	{
		Log::error("File content type missing");
		return generateBadRequestResponse();
	}

	// Log the detected content type for debugging
	Log::info("Detected file content type: " + fileContentType);

	// Check if the file's content type matches one of the supported image types
	std::string uploadDir;
	if (isSupportedFileType(fileContentType))
	{
		uploadDir = getProjectRootDir() + "/database/images";
	}
	else
	{
		uploadDir = getProjectRootDir() + "/database/files";
	}

	// Save the uploaded file
	if (!saveUploadedFile(body, boundary, "file", uploadDir))
	{
		Log::error("Failed to save uploaded file");
		return generateInternalServerErrorResponse();
	}

	Log::info("Successfully handled POST request, saved form data, and saved file");
	return generateOKResponse(client);
}
