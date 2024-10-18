#include <iostream>
#include <regex>
#include <string>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

// Function to extract the script name from the URI
std::string extract_script_name(const std::string& uri) {
    std::regex pattern(R"((/[^/]+\.[^/]+))"); // Regex to match a file with an extension
    std::smatch matches;
    if (std::regex_search(uri, matches, pattern)) {
        return matches[1].str(); // Return the captured script name (first group)
    }
    return "";
}

// Function to check if the file has one of the allowed extensions
bool has_allowed_extension(const fs::path& file, const std::vector<std::string>& allowed_extensions) {
    std::string extension = file.extension().string();
    for (const auto& ext : allowed_extensions) {
        if (extension == ext) {
            return true;
        }
    }
    return false;
}

int main() {
    std::string uri = "/cgi-bin/temp/env.cgi/test/test2/cheese";
    std::string script_name = extract_script_name(uri);

    if (script_name.empty()) {
        std::cout << "No script name found in the URI." << std::endl;
        return 1;
    }

    std::cout << "Extracted script name: " << script_name << std::endl;

    // Define the CGI-bin path (you can adjust this path as needed)
    fs::path cgi_bin = "/home/imisumi/Desktop/Webserv/root/webserv/cgi-bin";

    // Construct the full path to the script within the CGI-bin directory
    fs::path script_path = cgi_bin / script_name;

    // Define allowed extensions
    std::vector<std::string> allowed_extensions = {".cgi", ".py"};

    // Check if the script exists, has an allowed extension, and is executable
    if (fs::exists(script_path)) {
        if (has_allowed_extension(script_path, allowed_extensions)) {
            std::cout << "Script path: " << script_path << std::endl;
            std::cout << "Parent path: " << script_path.parent_path() << std::endl;
            std::cout << "Filename: " << script_path.filename() << std::endl;

            // Check if the file is executable
            // if (fs::status(script_path).permissions()) {
            if (fs::status(script_path).permissions() & fs::perms::owner_exec) {
                std::cout << "The script is executable." << std::endl;
            } else {
                std::cout << "The script is not executable." << std::endl;
            }
        } else {
            std::cout << "The script does not have an allowed extension." << std::endl;
        }
    } else {
        std::cout << "The script does not exist at the specified path." << std::endl;
    }

    return 0;
}
