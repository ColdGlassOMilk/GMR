#!/bin/bash
# GMR Environment Setup
# Source this file before building: source env.sh


# Paths set during setup
GMR_DIR="/c/Users/nick.brabant/OneDrive - NewWave Group North America/Documents/Personal/gmr"
export EMSDK="/c/Users/nick.brabant/OneDrive - NewWave Group North America/Documents/Personal/gmr/deps/emsdk"
export RAYLIB_WEB_PATH="/c/Users/nick.brabant/OneDrive - NewWave Group North America/Documents/Personal/gmr/libs/raylib-web"
export MRUBY_WEB_PATH="/c/Users/nick.brabant/OneDrive - NewWave Group North America/Documents/Personal/gmr/libs/mruby-web"

# Use wrapper scripts if they exist (handles paths with spaces)
if [[ -d "$GMR_DIR/bin" ]]; then
    export PATH="$GMR_DIR/bin:$PATH"
fi

# Add emscripten to PATH directly
if [[ -d "$EMSDK/upstream/emscripten" ]]; then
    export PATH="$EMSDK/upstream/emscripten:$PATH"
fi

# Add node to PATH
NODE_BIN=$(find "$EMSDK/node" -type d -name "bin" 2>/dev/null | head -1)
if [[ -n "$NODE_BIN" ]]; then
    export PATH="$NODE_BIN:$PATH"
fi

# Also source emsdk_env.sh for any other variables it sets
source "$EMSDK/emsdk_env.sh" 2>/dev/null || true

echo "GMR environment loaded"
echo "  EMSDK: $EMSDK"
echo "  RAYLIB_WEB_PATH: $RAYLIB_WEB_PATH"
echo "  MRUBY_WEB_PATH: $MRUBY_WEB_PATH"
if command -v emcc &> /dev/null; then
    echo "  emcc: $(emcc --version | head -1)"
else
    echo "  emcc: NOT FOUND"
fi
