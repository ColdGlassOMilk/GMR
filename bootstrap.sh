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

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

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
                echo -e "${RED}Please install Homebrew or Ruby manually${NC}"
                exit 1
            fi
            ;;
    esac

    echo -e "  ${GREEN}+ Ruby $(ruby --version | cut -d' ' -f2)${NC}"
fi

# Get the gem bin directory and add to PATH early (suppresses warning)
GEM_BIN=$(ruby -e 'puts Gem.user_dir')/bin
export PATH="$GEM_BIN:$PATH"

# Add to shell profile for persistence (only if not already added)
PROFILE_FILE=""
if [[ -f ~/.bashrc ]]; then
    PROFILE_FILE=~/.bashrc
elif [[ -f ~/.bash_profile ]]; then
    PROFILE_FILE=~/.bash_profile
fi

if [[ -n "$PROFILE_FILE" ]] && ! grep -q "Ruby gem binaries" "$PROFILE_FILE" 2>/dev/null; then
    echo "" >> "$PROFILE_FILE"
    echo "# Ruby gem binaries" >> "$PROFILE_FILE"
    echo 'export PATH="$(ruby -e '\''puts Gem.user_dir'\'')/bin:$PATH"' >> "$PROFILE_FILE"
    echo -e "${GREEN}> Added gem bin to $PROFILE_FILE${NC}"
fi

cd "$SCRIPT_DIR/cli"

# Check if bundler is installed
if gem list bundler -i > /dev/null 2>&1; then
    echo -e "${GREEN}> Bundler already installed${NC}"
else
    echo -e "\n${GREEN}> Installing bundler...${NC}"
    gem install bundler --no-document
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

# Extract version from lib/gmrcli/version.rb
GEMSPEC_VERSION=$(grep -oP 'VERSION\s*=\s*["\x27]\K[^"\x27]+' lib/gmrcli/version.rb 2>/dev/null || echo "0.0.0")

# Always reinstall gmrcli to pick up code changes
echo -e "${GREEN}> Building and installing gmrcli...${NC}"
gem uninstall gmrcli --quiet --executables 2>/dev/null || true
gem build gmrcli.gemspec --quiet 2>/dev/null || gem build gmrcli.gemspec
gem install ./gmrcli-*.gem --no-document
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
echo -e "For future sessions, reload your shell:"
echo ""
echo -e "  ${GREEN}source ~/.bashrc${NC}"
echo ""

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
        if [[ "$setup_type" =~ ^[Nn]$ ]]; then
            exec gmrcli setup --native-only -o text
        else
            exec gmrcli setup -o text
        fi
    fi
fi
