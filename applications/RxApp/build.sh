#!/bin/bash

echo "Building RxApp..."

# Clean previous build
echo "Cleaning previous build..."
rm -rf build

# Setup build directory
echo "Setting up build directory..."
meson setup build

if [ $? -ne 0 ]; then
    echo "Meson setup failed"
    exit 1
fi

# Compile
echo "Compiling RxApp..."
ninja -C build

if [ $? -eq 0 ]; then
    echo ""
    echo "Build successful!"
    echo "Run with: ./build/RxApp --help"
    echo ""
else
    echo "Build failed"
    exit 1
fi