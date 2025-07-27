#!/bin/bash

# ESP32 Audio Delay Project Build and Flash Script

set -e  # Exit on any error

echo "=== ESP32 Audio Delay Project Build Script ==="

# Check if ESP-IDF is properly set up
if [ -z "$IDF_PATH" ]; then
    echo "Error: ESP-IDF environment not set up. Please run:"
    echo "source \$HOME/esp/esp-idf/export.sh"
    exit 1
fi

echo "ESP-IDF Path: $IDF_PATH"

# Clean previous build (optional)
if [ "$1" = "clean" ]; then
    echo "Cleaning previous build..."
    idf.py fullclean
fi

# Configure the project
echo "Configuring project..."
idf.py set-target esp32

# Build the project
echo "Building project..."
idf.py build

# Flash if requested
if [ "$1" = "flash" ] || [ "$2" = "flash" ]; then
    echo "Flashing to device..."
    idf.py flash
    
    # Monitor if requested
    if [ "$1" = "monitor" ] || [ "$2" = "monitor" ] || [ "$3" = "monitor" ]; then
        echo "Starting monitor..."
        idf.py monitor
    fi
fi

echo "=== Build completed successfully ==="
echo ""
echo "Usage:"
echo "  ./build_and_flash.sh              - Build only"
echo "  ./build_and_flash.sh flash        - Build and flash"
echo "  ./build_and_flash.sh flash monitor - Build, flash and monitor"
echo "  ./build_and_flash.sh clean        - Clean and build"
echo "  ./build_and_flash.sh clean flash  - Clean, build and flash"
