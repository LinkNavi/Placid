#!/bin/bash
# download_stb.sh - Download stb_image.h for PNG/JPG loading

set -e

echo "=== Downloading stb_image.h ==="

mkdir -p include/stb

if [ ! -f include/stb/stb_image.h ]; then
    echo "Downloading stb_image.h..."
    
    if command -v curl &> /dev/null; then
        curl -L https://raw.githubusercontent.com/nothings/stb/master/stb_image.h \
             -o include/stb/stb_image.h
    elif command -v wget &> /dev/null; then
        wget https://raw.githubusercontent.com/nothings/stb/master/stb_image.h \
             -O include/stb/stb_image.h
    else
        echo "ERROR: Neither curl nor wget found!"
        echo "Please download manually from:"
        echo "  https://raw.githubusercontent.com/nothings/stb/master/stb_image.h"
        echo "And save to: include/stb/stb_image.h"
        exit 1
    fi
    
    echo "✓ Downloaded stb_image.h"
else
    echo "✓ stb_image.h already exists"
fi

echo ""
echo "=== stb_image.h ready ==="
echo "Location: include/stb/stb_image.h"
echo "Supports: PNG, JPG, TGA, BMP, PSD, GIF, HDR, PIC"
echo ""
