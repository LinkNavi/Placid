#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

# Check if ImGui exists, if not, prompt to run setup
if [ ! -f "$SCRIPT_DIR/include/imgui/imgui.h" ] && [ ! -f "$SCRIPT_DIR/imgui/imgui.h" ] && [ ! -f "$SCRIPT_DIR/external/imgui/imgui.h" ]; then
    echo "⚠ ImGui not found. Running setup..."
    if [ -f "$SCRIPT_DIR/setup.sh" ]; then
        bash "$SCRIPT_DIR/setup.sh"
    else
        echo "Warning: setup.sh not found. Continuing without ImGui..."
    fi
fi

echo "Creating build directory: $BUILD_DIR"
mkdir -p "$BUILD_DIR"

echo "Configuring CMake (Release)"
cmake -S "$SCRIPT_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release

echo "Building (using all CPUs)"
cmake --build "$BUILD_DIR" --config Release -- -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 1)

EXEC="$BUILD_DIR/bin/placid"
if [ ! -x "$EXEC" ]; then
    EXEC="$BUILD_DIR/placid"
fi

if [ ! -x "$EXEC" ]; then
    echo "Build succeeded but couldn't find executable at $EXEC"
    exit 1
fi

# Pass arguments to the executable
echo "Running $EXEC $@"
"$EXEC" "$@"
