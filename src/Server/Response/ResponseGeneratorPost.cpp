#include "ResponseGenerator.h"

#include "Core/Core.h"

#include "Utils/Utils.h"

#include <regex>
#include <algorithm>
#include <cctype>
#include <filesystem>

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

bool isBoundaryString(const std::string& value, const std::string& boundary)
{
    return value == boundary;
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

bool saveFormDataToFile(std::string& firstName, std::string lastName, std::string email, const std::string& boundary)
{
    if (isBoundaryString(firstName, boundary))
    {
        firstName = "EMPTY";
    }
    if (isBoundaryString(lastName, boundary))
    {
        lastName = "EMPTY";
    }
    if (isBoundaryString(email, boundary))
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
    file << "First Name: " << (firstName.empty() ? "EMPTY" : firstName) << "\n";
    file << "Last Name: " << (lastName.empty() ? "EMPTY" : lastName) << "\n";
    file << "Email: " << (email.empty() ? "EMPTY" : email) << "\n\n";

    file.close();
    return !file.fail();
}

bool saveUploadedFile(const std::string& body, const std::string& boundary, const std::string& fieldName,
                      const std::string& uploadDir, const uint64_t maxClientBodySize)
{
    std::regex fileRegex(boundary + R"([\r\n]+Content-Disposition: form-data; name=\")" + fieldName +
                         R"(\"; filename=\"([^\"]+)\")" + R"([\r\n]+Content-Type: ([^\r\n]+)[\r\n]+[\r\n]+)");
    std::smatch match;
    if (std::regex_search(body, match, fileRegex))
    {
        std::string fileName = match[1].str();
        std::string contentType = match[2].str();

        size_t fileStartPos = body.find(match[0]) + match[0].length();
        size_t fileEndPos = body.find(boundary, fileStartPos) - 4;

        std::string fileContent = body.substr(fileStartPos, fileEndPos - fileStartPos);

        if (fileContent.size() > maxClientBodySize)
        {
            return false;
        }

        if (fileName.empty())
        {
            return true;
        }

        std::string filePath = uploadDir + "/" + fileName;
        std::ofstream file(filePath, std::ios::binary);
        if (!file.is_open())
        {
            return false;
        }

        file.write(fileContent.c_str(), fileContent.size());
        file.close();
        if (file.fail())
        {
            return false;
        }
        return true;
    }

    return false;
}

static const std::unordered_map<std::string, std::string> supportedFileTypes = {
	{".html", "text/html"}, {".css", "text/css"},	 {".ico", "image/x-icon"},
	{".jpg", "image/jpeg"}, {".jpeg", "image/jpeg"}, {".png", "image/png"}};


std::string trim(const std::string& str)
{
	size_t first = str.find_first_not_of(' ');
	size_t last = str.find_last_not_of(' ');
	return (first == std::string::npos) ? "" : str.substr(first, last - first + 1);
}

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

bool ensureDirectoriesExist()
{
    std::string baseDir = getProjectRootDir() + "/database";
    std::string filesDir = baseDir + "/files";
    std::string imagesDir = baseDir + "/images";

    if (!std::filesystem::exists(baseDir))
    {
        if (!std::filesystem::create_directory(baseDir))
            return false;
    }
    if (!std::filesystem::exists(filesDir))
    {
        if (!std::filesystem::create_directory(filesDir))
            return false;
    }
    if (!std::filesystem::exists(imagesDir))
    {
        if (!std::filesystem::create_directory(imagesDir))
            return false;
    }
    return true;
}

bool savePlainTextToFile(const std::string& body, const uint64_t maxClientBodySize)
{
    if (body.size() > maxClientBodySize)
    {
        return false;
    }

    std::string filePath = getProjectRootDir() + "/database/plaintext.txt";
    std::ofstream file(filePath, std::ios_base::app);
    if (!file.is_open())
    {
        return false;
    }
    file << body << "\n\n";
    file.close();
    return !file.fail();
}

std::string ResponseGenerator::generatePayloadTooLargeResponse()
{
    std::string response = "HTTP/1.1 413 Payload Too Large\r\n";
    response += "Content-Type: text/html\r\n";
    std::string body = "<html><body><h1>413 Payload Too Large</h1></body></html>";
    response += "Content-Length: " + std::to_string(body.length()) + "\r\n";
    response += "\r\n";
    response += body;
    return response;
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
        return GenerateErrorResponse(HTTPStatusCode::BadRequest, client);
    }

    if (!ensureDirectoriesExist())
    {
        Log::error("Failed to create necessary directories");
        return generateInternalServerErrorResponse();
    }

    if (contentType.find("multipart/form-data") != std::string::npos)
    {
        std::string boundary = extractBoundary(contentType);
        if (boundary.empty())
        {
            Log::error("Boundary missing in Content-Type header");
			return GenerateErrorResponse(HTTPStatusCode::BadRequest, client);
        }

        std::string body = client.GetRequest().body;
        if (body.empty())
        {
            Log::error("Request body is empty");
			return GenerateErrorResponse(HTTPStatusCode::BadRequest, client);
        }

        std::string firstName = parseMultipartData(body, boundary, "firstname");
        std::string lastName = parseMultipartData(body, boundary, "lastname");
        std::string email = parseMultipartData(body, boundary, "email");

        if (!saveFormDataToFile(firstName, lastName, email, boundary))
        {
            Log::error("Failed to save form data to file");
            return generateInternalServerErrorResponse();
        }

        std::string fileContentType = parseMultipartContentType(body, boundary, "file");
        if (fileContentType.empty())
        {
            Log::error("File content type missing");
			return GenerateErrorResponse(HTTPStatusCode::BadRequest, client);
        }

        Log::info("Detected file content type: " + fileContentType);

        std::string uploadDir;
        if (isSupportedFileType(fileContentType))
        {
            uploadDir = getProjectRootDir() + "/database/images";
        }
        else
        {
            uploadDir = getProjectRootDir() + "/database/files";
        }

        if (!saveUploadedFile(body, boundary, "file", uploadDir, client.GetLocationSettings().maxBodySize))
        {
            Log::error("Failed to save uploaded file");
			return GenerateErrorResponse(HTTPStatusCode::PayloadTooLarge, client);
        }

        Log::info("Successfully handled POST request, saved form data, and saved file");
        return generateOKResponse(client);
    }
    else if (contentType.find("text/plain") != std::string::npos)
    {
        std::string body = client.GetRequest().body;
        if (body.empty())
        {
            Log::error("Request body is empty");
			return GenerateErrorResponse(HTTPStatusCode::BadRequest, client);
        }

        if (!savePlainTextToFile(body, client.GetLocationSettings().maxBodySize))
        {
            Log::error("Failed to save plain text or payload too large");
			return GenerateErrorResponse(HTTPStatusCode::PayloadTooLarge, client);
        }

        Log::info("Successfully handled POST request, saved plain text");
        return generateOKResponse(client);
    }
    else
    {
        Log::error("Unsupported Content-Type");
		return GenerateErrorResponse(HTTPStatusCode::BadRequest, client);
    }
}
