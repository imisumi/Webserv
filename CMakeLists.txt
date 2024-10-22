cmake_minimum_required(VERSION 3.16)  # Updated to 3.16 to use PCH
project(Webserv LANGUAGES CXX)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DWEBSERV_DEBUG)  # Define DEBUG_BUILD for Debug mode
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_definitions(-DWEBSERV_RELEASE)  # Define RELEASE_BUILD for Release mode
endif()

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

list(REMOVE_ITEM ALL_SOURCES "${CMAKE_SOURCE_DIR}/src/Config/test.cpp")

# Create the executable
add_executable(${PROJECT_NAME} ${ALL_SOURCES})

# Add include directories
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Add the pedantic flag
# target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wextra -pedantic)
# target_compile_options(${PROJECT_NAME} PRIVATE 
#     -Wall 
#     -Wextra 
# 	-Wpedantic
# 	-Wextra
#     -pedantic 
#     -Werror 
#     -Wshadow 
#     -Wconversion 
#     -Wsign-conversion 
#     -Wuninitialized 
#     -Wfloat-equal 
#     -Wdeprecated 
#     -std=c++20
# )
# target_compile_options(${PROJECT_NAME} PRIVATE -pedantic)

# Precompiled headers: Add pch.hpp
# target_precompile_headers(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src/pch.h)
