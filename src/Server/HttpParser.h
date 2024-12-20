#pragma once

#include <cassert>
#include <iostream>

#include <array>
#include <stdexcept>
#include <string_view>

#include <filesystem>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "Constants.h"

enum class HttpState : int
{
	Error = -1,
	Start = 0,
	Method,
	URI,
	URI_Path,
	URI_Query,
	Version,
	HeaderName,
	HeaderValue,
	EndOfLine,
	BodyBegin,
	Body,
	ChunkSize,
	ChunkSizeCR,
	ChunkData,
	ChunkDataCR,
	ChunkDataLF,
	LastChunkCR,
	LastChunkLF,
	Done,
};

class HttpRequest
{
public:
	enum class TransferEncoding
	{
		NONE,
		CHUNKED
	};

	// Basic properties
	std::string method;
	std::string uri;
	std::filesystem::path path;
	std::filesystem::path pathInfo;
	std::filesystem::path mappedPath;  //? Path to the file/directory on the server
	std::string query;
	std::string httpVersion;
	std::unordered_map<std::string, std::string> headers;
	std::string body;

	// For handling chunked transfer
	TransferEncoding transferEncoding = TransferEncoding::NONE;
	std::vector<std::string> chunks;  // Store received chunks

	// State tracking
	bool isComplete = false;		 // Whether the full request has been parsed
	bool isChunkedComplete = false;	 // For chunked transfers

	HttpRequest() = default;

	// int parse(const std::string& data);
	HttpState parseStream(const std::string& data);
	// HttpState parseStream(const std::array<char, BUFFER_SIZE>& data);

	const std::string getHeaderValue(const std::string& _key) const
	{
		for (const auto& [key, value] : headers)
		{
			if (key == _key)
			{
				return value;
			}
		}
		return std::string();
	}

	HttpState GetState() const { return m_State; }

	void print()
	{
		const char green[] = "\033[32m";
		const char white[] = "\033[37m";
		const char reset[] = "\033[0m";
		std::cout << green << "Method: " << white << method << reset << std::endl;
		std::cout << green << "URI: " << white << uri << reset << std::endl;
		std::cout << green << "Path: " << white << path.string() << reset << std::endl;
		std::cout << green << "Path Info: " << white << pathInfo.string() << reset << std::endl;
		std::cout << green << "Mapped Path: " << white << mappedPath.string() << reset << std::endl;
		// std::cout << green << "Path: " << white << path << reset << std::endl;
		std::cout << green << "Query: " << white << query << reset << std::endl;
		std::cout << green << "HTTP Version: " << white << httpVersion << reset << std::endl;
		for (const auto& [key, value] : headers)
		{
			std::cout << green << key << ": " << white << value << reset << std::endl;
		}
		if (getHeaderValue("content-type") == "text/plain")
			std::cout << green << "Body: " << white << body << reset << std::endl;
	}

private:
	std::vector<std::string> stringSplit(const std::string& str, char delimiter);
	std::string normalizePath(const std::string& uri);
	HttpState m_State = HttpState::Start;
	std::string m_CurrentHeaderName;

	// For chunked transfer encoding
	std::string chunkSizeStr;
	int currentChunkSize = 0;
	int bytesRemaining = 0;
};