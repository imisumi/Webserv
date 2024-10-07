#!/bin/bash

# Create the build directory if it doesn't exist
mkdir -p build

# Change to the build directory
cd build

# Run CMake with Debug configuration
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..

# Build the project
make -j8