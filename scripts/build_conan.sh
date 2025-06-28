#!/bin/bash

# Build script for fimdlp using Conan
set -e

echo "Building fimdlp with Conan..."

# Clean previous builds
rm -rf build_conan

# Install dependencies and build
conan install . --output-folder=build_conan --build=missing --profile:build=default --profile:host=default

# Build the project
cd build_conan
cmake .. -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build .

echo "Build completed successfully!"

# Run tests if requested
if [ "$1" = "--test" ]; then
    echo "Running tests..."
    ctest --output-on-failure
fi