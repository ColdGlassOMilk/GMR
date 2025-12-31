#!/bin/bash
#
# GMR Setup Script
# ================
# This script sets up the complete GMR development environment.
# Run this in MSYS2 MinGW64 terminal on Windows.
#
# Usage: ./setup.sh [options]
#   --native-only    Skip web/Emscripten setup (faster, native dev only)
#   --skip-web       Same as --native-only
#   --skip-pacman    Skip pacman package installation (Windows)
#   --clean          Clean everything and start fresh
#   --fix-ssl        Fix SSL certificate issues (run if you get SSL errors)
#   --help           Show this help
#

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEPS_DIR="$SCRIPT_DIR/deps"
LIBS_DIR="$SCRIPT_DIR/libs"

# Parse arguments
SKIP_PACMAN=false
SKIP_WEB=false
CLEAN=false
FIX_SSL=false

for arg in "$@"; do
    case $arg in
        --skip-pacman)    SKIP_PACMAN=true ;;
        --skip-web)       SKIP_WEB=true ;;
        --native-only)    SKIP_WEB=true ;;
        --clean)          CLEAN=true ;;
        --fix-ssl)        FIX_SSL=true ;;
        --help)
            head -19 "$0" | tail -13
            exit 0
            ;;
    esac
done

echo -e "${BLUE}"
echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║                     GMR Setup Script                          ║"
echo "║              Games Made with Ruby - Full Setup                ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo -e "${NC}"

# ==============================================================================
# Helper Functions
# ==============================================================================

log_step() {
    echo -e "\n${GREEN}▶ $1${NC}"
}

log_info() {
    echo -e "${BLUE}  ℹ $1${NC}"
}

log_warn() {
    echo -e "${YELLOW}  ⚠ $1${NC}"
}

log_error() {
    echo -e "${RED}  ✖ $1${NC}"
}

log_success() {
    echo -e "${GREEN}  ✔ $1${NC}"
}

check_command() {
    if command -v "$1" &> /dev/null; then
        return 0
    else
        return 1
    fi
}

# ==============================================================================
# SSL Certificate Fix
# ==============================================================================

fix_ssl_certificates() {
    log_step "Fixing SSL certificates..."
    
    log_info "Reinitializing pacman keyring..."
    rm -rf /etc/pacman.d/gnupg
    pacman-key --init
    pacman-key --populate msys2
    
    log_info "Updating time (SSL errors can be caused by wrong system time)..."
    # Sync time if ntpdate is available
    if check_command ntpdate; then
        ntpdate -u time.windows.com 2>/dev/null || true
    fi
    
    log_info "Updating CA certificates..."
    # Try to download and install ca-certificates manually if needed
    pacman -Sy --noconfirm ca-certificates msys2-keyring 2>/dev/null || {
        log_warn "Trying alternative method..."
        # Force refresh
        pacman -Syy --noconfirm 2>/dev/null || true
    }
    
    log_info "Updating core packages..."
    pacman -S --noconfirm --needed pacman pacman-mirrors msys2-runtime 2>/dev/null || true
    
    log_success "SSL fix complete. Please restart your MSYS2 terminal and run setup again."
}

# Run SSL fix only if requested
if [[ "$FIX_SSL" == true ]]; then
    fix_ssl_certificates
    exit 0
fi

# ==============================================================================
# Environment Detection
# ==============================================================================

log_step "Detecting environment..."

# Check if running in MSYS2/MinGW
if [[ -z "$MSYSTEM" ]]; then
    log_warn "Not running in MSYS2 environment"
    log_info "This script is designed for MSYS2 MinGW64"
    log_info "Please run from: MSYS2 MinGW64 terminal"
    
    # Try to continue anyway for Linux/WSL
    if [[ "$(uname -s)" == "Linux" ]]; then
        log_info "Detected Linux - will attempt Linux setup"
        IS_LINUX=true
    else
        log_error "Unsupported environment"
        exit 1
    fi
elif [[ "$MSYSTEM" != "MINGW64" ]]; then
    log_error "Please run this script from MSYS2 MinGW64 terminal, not $MSYSTEM"
    log_info "Open 'MSYS2 MinGW64' from the Start menu"
    exit 1
else
    log_success "Running in MSYS2 MinGW64"
    IS_LINUX=false
fi

# ==============================================================================
# Clean (if requested)
# ==============================================================================

if [[ "$CLEAN" == true ]]; then
    log_step "Cleaning previous setup..."
    rm -rf "$DEPS_DIR"
    rm -rf "$LIBS_DIR"
    rm -rf "$SCRIPT_DIR/build"
    rm -rf "$SCRIPT_DIR/build-web"
    log_success "Cleaned"
fi

# Create directories
mkdir -p "$DEPS_DIR"
mkdir -p "$LIBS_DIR"

# ==============================================================================
# Install System Packages
# ==============================================================================

if [[ "$SKIP_PACMAN" == false && "$IS_LINUX" != true ]]; then
    log_step "Installing system packages via pacman..."
    
    # Fix SSL certificate issues (common on fresh MSYS2 installs)
    log_info "Updating SSL certificates and keyring..."
    
    # First, try to update ca-certificates and keyring
    # Use --disable-download-timeout in case of slow connections
    pacman -Sy --noconfirm --disable-download-timeout ca-certificates 2>/dev/null || {
        log_warn "Certificate update failed, trying keyring refresh..."
        
        # Initialize and populate pacman keyring
        pacman-key --init 2>/dev/null || true
        pacman-key --populate msys2 2>/dev/null || true
        pacman-key --refresh-keys 2>/dev/null || true
        
        # Update pacman mirrorlist and certificates
        pacman -Sy --noconfirm --disable-download-timeout pacman-mirrors 2>/dev/null || true
        pacman -S --noconfirm --disable-download-timeout ca-certificates 2>/dev/null || true
    }
    
    # Full system update (helps resolve many package issues)
    log_info "Updating package database..."
    pacman -Syu --noconfirm --disable-download-timeout || {
        log_warn "Full update had issues, continuing anyway..."
    }
    
    # Install packages - including mruby from pacman!
    PACKAGES=(
        mingw-w64-x86_64-gcc
        mingw-w64-x86_64-gdb
        mingw-w64-x86_64-cmake
        mingw-w64-x86_64-make
        mingw-w64-x86_64-raylib
        mingw-w64-x86_64-mruby
        mingw-w64-x86_64-glfw
        git
        ruby
        bison
        unzip
        tar
    )
    
    log_info "Installing: ${PACKAGES[*]}"
    pacman -S --noconfirm --needed --disable-download-timeout "${PACKAGES[@]}"
    
    log_success "System packages installed (including raylib and mruby)"
elif [[ "$IS_LINUX" == true ]]; then
    log_step "Installing system packages via apt..."
    sudo apt update
    sudo apt install -y build-essential cmake git ruby bison unzip \
        libasound2-dev libx11-dev libxrandr-dev libxi-dev \
        libgl1-mesa-dev libglu1-mesa-dev libxcursor-dev libxinerama-dev
    log_success "System packages installed"
else
    log_info "Skipping pacman packages (--skip-pacman)"
fi

# ==============================================================================
# Build mruby from source (Linux only - Windows uses pacman)
# ==============================================================================

if [[ "$IS_LINUX" == true ]]; then
    log_step "Building mruby (native) for Linux..."
    
    MRUBY_DIR="$DEPS_DIR/mruby"
    
    if [[ -d "$MRUBY_DIR" ]]; then
        log_info "mruby directory exists, pulling latest..."
        cd "$MRUBY_DIR"
        git pull || true
    else
        log_info "Cloning mruby..."
        git clone --depth 1 https://github.com/mruby/mruby.git "$MRUBY_DIR"
        cd "$MRUBY_DIR"
    fi
    
    log_info "Building mruby..."
    rm -rf build
    rake -j$(nproc)
    
    sudo cp build/host/lib/libmruby.a /usr/local/lib/
    sudo cp -r include/* /usr/local/include/
    
    log_success "mruby (native) built and installed"
fi

# ==============================================================================
# Setup Emscripten
# ==============================================================================

if [[ "$SKIP_WEB" == false ]]; then
    log_step "Setting up Emscripten SDK..."
    
    EMSDK_DIR="$DEPS_DIR/emsdk"
    EMSCRIPTEN_PATH="$EMSDK_DIR/upstream/emscripten"
    
    # Install emsdk if needed
    if [[ ! -d "$EMSDK_DIR" ]]; then
        log_info "Cloning emsdk..."
        git clone --depth 1 https://github.com/emscripten-core/emsdk.git "$EMSDK_DIR"
    fi
    
    cd "$EMSDK_DIR"
    
    # Install emscripten if not present
    if [[ ! -d "$EMSCRIPTEN_PATH" ]]; then
        log_info "Installing latest Emscripten..."
        ./emsdk install latest
        ./emsdk activate latest
    else
        log_info "Emscripten already installed"
    fi
    
    # Set up environment
    export EMSDK="$EMSDK_DIR"
    export PATH="$EMSCRIPTEN_PATH:$PATH"
    
    # Add node to PATH
    if [[ -d "$EMSDK_DIR/node" ]]; then
        NODE_BIN=$(find "$EMSDK_DIR/node" -type d -name "bin" 2>/dev/null | head -1)
        if [[ -n "$NODE_BIN" ]]; then
            export PATH="$NODE_BIN:$PATH"
        fi
    fi
    
    # Source emsdk_env.sh for any additional variables
    source "$EMSDK_DIR/emsdk_env.sh" 2>/dev/null || true
    
    # ALWAYS create wrapper scripts (handles paths with spaces reliably)
    # This is necessary because PATH with spaces doesn't work well in MSYS2
    log_info "Creating Emscripten wrapper scripts..."
    mkdir -p "$SCRIPT_DIR/bin"
    
    for tool in emcc em++ emcmake emmake emar emranlib emconfig emrun emsize; do
        TOOL_PATH=""
        if [[ -f "$EMSCRIPTEN_PATH/$tool" ]]; then
            TOOL_PATH="$EMSCRIPTEN_PATH/$tool"
        elif [[ -f "$EMSCRIPTEN_PATH/$tool.py" ]]; then
            TOOL_PATH="$EMSCRIPTEN_PATH/$tool.py"
        fi
        
        if [[ -n "$TOOL_PATH" ]]; then
            echo '#!/bin/bash' > "$SCRIPT_DIR/bin/$tool"
            echo "\"$TOOL_PATH\" \"\$@\"" >> "$SCRIPT_DIR/bin/$tool"
            chmod +x "$SCRIPT_DIR/bin/$tool"
        fi
    done
    
    # Add our wrapper bin to PATH FIRST (so it takes precedence)
    export PATH="$SCRIPT_DIR/bin:$PATH"
    
    log_success "Created wrapper scripts in $SCRIPT_DIR/bin"
    
    # Verify tools work
    if ! emcc --version &> /dev/null; then
        log_error "emcc not working"
        log_info "Check $SCRIPT_DIR/bin/emcc"
        exit 1
    fi
    
    if ! "$SCRIPT_DIR/bin/emcmake" --help &> /dev/null 2>&1; then
        # emcmake doesn't have --help that returns 0, check if file exists
        if [[ ! -x "$SCRIPT_DIR/bin/emcmake" ]]; then
            log_error "emcmake wrapper not created"
            exit 1
        fi
    fi
    
    log_success "Emscripten SDK ready"
    log_info "emcc: $(emcc --version | head -1)"
    
    # ===========================================================================
    # Build raylib for Web
    # ===========================================================================
    
    log_step "Building raylib for Web..."
    
    RAYLIB_DIR="$DEPS_DIR/raylib"
    RAYLIB_WEB_DIR="$LIBS_DIR/raylib-web"
    
    if [[ -d "$RAYLIB_DIR" ]]; then
        log_info "raylib directory exists, pulling latest..."
        cd "$RAYLIB_DIR"
        git pull || true
    else
        log_info "Cloning raylib..."
        git clone --depth 1 https://github.com/raysan5/raylib.git "$RAYLIB_DIR"
        cd "$RAYLIB_DIR"
    fi
    
    # Build for web
    rm -rf build-web
    mkdir -p build-web
    cd build-web
    
    log_info "Configuring raylib for Emscripten..."
    emcmake cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DPLATFORM=Web \
        -DBUILD_EXAMPLES=OFF \
        -DCMAKE_INSTALL_PREFIX="$(printf '%q' "$RAYLIB_WEB_DIR")" \
        -DCMAKE_C_COMPILER=emcc \
        -DCMAKE_CXX_COMPILER=em++ \
        -DCMAKE_AR=emar \
        -DCMAKE_RANLIB=emranlib

    
    log_info "Building raylib..."
    emmake make -j"$(nproc)"

    
    log_info "Installing raylib to $RAYLIB_WEB_DIR..."
    emmake make install
    
    log_success "raylib for Web built and installed"
    
    # ===========================================================================
    # Build mruby for Web
    # ===========================================================================
    
    log_step "Building mruby for Web..."
    
    MRUBY_DIR="$DEPS_DIR/mruby"
    MRUBY_WEB_DIR="$LIBS_DIR/mruby-web"
    
    # Clone mruby if not present (on Windows we use pacman for native, but need source for web)
    if [[ ! -d "$MRUBY_DIR" ]]; then
        log_info "Cloning mruby for web build..."
        git clone --depth 1 https://github.com/mruby/mruby.git "$MRUBY_DIR"
    fi
    
    cd "$MRUBY_DIR"
    
    # Clean emscripten build directory
    rm -rf build/emscripten
    
    # Create Emscripten build config (minimal - no IO needed for GMR)
    cat > build_config/emscripten.rb << 'MRUBY_CONFIG'
MRuby::CrossBuild.new('emscripten') do |conf|
  toolchain :clang

  conf.cc do |cc|
    cc.command = 'emcc'
    cc.flags = %w(-Os)
  end

  conf.cxx do |cxx|
    cxx.command = 'em++'
    cxx.flags = %w(-Os)
  end

  conf.linker do |linker|
    linker.command = 'emcc'
  end

  conf.archiver do |archiver|
    archiver.command = 'emar'
  end

  # Minimal gems for GMR - no IO to avoid HAL issues
  conf.gem core: 'mruby-compiler'
  conf.gem core: 'mruby-error'
  conf.gem core: 'mruby-eval'
  conf.gem core: 'mruby-metaprog'
  conf.gem core: 'mruby-sprintf'
  conf.gem core: 'mruby-math'
  conf.gem core: 'mruby-time'
  conf.gem core: 'mruby-string-ext'
  conf.gem core: 'mruby-array-ext'
  conf.gem core: 'mruby-hash-ext'
  conf.gem core: 'mruby-numeric-ext'
  conf.gem core: 'mruby-proc-ext'
  conf.gem core: 'mruby-symbol-ext'
  conf.gem core: 'mruby-random'
  conf.gem core: 'mruby-object-ext'
  conf.gem core: 'mruby-kernel-ext'
  conf.gem core: 'mruby-class-ext'
  conf.gem core: 'mruby-enum-ext'
  conf.gem core: 'mruby-struct'
  conf.gem core: 'mruby-range-ext'
  conf.gem core: 'mruby-fiber'
  conf.gem core: 'mruby-enumerator'
  conf.gem core: 'mruby-compar-ext'
  conf.gem core: 'mruby-toplevel-ext'
end
MRUBY_CONFIG
    
    log_info "Building mruby for Emscripten..."
    MRUBY_CONFIG="$(pwd)/build_config/emscripten.rb" rake clean || true
    MRUBY_CONFIG="$(pwd)/build_config/emscripten.rb" rake
    
    # Copy to libs directory
    mkdir -p "$MRUBY_WEB_DIR/lib"
    mkdir -p "$MRUBY_WEB_DIR/include"
    cp build/emscripten/lib/libmruby.a "$MRUBY_WEB_DIR/lib/"
    cp -r include/* "$MRUBY_WEB_DIR/include/"
    
    log_success "mruby for Web built and installed"
    
    # ===========================================================================
    # Create environment setup script
    # ===========================================================================
    
    log_step "Creating environment setup script..."
    
    # Create env.sh with properly escaped paths
    cat > "$SCRIPT_DIR/env.sh" << 'ENVSCRIPT_START'
#!/bin/bash
# GMR Environment Setup
# Source this file before building: source env.sh

ENVSCRIPT_START

    # Append paths (these will be expanded now, with proper quoting in the file)
    echo "" >> "$SCRIPT_DIR/env.sh"
    echo "# Paths set during setup" >> "$SCRIPT_DIR/env.sh"
    echo "GMR_DIR=\"$SCRIPT_DIR\"" >> "$SCRIPT_DIR/env.sh"
    echo "export EMSDK=\"$EMSDK_DIR\"" >> "$SCRIPT_DIR/env.sh"
    echo "export RAYLIB_WEB_PATH=\"$RAYLIB_WEB_DIR\"" >> "$SCRIPT_DIR/env.sh"
    echo "export MRUBY_WEB_PATH=\"$MRUBY_WEB_DIR\"" >> "$SCRIPT_DIR/env.sh"
    
    cat >> "$SCRIPT_DIR/env.sh" << 'ENVSCRIPT_END'

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
ENVSCRIPT_END
    
    chmod +x "$SCRIPT_DIR/env.sh"
    log_success "Created env.sh"
    
else
    log_info "Skipping web setup (--skip-web)"
fi

# ==============================================================================
# Verify Installation
# ==============================================================================

log_step "Verifying installation..."

# Check native tools
echo -n "  g++: "
if check_command g++; then
    echo -e "${GREEN}$(g++ --version | head -1)${NC}"
else
    echo -e "${RED}NOT FOUND${NC}"
fi

echo -n "  cmake: "
if check_command cmake; then
    echo -e "${GREEN}$(cmake --version | head -1)${NC}"
else
    echo -e "${RED}NOT FOUND${NC}"
fi

echo -n "  raylib: "
if [[ -f /mingw64/lib/libraylib.a ]] || [[ -f /usr/local/lib/libraylib.a ]]; then
    echo -e "${GREEN}installed${NC}"
else
    echo -e "${RED}NOT FOUND${NC}"
fi

echo -n "  mruby: "
if [[ -f /mingw64/lib/libmruby.a ]] || [[ -f /usr/local/lib/libmruby.a ]] || [[ -f /mingw64/lib/libmruby_core.a ]]; then
    echo -e "${GREEN}installed${NC}"
else
    echo -e "${RED}NOT FOUND${NC}"
fi

if [[ "$SKIP_WEB" == false ]]; then
    echo -n "  emcc: "
    if check_command emcc; then
        echo -e "${GREEN}$(emcc --version | head -1)${NC}"
    else
        echo -e "${YELLOW}available after: source env.sh${NC}"
    fi
    
    echo -n "  raylib-web: "
    if [[ -f "$LIBS_DIR/raylib-web/lib/libraylib.a" ]]; then
        echo -e "${GREEN}installed${NC}"
    else
        echo -e "${RED}NOT FOUND${NC}"
    fi
    
    echo -n "  mruby-web: "
    if [[ -f "$LIBS_DIR/mruby-web/lib/libmruby.a" ]]; then
        echo -e "${GREEN}installed${NC}"
    else
        echo -e "${RED}NOT FOUND${NC}"
    fi
fi

# ==============================================================================
# Done!
# ==============================================================================

echo -e "\n${GREEN}"
echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║                    Setup Complete! 🎉                         ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo -e "${NC}"

echo -e "Next steps:\n"
echo -e "  ${BLUE}Native builds (Debug/Release):${NC}"
echo "    ./build.sh debug"
echo "    ./build.sh release"
echo ""

if [[ "$SKIP_WEB" == false ]]; then
    echo -e "  ${BLUE}Web build:${NC}"
    echo "    source env.sh     # Load Emscripten environment"
    echo "    ./build.sh web"
    echo ""
fi

echo -e "  ${BLUE}Run the game:${NC}"
echo "    ./gmr              # or gmr.exe on Windows"
echo ""
echo -e "  ${BLUE}Edit your game:${NC}"
echo "    Edit scripts/main.rb - changes hot-reload automatically!"
echo ""