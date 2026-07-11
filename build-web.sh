#!/usr/bin/env bash
set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$PROJECT_DIR/build"
OUTPUT_DIR="$BUILD_DIR/raylib-game-template"

source /home/kevin-james/emsdk-main/emsdk_env.sh

export CC=emcc
export CXX=em++

cmake -S "$PROJECT_DIR" -B "$BUILD_DIR" -DPLATFORM=Web
cmake --build "$BUILD_DIR" --parallel

cd "$OUTPUT_DIR"
exec python3 "$PROJECT_DIR/server.py"
