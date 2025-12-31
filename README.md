# <img src="logo.png" height="75" align="middle" /> GMR - Games Made with Ruby!

A hot-reloadable game engine using mruby and raylib. Write your game logic in Ruby, see changes instantly without recompiling.

[Check out the live Demo on itch.io!](https://coldglassomilk.itch.io/gmr)

![Demo Screenshot](gmr_ss.png)

## Features

- **Hot Reload**: Edit Ruby scripts and see changes instantly
- **Handle-based Resources**: Safe resource management - Ruby never touches raw pointers
- **Virtual Resolution**: Render at retro resolutions, scale up automatically with letterboxing
- **Cross-platform**: Windows, Linux, macOS

## Quick Start

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

1. Open "MSYS2 MinGW64" from Start menu (Or open VS Code -> Ctrl+` -> + New MinGW Session)
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

### Linux (Ubuntu/Debian)

```bash
# Install build tools
sudo apt update
sudo apt install build-essential cmake git ruby bison

# Install raylib dependencies
sudo apt install libasound2-dev libx11-dev libxrandr-dev libxi-dev \
    libgl1-mesa-dev libglu1-mesa-dev libxcursor-dev libxinerama-dev

# Build raylib from source
git clone https://github.com/raysan5/raylib.git
cd raylib && mkdir build && cd build
cmake -DBUILD_SHARED_LIBS=OFF ..
make -j$(nproc)
sudo make install
cd ../..

# Build mruby from source
git clone https://github.com/mruby/mruby.git
cd mruby
make -j$(nproc)
sudo cp build/host/lib/libmruby.a /usr/local/lib/
sudo cp -r include/* /usr/local/include/
cd ..

# Build GMR
cd gmr
cmake -B build
cmake --build build
./gmr
```

### macOS

```bash
# Install Homebrew if needed
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install dependencies
brew install cmake raylib mruby

# Build GMR
cd gmr
cmake -B build
cmake --build build
./gmr
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

## VS Code Setup (Recommended)

### Required Extensions

- **C/C++** (Microsoft) - IntelliSense and debugging
- **CMake Tools** (Microsoft) - CMake integration

### Opening the Project

1. Open VS Code
2. File → Open Folder → Select the `gmr` folder
3. CMake Tools will prompt to configure - select **"MinGW Makefiles"** and your compiler

### Building

- **Ctrl+Shift+B** - Build (default task)
- **F5** - Debug
- **Status bar** (bottom) - Click to select Debug/Release, build, etc.

## Project Structure

```
gmr/
├── CMakeLists.txt              # Build configuration
├── CMakePresets.json           # Build presets
├── include/gmr/                # C++ headers
│   ├── types.hpp               # Core types (Color, Vec2, handles)
│   ├── state.hpp               # Global state
│   ├── engine.hpp              # Master include
│   ├── resources/              # Resource managers
│   ├── bindings/               # mruby bindings
│   └── scripting/              # Script loading
├── src/                        # C++ implementation
├── scripts/                    # Ruby game code
│   └── main.rb                 # Entry point (edit this!)
├── .vscode/                    # VS Code configuration
└── build/                      # Build output (generated)
```

## Ruby API Reference

### Lifecycle

```ruby
def init          # Called once at startup
def update(dt)    # Called every frame (dt = seconds since last frame)
def draw          # Called every frame
```

### Graphics

```ruby
# Colors
set_color(r, g, b)              # Set drawing color (0-255)
set_color(r, g, b, a)           # With alpha
set_color([r, g, b])            # Array form
set_alpha(a)                    # Set alpha only
set_clear_color(r, g, b)        # Set background color

# Screen
clear_screen                    # Clear with set_clear_color
clear_screen(r, g, b)           # Clear with specific color

# Shapes
draw_rect(x, y, width, height)
draw_rect_lines(x, y, width, height)
draw_rect_rotate(x, y, width, height, angle)

draw_circle(x, y, radius)
draw_circle_lines(x, y, radius)
draw_circle_gradient(x, y, radius, [r1,g1,b1,a1], [r2,g2,b2,a2])

draw_line(x1, y1, x2, y2)
draw_line_thick(x1, y1, x2, y2, thickness)

draw_triangle(x1, y1, x2, y2, x3, y3)
draw_triangle_lines(x1, y1, x2, y2, x3, y3)

# Text
draw_text(text, x, y, font_size)
measure_text(text, font_size)   # Returns width in pixels
```

### Textures

```ruby
tex = load_texture("path/to/image.png")
draw_texture(tex, x, y)
draw_texture_ex(tex, x, y, rotation, scale)
draw_texture_pro(tex, src_x, src_y, src_w, src_h, dst_x, dst_y, dst_w, dst_h, rotation)
texture_width(tex)
texture_height(tex)
```

### Input - Keyboard

```ruby
key_down?(key)          # Is key currently held?
key_pressed?(key)       # Was key just pressed this frame?
key_released?(key)      # Was key just released this frame?
get_key_pressed         # Get last key pressed (or nil)
get_char_pressed        # Get last character pressed (or nil)
```

**Common Key Codes:**
| Key | Code | Key | Code |
|-----|------|-----|------|
| LEFT | 263 | RIGHT | 262 |
| UP | 265 | DOWN | 264 |
| SPACE | 32 | ENTER | 257 |
| ESCAPE | 256 | TAB | 258 |
| A-Z | 65-90 | 0-9 | 48-57 |
| F1-F12 | 290-301 | | |

### Input - Mouse

```ruby
mouse_x                     # X position
mouse_y                     # Y position
mouse_down?(button)         # Is button held? (0=left, 1=right, 2=middle)
mouse_pressed?(button)      # Was button just pressed?
mouse_released?(button)     # Was button just released?
mouse_wheel                 # Wheel movement this frame
```

### Audio

```ruby
sound = load_sound("path/to/sound.wav")
play_sound(sound)
stop_sound(sound)
set_sound_volume(sound, volume)  # 0.0 to 1.0
```

### Window

```ruby
screen_width                # Current screen width
screen_height               # Current screen height
set_window_size(w, h)
set_window_title(title)

toggle_fullscreen
set_fullscreen(true/false)
fullscreen?

monitor_count
monitor_width(index)
monitor_height(index)
monitor_refresh_rate(index)
monitor_name(index)

set_target_fps(fps)
get_fps                     # Current FPS
get_time                    # Time since start (seconds)
get_delta_time              # Time since last frame (seconds)
```

### Virtual Resolution

Render at a fixed low resolution, automatically scaled up:

```ruby
set_virtual_resolution(320, 240)    # Retro!
clear_virtual_resolution            # Back to native
virtual_resolution?                 # Is it enabled?
set_filter_point                    # Crisp pixels (default)
set_filter_bilinear                 # Smooth scaling
```

### Utility

```ruby
random_int(min, max)        # Random integer [min, max]
random_float                # Random float [0.0, 1.0)
quit                        # Exit the game
```

## Troubleshooting

### "cmake is not recognized" / "g++ is not recognized"

- Make sure `C:\msys64\mingw64\bin` is in your Windows PATH
- Restart your terminal/VS Code after adding to PATH

### "cannot find -lmruby"

mruby isn't installed. Follow the "Build mruby" steps above.

### "mruby.h: No such file"

mruby headers aren't installed. Make sure you ran:
```bash
cp -r include/* /mingw64/include/
```

### CMake can't find compiler

Specify it explicitly:
```bash
cmake -B build -G "MinGW Makefiles" \
    -DCMAKE_C_COMPILER=C:/msys64/mingw64/bin/gcc.exe \
    -DCMAKE_CXX_COMPILER=C:/msys64/mingw64/bin/g++.exe
```

### Scripts not reloading

- Make sure you're editing files in the `scripts/` folder
- Check the console for Ruby errors
- File must be saved (not just modified)

## License

MIT

