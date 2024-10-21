#pragma once

#include <filesystem>
#include <functional>
#include <string>
#include <unordered_map>

// Define the handler type
using RouteHandler = std::function<std::string()>;

// API class with a route map
class Api
{
public:
	// Add route with a handler
	void addApiRoute(const std::string& route, RouteHandler handler) { m_ApiRoutes[route] = handler; }
	bool isApiRoute(const std::string& route) const { return m_ApiRoutes.find(route) != m_ApiRoutes.end(); }

	// Check if a route exists and call its handler
	std::string handleRoute(const std::string& path)
	{
		if (m_ApiRoutes.find(path) != m_ApiRoutes.end())
		{
			return m_ApiRoutes[path]();	 // Call the route handler and return the result
		}
		return "HTTP/1.1 404 Not Found\r\n"
			   "Content-Type: text/plain\r\n"
			   "Content-Length: 21\r\n"
			   "\r\n"
			   "404 - Route not found";
	}

	static std::string getImages(const std::filesystem::path& path);
	static std::string getFiles(const std::filesystem::path& path);

private:
	std::unordered_map<std::string, RouteHandler> m_ApiRoutes;	// Route map
};