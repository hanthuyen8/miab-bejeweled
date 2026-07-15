#!/usr/bin/env bash
# Builds the Emscripten/web target. Output: build-web/seajeweled.{html,js,wasm,data}
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build-web"

command -v emcmake >/dev/null || { echo "emcmake not found — install Emscripten (brew install emscripten)" >&2; exit 1; }

emcmake cmake -S "$SCRIPT_DIR" -B "$BUILD_DIR"
emmake cmake --build "$BUILD_DIR" -- -j"$(sysctl -n hw.ncpu 2>/dev/null || nproc)"
