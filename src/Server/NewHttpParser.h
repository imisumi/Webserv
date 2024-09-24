#pragma once

#include <iostream>
#include <cassert>

#include <string_view>
#include <array>
#include <stdexcept>

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <sstream>
#include <filesystem>

enum class HttpParserState
{
	Start,
	Method,
	URI,
	URI_Path,
	URI_Query,
	Version,
	HeaderName,
	HeaderValue,
	Body,
	Done,
	EndOfLine,
	BodyBegin,
	Error
};

class NewHttpRequest
{
public:
	// enum class TransferEncoding { NONE, CHUNKED };
	
	// Basic properties
	std::string method;
	std::string uri;
	std::filesystem::path path;
	std::filesystem::path mappedPath; //? Path to the file/directory on the server
	std::string query;
	std::string httpVersion;
	std::unordered_map<std::string, std::string> headers;
	std::string body;

	// For handling chunked transfer
	// TransferEncoding transferEncoding = TransferEncoding::NONE;
	std::vector<std::string> chunks;  // Store received chunks

	// State tracking
	bool isComplete = false;  // Whether the full request has been parsed
	bool isChunkedComplete = false;  // For chunked transfers

	NewHttpRequest() = default;

	int parse(const std::string& data);

	const std::string getHeaderValue(const std::string& key) const
	{
		auto it = headers.find(key);
		if (it != headers.end())
			return it->second;
		else
			return std::string();
	}

	void print()
	{
		const char green[] = "\033[32m";
		const char white[] = "\033[37m";
		const char reset[] = "\033[0m";
		std::cout << green << "Method: " << white << method << reset << std::endl;
		std::cout << green << "URI: " << white << uri << reset << std::endl;
		std::cout << green << "Path: " << white << path.string() << reset << std::endl;
		std::cout << green << "Mapped Path: " << white << mappedPath.string() << reset << std::endl;
		// std::cout << green << "Path: " << white << path << reset << std::endl;
		std::cout << green << "Query: " << white << query << reset << std::endl;
		std::cout << green << "HTTP Version: " << white << httpVersion << reset << std::endl;
		for (const auto& [key, value] : headers)
		{
			std::cout << green << key << ": " << white << value << reset << std::endl;
		}
		std::cout << green << "Body: " << white << body << reset << std::endl;
	}

private:
};