#include "ResponseGenerator.h"

#include "Core/Core.h"


#include <sys/stat.h>


#include "Cgi/Cgi.h"


static const char s_ForbiddenResponse[] = 
		"HTTP/1.1 403 Forbidden\r\n"
		"Content-Length: 28\r\n" //? length has to match the length of the body
		"Connection: close\r\n"
		"Content-Type: text/plain\r\n"
		"\r\n"
		"403 Forbidden: Access Denied"; //? body of the response

static const char s_NotFoundResponse[] = 
		"HTTP/1.1 404 Not Found\r\n"
		"Content-Length: 33\r\n" //? length has to match the length of the body
		"Connection: close\r\n"
		"Content-Type: text/plain\r\n"
		"\r\n"
		"404 Not Found: Resource Not Found"; //? body of the response

static const char s_InternalServerErrorResponse[] = 
		"HTTP/1.1 500 Internal Server Error\r\n"
		"Content-Length: 39\r\n" //? length has to match the length of the body
		"Connection: close\r\n"
		"Content-Type: text/plain\r\n"
		"\r\n"
		"500 Internal Server Error: Server Error"; //? body of the response

static const char s_OkResponse[] = 
		"HTTP/1.1 200 OK\r\n"
		"Content-Length: 6\r\n" //? length has to match the length of the body
		"Connection: close\r\n"
		"Content-Type: text/plain\r\n"
		"\r\n"
		"200 OK"; //? body of the response

static const char s_MethodNotAllowedResponse[] =
		"HTTP/1.1 405 Method Not Allowed\r\n"
		"Content-Length: 42\r\n" //? length has to match the length of the body
		"Connection: close\r\n"
		"Content-Type: text/plain\r\n"
		"\r\n"
		"405 Method Not Allowed: Method Not Allowed"; //? body of the response

static const char s_MethodNotImplementedResponse[] =
		"HTTP/1.1 501 Not Implemented\r\n"
		"Content-Length: 43\r\n" //? length has to match the length of the body
		"Connection: close\r\n"
		"Content-Type: text/plain\r\n"
		"\r\n"
		"501 Not Implemented: Method Not Implemented"; //? body of the response

static const char s_BadRequestResponse[] =
		"HTTP/1.1 400 Bad Request\r\n"
		"Content-Length: 32\r\n" //? length has to match the length of the body
		"Connection: close\r\n"
		"Content-Type: text/plain\r\n"
		"\r\n"
		"400 Bad Request: Invalid Request"; //? body of the response


//? Multipurpose Internet Mail Extensions (MIME) type
static const std::unordered_map<std::string, std::string> s_SupportedMineTypes = {
			{ ".html", "text/html" },
			{ ".css", "text/css" },
			{ ".ico", "image/x-icon" },
			{ ".jpg", "image/jpeg" },
			{ ".jpeg", "image/jpeg" },
			{ ".png", "image/png" }
		};

std::string generateDirectoryListing(const std::string& path)
{
	// Check if the path exists
	if (!std::filesystem::exists(path)) {
		std::cerr << "Error: The directory '" << path << "' does not exist." << std::endl;
		return "";
	}

	// HTML template for the directory listing
	std::string html_template = R"(
	<!DOCTYPE html>
	<html lang="en">
	<head>
		<meta charset="UTF-8">
		<meta name="viewport" content="width=device-width, initial-scale=1.0">
		<title>Directory Listing</title>
		<style>
			body {
				font-family: Arial, sans-serif;
				margin: 20px;
				background-color: #121212;
				color: #f0f0f0;
			}
			table {
				width: 100%;
				border-collapse: collapse;
				margin-top: 20px;
				background-color: #1e1e1e;
			}
			th, td {
				padding: 12px 15px;
				text-align: left;
				border-bottom: 1px solid #333;
			}
			th {
				background-color: #333;
				color: #ffffff;
				text-align: left;
				font-weight: bold;
			}
			td {
				color: #e0e0e0;
			}
			a {
				text-decoration: none;
				color: #1e90ff;
			}
			a:hover {
				text-decoration: underline;
			}
			tr:hover {
				background-color: #333;
			}
		</style>
	</head>
	<body>
		<h1>Directory Listing for: {path}</h1>
		<table>
			<thead>
				<tr>
					<th>Name</th>
					<th>Type</th>
					<th>Size (Bytes)</th>
				</tr>
			</thead>
			<tbody>
				{rows}
			</tbody>
		</table>
	</body>
	</html>
	)";

	// Vector to store the rows for the table
	std::vector<std::string> rows;

	// Iterate over directory entries
	for (const auto& entry : std::filesystem::directory_iterator(path)) {
		std::string item_name = entry.path().filename().string();
		std::string item_type = entry.is_directory() ? "Directory" : "File";
		std::string item_size = entry.is_regular_file() ? std::to_string(std::filesystem::file_size(entry.path())) : "-";

		// Generate a row for each item
		std::string row = R"(
		<tr>
			<td><a href=")" + item_name + R"(">)" + item_name + R"(</a></td>
			<td>)" + item_type + R"(</td>
			<td>)" + item_size + R"(</td>
		</tr>
		)";

		// Add the row to the vector
		rows.push_back(row);
	}
	// for (const auto& entry : std::filesystem::directory_iterator(path)) {
    //     std::string item_name = entry.path().filename().string();
    //     std::string item_type = entry.is_directory() ? "Directory" : "File";
    //     std::string item_size = entry.is_regular_file() ? std::to_string(std::filesystem::file_size(entry.path())) : "-";

    //     // Generate a row for each item, prepending the path to the link
    //     std::string row = R"(
    //     <tr>
    //         <td><a href=")" + path + "/" + item_name + R"(">)" + item_name + R"(</a></td>
    //         <td>)" + item_type + R"(</td>
    //         <td>)" + item_size + R"(</td>
    //     </tr>
    //     )";

    //     // Add the row to the vector
    //     rows.push_back(row);
    // }

	// Combine all rows into a single string
	std::string rows_html;
	for (const auto& row : rows) {
		rows_html += row + "\n";
	}

	// Replace placeholders in the HTML template
	std::string html_content = html_template;
	size_t pos = html_content.find("{path}");
	html_content.replace(pos, 6, path);
	pos = html_content.find("{rows}");
	html_content.replace(pos, 6, rows_html);

	// return "Content-Type: text/html\r\n\r\n" + html_content;
	return html_content;

	// Output the HTML content
	// std::cout << "Content-Type: text/html\r\n\r\n";
	// std::cout << html_content << std::endl;
}

//? assume that the file exists
//? return the contents of the file or nullopt if the file cannot be opened due to permissions
std::optional<std::string> ResponseGenerator::readFileContents(const std::filesystem::path& path)
{
	LOG_DEBUG("Reading file contents: {}", path.string());
	std::ifstream file(path);
	if (!file.is_open())
	{
		LOG_ERROR("Failed to open file: {}", path.string());
		return std::nullopt;
	}

	std::stringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}

std::string ResponseGenerator::determineContentType(const std::filesystem::path& file)
{
	// log file
	// LOG_INFO("File: {}", file.string());
	std::string extension = file.extension().string();
	LOG_INFO("Extension: {}", extension);
	auto it = s_SupportedMineTypes.find(extension);

	if (it != s_SupportedMineTypes.end())
	{
		LOG_INFO("Content-Type: {}", it->second);
		return it->second;
	}
	return std::string();
}

std::string ResponseGenerator::HTTPStatusCodeToString(HTTPStatusCode code)
{
	switch (code)
	{
		case HTTPStatusCode::OK:					return "200 OK";
		case HTTPStatusCode::OKCreated:				return "201 Created";
		case HTTPStatusCode::MovedToNewURL:			return "301 Moved Permanently";
		case HTTPStatusCode::NotModified:			return "304 Not Modified";
		case HTTPStatusCode::BadRequest:			return "400 Bad Request";
		case HTTPStatusCode::Unauthorized:			return "401 Unauthorized";
		case HTTPStatusCode::Forbidden:				return "403 Forbidden";
		case HTTPStatusCode::NotFound:				return "404 Not Found";
		case HTTPStatusCode::MethodNotAllowed:		return "405 Method Not Allowed";
		case HTTPStatusCode::InternalServerError:	return "500 Internal Server Error";
		default:									return "";
	}
}

std::string ResponseGenerator::ContentTypeToString(ContentType type)
{
	switch (type)
	{
		case ContentType::HTML:		return "text/html";
		case ContentType::TEXT:		return "text/plain";
		case ContentType::IMAGE:	return "image/x-icon";
		default:					return "";
	}
}

std::string getProjectRootDir() {
	return std::filesystem::current_path();
	const char* rootDir = std::getenv("WEBSERV_ROOT");
	if (!rootDir) {
		LOG_ERROR("WEBSERV_ROOT environment variable not set!");
		throw std::runtime_error("WEBSERV_ROOT environment variable is not set.");
	}
	return std::string(rootDir);
}

std::string ResponseGenerator::getCurrentDateAndTime()
{
	// Get current time
	auto now = std::chrono::system_clock::now();
	std::time_t current_time = std::chrono::system_clock::to_time_t(now);
	std::tm* local_time = std::localtime(&current_time);

	// Format the date and time
	std::ostringstream oss;
	oss << std::put_time(local_time, "%a, %d %b %Y %H:%M:%S CET");

	return oss.str();
}

static std::string getFileModificationTime(const std::filesystem::path& filePath)
{
	//? Since this is the body of the response, we assume the file exists
	// Get the last write time of the file
	auto ftime = std::filesystem::last_write_time(filePath);

	// Convert file time to system clock time
	auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
		ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now()
	);

	// Convert to time_t for easy formatting
	std::time_t time = std::chrono::system_clock::to_time_t(sctp);

	// Format the time as a string
	std::ostringstream oss;
	oss << std::put_time(std::localtime(&time), "%a, %d %b %Y %H:%M:%S GMT");

	return oss.str();
}

static std::string generateETag(const std::string& content)
{
	// Use std::hash to create a hash value for the content
	std::hash<std::string> hasher;
	size_t hash = hasher(content);

	// Convert the hash to a hexadecimal string
	std::ostringstream ss;
	ss << std::hex << hash;
	return ss.str();
}

static std::string generateETagv2(const std::filesystem::path& path)
{
	struct stat fileStat;

	if (stat(path.c_str(), &fileStat) != 0)
	{
		LOG_ERROR("Failed to get file stats: {}", path.string());
		return std::string();
	}

	auto inode = fileStat.st_ino;
	auto fileSize = fileStat.st_size;
	auto fileTime = fileStat.st_mtime;
	auto permissions = fileStat.st_mode; //? unconventional but prevents a check for has read permissions before returning 304

	// Combine these into the ETag
	std::ostringstream ss;
	ss << std::hex << inode << "-" << fileSize << "-" << fileTime << "-" << permissions;
	// return ss.str();
	return "\"" + ss.str() + "\"";
}

std::string ResponseGenerator::buildHttpResponse(const std::string& body, HTTPStatusCode code, const HttpRequest& request)
{
	std::ostringstream response;

	std::string contentType = determineContentType(request.getUri());
	if (contentType.empty())
	{
		return generateForbiddenResponse();
	}

	const std::string statusCode = HTTPStatusCodeToString(code);

	WEB_ASSERT(!statusCode.empty(), "Invalid HTTP status code! (add a custom code or use a valid one)");

	std::string connection = request.getHeader("Connection");
	if (connection != "keep-alive")
	{
		// connection = "keep-alive";
		// connection = "close";
	}
	//tofix hardcoded not kept alive
	// connection = "close";
	// connection = "keep-alive";


	// response << "HTTP/1.1 200 OK\r\n"
	response << "HTTP/1.1 " << statusCode << "\r\n"
			<< "Server: Webserv/1.0\r\n"
			<< "Date: " << getCurrentDateAndTime() << "\r\n"
			<< "Content-Type: " << contentType << "\r\n"
			<< "Content-Length: " << body.size() << "\r\n"
			<< "Last-Modified: " << getFileModificationTime(request.getUri()) << "\r\n"
			//TODO: hard coded values should check this
			// << "Connection: keep-alive\r\n"
			<< "Connection: " << connection << "\r\n"
			// << "Connection: close\r\n"
			// << "ETag: \"" << generateETag(body) << "\"\r\n"
			<< "ETag: " << generateETagv2(request.getUri()) << "\r\n"
			<< "Accept-Ranges: bytes\r\n"
			// << "Cache-Control: max-age=3600\r\n"  // Cache for 1 hour
			<< "\r\n";  // End of headers

	if (!body.empty())
		response << body;

	return response.str();
}

std::string ResponseGenerator::buildHttpResponse(const std::string& body, HTTPStatusCode code, const NewHttpRequest& request)
{
	std::ostringstream response;

	std::string contentType = determineContentType(request.mappedPath);
	if (contentType.empty())
	{
		return generateForbiddenResponse();
	}

	const std::string statusCode = HTTPStatusCodeToString(code);

	WEB_ASSERT(!statusCode.empty(), "Invalid HTTP status code! (add a custom code or use a valid one)");

	std::string connection = request.getHeaderValue("Connection");
	if (connection != "keep-alive")
	{
		// connection = "keep-alive";
		// connection = "close";
	}
	//tofix hardcoded not kept alive
	// connection = "close";
	// connection = "keep-alive";


	// response << "HTTP/1.1 200 OK\r\n"
	response << "HTTP/1.1 " << statusCode << "\r\n"
			<< "Server: Webserv/1.0\r\n"
			<< "Date: " << getCurrentDateAndTime() << "\r\n"
			<< "Content-Type: " << contentType << "\r\n"
			<< "Content-Length: " << body.size() << "\r\n"
			<< "Last-Modified: " << getFileModificationTime(request.mappedPath) << "\r\n"
			//TODO: hard coded values should check this
			// << "Connection: keep-alive\r\n"
			<< "Connection: " << connection << "\r\n"
			// << "Connection: close\r\n"
			// << "ETag: \"" << generateETag(body) << "\"\r\n"
			<< "ETag: " << generateETagv2(request.mappedPath) << "\r\n"
			<< "Accept-Ranges: bytes\r\n"
			// << "Cache-Control: max-age=3600\r\n"  // Cache for 1 hour
			<< "\r\n";  // End of headers

	if (!body.empty())
		response << body;

	return response.str();
}


std::string ResponseGenerator::buildHttpResponse(ContentType type, const std::string& body, HTTPStatusCode code)
{
	std::ostringstream response;

	// response << "HTTP/1.1 200 OK\r\n"
	response << "HTTP/1.1 " << HTTPStatusCodeToString(code) << "\r\n"
			<< "Server: Webserv/1.0\r\n"
			<< "Date: " << getCurrentDateAndTime() << "\r\n"



			<< "Content-Type: " << ContentTypeToString(type) << "\r\n"
			// << "Content-Type: text/html\r\n"
			// << "Content-Type: image/x-icon\r\n"


			<< "Content-Length: " << body.size() << "\r\n"
			// << "Last-Modified: " << getFileModifigit push --set-upstream origin merged-postandmaincationTime(path) << "\r\n"
			//TODO: hard coded values should check this
			<< "Connection: keep-alive\r\n"
			// << "ETag: \"" << generateETag(body) << "\"\r\n"
			// << "Accept-Ranges: bytes\r\n"
			// Disable caching
			// << "Cache-Control: no-store, no-cache, must-revalidate, proxy-revalidate, max-age=0\r\n"
			// << "Pragma: no-cache\r\n"
			// << "Expires: 0\r\n"
			<< "\r\n";  // End of headers

	if (!body.empty())
		response << body;

	return response.str();
}

// std::string ResponseGenerator::buildHttpResponse(ContentType type, const std::string& body, HTTPStatusCode code)
// {
// 	std::ostringstream response;

// 	// response << "HTTP/1.1 200 OK\r\n"
// 	response << "HTTP/1.1 " << HTTPStatusCodeToString(code) << "\r\n"
// 			<< "Server: Webserv/1.0\r\n"
// 			<< "Date: " << getCurrentDateAndTime() << "\r\n"



// 			<< "Content-Type: " << ContentTypeToString(type) << "\r\n"
// 			// << "Content-Type: text/html\r\n"
// 			// << "Content-Type: image/x-icon\r\n"


// 			<< "Content-Length: " << body.size() << "\r\n"
// 			// << "Last-Modified: " << getFileModifigit push --set-upstream origin merged-postandmaincationTime(path) << "\r\n"
// 			//TODO: hard coded values should check this
// 			<< "Connection: keep-alive\r\n"
// 			// << "ETag: \"" << generateETag(body) << "\"\r\n"
// 			// << "Accept-Ranges: bytes\r\n"
// 			// Disable caching
// 			// << "Cache-Control: no-store, no-cache, must-revalidate, proxy-revalidate, max-age=0\r\n"
// 			// << "Pragma: no-cache\r\n"
// 			// << "Expires: 0\r\n"
// 			<< "\r\n";  // End of headers

// 	if (!body.empty())
// 		response << body;

// 	return response.str();
// }


std::string ReadImageFile(const std::filesystem::path& path)
{
	std::ifstream file(path, std::ios::binary);
	if (!file.is_open())
	{
		LOG_ERROR("Failed to open file: {}", path.string());
		return std::string();
	}
	// Read the file content into a string
	std::ostringstream oss;
	oss << file.rdbuf();
	std::string favicon_content = oss.str();
	file.close();
	return favicon_content;
}

const std::string ResponseGenerator::generateDirectoryListingResponse(const std::filesystem::path& path)
{
	std::string dirListing = generateDirectoryListing(path.string());
	std::string restult = "HTTP/1.1 200 OK\r\n";
	restult += "Content-Type: text/html\r\n";
	restult += "Content-Length: " + std::to_string(dirListing.size()) + "\r\n";
	restult += "Connection: keep-alive\r\n";
	restult += "\r\n";
	restult += dirListing;

	return restult;
}

#include "Api/Api.h"
#include "Utils/Utils.h"

/*
	check wether the requested file has been modified based on the last-modified and etag headers
*/
bool ResponseGenerator::isFileModified(const NewHttpRequest& request)
{
	// const NewHttpRequest& request = client.GetNewRequest();
	const std::string& ifNoneMatch = request.getHeaderValue("if-none-match");
	const std::string& ifModifiedSince = request.getHeaderValue("if-modified-since");
	const std::string& etag = generateETagv2(request.mappedPath);
	const std::string& lastModified = getFileModificationTime(request.mappedPath);
	if (ifNoneMatch == etag && ifModifiedSince == lastModified)
	{
		LOG_DEBUG("Resource has not been modified");
		return false;
	}
	LOG_INFO("Resource has been modified");
	LOG_INFO("If-None-Match    : {}", ifNoneMatch);
	LOG_INFO("ETag             : {}", etag);
	LOG_INFO("If-Modified-Since: {}", ifModifiedSince);
	LOG_INFO("Last-Modified    : {}", lastModified);
	return true;
}

const std::string ResponseGenerator::handleGetRequest(const Client& client)
{
	Utils::Timer timer;
	LOG_INFO("Handling GET request");



	Api api;
	api.addApiRoute("/api/v1/images");
	api.addApiRoute("/api/v1/files");

	if (api.isApiRoute(client.GetNewRequest().path.string()))
	{
		if (client.GetNewRequest().path.string() == "/api/v1/images")
		{
			std::filesystem::path databaseImagePath = std::filesystem::current_path() / "database" / "images";
			return Api::getImages(databaseImagePath);
		}
		else if (client.GetNewRequest().path.string() == "/api/v1/files")
		{
			std::filesystem::path databaseFilePath = std::filesystem::current_path() / "database" / "files";
			return Api::getFiles(databaseFilePath);
		}
	}
	//? Validate the requested path


	const std::filesystem::path& path = client.GetNewRequest().mappedPath;
	if (std::filesystem::exists(path))
	{
		//? Check if the requested path is a directory or a file
		if (std::filesystem::is_directory(path))
		{
			LOG_DEBUG("Requested path is a directory");

			const std::vector<std::string>& indexes = client.GetLocationSettings().index;
			LOG_INFO("----------- {}", client.GetLocationSettings().index[0]);
			LOG_INFO("Index: {}", indexes[0]);
			LOG_INFO("Server name: {}", client.GetServerConfig()->GetServerName());

			for (const auto& index : indexes)
			{
				LOG_INFO("Looking for: {}", index);
				std::filesystem::path indexPath = path / index;
				if (std::filesystem::exists(indexPath))
				{
					LOG_INFO("Index found: {}", indexPath.string());

					if (!isFileModified(client.GetNewRequest()))
					{
						return generateNotModifiedResponse();
					}

					// client.GetRequest().setUri(indexPath);
					// HttpRequest updatedRequest = client.GetRequest();
					// updatedRequest.setUri(indexPath);

					NewHttpRequest updatedRequest = client.GetNewRequest();
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
			LOG_DEBUG("Requested path is a file");
			const ServerSettings::LocationSettings& location = client.GetLocationSettings();

			//? check if the file is a CGI script
			if (location.cgi.size() > 0)
			{
				LOG_INFO("Requested path is a CGI script");
				for (const auto& cgi : location.cgi)
				{
					LOG_INFO("CGI: {}", cgi);
					if (path.extension() == cgi)
					{
						LOG_INFO("CGI script found: {}", path.string());
						return Cgi::executeCGI(client, client.GetNewRequest());
					}
				}
				return ResponseGenerator::generateForbiddenResponse();
			}

			// if (!isFileModified(client.GetNewRequest()))
			// {
			// 	return generateNotModifiedResponse();
			// }

			return generateFileResponse(client.GetNewRequest());
		}
		else
		{
			//TODO: send 405 response???
			LOG_ERROR("Requested path is not a file or directory");

			return generateNotFoundResponse();
		}
	}
	LOG_DEBUG("Requested path does not exist");
	return generateNotFoundResponse();
}

#include <regex>

bool isBoundaryString(const std::string &value) {
    return value.find("-----------------------------") != std::string::npos;
}

std::string extractBoundary(const std::string &contentType) {
    std::regex boundaryRegex("boundary=(.*)");
    std::smatch match;
    if (std::regex_search(contentType, match, boundaryRegex)) {
        return "--" + match[1].str();
    }
    return "";
}

std::string parseMultipartData(const std::string &body, const std::string &boundary, const std::string &fieldName) {
    std::regex fieldRegex(boundary + R"([\r\n]+Content-Disposition: form-data; name=\")" + fieldName + R"(\"[\r\n]+[\r\n]+([^\r\n]*)[\r\n]+)");
    std::smatch match;
    if (std::regex_search(body, match, fieldRegex)) {
        return match[1].str();
    }
    return "";
}

bool saveFormDataToFile(std::string &firstName, std::string lastName, std::string email) {
	if (isBoundaryString(firstName)) {
        firstName = "EMPTY";
    }
    if (isBoundaryString(lastName)) {
        lastName = "EMPTY";
    }
    if (isBoundaryString(email)) {
        email = "EMPTY";
    }

    std::string filePath = getProjectRootDir() + "/database/text.txt";
	std::ofstream file(filePath, std::ios_base::app);
	if (!file.is_open()) {
		std::cerr << "[ERROR] Failed to open file: " << filePath << std::endl;
		return false;
	}
    file << "Last Name: " << (firstName.empty() ? "EMPTY" : firstName) << "\n";
    file << "Last Name: " << (lastName.empty() ? "EMPTY" : lastName) << "\n";
    file << "Email: " << (email.empty() ? "EMPTY" : email) << "\n\n";

    file.close();
    return !file.fail();
}
bool saveUploadedFile(const std::string &body, const std::string &boundary, const std::string &fieldName, const std::string &uploadDir) {
    std::regex fileRegex(boundary + R"([\r\n]+Content-Disposition: form-data; name=\")" + fieldName + R"(\"; filename=\"([^\"]+)\")" +
                         R"([\r\n]+Content-Type: ([^\r\n]+)[\r\n]+[\r\n]+)");
    std::smatch match;
    if (std::regex_search(body, match, fileRegex)) {
        std::string fileName = match[1].str();
        std::string contentType = match[2].str();

        // Vind het begin van het bestand na de headers
        size_t fileStartPos = body.find(match[0]) + match[0].length();
        size_t fileEndPos = body.find(boundary, fileStartPos) - 4;  // -4 to remove preceding \r\n--

        std::string fileContent = body.substr(fileStartPos, fileEndPos - fileStartPos);

        if (fileName.empty()) {
            std::cerr << "[INFO] No file uploaded." << std::endl;
            return true;
        }

        std::string filePath = uploadDir + "/" + fileName;
        std::ofstream file(filePath, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "[ERROR] Failed to open file: " << filePath << std::endl;
            return false;
        }

        file.write(fileContent.c_str(), fileContent.size());
        file.close();
        if (file.fail()) {
            std::cerr << "[ERROR] Failed to write to file: " << filePath << std::endl;
            return false;
        }
        return true;
    }

    std::cerr << "[ERROR] Regex failed to match for file field: " << fieldName << std::endl;
    return false;
}


static const std::unordered_map<std::string, std::string> supportedFileTypes = {
    { ".html", "text/html" },
    { ".css", "text/css" },
    { ".ico", "image/x-icon" },
    { ".jpg", "image/jpeg" },
    { ".jpeg", "image/jpeg" },
    { ".png", "image/png" }
};


#include <algorithm>
#include <cctype>

// Function to trim whitespace from both ends of a string
std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(' ');
    size_t last = str.find_last_not_of(' ');
    return (first == std::string::npos) ? "" : str.substr(first, last - first + 1);
}

// Function to convert string to lowercase
std::string toLowerCase(const std::string& str) {
    std::string lowerStr = str;
    std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), ::tolower);
    return lowerStr;
}

bool isSupportedFileType(const std::string& contentType) {
    std::string trimmedContentType = trim(contentType);   // Trim whitespace
    std::string lowerContentType = toLowerCase(trimmedContentType); // Convert to lowercase for case-insensitive comparison

    for (const auto& [extension, mimeType] : supportedFileTypes) {
        if (lowerContentType == mimeType) {
            return true;
        }
    }
    return false;
}

std::string ResponseGenerator::parseMultipartContentType(const std::string& body, const std::string& boundary, const std::string& fieldName) {
    size_t pos = body.find("Content-Disposition: form-data; name=\"" + fieldName + "\"");
    if (pos == std::string::npos) return "";

    pos = body.find("Content-Type:", pos);
    if (pos == std::string::npos) return "";

    size_t endPos = body.find("\r\n", pos);
    std::string contentType = body.substr(pos + 13, endPos - (pos + 13)); // Extract Content-Type value
    return contentType;
}

const std::string ResponseGenerator::handlePostRequest(const Client& client)
{
    Utils::Timer timer;
    LOG_INFO("Handling POST request");

    std::string contentType = client.GetNewRequest().getHeaderValue("content-type");
    if (contentType.empty()) {
        LOG_ERROR("Missing Content-Type header");
        return generateBadRequestResponse();
    }

    std::string boundary = extractBoundary(contentType);
    if (boundary.empty()) {
        LOG_ERROR("Boundary missing in Content-Type header");
        return generateBadRequestResponse();
    }

    std::string body = client.GetNewRequest().body;
    if (body.empty()) {
        LOG_ERROR("Request body is empty");
        return generateBadRequestResponse();
    }

    // Parse form data fields
    std::string firstName = parseMultipartData(body, boundary, "firstname");
    std::string lastName = parseMultipartData(body, boundary, "lastname");
    std::string email = parseMultipartData(body, boundary, "email");

    if (!saveFormDataToFile(firstName, lastName, email)) {
        LOG_ERROR("Failed to save form data to file");
        return generateInternalServerErrorResponse();
    }

    // Parse file data and content type
    std::string fileContentType = parseMultipartContentType(body, boundary, "file");
    if (fileContentType.empty()) {
        LOG_ERROR("File content type missing");
        return generateBadRequestResponse();
    }

    // Log the detected content type for debugging
    LOG_INFO("Detected file content type: " + fileContentType);

    // Check if the file's content type matches one of the supported image types
    std::string uploadDir;
    if (isSupportedFileType(fileContentType)) {
        uploadDir = getProjectRootDir() + "/database/images";
    } else {
        uploadDir = getProjectRootDir() + "/database/files";
    }

    // Save the uploaded file
    if (!saveUploadedFile(body, boundary, "file", uploadDir)) {
        LOG_ERROR("Failed to save uploaded file");
        return generateInternalServerErrorResponse();
    }

    LOG_INFO("Successfully handled POST request, saved form data, and saved file");
    return generateOKResponse(client);
}


// const std::string ResponseGenerator::handleDeleteRequest(const Client& Client, const HttpRequest& request) 
// {
//     Utils::Timer timer;
//     LOG_INFO("Handling DELETE request");
// 	std::filesystem::path uri = request.getUri();
// 	//CURRENTLY OUR SERVER ONLY DELETES FILES FROM /DATABASE/IMAGES nowhere else. and then takes the last arg from uri
// 	//example /home/kaltevog/Desktop/Webserv(<-dynamic part)/database/images(<-hardcoded part)/webcopy.png(<-last arg of delete req uri)
//     std::string basePath = getProjectRootDir() + "/database/images";
//     std::filesystem::path fullPath = basePath / uri.filename();
//     LOG_INFO("Attempting to delete file: {}", fullPath.string());
//     if (std::filesystem::exists(fullPath) && std::filesystem::is_regular_file(fullPath))
//     {
//         try {
//             std::filesystem::remove(fullPath);
//             LOG_INFO("File deleted successfully: {}", fullPath.string());
//             return buildHttpResponse(ContentType::TEXT, "File deleted successfully", HTTPStatusCode::OK);
//         } catch (const std::filesystem::filesystem_error& e) {
//             LOG_ERROR("Failed to delete file: {}", e.what());
//             return generateInternalServerErrorResponse();
//         }
//     }
//     else
//     {
//         LOG_ERROR("Requested file does not exist or is not a regular file: {}", fullPath.string());
//         return generateNotFoundResponse();
//     }
// }

void printPermissions(std::filesystem::perms p) {
    std::cout << ((p & std::filesystem::perms::owner_read) != std::filesystem::perms::none ? "r" : "-")
              << ((p & std::filesystem::perms::owner_write) != std::filesystem::perms::none ? "w" : "-")
              << ((p & std::filesystem::perms::owner_exec) != std::filesystem::perms::none ? "x" : "-")
              << ((p & std::filesystem::perms::group_read) != std::filesystem::perms::none ? "r" : "-")
              << ((p & std::filesystem::perms::group_write) != std::filesystem::perms::none ? "w" : "-")
              << ((p & std::filesystem::perms::group_exec) != std::filesystem::perms::none ? "x" : "-")
              << ((p & std::filesystem::perms::others_read) != std::filesystem::perms::none ? "r" : "-")
              << ((p & std::filesystem::perms::others_write) != std::filesystem::perms::none ? "w" : "-")
              << ((p & std::filesystem::perms::others_exec) != std::filesystem::perms::none ? "x" : "-")
              << std::endl;
}

const std::string ResponseGenerator::handleDeleteRequest(const Client& Client)
{
	const NewHttpRequest& request = Client.GetNewRequest();

	// delete the file
	if (std::filesystem::exists(request.mappedPath) && std::filesystem::is_regular_file(request.mappedPath))
	{
		if (std::filesystem::remove(request.mappedPath))
		{
			LOG_INFO("File deleted successfully: {}", request.mappedPath.string());
			return OkResponse();
		}
		else
		{
			LOG_ERROR("Failed to delete file: {}", request.mappedPath.string());
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

//TODO: instead of sending path maybe update the path in the HrrpRequest
std::string ResponseGenerator::generateOKResponse(const Client& client)
{
	LOG_INFO("Generating 200 OK response");

	const NewHttpRequest& request = client.GetNewRequest();

	auto fileContents = readFileContents(request.mappedPath);
	if (fileContents == std::nullopt)
	{
		LOG_CRITICAL("Failed to read file contents: {}", request.mappedPath.string());
		return generateForbiddenResponse();
	}
	// return buildHttpResponse(request.getUri(), *fileContents, HTTPStatusCode::OK, request);
	return buildHttpResponse(*fileContents, HTTPStatusCode::OK, request);

	return "";
}

std::string ResponseGenerator::generateOKResponse(const std::filesystem::path& path, const NewHttpRequest& request)
{
	LOG_INFO("Generating 200 OK response");

	auto fileContents = readFileContents(path);
	if (fileContents == std::nullopt)
	{
		LOG_CRITICAL("Failed to read file contents: {}", request.mappedPath.string());
		return generateForbiddenResponse();
	}
	// return buildHttpResponse(request.getUri(), *fileContents, HTTPStatusCode::OK, request);
	return buildHttpResponse(*fileContents, HTTPStatusCode::OK, request);
}

std::string ResponseGenerator::generateForbiddenResponse()
{
	LOG_INFO("Generating 403 Forbidden response");

	const char* root = std::getenv("HTML_ROOT_DIR");
	if (root == nullptr)
	{
		WEB_ASSERT(false, "HTML_ROOT_DIR environment variable not set!");
		return nullptr;
	}

	std::filesystem::path value(root);

	auto fileContents = readFileContents(std::filesystem::path(value / "403-Forbidden.html"));
	if (fileContents == std::nullopt)
	{
		LOG_CRITICAL("Failed to read file contents: {}",  "403-Forbidden.html");
		return std::string(s_ForbiddenResponse);
	}
	return buildHttpResponse(ContentType::HTML, *fileContents, HTTPStatusCode::Forbidden);
}

std::string ResponseGenerator::generateFileResponse(const HttpRequest& request)
{
	LOG_TRACE("Generating file response");
	LOG_TRACE("Request URI: {}", request.getUri().string());
	std::string fileContents = ReadImageFile(request.getUri());
	if (fileContents.empty())
	{
		LOG_CRITICAL("Failed to read file contents: {}", request.getUri().string());
		// return generateForbiddenResponse(path);
		return generateForbiddenResponse();
	}
	// return buildHttpResponse(request.getUri(), fileContents, HTTPStatusCode::OK, request);
	return buildHttpResponse(fileContents, HTTPStatusCode::OK, request);
}

std::string ResponseGenerator::generateFileResponse(const NewHttpRequest& request)
{
	LOG_TRACE("Generating file response");
	LOG_TRACE("Request URI: {}", request.mappedPath.string());
	std::string fileContents = ReadImageFile(request.mappedPath);
	if (fileContents.empty())
	{
		LOG_CRITICAL("Failed to read file contents: {}", request.mappedPath.string());
		// return generateForbiddenResponse(path);
		return generateForbiddenResponse();
	}
	// return buildHttpResponse(request.getUri(), fileContents, HTTPStatusCode::OK, request);
	return buildHttpResponse(fileContents, HTTPStatusCode::OK, request);
}

std::string ResponseGenerator::generateNotFoundResponse()
{
	LOG_INFO("Generating 404 Not Found response");

	const char* root = std::getenv("HTML_ROOT_DIR");
	if (root == nullptr)
	{
		WEB_ASSERT(false, "HTML_ROOT_DIR environment variable not set!");
		return nullptr;
	}

	std::filesystem::path value(root);

	auto fileContents = readFileContents(std::filesystem::path(value / "404-NotFound.html"));
	if (fileContents == std::nullopt)
	{
		LOG_CRITICAL("Failed to read file contents: {}",  "404-NotFound.html");
		return std::string(s_NotFoundResponse);
	}
	return buildHttpResponse(ContentType::HTML, *fileContents, HTTPStatusCode::NotFound);
}


std::string ResponseGenerator::generateNotModifiedResponse()
{
	LOG_INFO("Generating 304 Not Modified response");

	std::ostringstream res;

	res << "HTTP/1.1 304 Not Modified\r\n";
	res << "Server: Webserv/1.0\r\n";
	res << "Date: " << getCurrentDateAndTime() << "\r\n";
	res << "Connection: keep-alive\r\n";
	res << "\r\n";  // End of headers

	return res.str();
}

std::string ResponseGenerator::generateNotImplementedResponse()
{
	LOG_INFO("Generating 404 Not Found response");

	const char* root = std::getenv("HTML_ROOT_DIR");
	if (root == nullptr)
	{
		WEB_ASSERT(false, "HTML_ROOT_DIR environment variable not set!");
		return nullptr;
	}

	std::filesystem::path value(root);

	auto fileContents = readFileContents(std::filesystem::path(value / "501-NotImplemented.html"));
	if (fileContents == std::nullopt)
	{
		LOG_CRITICAL("Failed to read file contents: {}",  "501-NotImplemented.html");
		return std::string(s_NotFoundResponse);
	}
	return buildHttpResponse(ContentType::HTML, *fileContents, HTTPStatusCode::NotFound);
}

std::string ResponseGenerator::generateBadRequestResponse()
{
	LOG_INFO("Generating 400 Bad Request response");

	const char* root = std::getenv("HTML_ROOT_DIR");
	if (root == nullptr)
	{
		WEB_ASSERT(false, "HTML_ROOT_DIR environment variable not set!");
		return nullptr;
	}

	std::filesystem::path value(root);

	auto fileContents = readFileContents(std::filesystem::path(value / "400-BadRequest.html"));
	if (fileContents == std::nullopt)
	{
		LOG_CRITICAL("Failed to read file contents: {}",  "400-BadRequest.html");
		return std::string(s_NotFoundResponse);
	}
	return buildHttpResponse(ContentType::HTML, *fileContents, HTTPStatusCode::NotFound);
}

std::string ResponseGenerator::generateInternalServerErrorResponse()
{
	LOG_INFO("Generating 500 Internal Server Error response");

	const char* root = std::getenv("HTML_ROOT_DIR");
	if (root == nullptr)
	{
		WEB_ASSERT(false, "HTML_ROOT_DIR environment variable not set!");
		return nullptr;
	}

	std::filesystem::path value(root);

	auto fileContents = readFileContents(std::filesystem::path(value / "500-InternalServerError.html"));
	if (fileContents == std::nullopt)
	{
		LOG_CRITICAL("Failed to read file contents: {}",  "500-InternalServerError.html");
		return std::string(s_NotFoundResponse);
	}
	return buildHttpResponse(ContentType::HTML, *fileContents, HTTPStatusCode::NotFound);
}













const std::string ResponseGenerator::InternalServerError(const Config& config)
{
	static std::string response = s_InternalServerErrorResponse;

	return response;
}

const std::string ResponseGenerator::OkResponse()
{
	static std::string response = s_OkResponse;

	return response;
}

const std::string ResponseGenerator::InternalServerError()
{
	static std::string response = s_InternalServerErrorResponse;

	return response;
}

const std::string ResponseGenerator::MethodNotAllowed()
{
	static std::string response = s_MethodNotAllowedResponse;

	return response;
}

const std::string ResponseGenerator::MethodNotImplemented()
{
	static std::string response = s_MethodNotImplementedResponse;

	return response;
}

const std::string ResponseGenerator::BadRequest()
{
	static std::string response = s_BadRequestResponse;

	return response;
}

const std::string ResponseGenerator::NotFound()
{
	static std::string response = s_NotFoundResponse;

	return response;
}