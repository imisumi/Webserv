#include "NewHttpParser.h"

#include "Core/Log.h"

#define CR '\r'
#define LF '\n'

#define PROXY_CONNECTION "proxy-connection"
#define CONNECTION "connection"
#define CONTENT_LENGTH "content-length"
#define TRANSFER_ENCODING "transfer-encoding"
#define UPGRADE "upgrade"
#define CHUNKED "chunked"
#define KEEP_ALIVE "keep-alive"
#define CLOSE "close"

//TODO: change to unordered set
constexpr std::array<std::string_view, 3> supportedVersions = {
	"HTTP/1.1"
};

constexpr std::array<std::string_view, 3> supportedMethods = {
	"DELETE", "GET", "POST"
};

constexpr std::array<std::string_view, 11> singleValueHeaders = {
	"connection",
	"content-length",
	"host",
	"location",
	"content-type",
	"content-encoding",
	"content-disposition",
	"date",
	"last-modified",
	"etag",
	"content-location"
};

constexpr bool isValidVersion(std::string_view version)
{
	for (const auto& supported : supportedVersions)
	{
		if (version == supported)
			return true;
	}
	return false;
}

constexpr bool isValidMethod(std::string_view method)
{
	for (const auto& supported : supportedMethods)
	{
		if (method == supported)
			return true;
	}
	return false;
}

constexpr bool isSingleValueHeader(std::string_view header)
{
	for (const auto& single : singleValueHeaders)
	{
		if (header == single)
			return true;
	}
	return false;
}

int NewHttpRequest::parse(const std::string& data)
{
	std::string headerName;
	HttpParserState state = HttpParserState::Start;
	for (char c : data)
	{
		switch (state)
		{
			case HttpParserState::Start:
				if (std::isalpha(c))
				{
					state = HttpParserState::Method;
				}
				else
				{
					LOG_ERROR("Invalid character in request line: {}", c);
					return -1;
				}
			case HttpParserState::Method:
				if (std::isspace(c))
				{
					if (!isValidMethod(method))
					{
						LOG_ERROR("Invalid method: {}", method);
						return -1;
					}
					state = HttpParserState::URI;
				}
				else
					method += c;
				break;
			case HttpParserState::URI:
				if (c == '/')
					state = HttpParserState::URI_Path;
				else
				{
					LOG_ERROR("Invalid character in URI: {}", c);
					return -1;
				}
			case HttpParserState::URI_Path:
				if (c == '?')
					state = HttpParserState::URI_Query;
				else if (c == ' ')
					state = HttpParserState::Version;
				else if (c == CR || c == LF)
				{
					LOG_ERROR("Invalid character in URI path: {}", c);
					return -1;
				}
				else
				{
					path += c;
					uri += c;
				}
				break;
			case HttpParserState::URI_Query:
				if (c == ' ')
					state = HttpParserState::Version;
				else if (c == CR || c == LF)
					return -1;
				else
				{
					query += c;
					uri += c;
				}
				break;
			case HttpParserState::Version:
				if (c == CR)
				{
					state = HttpParserState::EndOfLine;
					if (!isValidVersion(httpVersion))
					{
						LOG_ERROR("Invalid HTTP version: {}", httpVersion);
						return -1;
					}
				}
				else if (c == LF)
				{
					LOG_ERROR("Invalid character in HTTP version: {}", c);
					return -1;
				}
				else
					httpVersion += c;
				break;
			case HttpParserState::EndOfLine:
				if (c == LF)
				{
					headerName.clear();
					state = HttpParserState::HeaderName;
				}
				else
				{
					LOG_ERROR("Invalid character in end of line: {}", c);
					return -1;
				}
				break;
			case HttpParserState::HeaderName:
				if (c == ':')
				{
					if (isSingleValueHeader(headerName))
					{
						if (headers.find(headerName) != headers.end())
						{
							LOG_ERROR("Duplicate header: {}", headerName);
							return -1;
						}
					}
					// state = HttpParserState::HeaderValue;
				}
				else if (c == ' ')
				{
					state = HttpParserState::HeaderValue;
				}
				else if (c == CR)
					state = HttpParserState::BodyBegin;
				else if (c == LF)
				{
					LOG_ERROR("Invalid character in header name: {}", c);
					return -1;
				}
				else
					headerName += std::tolower(c);
				break;
			case HttpParserState::HeaderValue:
				// if (c == ' ')
				// 	continue;
				if (c == CR)
					state = HttpParserState::EndOfLine;
				else if (c == LF)
				{
					LOG_ERROR("Invalid character in header value: {}", c);
					return -1;
				}
				else
					headers[headerName] += c;
				break;
			case HttpParserState::BodyBegin:
				if (c == LF)
					state = HttpParserState::Body;
				else
				{
					LOG_ERROR("Invalid character in body begin: {}", c);
					return -1;
				}
				break;
			case HttpParserState::Body:
				body += c;
				break;

			case HttpParserState::Error:
			{
				LOG_ERROR("Error parsing request");
				return -1;
			}
			case HttpParserState::Done:
				return 0;
		}
	}
	if (getHeaderValue("host").empty())
	{
		LOG_ERROR("Host header is missing");
		return -1;
	}
	if (state != HttpParserState::Body && state != HttpParserState::HeaderName)
	{
		LOG_ERROR("Invalid end of request");
		return -1;
	}

	path = normalizePath(path);
	mappedPath = path;
	return 0;
}

std::vector<std::string> NewHttpRequest::stringSplit(const std::string& str, char delimiter)
{
	std::vector<std::string> tokens;
	std::string token;
	
	for (char ch : str)
	{
		if (ch == delimiter)
		{
			if (!token.empty())
			{
				tokens.push_back(std::move(token));  // Move token to avoid extra copies
				token.clear();  // Clear string after moving
			}
		}
		else
		{
			token += ch;
		}
	}
	
	if (!token.empty())
	{
		tokens.push_back(std::move(token));
	}
	
	return tokens;
}


// Function to normalize a given URI
std::string NewHttpRequest::normalizePath(const std::string& uri)
{
	std::vector<std::string> parts = stringSplit(uri, '/');
	std::vector<std::string> stack;

	for (const auto& part : parts)
	{
		if (part == "." || part.empty())
		{
			// Skip current directory and empty parts (e.g., multiple slashes)
			continue;
		}
		else if (part == "..")
		{
			// Go to parent directory if not empty
			if (!stack.empty())
			{
				stack.pop_back();
			}
		}
		else
		{
			// Normal directory or file, push to the stack
			stack.push_back(part);
		}
	}

	// Reconstruct the normalized path
	std::string normalizedPath = "/";
	for (const auto& part : stack)
	{
		normalizedPath += part + "/";
	}

	// Remove trailing slash if it's not the root
	if (normalizedPath.size() > 1 && normalizedPath.back() == '/')
	{
		normalizedPath.pop_back();
	}

	return normalizedPath;
}


void test1()
{
	const char* data = "POST /index.html?query=example&sort=asc HTTP/1.1\r\n"
					"Host: localhost\r\n"
					"Connection: close\r\n";

	NewHttpRequest request;
	int result = request.parse(data);
	if (result == 0) {
		std::cout << "Test 1 Passed: Valid POST request with query parameters" << std::endl;
	} else {
		std::cout << "Test 1 Failed" << std::endl;
	}
}

void test2()
{
	const char* data = "GET /home.html HTTP/1.1\r\n"
					"Host: localhost\r\n"
					"Connection: keep-alive\r\n";

	NewHttpRequest request;
	int result = request.parse(data);
	if (result == 0) {
		std::cout << "Test 2 Passed: Valid GET request without query parameters" << std::endl;
	} else {
		std::cout << "Test 2 Failed" << std::endl;
	}
}

void test3()
{
	const char* data = "POST /submit HTTP/1.1\r\n"
					"Host: example.com\r\n"
					"User-Agent: TestAgent/1.0\r\n"
					"Connection: close\r\n";

	NewHttpRequest request;
	int result = request.parse(data);
	if (result == 0) {
		std::cout << "Test 3 Passed: Valid POST request with multiple headers" << std::endl;
	} else {
		std::cout << "Test 3 Failed" << std::endl;
	}
}

void test4()
{
	const char* data = "GET /home.html HTTP/1.1\r\n";

	NewHttpRequest request;
	int result = request.parse(data);
	if (result != 0) {
		std::cout << "Test 4 Passed: Malformed request with missing headers" << std::endl;
	} else {
		std::cout << "Test 4 Failed" << std::endl;
	}
}

void test5()
{
	const char* data = "GET /home.html HTTP/1.5\r\n"
					"Host: localhost\r\n"
					"Connection: close\r\n";

	NewHttpRequest request;
	int result = request.parse(data);
	if (result != 0) {
		std::cout << "Test 5 Passed: Malformed request with invalid HTTP version" << std::endl;
	} else {
		std::cout << "Test 5 Failed" << std::endl;
	}
}

void test6()
{
	const char* data = "POST /submit? HTTP/1.1\r\n"
					"Host: localhost\r\n"
					"Connection: close\r\n";

	NewHttpRequest request;
	int result = request.parse(data);
	if (result == 0) {
		std::cout << "Test 6 Passed: Valid POST request with empty query string" << std::endl;
	} else {
		std::cout << "Test 6 Failed" << std::endl;
	}
}


// According to RFC 7230, Section 3.2:

// "Each header field consists of a case-insensitive field name followed by a colon (:),
// optional leading whitespace, the field value, and optional trailing whitespace."
void test7()
{
	const char* data = "GET /about HTTP/1.1\r\n"
					"HOST: example.com\r\n"
					"CONNECTION: keep-alive\r\n";

	NewHttpRequest request;
	int result = request.parse(data);
	if (result == 0) {
		std::cout << "Test 7 Passed: Valid GET request with uppercase headers" << std::endl;
	} else {
		std::cout << "Test 7 Failed" << std::endl;
	}
}

// Host Header: In HTTP/1.1, the Host header is mandatory for all requests. 
// A request without it is technically non-compliant with the HTTP/1.1 standard. 
// According to RFC 7230, Section 5.4:
void test8()
{
	const char* data = "GET /home.html HTTP/1.1\r\n"
						"Connection: keep-alive\r\n";

	NewHttpRequest request;
	int result = request.parse(data);
	if (result != 0) {
		std::cout << "Test 8 Passed: Malformed request with missing headers" << std::endl;
	} else {
		std::cout << "Test 8 Failed" << std::endl;
	}
}

void test9()
{
	const char* data = "GET /home.html HTTP/1.1\r\n"
						"Host: example.com\r\n"
						"Host: example.com\r\n";

	NewHttpRequest request;
	int result = request.parse(data);
	if (result != 0) {
		std::cout << "Test 9 Passed: Malformed request with duplicate single header" << std::endl;
	} else {
		std::cout << "Test 9 Failed" << std::endl;
	}
}



// int main()
// {
// 	test1();
// 	test2();
// 	test3();
// 	test4();
// 	test5();
// 	test6();
// 	test7();
// 	test8();
// 	test9();

// 	return 0;

// 	// const char* data = "POST /index.html?query=example&sort=asc HTTP/1.1\r\n";
// 	const char* data = "POST /index.html?query=example&sort=asc HTTP/1.1\r\n"
// 						"Host: localhost\r\n"
// 						// "Connection: close\r\n"
// 						"Connection: close\r\n";
// 	// const char* data = "POST /index.html?query=example&sort=as\r\n";

// 	NewHttpRequest request;
// 	int result = request.parse(data);
// 	if (result == 0)
// 	{
// 		std::cout << "All tests passed!" << std::endl;
// 	}
// 	else
// 	{
// 		std::cout << "Tests failed!" << std::endl;
// 		return -1;
// 	}

// 	std::cout << "Method: " << request.method << std::endl;
// 	std::cout << "URI: " << request.uri << std::endl;
// 	std::cout << "Path: " << request.path << std::endl;
// 	std::cout << "Query: " << request.query << std::endl;
// 	std::cout << "HTTP Version: " << request.httpVersion << std::endl;
// 	for (const auto& [key, value] : request.headers)
// 	{
// 		std::cout << key << ": " << value << std::endl;
// 	}


// 	// std::cout << "All tests passed!" << std::endl;
// 	return 0;
// }