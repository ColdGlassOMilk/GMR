#!/bin/bash
#
# GMR Build Script
# ================
# Build GMR for different platforms and configurations.
#
# Usage: ./build.sh [target] [options]
#
# Targets:
#   debug       Build native debug (default)
#   release     Build native release
#   web         Build for web/Emscripten
#   all         Build all targets
#   clean       Clean all build directories
#   run         Build debug and run
#   run-release Build release and run
#   serve       Build web and start local server
#
# Options:
#   --rebuild   Clean before building
#   --verbose   Show detailed build output
#

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Defaults
TARGET="debug"
REBUILD=false
VERBOSE=""

# Parse arguments
for arg in "$@"; do
    case $arg in
        debug|release|web|all|clean|run|run-release|serve)
            TARGET="$arg"
            ;;
        --rebuild)
            REBUILD=true
            ;;
        --verbose)
            VERBOSE="VERBOSE=1"
            ;;
        --help|-h)
            head -22 "$0" | tail -20
            exit 0
            ;;
    esac
done

log() {
    echo -e "${GREEN}▶ $1${NC}"
}

error() {
    echo -e "${RED}✖ $1${NC}"
    exit 1
}

# Detect platform
detect_platform() {
    if [[ "$MSYSTEM" == "MINGW64" ]]; then
        echo "mingw"
    elif [[ "$(uname -s)" == "Linux" ]]; then
        echo "linux"
    elif [[ "$(uname -s)" == "Darwin" ]]; then
        echo "macos"
    else
        echo "unknown"
    fi
}

PLATFORM=$(detect_platform)

# Get number of CPU cores
if [[ "$PLATFORM" == "mingw" ]]; then
    NPROC=$(nproc 2>/dev/null || echo 4)
else
    NPROC=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
fi

# ==============================================================================
# Build Functions
# ==============================================================================

build_native_debug() {
    log "Building native DEBUG..."
    
    if [[ "$REBUILD" == true ]]; then
        rm -rf build
    fi
    
    mkdir -p build
    cd build
    
    if [[ "$PLATFORM" == "mingw" ]]; then
        cmake .. -G "Ninja" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_MAKE_PROGRAM=C:/msys64/mingw64/bin/ninja.exe
        ninja -j$NPROC $VERBOSE
    else
        cmake .. -G "Ninja" -DCMAKE_BUILD_TYPE=Debug
        ninja -j$NPROC $VERBOSE
    fi
    
    cd ..
    echo -e "${GREEN}✔ Debug build complete: ./gmr${NC}"
}

build_native_release() {
    log "Building native RELEASE..."
    
    if [[ "$REBUILD" == true ]]; then
        rm -rf build
    fi
    
    mkdir -p build
    cd build
    
    if [[ "$PLATFORM" == "mingw" ]]; then
        cmake .. -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DCMAKE_MAKE_PROGRAM=C:/msys64/mingw64/bin/ninja.exe
        ninja -j$NPROC $VERBOSE
    else
        cmake .. -G "Ninja" -DCMAKE_BUILD_TYPE=Release
        ninja -j$NPROC $VERBOSE
    fi
    
    cd ..
    echo -e "${GREEN}✔ Release build complete: ./gmr${NC}"
}

build_web() {
    log "Building for WEB..."
    
    # Check for Emscripten
    if ! command -v emcc &> /dev/null; then
        # First check if wrapper scripts exist
        if [[ -d "$SCRIPT_DIR/bin" && -f "$SCRIPT_DIR/bin/emcc" ]]; then
            export PATH="$SCRIPT_DIR/bin:$PATH"
        fi
        
        # If still not found, try to source env.sh
        if ! command -v emcc &> /dev/null && [[ -f "$SCRIPT_DIR/env.sh" ]]; then
            echo -e "${YELLOW}  Loading Emscripten environment...${NC}"
            source "$SCRIPT_DIR/env.sh"
        fi
    fi
    
    # Verify emcc is available now
    if ! command -v emcc &> /dev/null; then
        error "emcc not found. Run: source env.sh (or run setup.sh first)"
    fi
    
    # Check for web libraries
    if [[ -z "$RAYLIB_WEB_PATH" ]]; then
        export RAYLIB_WEB_PATH="$SCRIPT_DIR/deps/raylib/web"
    fi
    if [[ -z "$MRUBY_WEB_PATH" ]]; then
        export MRUBY_WEB_PATH="$SCRIPT_DIR/deps/mruby/web"
    fi

    if [[ ! -f "$RAYLIB_WEB_PATH/lib/libraylib.a" ]]; then
        error "raylib-web not found at $RAYLIB_WEB_PATH. Run setup.sh first."
    fi
    if [[ ! -f "$MRUBY_WEB_PATH/lib/libmruby.a" ]]; then
        error "mruby-web not found at $MRUBY_WEB_PATH. Run setup.sh first."
    fi
    
    if [[ "$REBUILD" == true ]]; then
        rm -rf build-web
    fi
    
    mkdir -p build-web
    cd build-web
    
    emcmake cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DPLATFORM=Web
    
    emmake make -j$NPROC $VERBOSE
    
    cd ..
    echo -e "${GREEN}✔ Web build complete: build-web/gmr.html${NC}"
}

clean_all() {
    log "Cleaning all build directories..."
    rm -rf build build-web
    rm -f gmr gmr.exe
    echo -e "${GREEN}✔ Clean complete${NC}"
}

run_game() {
    if [[ "$PLATFORM" == "mingw" ]]; then
        ./gmr.exe
    else
        ./gmr
    fi
}

serve_web() {
    if [[ ! -f "build-web/gmr.html" ]]; then
        build_web
    fi
    
    log "Starting local web server..."
    echo -e "${BLUE}  Open http://localhost:8080/gmr.html in your browser${NC}"
    echo -e "${YELLOW}  Press Ctrl+C to stop${NC}"
    
    cd build-web
    
    # Try different servers
    if command -v python3 &> /dev/null; then
        python3 -m http.server 8080
    elif command -v python &> /dev/null; then
        python -m SimpleHTTPServer 8080
    elif command -v php &> /dev/null; then
        php -S localhost:8080
    else
        error "No web server found. Install Python or PHP."
    fi
}

# ==============================================================================
# Main
# ==============================================================================

case $TARGET in
    debug)
        build_native_debug
        ;;
    release)
        build_native_release
        ;;
    web)
        build_web
        ;;
    all)
        build_native_debug
        build_native_release
        build_web
        ;;
    clean)
        clean_all
        ;;
    run)
        build_native_debug
        run_game
        ;;
    run-release)
        build_native_release
        run_game
        ;;
    serve)
        serve_web
        ;;
    *)
        echo "Unknown target: $TARGET"
        echo "Run ./build.sh --help for usage"
        exit 1
        ;;
esac
