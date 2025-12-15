#!/bin/bash
# find_deps.sh - Locate GLAD and ImGui

echo "=== Searching for dependencies ==="
echo ""

echo "Looking for GLAD..."
GLAD_LOCATIONS=(
    "include/glad/src/gl.c"
    "src/glad/gl.c"
    "glad/src/gl.c"
    "external/glad/src/gl.c"
)

GLAD_FOUND=false
for loc in "${GLAD_LOCATIONS[@]}"; do
    if [ -f "$loc" ]; then
        echo "✓ Found GLAD at: $loc"
        GLAD_FOUND=true
        break
    fi
done

if [ "$GLAD_FOUND" = false ]; then
    echo "✗ GLAD not found in standard locations"
    echo "  Searching entire project..."
    find . -name "gl.c" -path "*/glad/*" 2>/dev/null | head -5
fi

echo ""
echo "Looking for ImGui..."
IMGUI_LOCATIONS=(
    "include/imgui/imgui.h"
    "imgui/imgui.h"
    "external/imgui/imgui.h"
)

IMGUI_FOUND=false
for loc in "${IMGUI_LOCATIONS[@]}"; do
    if [ -f "$loc" ]; then
        echo "✓ Found ImGui at: $(dirname $loc)"
        IMGUI_FOUND=true
        break
    fi
done

if [ "$IMGUI_FOUND" = false ]; then
    echo "✗ ImGui not found"
    echo "  Run: ./setup.sh"
fi

echo ""
echo "Checking for ENet..."
if pkg-config --exists libenet; then
    echo "✓ ENet installed: $(pkg-config --modversion libenet)"
else
    echo "✗ ENet not installed"
    echo "  Run: sudo apt install libenet-dev"
fi

echo ""
echo "=== Summary ==="
if [ "$GLAD_FOUND" = true ] && [ "$IMGUI_FOUND" = true ]; then
    echo "✓ All dependencies found! You can run: ./run.sh"
else
    echo "✗ Missing dependencies. See messages above."
fi
