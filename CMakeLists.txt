cmake_minimum_required(VERSION 3.16)  # Updated to 3.16 to use PCH
project(Webserv LANGUAGES CXX)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Set the output directory
set(FullOutputDir ${CMAKE_SOURCE_DIR}/bin/${CMAKE_SYSTEM_NAME})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${FullOutputDir})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${FullOutputDir})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${FullOutputDir})

# Find all the source files
file(GLOB_RECURSE ALL_SOURCES 
"${CMAKE_SOURCE_DIR}/src/*.cpp"
"${CMAKE_SOURCE_DIR}/src/**/*.cpp"
"${CMAKE_SOURCE_DIR}/src/**/**/*.cpp"
)

# Add the spdlog submodule to the project

# Create the executable
add_executable(${PROJECT_NAME} ${ALL_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Precompiled headers: Add pch.hpp
target_precompile_headers(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src/pch.h)
