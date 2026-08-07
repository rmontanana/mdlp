#!/bin/bash

# Script to create and upload fimdlp Conan package
set -e

PACKAGE_NAME="fimdlp"
# Read from CMakeLists.txt, the same single source conanfile.py::set_version uses.
# It was hardcoded to 2.1.0 and silently went stale across two releases.
PACKAGE_VERSION="$(grep -A2 'project(fimdlp' CMakeLists.txt | sed -n 's/.*VERSION \([0-9.]*\).*/\1/p' | head -1)"
REMOTE_NAME="cimmeria"

if [ -z "$PACKAGE_VERSION" ]; then
    echo "Could not read the version from CMakeLists.txt" >&2
    exit 1
fi

echo "Creating Conan package for $PACKAGE_NAME/$PACKAGE_VERSION..."

# Create the package
conan create . --profile:build=default --profile:host=default

echo "Package created successfully!"

# Test the package
echo "Testing package..."
conan test test_package $PACKAGE_NAME/$PACKAGE_VERSION@ --profile:build=default --profile:host=default

echo "Package tested successfully!"

# Upload to Cimmeria (if remote is configured)
if conan remote list | grep -q "$REMOTE_NAME"; then
    echo "Uploading package to $REMOTE_NAME..."
    conan upload $PACKAGE_NAME/$PACKAGE_VERSION --remote=$REMOTE_NAME
    echo "Package uploaded to $REMOTE_NAME successfully!"
else
    echo "Remote '$REMOTE_NAME' not configured. To upload the package:"
    echo "1. Add the remote: conan remote add $REMOTE_NAME <cimmeria-url>"
    echo "2. Login: conan remote login $REMOTE_NAME <username>"
    echo "3. Upload: conan upload $PACKAGE_NAME/$PACKAGE_VERSION --remote=$REMOTE_NAME"
fi