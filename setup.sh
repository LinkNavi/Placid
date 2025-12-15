#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "=== Placid Setup Script ==="
echo ""

# Check for ImGui
IMGUI_LOCATIONS=(
    "include/imgui"
    "external/imgui"
    "imgui"
)

IMGUI_FOUND=false
for loc in "${IMGUI_LOCATIONS[@]}"; do
    if [ -f "$loc/imgui.h" ]; then
        echo "✓ ImGui found at $loc"
        IMGUI_FOUND=true
        break
    fi
done

if [ "$IMGUI_FOUND" = false ]; then
    echo "✗ ImGui not found"
    echo ""
    echo "Would you like to download ImGui? (y/n)"
    read -r response
    
    if [[ "$response" =~ ^[Yy]$ ]]; then
        echo "Downloading ImGui..."
        mkdir -p include
        cd include
        
        if command -v git &> /dev/null; then
            git clone --depth 1 https://github.com/ocornut/imgui.git
            echo "✓ ImGui downloaded to include/imgui"
        else
            echo "✗ git not found. Please install git or download ImGui manually:"
            echo "  https://github.com/ocornut/imgui"
            echo "  Extract to: include/imgui/"
            exit 1
        fi
        
        cd "$SCRIPT_DIR"
    else
        echo ""
        echo "Continuing without ImGui. Editor UI will not be available."
        echo "To add ImGui later:"
        echo "  cd include"
        echo "  git clone https://github.com/ocornut/imgui.git"
    fi
fi

echo ""

# Check for stb_image.h
if [ ! -f "include/stb/stb_image.h" ]; then
    echo "✗ stb_image.h not found"
    echo ""
    echo "Would you like to download stb_image.h? (y/n)"
    echo "(Required for PNG/JPG texture loading)"
    read -r response
    
    if [[ "$response" =~ ^[Yy]$ ]]; then
        echo "Downloading stb_image.h..."
        mkdir -p include/stb
        
        if command -v curl &> /dev/null; then
            curl -L https://raw.githubusercontent.com/nothings/stb/master/stb_image.h \
                 -o include/stb/stb_image.h
            echo "✓ stb_image.h downloaded"
        elif command -v wget &> /dev/null; then
            wget https://raw.githubusercontent.com/nothings/stb/master/stb_image.h \
                 -O include/stb/stb_image.h
            echo "✓ stb_image.h downloaded"
        else
            echo "✗ Neither curl nor wget found!"
            echo "Please download manually from:"
            echo "  https://raw.githubusercontent.com/nothings/stb/master/stb_image.h"
            echo "And save to: include/stb/stb_image.h"
        fi
    else
        echo ""
        echo "Skipping stb_image.h. Texture loading will not be available."
        echo "To add later, download from:"
        echo "  https://github.com/nothings/stb/blob/master/stb_image.h"
    fi
else
    echo "✓ stb_image.h found"
fi

echo ""
echo "=== Setup Complete ==="
echo ""
echo "Next steps:"
echo "  ./build.sh         # Build the project"
echo "  ./run.sh           # Build and run"
echo ""
echo "Available executables after building:"
echo "  ./build/bin/editor    # Standalone map editor"
echo "  ./build/bin/placid    # Full game with multiplayer"
echo ""
