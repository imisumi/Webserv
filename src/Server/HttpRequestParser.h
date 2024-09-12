#pragma once


#include <string>
#include <sstream>
#include <cctype>
#include <unordered_map>
#include <filesystem>

enum class ParserState
{
	Start,
	Method,
	URI,
	Version,
	HeaderName,
	HeaderValue,
	Body,
	Done,
	Error
};

#define BIT(n) (1u << (n))

enum class RequestMethod : uint8_t
{
	GET      = BIT(0),
	POST     = BIT(1),
	PUT      = BIT(2),
	PATCH    = BIT(3),
	DELETE   = BIT(4),
	HEAD     = BIT(5),
	OPTIONS  = BIT(6),

	UNKNOWN = 0
};

class HttpRequest
{
public:
	std::string getHeader(const std::string& name) const
	{
		auto it = headers.find(name);
		if (it != headers.end())
		{
			return it->second;
		}
		return "";
	}

	// void setUri(const std::filesystem::path& path) const
	// {
	// 	uri = path;
	// }

	void setUri(const std::filesystem::path& path)
	{
		uri = path;
	}

	std::filesystem::path getUri() const
	{
		return uri;
	}

	std::filesystem::path getOriginalUri() const
	{
		return original_uri;
	}

public:
	RequestMethod method = RequestMethod::UNKNOWN;
	std::filesystem::path original_uri;
	std::filesystem::path uri;
	std::string version;
	std::unordered_map<std::string, std::string> headers;
	std::string body;
};

class HttpRequestParser {
public:
	bool parse(const std::string& data);

	const HttpRequest& getRequest() const { return request; }


	void reset() {
		state = ParserState::Start;
		request = {};
		currentHeaderName.clear();
	}

private:
	bool setError()
	{
		state = ParserState::Error;
		return false;
	}

	bool finalize()
	{
		if (state == ParserState::Body || state == ParserState::HeaderName) {
			state = ParserState::Done;
			return true;
		}
		return setError();
	}

	ParserState state = ParserState::Start;
	HttpRequest request;
	std::string currentHeaderName;
};
