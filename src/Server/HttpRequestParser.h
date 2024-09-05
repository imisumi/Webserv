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

#define BIT(n) (1 << (n))

enum class RequestMethod
{
	GET      = BIT(0),
	POST     = BIT(1),
	PUT      = BIT(2),
	PATCH    = BIT(3),
	DELETE   = BIT(4),
	HEAD     = BIT(5),
	OPTIONS  = BIT(6),

	UNKNOWN = -1
};

class HttpRequest
{
public:
    // Method to get a specific header value
    std::string getHeader(const std::string& name) const
    {
        auto it = headers.find(name);
        if (it != headers.end())
        {
            return it->second;
        }
        return "";
    }

    // Method to set the URI
    void setUri(const std::filesystem::path& path)
    {
        uri = path;
    }

    // Method to get the URI
    std::filesystem::path getUri() const
    {
        return uri;
    }

    // Method to get the body content
    std::string getBody() const
    {
        return body;
    }
public:
	RequestMethod method = RequestMethod::UNKNOWN;
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
