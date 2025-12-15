#!/bin/bash

# Placid build script

set -e

echo "=== Building Placid ==="

# Create build directory
mkdir -p build
cd build

# Configure with CMake
echo "Configuring..."
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
echo "Building..."
cmake --build . -j$(nproc)

echo ""
echo "=== Build Complete ==="
echo ""
echo "Executables:"
echo "  ./build/placid         - Main game"
echo "  ./build/editor         - Map editor"
echo "  ./build/test_network   - Network test"
echo ""
