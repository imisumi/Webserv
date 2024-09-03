#include "HttpRequestParser.h"





static RequestMethod GetRequestMethodFromString(std::string_view request)
{
	if (request == "GET")			return RequestMethod::GET;
	if (request == "POST")			return RequestMethod::POST;
	if (request == "PUT")			return RequestMethod::PUT;
	if (request == "PATCH")			return RequestMethod::PATCH;
	if (request == "DELETE")		return RequestMethod::DELETE;
	if (request == "HEAD")			return RequestMethod::HEAD;
	if (request == "OPTIONS")		return RequestMethod::OPTIONS;
	return RequestMethod::UNKNOWN;
}

bool HttpRequestParser::parse(const std::string& data)
{
	auto it = data.begin();
	std::string requestMethod;
	while (it != data.end())
	{
		char ch = *it;
		switch (state)
		{
			case ParserState::Start:
				if (std::isalpha(ch))
				{
					state = ParserState::Method;
					// request.method += ch;
					requestMethod += ch;
				}
				else
					return setError();
				break;

			case ParserState::Method:
				if (std::isspace(ch))
				{
					state = ParserState::URI;
					request.method = GetRequestMethodFromString(requestMethod);
				}
				else
					requestMethod += ch;
					// request.method += ch;
				break;

			case ParserState::URI:
				if (std::isspace(ch))
					state = ParserState::Version;
				else
					request.uri += ch;
				break;

			case ParserState::Version:
				if (ch == '\r') {
					// Expecting the end of the line
				} else if (ch == '\n') {
					state = ParserState::HeaderName;
				} else {
					request.version += ch;
				}
				break;

			case ParserState::HeaderName:
				if (ch == ':') {
					state = ParserState::HeaderValue;
					it += 1; // Skip the space after the colon
				} else if (ch == '\r') {
					// End of headers
					state = ParserState::Body;
				} else
					currentHeaderName += ch;
				break;

			// case ParserState::HeaderValue:
			// 	if (ch == '\n')
			// 	{
			// 		state = ParserState::HeaderName;
			// 		currentHeaderName.clear();
			// 	} else if (ch == '\r') {
			// 		// Ignore CR
			// 	} else {
			// 		request.headers[currentHeaderName] += ch;
			// 	}
			// 	break;
			case ParserState::HeaderValue:
				if (ch == '\n')
				{
					state = ParserState::HeaderName;
					currentHeaderName.clear();
				} else if (ch == '\r') {
					// Ignore CR
				} else {
					request.headers[currentHeaderName] += ch;
				}
				break;

			case ParserState::Body:	request.body += ch;		break;
			case ParserState::Done:							return true;
			default:										return setError();
		}
		++it;
	}
	return finalize();
}