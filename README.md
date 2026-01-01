# <img src="logo.png" height="75" align="middle" /> GMR - Games Made with Ruby

**A modern, cross-platform game framework combining the elegance of Ruby with the performance of C++.** Build games with hot-reloading Ruby scripts powered by mruby and raylib, deploy natively to Windows/Linux/macOS or compile to WebAssembly for the browser.

[Check out the live Demo on itch.io!](https://coldglassomilk.itch.io/gmr)

> **⚠️ Early Stage Project:** GMR is in active development and not production-ready. The API bindings are currently MVP-level (graphics, input, audio basics), and advanced features like networking, physics, and extended audio are not yet implemented. That said, it's stable enough for experimentation, game jams, and small projects. Contributions welcome!

## Features

- **🔥 Hot Reload** - Edit Ruby scripts and see changes instantly without recompiling or restarting
- **💻 Live REPL** - Built-in developer console for executing Ruby code in real-time while your game runs
- **🎮 Full Game Engine** - Graphics, audio, input, resource management - everything you need
- **🌐 Web & Native** - Deploy as native executables or compile to WebAssembly for browser play
- **🛡️ Memory Safe** - Handle-based resources mean Ruby never touches raw pointers
- **⚡ Fast** - mruby compiles Ruby to bytecode, raylib provides hardware-accelerated rendering
- **📦 Cross-platform** - Windows, Linux, macOS support out of the box
- **🎯 Simple API** - Clean, intuitive Ruby API designed for rapid game development
- **🔧 Modern Tooling** - CMake presets, VSCode integration, batch/shell scripts

## Quick Start

### Windows

1. **Install [MSYS2](https://www.msys2.org/)** (default path: `C:\msys64`)

2. **Open MSYS2 MinGW64** terminal and run:
   ```bash
   cd /c/path/to/gmr
   ./setup.sh              # Full setup (native + web)
   ./build.sh run          # Build and run!
   ```

3. **Edit `scripts/main.rb`** - changes reload automatically while running!

### Linux (Ubuntu/Debian)

```bash
cd gmr
./setup.sh              # Automated setup
./build.sh run          # Build and run
```

### macOS

```bash
brew install cmake raylib mruby
cd gmr
cmake -B build && cmake --build build
./gmr
```

## Build Options

```bash
./setup.sh              # Full setup (native + web)
./setup.sh --native-only   # Skip web/Emscripten (faster)
./setup.sh --web-only      # Only web libraries

./build.sh debug        # Debug build
./build.sh release      # Optimized release
./build.sh web          # WebAssembly build
./build.sh run          # Build and run debug
./build.sh serve        # Build web & start server

# Or use CMake presets directly:
cmake --preset windows-release && cmake --build build
cmake --preset web-release && cmake --build build-web
```

## Making Your First Game

Edit `scripts/main.rb`:

```ruby
def init
  set_window_title("My Awesome Game")
  set_virtual_resolution(320, 240)  # Retro pixel-perfect rendering
  $player_x = 160
  $player_y = 120
  $speed = 100
end

def update(dt)
  # Movement
  $player_x += $speed * dt if key_down?(262)  # Right
  $player_x -= $speed * dt if key_down?(263)  # Left
  $player_y -= $speed * dt if key_down?(265)  # Up
  $player_y += $speed * dt if key_down?(264)  # Down
end

def draw
  clear_screen(20, 20, 40)

  # Draw player
  set_color(100, 200, 255)
  draw_circle($player_x.to_i, $player_y.to_i, 8)

  # Draw UI
  set_color(255, 255, 255)
  draw_text("FPS: #{get_fps}", 10, 10, 20)
end
```

Save the file and watch it reload instantly!

### Live REPL / Dev Console

Press **`** (backtick/tilde key) to open the developer console while your game is running. You can execute Ruby code in real-time:

```ruby
# Try these in the console:
$speed = 200              # Make the player move faster
$player_x = 160          # Teleport player to center
puts "Speed: #{$speed}"  # Print to console
quit                     # Exit the game
```

This is perfect for tweaking values, debugging, and experimenting without reloading. Any global variables or functions defined in your scripts are accessible!

## API Reference

### Lifecycle Hooks

```ruby
def init          # Called once at startup
def update(dt)    # Called every frame (dt = delta time in seconds)
def draw          # Called every frame for rendering
```

### Graphics - Drawing

```ruby
# Screen
clear_screen                    # Clear with background color
clear_screen(r, g, b)           # Clear with specific color
set_clear_color(r, g, b)        # Set background color

# Colors
set_color(r, g, b)              # RGB (0-255)
set_color(r, g, b, a)           # RGBA
set_color([r, g, b])            # Array form
set_alpha(a)                    # Set alpha only

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

### Graphics - Textures

```ruby
tex = load_texture("assets/sprite.png")
draw_texture(tex, x, y)
draw_texture_ex(tex, x, y, rotation, scale)
draw_texture_pro(tex, src_x, src_y, src_w, src_h,
                 dst_x, dst_y, dst_w, dst_h, rotation)
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

**Key Codes:**
| Key | Code | Key | Code |
|-----|------|-----|------|
| LEFT | 263 | RIGHT | 262 |
| UP | 265 | DOWN | 264 |
| SPACE | 32 | ENTER | 257 |
| ESC | 256 | TAB | 258 |
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
sound = load_sound("assets/jump.wav")
play_sound(sound)
stop_sound(sound)
set_sound_volume(sound, volume)  # 0.0 to 1.0
```

### Window & Display

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

Render at a fixed low resolution, automatically scaled up with pixel-perfect letterboxing:

```ruby
set_virtual_resolution(320, 240)    # Retro 4:3
set_virtual_resolution(320, 180)    # Retro 16:9
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

## VSCode Integration

### Recommended Extensions
- **C/C++** (Microsoft) - IntelliSense and debugging
- **CMake Tools** (Microsoft) - Build system integration

### Usage
1. Open folder in VSCode
2. **Ctrl+Shift+B** - Build
3. **Ctrl+Shift+P** → "Tasks: Run Task" → "Run: Debug" / "Serve: Web"
4. Edit `scripts/main.rb` and save to hot-reload

## Advanced Topics

### Bytecode Compilation

Release and web builds automatically compile Ruby scripts to bytecode for faster startup and smaller size:

```bash
./build.sh release      # Compiles scripts to bytecode
./build.sh web          # Web builds always use bytecode
```

Debug builds load scripts from disk for hot-reloading.

### Project Organization

```
gmr/
├── scripts/            # Your game code (Ruby)
│   └── main.rb        # Entry point
├── assets/            # Images, sounds, etc.
├── src/               # Engine C++ code
├── include/gmr/       # Engine headers
├── CMakeLists.txt     # Build configuration
├── CMakePresets.json  # Build presets
└── deps/              # Dependencies (built by setup.sh)
    ├── raylib/
    └── mruby/
```

## Troubleshooting

**"cmake is not recognized"**
- Add `C:\msys64\mingw64\bin` to Windows PATH
- Restart terminal/VSCode

**"cannot find -lmruby" or "mruby.h: No such file"**
- Run `./setup.sh` to install dependencies

**Scripts not hot-reloading**
- Only works in debug builds (not release/web)
- Make sure file is saved
- Check console for Ruby syntax errors

**Web build fails**
- Run `source env.sh` before building
- Or use `./build.sh web` which sources it automatically

## License

MIT License - completely free and open source, no royalties, no restrictions.

Copyright (c) 2025 GMR Contributors

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
