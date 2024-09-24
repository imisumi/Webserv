#pragma once 

#include <unordered_map>
#include <string>
#include <functional>
#include <filesystem>
#include <unordered_set>

class Api
{
public:
	Api() {};
	~Api() {};

	bool isApiRoute(const std::string& route) const { return m_ApiRoutes.find(route) != m_ApiRoutes.end(); }

	// void addApiRoute(const std::string& route, std::function<void()> callback);
	void addApiRoute(const std::string& route) { m_ApiRoutes.insert(route); }

	// std::string handleRequest(const std::string& route);

	static std::string getImages(const std::filesystem::path& path);

private:
	// std::string getFiles();
	// std::string getImages();


private:
	std::unordered_set<std::string> m_ApiRoutes;
	// std::unordered_map<std::string, std::string> m_ApiRoutes;
	// std::unordered_map<std::string, std::function<void()>> m_ApiRoutes;
};