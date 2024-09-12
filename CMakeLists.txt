# cmake_minimum_required(VERSION 3.10)
# project(Webserv LANGUAGES CXX)

# # Set the C++ standard
# set(CMAKE_CXX_STANDARD 20)
# set(CMAKE_CXX_STANDARD_REQUIRED True)

# # Set the output directory
# set(FullOutputDir ${CMAKE_SOURCE_DIR}/bin/${CMAKE_SYSTEM_NAME})
# set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${FullOutputDir})
# set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${FullOutputDir})
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${FullOutputDir})

# # Find all the source files
# file(GLOB_RECURSE ALL_SOURCES 
# "${CMAKE_SOURCE_DIR}/src/*.cpp"
# "${CMAKE_SOURCE_DIR}/src/**/*.cpp"
# "${CMAKE_SOURCE_DIR}/src/**/**/*.cpp"
# )

# # Add the spdlog submodule to the project
# add_subdirectory(${CMAKE_SOURCE_DIR}/dep/spdlog)

# # Create the executable
# add_executable(${PROJECT_NAME} ${ALL_SOURCES})

# # Include the spdlog headers
# target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/dep/spdlog/include)
# target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src)

# # Link the spdlog library
# target_link_libraries(${PROJECT_NAME} PRIVATE spdlog::spdlog)








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
add_subdirectory(${CMAKE_SOURCE_DIR}/dep/spdlog)

# Create the executable
add_executable(${PROJECT_NAME} ${ALL_SOURCES})

# Include the spdlog headers
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/dep/spdlog/include)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Link the spdlog library
target_link_libraries(${PROJECT_NAME} PRIVATE spdlog::spdlog)

# Precompiled headers: Add pch.hpp
target_precompile_headers(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src/pch.h)
