#!/bin/bash

# Remove the build directory if it exists to ensure a clean build
rm -rf build

# Create the build directory
mkdir -p build

# Change to the build directory
cd build

# Run CMake with Debug configuration
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..

# Build the project
make -j8
