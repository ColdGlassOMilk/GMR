# <img src="game/assets/logo.png" height="75" align="middle" /> GMR - Games Made with Ruby

**A modern, cross-platform game framework combining the elegance of Ruby with the performance of C++.** Build games with hot-reloading Ruby scripts powered by mruby and raylib, deploy natively to Windows/Linux/macOS or compile to WebAssembly for the browser.

[Check out the live Demo on itch.io!](https://coldglassomilk.itch.io/gmr)

> **⚠️ Early Stage Project:** GMR is in active development and not production-ready. The API bindings are currently MVP-level (graphics, input, audio basics), and advanced features like networking, physics, and extended audio are not yet implemented. That said, it's stable enough for experimentation, game jams, and small projects. Contributions welcome!

## Features

- 🔥 **Hot Reload** - Edit Ruby scripts and see changes instantly without recompiling or restarting
- 💻 **Live REPL** - Built-in developer console for executing Ruby code in real-time while your game runs
- 🎮 **Full Game Engine** - Graphics, audio, input, resource management - everything you need
- 🌐 **Web & Native** - Deploy as native executables or compile to WebAssembly for browser play
- 🛡️ **Memory Safe** - Handle-based resources mean Ruby never touches raw pointers
- ⚡ **Fast** - mruby compiles Ruby to bytecode, raylib provides hardware-accelerated rendering
- 📦 **Cross-platform** - Windows, Linux, macOS support out of the box
- 🎯 **Simple API** - Clean, Ruby-style API designed for rapid game development
- 🔧 **Modern Tooling** - `gmrcli` utility for build/run/release, CMake presets, VSCode integration

## Quick Start

### Windows

1. **Install [MSYS2](https://www.msys2.org/)** (default path: `C:\msys64`)

2. **Open MSYS2 MinGW64** terminal and run:
   ```bash
   git clone https://github.com/ColdGlassOMilk/GMR
   cd GMR
   ./bootstrap.sh         # Install gmrcli & run setup
   gmrcli build           # Build the engine
   gmrcli run             # Run the game!
   ```

3. **Edit `scripts/main.rb`** - changes reload automatically while running!

### Linux (Ubuntu/Debian)

```bash
cd gmr
./bootstrap.sh         # Install gmrcli & run setup
gmrcli build           # Build
gmrcli run             # Run
```

### macOS

```bash
cd gmr
./bootstrap.sh         # Install gmrcli & run setup
gmrcli build           # Build
gmrcli run             # Run
```

## CLI Commands

GMR uses the `gmrcli` tool for all operations:

```bash
# Setup
gmrcli setup              # Full setup (native + web)
gmrcli setup --native-only   # Skip web/Emscripten (faster)
gmrcli setup --web-only      # Only web/Emscripten
gmrcli setup --clean         # Clean rebuild everything

# Build
gmrcli build debug        # Debug build (hot-reload enabled)
gmrcli build release      # Optimized release build
gmrcli build web          # WebAssembly build
gmrcli build clean        # Remove build artifacts

# Run
gmrcli run                # Run native build
gmrcli run web            # Start local web server

# Project
gmrcli new my-game        # Create new game project
gmrcli info               # Show environment info
gmrcli help               # Show all commands
```

### CMake Presets (Alternative)

```bash
cmake --preset windows-release && cmake --build build
cmake --preset web-release && cmake --build build-web
```

## Making Your First Game

Edit `scripts/main.rb`:

```ruby
def init
  GMR::Window.set_title("My Game")
  $x, $y = 160, 120
end

def update(dt)
  $x += 100 * dt if GMR::Input.key_down?(:right)
  $x -= 100 * dt if GMR::Input.key_down?(:left)
  $y -= 100 * dt if GMR::Input.key_down?(:up)
  $y += 100 * dt if GMR::Input.key_down?(:down)
end

def draw
  GMR::Graphics.clear([20, 20, 40])
  GMR::Graphics.draw_circle($x, $y, 8, [100, 200, 255])
  GMR::Graphics.draw_text("Arrow keys to move!", 10, 10, 20, [255, 255, 255])
end
```

Save the file and watch it reload instantly!

### Live REPL / Dev Console

Press **`** (backtick/tilde key) to open the developer console while your game is running. You can execute Ruby code in real-time:

```ruby
$x = 160                  # Teleport player
$speed = 200              # Change speed (if you add a $speed variable)
puts GMR::Time.fps        # Print current FPS
GMR::System.quit          # Exit the game
```

This is perfect for tweaking values, debugging, and experimenting without reloading. Any global variables or functions defined in your scripts are accessible!

## API Reference

GMR uses a clean, stateless API organized into modules. Colors are always `[r, g, b]` or `[r, g, b, a]` arrays (0-255).

### Lifecycle Hooks

```ruby
def init          # Called once at startup
def update(dt)    # Called every frame (dt = delta time in seconds)
def draw          # Called every frame for rendering
```

### GMR::Graphics

```ruby
# Clear screen
GMR::Graphics.clear([r, g, b])

# Shapes (color is always last)
GMR::Graphics.draw_rect(x, y, w, h, color)
GMR::Graphics.draw_rect_outline(x, y, w, h, color)
GMR::Graphics.draw_circle(x, y, radius, color)
GMR::Graphics.draw_circle_outline(x, y, radius, color)
GMR::Graphics.draw_line(x1, y1, x2, y2, color)
GMR::Graphics.draw_triangle(x1, y1, x2, y2, x3, y3, color)

# Text
GMR::Graphics.draw_text(text, x, y, size, color)
GMR::Graphics.measure_text(text, size)  # Returns width in pixels
```

### GMR::Graphics::Texture

```ruby
# Load and draw images
sprite = GMR::Graphics::Texture.load("assets/player.png")
sprite.width                              # Texture dimensions
sprite.height
sprite.draw(x, y)                         # Draw at position
sprite.draw(x, y, [255, 0, 0])            # Draw with tint
sprite.draw_ex(x, y, rotation, scale, color)
```

### GMR::Input

Use readable symbols for keys and mouse buttons:

```ruby
# Keyboard
GMR::Input.key_down?(:space)      # Currently held?
GMR::Input.key_pressed?(:escape)  # Just pressed this frame?
GMR::Input.key_released?(:enter)  # Just released?

# Mouse
GMR::Input.mouse_x                # Position (virtual resolution aware)
GMR::Input.mouse_y
GMR::Input.mouse_down?(:left)     # :left, :right, :middle
GMR::Input.mouse_wheel            # Scroll wheel delta
```

**Supported symbols:** `:space`, `:escape`, `:enter`, `:up`, `:down`, `:left`, `:right`, `:a` through `:z`, `:0` through `:9`, `:f1` through `:f12`

### GMR::Audio::Sound

```ruby
sfx = GMR::Audio::Sound.load("assets/jump.wav")
sfx.play
sfx.stop
sfx.volume = 0.5  # 0.0 to 1.0
```

### GMR::Window

```ruby
GMR::Window.width                 # Screen dimensions
GMR::Window.height
GMR::Window.set_size(800, 600)
GMR::Window.set_title("My Game")
GMR::Window.toggle_fullscreen
GMR::Window.fullscreen?
```

### Virtual Resolution

Render at a fixed low resolution, automatically scaled with pixel-perfect letterboxing:

```ruby
GMR::Window.set_virtual_resolution(320, 240)  # Retro 4:3
GMR::Window.clear_virtual_resolution          # Back to native
GMR::Window.set_filter_point                  # Crisp pixels (default)
GMR::Window.set_filter_bilinear               # Smooth scaling
```

### GMR::Time

```ruby
GMR::Time.delta           # Seconds since last frame
GMR::Time.elapsed         # Seconds since start
GMR::Time.fps             # Current FPS
GMR::Time.set_target_fps(60)
```

### GMR::System

```ruby
GMR::System.quit                    # Exit game
GMR::System.random_int(1, 100)      # Random integer
GMR::System.random_float            # Random 0.0 to 1.0
GMR::System.platform                # "windows", "linux", "macos", "web"
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
gmrcli build release   # Compiles scripts to bytecode
gmrcli build web       # Web builds always use bytecode
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

**"gmrcli: command not found"**
- Run `source ~/.bashrc` to reload your shell
- Or open a new terminal window

**"cmake is not recognized"**
- Run `gmrcli setup` to install dependencies
- Or add `C:\msys64\mingw64\bin` to Windows PATH

**"cannot find -lmruby" or "mruby.h: No such file"**
- Run `gmrcli setup` to install dependencies

**Scripts not hot-reloading**
- Only works in debug builds (not release/web)
- Make sure file is saved
- Check console for Ruby syntax errors

**Web build fails**
- Run `gmrcli setup` to install Emscripten
- Ensure setup completed without `--native-only`

**Setup taking too long**
- Use `gmrcli setup --native-only` to skip web/Emscripten
- Web setup downloads ~1GB and takes 10-20 minutes

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
