#!/bin/bash
#
# GMR Bootstrap Script
# ====================
# This minimal script installs Ruby and the gmrcli CLI tool.
# After running this, use 'gmrcli' for all other operations.
#
# Usage: ./bootstrap.sh [--skip-setup]
#
# Options:
#   --skip-setup    Skip the interactive setup prompt (useful for IDE automation)
#

set -e

# Parse arguments
SKIP_SETUP=false
for arg in "$@"; do
    case $arg in
        --skip-setup)
            SKIP_SETUP=true
            shift
            ;;
    esac
done

RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
DIM='\033[2m'
NC='\033[0m'

# Get script directory (works in both bash and zsh)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]:-$0}")" && pwd)"

echo -e "${BLUE}"
echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║                     GMR Bootstrap                             ║"
echo "║               Installing gmrcli Utility                       ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo -e "${NC}"

# Detect environment
if [[ -z "$MSYSTEM" ]]; then
    if [[ "$(uname -s)" == "Linux" ]]; then
        PLATFORM="linux"
    elif [[ "$(uname -s)" == "Darwin" ]]; then
        PLATFORM="macos"
    else
        echo -e "${RED}Unsupported platform${NC}"
        exit 1
    fi
elif [[ "$MSYSTEM" != "MINGW64" ]]; then
    echo -e "${RED}Please run from MSYS2 MinGW64 terminal${NC}"
    exit 1
else
    PLATFORM="mingw64"
fi

echo -e "${GREEN}> Detected: $PLATFORM${NC}"

# Check if Ruby is installed
if command -v ruby &> /dev/null; then
    echo -e "${GREEN}> Ruby already installed${NC} ${DIM}($(ruby --version | cut -d' ' -f2))${NC}"
else
    echo -e "\n${GREEN}> Installing Ruby...${NC}"

    case $PLATFORM in
        mingw64)
            pacman -S --noconfirm --needed ruby > /dev/null 2>&1 || {
                echo "  Updating pacman..."
                pacman -Sy --noconfirm > /dev/null 2>&1
                pacman -S --noconfirm --needed ruby
            }
            ;;
        linux)
            if command -v apt &> /dev/null; then
                sudo apt update && sudo apt install -y ruby ruby-dev
            elif command -v dnf &> /dev/null; then
                sudo dnf install -y ruby ruby-devel
            elif command -v pacman &> /dev/null; then
                sudo pacman -S --noconfirm ruby
            else
                echo -e "${RED}Please install Ruby manually${NC}"
                exit 1
            fi
            ;;
        macos)
            if command -v brew &> /dev/null; then
                brew install ruby
            else
                echo -e "${RED}Please install Homebrew${NC}"
                exit 1
            fi
            ;;
    esac

    echo -e "  ${GREEN}+ Ruby $(ruby --version | cut -d' ' -f2)${NC}"
fi

# Check if GCC/G++ is installed
if command -v gcc &> /dev/null && command -v g++ &> /dev/null; then
    echo -e "${GREEN}> GCC/G++ already installed${NC} ${DIM}($(gcc --version | head -1 | cut -d' ' -f4))${NC}"
else
    echo -e "\n${GREEN}> Installing GCC/G++ compiler toolchain...${NC}"

    case $PLATFORM in
        mingw64)
            pacman -S --noconfirm --needed mingw-w64-x86_64-gcc > /dev/null 2>&1 || {
                echo "  Updating pacman..."
                pacman -Sy --noconfirm > /dev/null 2>&1
                pacman -S --noconfirm --needed mingw-w64-x86_64-gcc
            }
            ;;
        linux)
            if command -v apt &> /dev/null; then
                sudo apt update && sudo apt install -y build-essential
            elif command -v dnf &> /dev/null; then
                sudo dnf install -y gcc gcc-c++ make
            elif command -v pacman &> /dev/null; then
                sudo pacman -S --noconfirm base-devel
            elif command -v zypper &> /dev/null; then
                sudo zypper install -y gcc gcc-c++ make
            elif command -v apk &> /dev/null; then
                sudo apk add build-base gcc g++ make
            else
                echo -e "${RED}Please install GCC/G++ manually${NC}"
                exit 1
            fi
            ;;
        macos)
            # macOS typically has compilers from Xcode Command Line Tools
            if ! command -v gcc &> /dev/null; then
                echo -e "${BLUE}> Installing Xcode Command Line Tools...${NC}"
                xcode-select --install
                echo -e "${DIM}Please complete the Xcode installation and re-run this script${NC}"
                exit 1
            fi
            ;;
    esac

    echo -e "  ${GREEN}+ GCC $(gcc --version | head -1 | cut -d' ' -f4)${NC}"
    if command -v g++ &> /dev/null; then
        echo -e "  ${GREEN}+ G++ $(g++ --version | head -1 | cut -d' ' -f4)${NC}"
    fi
fi

# Check if X11 development libraries are installed (needed for raylib/GLFW on Linux)
if [[ "$PLATFORM" == "linux" ]]; then
    # Check for X11 library (different paths on different distros)
    if pkg-config --exists x11 2>/dev/null; then
        echo -e "${GREEN}> X11 development libraries already installed${NC}"
    else
        echo -e "\n${GREEN}> Installing X11 development libraries...${NC}"

        if command -v apt &> /dev/null; then
            sudo apt install -y libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libgl1-mesa-dev
        elif command -v dnf &> /dev/null; then
            sudo dnf install -y libX11-devel libXrandr-devel libXinerama-devel libXcursor-devel libXi-devel mesa-libGL-devel
        elif command -v pacman &> /dev/null; then
            sudo pacman -S --noconfirm libx11 libxrandr libxinerama libxcursor libxi mesa
        elif command -v zypper &> /dev/null; then
            sudo zypper install -y libX11-devel libXrandr-devel libXinerama-devel libXcursor-devel libXi-devel Mesa-libGL-devel
        elif command -v apk &> /dev/null; then
            sudo apk add libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev mesa-dev
        else
            echo -e "${RED}Please install X11 development libraries manually${NC}"
            exit 1
        fi

        echo -e "  ${GREEN}+ X11 development libraries installed${NC}"
    fi
fi

# Check if CMake is installed
if command -v cmake &> /dev/null; then
    echo -e "${GREEN}> CMake already installed${NC} ${DIM}($(cmake --version | head -1 | cut -d' ' -f3))${NC}"
else
    echo -e "\n${GREEN}> Installing CMake...${NC}"

    case $PLATFORM in
        mingw64)
            pacman -S --noconfirm --needed mingw-w64-x86_64-cmake > /dev/null 2>&1 || {
                echo "  Updating pacman..."
                pacman -Sy --noconfirm > /dev/null 2>&1
                pacman -S --noconfirm --needed mingw-w64-x86_64-cmake
            }
            ;;
        linux)
            if command -v apt &> /dev/null; then
                sudo apt update && sudo apt install -y cmake
            elif command -v dnf &> /dev/null; then
                sudo dnf install -y cmake
            elif command -v pacman &> /dev/null; then
                sudo pacman -S --noconfirm cmake
            else
                echo -e "${RED}Please install CMake manually${NC}"
                exit 1
            fi
            ;;
        macos)
            if command -v brew &> /dev/null; then
                brew install cmake
            else
                echo -e "${RED}Please install Homebrew first${NC}"
                echo -e "${DIM}Visit: https://brew.sh${NC}"
                exit 1
            fi
            ;;
    esac

    echo -e "  ${GREEN}+ CMake $(cmake --version | head -1 | cut -d' ' -f3)${NC}"
fi

# Check if Ninja is installed
if command -v ninja &> /dev/null; then
    echo -e "${GREEN}> Ninja already installed${NC} ${DIM}($(ninja --version))${NC}"
else
    echo -e "\n${GREEN}> Installing Ninja...${NC}"

    case $PLATFORM in
        mingw64)
            pacman -S --noconfirm --needed mingw-w64-x86_64-ninja > /dev/null 2>&1 || {
                echo "  Updating pacman..."
                pacman -Sy --noconfirm > /dev/null 2>&1
                pacman -S --noconfirm --needed mingw-w64-x86_64-ninja
            }
            ;;
        linux)
            if command -v apt &> /dev/null; then
                sudo apt update && sudo apt install -y ninja-build
            elif command -v dnf &> /dev/null; then
                sudo dnf install -y ninja-build
            elif command -v pacman &> /dev/null; then
                sudo pacman -S --noconfirm ninja
            else
                echo -e "${RED}Please install Ninja manually${NC}"
                exit 1
            fi
            ;;
        macos)
            if command -v brew &> /dev/null; then
                brew install ninja
            else
                echo -e "${RED}Please install Homebrew first${NC}"
                echo -e "${DIM}Visit: https://brew.sh${NC}"
                exit 1
            fi
            ;;
    esac

    echo -e "  ${GREEN}+ Ninja $(ninja --version)${NC}"
fi

# Get the gem bin directory and add to PATH early (suppresses warning)
# mingw64 installs gems to user dir; macOS/Linux use system gem bindir
case $PLATFORM in
    mingw64)
        GEM_BIN=$(ruby -e 'puts Gem.user_dir')/bin
        ;;
    *)
        GEM_BIN=$(ruby -e 'puts Gem.bindir')
        ;;
esac
export PATH="$GEM_BIN:$PATH"

# Add to shell profile for persistence (only if not already added)
# Detect appropriate shell profile file
PROFILE_FILE=""
if [[ -n "$ZSH_VERSION" ]] || [[ "$SHELL" == */zsh ]]; then
    # zsh user
    if [[ -f ~/.zshrc ]]; then
        PROFILE_FILE=~/.zshrc
    fi
elif [[ -f ~/.bashrc ]]; then
    PROFILE_FILE=~/.bashrc
elif [[ -f ~/.bash_profile ]]; then
    PROFILE_FILE=~/.bash_profile
fi

if [[ -n "$PROFILE_FILE" ]]; then
    # Determine the correct PATH line for this platform
    if [[ "$PLATFORM" == "mingw64" ]]; then
        GEM_PATH_LINE='export PATH="$(ruby -e '\''puts Gem.user_dir'\'')/bin:$PATH"'
    else
        GEM_PATH_LINE='export PATH="$(ruby -e '\''puts Gem.bindir'\''):$PATH"'
    fi

    if ! grep -q "Ruby gem binaries" "$PROFILE_FILE" 2>/dev/null; then
        # No entry yet — add it
        echo "" >> "$PROFILE_FILE"
        echo "# Ruby gem binaries" >> "$PROFILE_FILE"
        echo "$GEM_PATH_LINE" >> "$PROFILE_FILE"
        echo -e "${GREEN}> Added gem bin to $PROFILE_FILE${NC}"
    elif ! grep -qF "$GEM_PATH_LINE" "$PROFILE_FILE" 2>/dev/null; then
        # Entry exists but with wrong/stale path — remove old block and re-add
        grep -v "# Ruby gem binaries" "$PROFILE_FILE" | grep -v 'Gem\.bindir' | grep -v 'Gem\.user_dir' > "${PROFILE_FILE}.tmp"
        mv "${PROFILE_FILE}.tmp" "$PROFILE_FILE"
        echo "" >> "$PROFILE_FILE"
        echo "# Ruby gem binaries" >> "$PROFILE_FILE"
        echo "$GEM_PATH_LINE" >> "$PROFILE_FILE"
        echo -e "${GREEN}> Updated gem bin path in $PROFILE_FILE${NC}"
    fi
fi

cd "$SCRIPT_DIR/cli"

# Check if bundler is installed
if gem list bundler -i > /dev/null 2>&1; then
    echo -e "${GREEN}> Bundler already installed${NC}"
else
    echo -e "\n${GREEN}> Installing bundler...${NC}"
    gem install bundler --no-document
fi

# Check if rake is installed (needed for mruby builds)
if gem list rake -i > /dev/null 2>&1; then
    echo -e "${GREEN}> Rake already installed${NC}"
else
    echo -e "\n${GREEN}> Installing rake...${NC}"
    gem install rake --no-document
fi

# Check if bison is installed (needed for mruby parser generation)
if [[ "$PLATFORM" == "linux" ]] || [[ "$PLATFORM" == "macos" ]]; then
    if command -v bison &> /dev/null; then
        echo -e "${GREEN}> Bison already installed${NC} ${DIM}($(bison --version | head -1 | cut -d' ' -f4))${NC}"
    else
        echo -e "\n${GREEN}> Installing bison...${NC}"

        case $PLATFORM in
            linux)
                if command -v apt &> /dev/null; then
                    sudo apt install -y bison
                elif command -v dnf &> /dev/null; then
                    sudo dnf install -y bison
                elif command -v pacman &> /dev/null; then
                    sudo pacman -S --noconfirm bison
                elif command -v zypper &> /dev/null; then
                    sudo zypper install -y bison
                elif command -v apk &> /dev/null; then
                    sudo apk add bison
                else
                    echo -e "${RED}Please install bison manually${NC}"
                    exit 1
                fi
                ;;
            macos)
                if command -v brew &> /dev/null; then
                    brew install bison
                else
                    echo -e "${RED}Please install Homebrew first${NC}"
                    exit 1
                fi
                ;;
        esac

        echo -e "  ${GREEN}+ Bison $(bison --version | head -1 | cut -d' ' -f4)${NC}"
    fi
fi

# Check if dependencies need updating (compare Gemfile.lock timestamp)
NEEDS_BUNDLE=false
if [[ ! -f "Gemfile.lock" ]]; then
    NEEDS_BUNDLE=true
elif [[ "Gemfile" -nt "Gemfile.lock" ]]; then
    NEEDS_BUNDLE=true
fi

if $NEEDS_BUNDLE; then
    echo -e "${GREEN}> Installing gem dependencies...${NC}"
    bundle install --quiet
else
    echo -e "${GREEN}> Gem dependencies up to date${NC}"
fi

# Extract version from lib/gmrcli/version.rb (portable, works on Mac/Linux)
GEMSPEC_VERSION=$(grep -E 'VERSION\s*=' lib/gmrcli/version.rb 2>/dev/null | sed -E 's/.*VERSION\s*=\s*["\x27]([^"\x27]+)["\x27].*/\1/' || echo "0.0.0")

# Always reinstall gmrcli to pick up code changes
echo -e "${GREEN}> Building and installing gmrcli...${NC}"
gem uninstall gmrcli --quiet --executables 2>/dev/null || true
gem build gmrcli.gemspec --quiet 2>/dev/null || gem build gmrcli.gemspec
if [[ "$PLATFORM" == "mingw64" ]]; then
    gem install ./gmrcli-*.gem --no-document --user-install
else
    gem install ./gmrcli-*.gem --no-document
fi
rm -f gmrcli-*.gem
echo -e "  ${GREEN}+ gmrcli ${GEMSPEC_VERSION}${NC}"

echo -e "\n${GREEN}"
echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║                   Bootstrap Complete!                         ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo -e "${NC}"

echo -e "The ${BLUE}gmrcli${NC} command is ready.\n"
echo "Commands:"
echo -e "  ${BLUE}gmrcli setup${NC}              # Full development environment setup"
echo -e "  ${BLUE}gmrcli setup --native-only${NC} # Native only (faster)"
echo -e "  ${BLUE}gmrcli build debug${NC}        # Build debug version"
echo -e "  ${BLUE}gmrcli run${NC}                # Run the game"
echo -e "  ${BLUE}gmrcli help${NC}               # All commands"
echo ""
echo "Output modes:"
echo -e "  ${DIM}(default)${NC}                   # Machine-readable JSON output"
echo -e "  ${BLUE}gmrcli -o text${NC}            # Human-readable text output"
echo -e "  ${BLUE}gmrcli --protocol-version v1${NC} # Lock to protocol version"
echo ""
# Show appropriate reload command based on detected shell profile
if [[ -n "$PROFILE_FILE" ]]; then
    echo -e "For future sessions, reload your shell:"
    echo ""
    echo -e "  ${GREEN}source $PROFILE_FILE${NC}"
    echo ""
fi

# Ask if user wants to run setup now (skip if --skip-setup was passed)
if $SKIP_SETUP; then
    echo -e "${DIM}Skipping setup (--skip-setup flag)${NC}"
else
    echo -e -n "Would you like to run ${BLUE}gmrcli setup${NC} now? [Y/n] "
    read -r response
    response=${response:-Y}

    if [[ "$response" =~ ^[Yy]$ ]]; then
        echo ""
        echo -e -n "Run full setup or native-only (faster)? [${GREEN}f${NC}ull/${GREEN}n${NC}ative-only] "
        read -r setup_type
        setup_type=${setup_type:-f}

        echo ""
        # Use -o text for interactive terminal output
        # Note: Command must come before options due to Thor's default_task behavior
        # Use full path to gmrcli since PATH may not be fully updated in current session
        if [[ "$setup_type" =~ ^[Nn]$ ]]; then
            exec "$GEM_BIN/gmrcli" setup --native-only -o text
        else
            exec "$GEM_BIN/gmrcli" setup -o text
        fi
    fi
fi
