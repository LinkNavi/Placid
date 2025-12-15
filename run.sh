#!/usr/bin/env bash
set -euo pipefail


SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"


echo "Creating build directory: $BUILD_DIR"
mkdir -p "$BUILD_DIR"


echo "Configuring CMake (Release)"
cmake -S "$SCRIPT_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release


echo "Building (using all CPUs)"
cmake --build "$BUILD_DIR" --config Release -- -j$(nproc || 1)


EXEC="$BUILD_DIR/bin/placid"
if [ ! -x "$EXEC" ]; then
# fallback if CMake layouts differ
EXEC="$BUILD_DIR/placid"
fi


if [ ! -x "$EXEC" ]; then
echo "Build succeeded but couldn't find executable at $EXEC"
exit 1
fi


echo "Running $EXEC"
"$EXEC"
