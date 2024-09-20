// #include <iostream>
// #include <cassert>

// #include <string_view>
// #include <array>
// #include <stdexcept>

// #include <string>
// #include <unordered_map>
// #include <vector>
// #include <memory>
// #include <sstream>

// #define CR '\r'
// #define LF '\n'

// #define PROXY_CONNECTION "proxy-connection"
// #define CONNECTION "connection"
// #define CONTENT_LENGTH "content-length"
// #define TRANSFER_ENCODING "transfer-encoding"
// #define UPGRADE "upgrade"
// #define CHUNKED "chunked"
// #define KEEP_ALIVE "keep-alive"
// #define CLOSE "close"

// //TODO: change to unordered set
// constexpr std::array<std::string_view, 3> supportedVersions = {
// 	"HTTP/1.1"
// };

// constexpr std::array<std::string_view, 3> supportedMethods = {
// 	"DELETE", "GET", "POST"
// };

// constexpr std::array<std::string_view, 11> singleValueHeaders = {
// 	"connection",
// 	"content-length",
// 	"host",
// 	"location",
// 	"content-type",
// 	"content-encoding",
// 	"content-disposition",
// 	"date",
// 	"last-modified",
// 	"etag",
// 	"content-location"
// };

// constexpr bool isValidVersion(std::string_view version)
// {
// 	for (const auto& supported : supportedVersions)
// 	{
// 		if (version == supported)
// 			return true;
// 	}
// 	return false;
// }

// constexpr bool isValidMethod(std::string_view method)
// {
// 	for (const auto& supported : supportedMethods)
// 	{
// 		if (method == supported)
// 			return true;
// 	}
// 	return false;
// }

// constexpr bool isSingleValueHeader(std::string_view header)
// {
// 	for (const auto& single : singleValueHeaders)
// 	{
// 		if (header == single)
// 			return true;
// 	}
// 	return false;
// }

// enum class ParserState
// {
// 	Start,
// 	Method,
// 	URI,
// 	URI_Path,
// 	URI_Query,
// 	Version,
// 	HeaderName,
// 	HeaderValue,
// 	Body,
// 	Done,
// 	EndOfLine,
// 	BodyBegin,
// 	Error
// };

// class HttpRequest
// {
// public:
// 	// enum class TransferEncoding { NONE, CHUNKED };
	
// 	// Basic properties
// 	std::string method;
// 	std::string uri;
// 	std::string path;
// 	std::string query;
// 	std::string httpVersion;
// 	std::unordered_map<std::string, std::string> headers;
// 	std::string body;

// 	// For handling chunked transfer
// 	// TransferEncoding transferEncoding = TransferEncoding::NONE;
// 	std::vector<std::string> chunks;  // Store received chunks

// 	// State tracking
// 	bool isComplete = false;  // Whether the full request has been parsed
// 	bool isChunkedComplete = false;  // For chunked transfers

// 	HttpRequest() = default;

// 	int parse(const std::string& data);

// 	const std::string getHeaderValue(const std::string& key) const
// 	{
// 		auto it = headers.find(key);
// 		if (it != headers.end())
// 			return it->second;
// 		else
// 			return std::string();
// 	}

// 	void print()
// 	{
// 		std::cout << "Method: " << method << std::endl;
// 		std::cout << "URI: " << uri << std::endl;
// 		std::cout << "Path: " << path << std::endl;
// 		std::cout << "Query: " << query << std::endl;
// 		std::cout << "HTTP Version: " << httpVersion << std::endl;
// 		for (const auto& [key, value] : headers)
// 		{
// 			std::cout << key << ": " << value << std::endl;
// 		}
// 	}

// private:
// };

// int HttpRequest::parse(const std::string& data)
// {
// 	std::string headerName;
// 	ParserState state = ParserState::Start;
// 	for (char c : data)
// 	{
// 		switch (state)
// 		{
// 			case ParserState::Start:
// 				if (std::isalpha(c))
// 				{
// 					state = ParserState::Method;
// 				}
// 				else
// 					return -1;
// 			case ParserState::Method:
// 				if (std::isspace(c))
// 				{
// 					if (!isValidMethod(method))
// 						return -1;
// 					state = ParserState::URI;
// 				}
// 				else
// 					method += c;
// 				break;
// 			case ParserState::URI:
// 				if (c == '/')
// 					state = ParserState::URI_Path;
// 				else
// 				{
// 					return -1;
// 				}
// 			case ParserState::URI_Path:
// 				if (c == '?')
// 					state = ParserState::URI_Query;
// 				else if (c == ' ')
// 					state = ParserState::Version;
// 				else if (c == CR || c == LF)
// 					return -1;
// 				else
// 				{
// 					path += c;
// 					uri += c;
// 				}
// 				break;
// 			case ParserState::URI_Query:
// 				if (c == ' ')
// 					state = ParserState::Version;
// 				else if (c == CR || c == LF)
// 					return -1;
// 				else
// 				{
// 					query += c;
// 					uri += c;
// 				}
// 				break;
// 			case ParserState::Version:
// 				if (c == CR)
// 				{
// 					state = ParserState::EndOfLine;
// 					if (!isValidVersion(httpVersion))
// 						return -1;
// 				}
// 				else if (c == LF)
// 					return -1;
// 				else
// 					httpVersion += c;
// 				break;
// 			case ParserState::EndOfLine:
// 				if (c == LF)
// 				{
// 					headerName.clear();
// 					state = ParserState::HeaderName;
// 				}
// 				else
// 					return -1;
// 				break;
// 			case ParserState::HeaderName:
// 				if (c == ':')
// 				{
// 					if (isSingleValueHeader(headerName))
// 					{
// 						if (headers.find(headerName) != headers.end())
// 							return -1;
// 					}
// 					state = ParserState::HeaderValue;
// 				}
// 				else if (c == CR)
// 					state = ParserState::BodyBegin;
// 				else if (c == LF)
// 					return -1;
// 				else
// 					headerName += std::tolower(c);
// 				break;
// 			case ParserState::HeaderValue:
// 				if (c == ' ')
// 					continue;
// 				else if (c == CR)
// 					state = ParserState::EndOfLine;
// 				else if (c == LF)
// 					return -1;
// 				else
// 					headers[headerName] += c;
// 				break;
// 			case ParserState::BodyBegin:
// 				if (c == LF)
// 					state = ParserState::Body;
// 				else
// 					return -1;
// 				break;
// 			case ParserState::Body:
// 				body += c;
// 				break;
// 		}
// 	}
// 	if (getHeaderValue("host").empty())
// 		return -1;
// 	if (state == ParserState::Body || state == ParserState::HeaderName)
// 		return 0;
// 	else
// 		return -1;
// 	return 0;
// }



// void test1()
// {
// 	const char* data = "POST /index.html?query=example&sort=asc HTTP/1.1\r\n"
// 					"Host: localhost\r\n"
// 					"Connection: close\r\n";

// 	HttpRequest request;
// 	int result = request.parse(data);
// 	if (result == 0) {
// 		std::cout << "Test 1 Passed: Valid POST request with query parameters" << std::endl;
// 	} else {
// 		std::cout << "Test 1 Failed" << std::endl;
// 	}
// }

// void test2()
// {
// 	const char* data = "GET /home.html HTTP/1.1\r\n"
// 					"Host: localhost\r\n"
// 					"Connection: keep-alive\r\n";

// 	HttpRequest request;
// 	int result = request.parse(data);
// 	if (result == 0) {
// 		std::cout << "Test 2 Passed: Valid GET request without query parameters" << std::endl;
// 	} else {
// 		std::cout << "Test 2 Failed" << std::endl;
// 	}
// }

// void test3()
// {
// 	const char* data = "POST /submit HTTP/1.1\r\n"
// 					"Host: example.com\r\n"
// 					"User-Agent: TestAgent/1.0\r\n"
// 					"Connection: close\r\n";

// 	HttpRequest request;
// 	int result = request.parse(data);
// 	if (result == 0) {
// 		std::cout << "Test 3 Passed: Valid POST request with multiple headers" << std::endl;
// 	} else {
// 		std::cout << "Test 3 Failed" << std::endl;
// 	}
// }

// void test4()
// {
// 	const char* data = "GET /home.html HTTP/1.1\r\n";

// 	HttpRequest request;
// 	int result = request.parse(data);
// 	if (result != 0) {
// 		std::cout << "Test 4 Passed: Malformed request with missing headers" << std::endl;
// 	} else {
// 		std::cout << "Test 4 Failed" << std::endl;
// 	}
// }

// void test5()
// {
// 	const char* data = "GET /home.html HTTP/1.5\r\n"
// 					"Host: localhost\r\n"
// 					"Connection: close\r\n";

// 	HttpRequest request;
// 	int result = request.parse(data);
// 	if (result != 0) {
// 		std::cout << "Test 5 Passed: Malformed request with invalid HTTP version" << std::endl;
// 	} else {
// 		std::cout << "Test 5 Failed" << std::endl;
// 	}
// }

// void test6()
// {
// 	const char* data = "POST /submit? HTTP/1.1\r\n"
// 					"Host: localhost\r\n"
// 					"Connection: close\r\n";

// 	HttpRequest request;
// 	int result = request.parse(data);
// 	if (result == 0) {
// 		std::cout << "Test 6 Passed: Valid POST request with empty query string" << std::endl;
// 	} else {
// 		std::cout << "Test 6 Failed" << std::endl;
// 	}
// }


// // According to RFC 7230, Section 3.2:

// // "Each header field consists of a case-insensitive field name followed by a colon (:),
// // optional leading whitespace, the field value, and optional trailing whitespace."
// void test7()
// {
// 	const char* data = "GET /about HTTP/1.1\r\n"
// 					"HOST: example.com\r\n"
// 					"CONNECTION: keep-alive\r\n";

// 	HttpRequest request;
// 	int result = request.parse(data);
// 	if (result == 0) {
// 		std::cout << "Test 7 Passed: Valid GET request with uppercase headers" << std::endl;
// 	} else {
// 		std::cout << "Test 7 Failed" << std::endl;
// 	}
// }

// // Host Header: In HTTP/1.1, the Host header is mandatory for all requests. 
// // A request without it is technically non-compliant with the HTTP/1.1 standard. 
// // According to RFC 7230, Section 5.4:
// void test8()
// {
// 	const char* data = "GET /home.html HTTP/1.1\r\n"
// 						"Connection: keep-alive\r\n";

// 	HttpRequest request;
// 	int result = request.parse(data);
// 	if (result != 0) {
// 		std::cout << "Test 8 Passed: Malformed request with missing headers" << std::endl;
// 	} else {
// 		std::cout << "Test 8 Failed" << std::endl;
// 	}
// }

// void test9()
// {
// 	const char* data = "GET /home.html HTTP/1.1\r\n"
// 						"Host: example.com\r\n"
// 						"Host: example.com\r\n";

// 	HttpRequest request;
// 	int result = request.parse(data);
// 	if (result != 0) {
// 		std::cout << "Test 9 Passed: Malformed request with duplicate single header" << std::endl;
// 	} else {
// 		std::cout << "Test 9 Failed" << std::endl;
// 	}
// }



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

// 	HttpRequest request;
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