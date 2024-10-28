#include "ResponseGenerator.h"

#include "Core/Core.h"

#include <unordered_map>
#include <sys/stat.h>


#include "Cgi/Cgi.h"


// static const std::unordered_map<uint16_t, std::string> s_RedirectResponseMap = {
// 	{301,	"HTTP/1.1 301 Moved Permanently\r\n"
// 			"Content-Length: 0\r\n"
// 			"Connection: close\r\n"},
// 	{302,	"HTTP/1.1 302 Found\r\n"
// 			"Content-Length: 0\r\n"
// 			"Connection: close\r\n"},
// 	{303, 	"HTTP/1.1 303 See Other\r\n"
// 			"Content-Length: 0\r\n"
// 			"Connection: close\r\n"},
// 	{307, 	"HTTP/1.1 307 Temporary Redirect\r\n"
// 			"Content-Length: 0\r\n"
// 			"Connection: close\r\n"},
// 	{308, 	"HTTP/1.1 308 Permanent Redirect\r\n"
// 			"Content-Length: 0\r\n"
// 			"Connection: close\r\n"},
// };

static const std::unordered_map<uint16_t, std::string> s_RedirectResponseMap = {
	{301,	"HTTP/1.1 301 Moved Permanently\r\n"
			"Content-Length: 0\r\n"
			"Connection: close\r\n"
			"Cache-Control: no-cache, no-store, must-revalidate\r\n"
			"Pragma: no-cache\r\n"
			"Expires: 0\r\n"},
	{302,	"HTTP/1.1 302 Found\r\n"
			"Content-Length: 0\r\n"
			"Connection: close\r\n"
			"Cache-Control: no-cache, no-store, must-revalidate\r\n"
			"Pragma: no-cache\r\n"
			"Expires: 0\r\n"},
	{303, 	"HTTP/1.1 303 See Other\r\n"
			"Content-Length: 0\r\n"
			"Connection: close\r\n"
			"Cache-Control: no-cache, no-store, must-revalidate\r\n"
			"Pragma: no-cache\r\n"
			"Expires: 0\r\n"},
	{307, 	"HTTP/1.1 307 Temporary Redirect\r\n"
			"Content-Length: 0\r\n"
			"Connection: close\r\n"
			"Cache-Control: no-cache, no-store, must-revalidate\r\n"
			"Pragma: no-cache\r\n"
			"Expires: 0\r\n"},
	{308, 	"HTTP/1.1 308 Permanent Redirect\r\n"
			"Content-Length: 0\r\n"
			"Connection: close\r\n"
			"Cache-Control: no-cache, no-store, must-revalidate\r\n"
			"Pragma: no-cache\r\n"
			"Expires: 0\r\n"},
};


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

// static const char s_InternalServerErrorResponse[] = 
// 		"HTTP/1.1 500 Internal Server Error\r\n"
// 		"Content-Length: 39\r\n" //? length has to match the length of the body
// 		"Connection: close\r\n"
// 		"Content-Type: text/plain\r\n"
// 		"\r\n"
// 		"500 Internal Server Error: Server Error"; //? body of the response

static const std::string s_InternalServerErrorResponse = 
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

std::string generateDirectoryListing(const std::string& path, const Client& client)
{
	// Check if the path exists
	if (!std::filesystem::exists(path)) {
		std::cerr << "Error: The directory '" << path << "' does not exist." << std::endl;
		return ResponseGenerator::GenerateErrorResponse(HTTPStatusCode::NotFound, client);
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

	return html_content;
}

//? assume that the file exists
//? return the contents of the file or nullopt if the file cannot be opened due to permissions
std::optional<std::string> ResponseGenerator::readFileContents(const std::filesystem::path& path)
{
	Log::debug("Reading file contents: {}", path.string());
	std::ifstream file(path);
	if (!file.is_open())
	{
		Log::error("Failed to open file: {}", path.string());
		return std::nullopt;
	}

	std::stringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}

std::string ResponseGenerator::determineContentType(const std::filesystem::path& file)
{
	// log file
	// Log::info("File: {}", file.string());
	std::string extension = file.extension().string();
	Log::info("Extension: {}", extension);
	auto it = s_SupportedMineTypes.find(extension);

	if (it != s_SupportedMineTypes.end())
	{
		Log::info("Content-Type: {}", it->second);
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
		Log::error("Failed to get file stats: {}", path.string());
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

std::string ResponseGenerator::buildHttpResponse(const std::string& body, HTTPStatusCode code, const HttpRequest& request, const Client& client)
{
	std::ostringstream response;

	std::string contentType = determineContentType(request.mappedPath);
	if (contentType.empty())
	{
		return GenerateErrorResponse(HTTPStatusCode::Forbidden, client);
	}

	const std::string statusCode = HTTPStatusCodeToString(code);

	WEB_ASSERT(!statusCode.empty(), "Invalid HTTP status code! (add a custom code or use a valid one)");

	std::string connection = request.getHeaderValue("Connection");
	if (connection != "keep-alive")
	{
		// connection = "keep-alive";
		connection = "close";
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



std::string ReadImageFile(const std::filesystem::path& path)
{
	std::ifstream file(path, std::ios::binary);
	if (!file.is_open())
	{
		Log::error("Failed to open file: {}", path.string());
		return std::string();
	}
	// Read the file content into a string
	std::ostringstream oss;
	oss << file.rdbuf();
	std::string favicon_content = oss.str();
	file.close();
	return favicon_content;
}

const std::string ResponseGenerator::generateDirectoryListingResponse(const std::filesystem::path& path, const Client& client)
{
	std::string dirListing = generateDirectoryListing(path.string(), client);
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
bool ResponseGenerator::isFileModified(const HttpRequest& request)
{
	// const NewHttpRequest& request = client.GetRequest();
	const std::string& ifNoneMatch = request.getHeaderValue("if-none-match");
	const std::string& ifModifiedSince = request.getHeaderValue("if-modified-since");
	const std::string& etag = generateETagv2(request.mappedPath);
	const std::string& lastModified = getFileModificationTime(request.mappedPath);
	if (ifNoneMatch == etag && ifModifiedSince == lastModified)
	{
		Log::debug("Resource has not been modified");
		return false;
	}
	Log::info("Resource has been modified");
	Log::info("If-None-Match    : {}", ifNoneMatch);
	Log::info("ETag             : {}", etag);
	Log::info("If-Modified-Since: {}", ifModifiedSince);
	Log::info("Last-Modified    : {}", lastModified);
	return true;
}


//TODO: instead of sending path maybe update the path in the HrrpRequest
std::string ResponseGenerator::generateOKResponse(const Client& client)
{
	Log::info("Generating 200 OK response");
	const HttpRequest& request = client.GetRequest();

	auto fileContents = readFileContents(request.mappedPath);
	if (fileContents == std::nullopt)
	{
		Log::critical("Failed to read file contents: {}", request.mappedPath.string());
		return GenerateErrorResponse(HTTPStatusCode::Forbidden, client);
	}
	return buildHttpResponse(*fileContents, HTTPStatusCode::OK, request, client);
}

std::string ResponseGenerator::generateOKResponse(const std::filesystem::path& path, const HttpRequest& request, const Client& client)
{
	Log::info("Generating 200 OK response");

	auto fileContents = readFileContents(path);
	if (fileContents == std::nullopt)
	{
		Log::critical("Failed to read file contents: {}", request.mappedPath.string());
		return GenerateErrorResponse(HTTPStatusCode::Forbidden, client);
	}
	return buildHttpResponse(*fileContents, HTTPStatusCode::OK, request, client);
}

std::string ResponseGenerator::generateFileResponse(const HttpRequest& request, const Client& client)
{
	Log::trace("Generating file response");
	Log::trace("Request URI: {}", request.mappedPath.string());
	std::string fileContents = ReadImageFile(request.mappedPath);
	if (fileContents.empty())
	{
		Log::critical("Failed to read file contents: {}", request.mappedPath.string());
		return GenerateErrorResponse(HTTPStatusCode::Forbidden, client);
	}
	return buildHttpResponse(fileContents, HTTPStatusCode::OK, request, client);
}

std::string ResponseGenerator::generateNotModifiedResponse()
{
	Log::info("Generating 304 Not Modified response");

	std::ostringstream res;

	res << "HTTP/1.1 304 Not Modified\r\n";
	res << "Server: Webserv/1.0\r\n";
	res << "Date: " << getCurrentDateAndTime() << "\r\n";
	res << "Connection: keep-alive\r\n";
	res << "\r\n";  // End of headers

	return res.str();
}

std::string ResponseGenerator::InternalServerError()
{
	Log::info("Generating 500 Internal Server Error response");

	// const static std::string response = s_InternalServerErrorResponse;
	return s_InternalServerErrorResponse;


	// return response;
}



std::string ResponseGenerator::GenerateRedirectResponse(const uint16_t redirectCode, const std::string& location)
{
	std::string response;
	auto	it = s_RedirectResponseMap.find(redirectCode);

	if (it == s_RedirectResponseMap.end())
		return InternalServerError();
	response = it->second + "Location: " + location + "\r\n\r\n";
	return response;
}

std::string ResponseGenerator::GenerateErrorResponse(const HTTPStatusCode code, const Client& client)
{
	auto it = client.GetLocationSettings().errorPageMap.find(static_cast<uint16_t>(code));

	if (it != client.GetLocationSettings().errorPageMap.end())
	{
		std::string	path;

		if (it->second.c_str()[0] == '/')
			path = client.GetServerConfig()->GetGlobalSettings().root.string() + it->second.string();
		else
			path = client.GetLocationSettings().root.string() + it->second.string();
		
		if (!std::filesystem::exists(path))
		{
			if (code == HTTPStatusCode::NotFound)
				return NotFound();
			return GenerateErrorResponse(HTTPStatusCode::NotFound, client);
		}

		auto fileContents = readFileContents(std::filesystem::path(path));

		if (fileContents == std::nullopt)
		{
			Log::critical("Failed to read file contents: {}",  "500-InternalServerError.html");
			return InternalServerError();
		}

		return buildHttpResponse(ContentType::HTML, *fileContents, code);
	}

	switch (code)
	{
		case HTTPStatusCode::BadRequest:		return BadRequest();
		case HTTPStatusCode::Unauthorized:		return ""; // TODO: add unauthorized response
		case HTTPStatusCode::Forbidden:			return Forbidden();
		case HTTPStatusCode::NotFound:			return NotFound();
		case HTTPStatusCode::MethodNotAllowed:	return MethodNotAllowed();
		case HTTPStatusCode::PayloadTooLarge:	return generatePayloadTooLargeResponse();
		default:								return InternalServerError();
	}

	return InternalServerError();
}


std::string ResponseGenerator::Forbidden()
{
	static std::string response = s_ForbiddenResponse;

	return response;
}

std::string ResponseGenerator::OkResponse()
{
	static std::string response = s_OkResponse;

	return response;
}

std::string ResponseGenerator::MethodNotAllowed()
{
	static std::string response = s_MethodNotAllowedResponse;

	return response;
}

std::string ResponseGenerator::MethodNotImplemented()
{
	static std::string response = s_MethodNotImplementedResponse;

	return response;
}

std::string ResponseGenerator::BadRequest()
{
	static std::string response = s_BadRequestResponse;

	return response;
}

std::string ResponseGenerator::NotFound()
{
	static std::string response = s_NotFoundResponse;

	return response;
}