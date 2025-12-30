# GMR Quick Start Guide

Get up and running in minutes!

## Windows Setup

### Prerequisites

1. **Install MSYS2** from https://www.msys2.org/
   - Download and run the installer
   - Use default path: `C:\msys64`
   - Complete the installation

That's it! The setup script handles everything else.

### Setup & Build

**Option A: Double-click (easiest)**

1. Double-click `setup.bat`
2. Wait for setup to complete (~5 minutes native, ~15 min with web)
3. Double-click `build.bat` to build
4. Run `gmr.exe`

**Option B: Command line**

1. Open "MSYS2 MinGW64" from Start menu
2. Navigate to the GMR folder:
   ```bash
   cd /c/path/to/gmr
   ```
3. Run setup:
   ```bash
   ./setup.sh              # Full setup (native + web)
   ./setup.sh --native-only  # Faster! Skip web/Emscripten setup
   ```
4. Build and run:
   ```bash
   ./build.sh run
   ```

## Build Commands

```bash
./build.sh debug       # Debug build (default)
./build.sh release     # Optimized release build
./build.sh web         # WebAssembly build
./build.sh all         # Build everything
./build.sh clean       # Remove build files

./build.sh run         # Build debug & run
./build.sh run-release # Build release & run
./build.sh serve       # Build web & start server
```

Or on Windows, use `build.bat`:
```cmd
build.bat debug
build.bat release
build.bat run
```

## Web Builds

For web builds, first load the Emscripten environment:

```bash
source env.sh
./build.sh web
./build.sh serve      # Opens http://localhost:8080/gmr.html
```

## Making Your Game

Edit `scripts/main.rb` - changes auto-reload while the game is running!

```ruby
def init
  set_window_title("My Game")
  $x = 400
  $y = 300
end

def update(dt)
  $x += 100 * dt if key_down?(262)  # Right arrow
  $x -= 100 * dt if key_down?(263)  # Left arrow
end

def draw
  clear_screen(20, 20, 40)
  set_color(255, 100, 100)
  draw_circle($x.to_i, $y.to_i, 30)
end
```

## Troubleshooting

**SSL certificate errors during setup**
```bash
# Run the SSL fix, then restart terminal and try again
./setup.sh --fix-ssl
# Close and reopen MSYS2 MinGW64, then:
./setup.sh
```

If that doesn't work, manually update MSYS2 first:
```bash
pacman -Syu
# If prompted to close terminal, do so, reopen, and run again:
pacman -Syu
# Then run setup
./setup.sh
```

**Web build: "emcc not found"**

If your project is in a path with spaces (like OneDrive), Emscripten may have issues:
```bash
# The setup script creates wrappers automatically, but if it fails:
# Option 1: Source the environment manually
source env.sh
./build.sh web

# Option 2: Move project to a path without spaces (recommended for web builds)
# e.g., C:\dev\gmr or /c/dev/gmr
```

**"setup.sh: Permission denied"**
```bash
chmod +x setup.sh build.sh
```

**"MSYS2 not found" (Windows batch files)**
- Install MSYS2 to the default location `C:\msys64`

**Build errors after setup**
```bash
./setup.sh --clean    # Start fresh
```

**Web build: "emcc not found"**
```bash
source env.sh         # Load Emscripten first
./build.sh web
```

## Project Structure

```
gmr/
├── setup.sh          # One-time setup (run this first!)
├── setup.bat         # Windows launcher for setup
├── build.sh          # Build script
├── build.bat         # Windows launcher for build
├── env.sh            # Environment for web builds (created by setup)
├── scripts/
│   └── main.rb       # Your game code goes here!
├── bin/              # Emscripten wrappers (created by setup if needed)
├── deps/             # Downloaded dependencies (created by setup)
└── libs/             # Compiled web libraries (created by setup)
```

## What Setup Does

**On Windows (MSYS2/MinGW64):**
1. Installs build tools, raylib, and mruby via pacman (fast!)
2. Downloads Emscripten SDK for web builds
3. Builds raylib and mruby for WebAssembly
4. Creates `env.sh` for web build environment

**On Linux:**
1. Installs build tools and raylib via apt
2. Builds mruby from source
3. Sets up Emscripten (same as Windows)

First-time setup takes 5-10 minutes for native only, or 15-20 minutes with web support.