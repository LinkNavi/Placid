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
echo "=== Setup Complete ==="
echo ""
echo "Next steps:"
echo "  ./run.sh           # Build and run editor"
echo "  ./run.sh --server  # Build and run server"
echo ""
