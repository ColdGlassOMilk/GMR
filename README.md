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
   gmrcli build -o text   # Build the engine
   gmrcli run             # Run the game!
   ```

3. **Edit `scripts/main.rb`** - changes reload automatically while running!

> **Note:** `gmrcli` outputs JSON by default for IDE/automation use. Add `-o text` for traditional colored text output in the terminal.

### Linux (Ubuntu/Debian)

```bash
cd gmr
./bootstrap.sh             # Install gmrcli & run setup
gmrcli build -o text       # Build
gmrcli run                 # Run
```

### macOS

```bash
cd gmr
./bootstrap.sh             # Install gmrcli & run setup
gmrcli build -o text       # Build
gmrcli run                 # Run
```

## CLI Commands

GMR uses the `gmrcli` tool for all operations. The CLI is **machine-first** by default, outputting structured JSON for IDE and automation integration. Use `-o text` for traditional text output.

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
gmrcli version            # Show version and protocol info
gmrcli help               # Show all commands
```

### Output Modes

```bash
# Machine-readable JSON (default)
gmrcli build debug            # Outputs NDJSON events + final JSON envelope

# Human-readable text
gmrcli build debug -o text    # Colored text output with spinners

# Protocol versioning (for stable integrations)
gmrcli build debug --protocol-version v1

# Environment variable override
GMRCLI_OUTPUT=text gmrcli build debug   # Force text output
```

### JSON Output Format

All commands emit structured JSON following a stable protocol:

**Success envelope:**
```json
{
  "protocol": "v1",
  "status": "success",
  "command": "build",
  "result": {
    "target": "debug",
    "output_path": "/path/to/release/gmr.exe",
    "artifacts": [{"type": "executable", "path": "..."}]
  },
  "metadata": {
    "timestamp": "2024-01-15T10:30:45.123Z",
    "duration_ms": 1234,
    "gmrcli_version": "0.1.0"
  }
}
```

**Error envelope:**
```json
{
  "protocol": "v1",
  "status": "error",
  "command": "build",
  "error": {
    "code": "BUILD.CMAKE_FAILED",
    "message": "CMake configuration failed",
    "suggestions": ["Run 'gmrcli setup' if you haven't already"]
  }
}
```

**Exit codes:**
- `0` - Success
- `1` - Generic/internal error
- `2` - Protocol error
- `10-19` - Setup errors
- `20-29` - Build errors
- `30-39` - Run errors
- `40-49` - Project errors
- `50-59` - Platform errors
- `130` - User interrupt (Ctrl+C)

### CMake Presets (Alternative)

```bash
cmake --preset windows-release && cmake --build build
cmake --preset web-release && cmake --build build-web
```

## Making Your First Game

Edit `scripts/main.rb`:

```ruby
include GMR

def init
  Window.set_title("My Game")
  $x, $y = 160, 120
end

def update(dt)
  $x += 100 * dt if Input.key_down?(:right)
  $x -= 100 * dt if Input.key_down?(:left)
  $y -= 100 * dt if Input.key_down?(:up)
  $y += 100 * dt if Input.key_down?(:down)
end

def draw
  Graphics.clear([20, 20, 40])
  Graphics.draw_circle($x, $y, 8, [100, 200, 255])
  Graphics.draw_text("Arrow keys to move!", 10, 10, 20, [255, 255, 255])
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
sprite.draw(x, y, [255, 0, 0])            # Draw with color tint
sprite.draw_ex(x, y, rotation, scale)     # Draw with rotation and scale
sprite.draw_pro(sx, sy, sw, sh, dx, dy, dw, dh, rotation)  # Advanced: source rect to dest rect
```

### GMR::Graphics::Tilemap

Efficient tile-based rendering for large worlds. Uses a tileset texture where tiles are arranged in a grid.

```ruby
# Create a tilemap
tileset = GMR::Graphics::Texture.load("assets/tiles.png")
map = GMR::Graphics::Tilemap.new(
  tileset,    # Tileset texture
  16,         # Tile width in pixels
  16,         # Tile height in pixels
  100,        # Map width in tiles
  50          # Map height in tiles
)

# Set tiles (tile indices start at 0, -1 = empty)
map.set(10, 5, 3)                         # Set tile at (10, 5) to index 3
map.fill(0)                               # Fill entire map with tile 0
map.fill_rect(5, 5, 10, 10, 1)            # Fill rectangular region

# Query tiles
tile = map.get(10, 5)                     # Get tile index at position
map.width                                 # Map dimensions in tiles
map.height
map.tile_width                            # Tile dimensions in pixels
map.tile_height

# Draw
map.draw(0, 0)                            # Draw entire map at position
map.draw(0, 0, [255, 255, 255])           # Draw with color tint

# Draw visible region (for scrolling/culling)
map.draw_region(0, 0, cam_x / 16, cam_y / 16, 21, 16)

# Define tile properties (once per tile type, not per map cell)
map.define_tile(0, { solid: false })                  # sky
map.define_tile(4, { solid: true })                   # grass
map.define_tile(8, { solid: true, hazard: true, damage: 10 })  # spikes
map.define_tile(12, { solid: false, water: true })    # water

# Query tile properties at map position
map.tile_properties(10, 5)            # => { solid: true, ... } or nil
map.tile_property(10, 5, :solid)      # => true/false/nil

# Convenience methods for common checks
map.solid?(10, 5)                     # Check :solid property
map.wall?(10, 5)                      # Alias for solid?
map.hazard?(10, 5)                    # Check :hazard property
map.platform?(10, 5)                  # Check :platform property
map.ladder?(10, 5)                    # Check :ladder property
map.water?(10, 5)                     # Check :water property
map.slippery?(10, 5)                  # Check :slippery property
map.damage(10, 5)                     # Get damage value (0 if not defined)
```

**Tile indexing:** Tiles in the tileset are numbered left-to-right, top-to-bottom starting from 0. For a 64x64 tileset with 16x16 tiles, indices are: 0-3 (first row), 4-7 (second row), etc.

**Tile properties:** Properties are defined per tile *type* (index), not per map cell. This is memory-efficient - a 100x100 map with 10,000 cells only stores properties once per unique tile type used.

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
GMR::System.build_type              # "debug" or "release"

# Error state
GMR::System.in_error_state?         # Check if script error occurred
GMR::System.last_error              # Get error details hash (or nil)
```

### Error Handling

Script errors include full file/line information and are deduplicated (won't spam the console). Error state is cleared on hot reload.

```ruby
if GMR::System.in_error_state?
  err = GMR::System.last_error
  puts "#{err[:class]}: #{err[:message]}"
  puts "  at #{err[:file]}:#{err[:line]}"
  err[:backtrace].each { |line| puts "    #{line}" }
end
```

The error hash contains:
- `:class` - Exception type (e.g., "NoMethodError")
- `:message` - Error message
- `:file` - Source file name
- `:line` - Line number
- `:backtrace` - Array of stack trace entries

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
├── engine/language/   # Language metadata for IDE integration
├── CMakeLists.txt     # Build configuration
├── CMakePresets.json  # Build presets
└── deps/              # Dependencies (built by setup.sh)
    ├── raylib/
    └── mruby/
```

### Language Metadata (IDE Integration)

GMR exports machine-readable language metadata for IDE/editor integration. These files enable syntax highlighting and IntelliSense for GMR Ruby scripts:

```
engine/language/
├── syntax.json    # Lexical features (keywords, operators, constants)
├── api.json       # Semantic API (function signatures, params, docs)
└── version.json   # Engine and language version info
```

**syntax.json** - Token-level data for syntax highlighting:
- Ruby keywords and operators
- GMR module names (`GMR::Graphics`, etc.)
- Input constants (`KEY_SPACE`, `MOUSE_LEFT`, etc.)
- Lifecycle hooks (`init`, `update`, `draw`)

**api.json** - Semantic data for IntelliSense/autocomplete:
- Function signatures with parameter types
- Return types and documentation
- Code examples for each function

**version.json** - Compatibility information:
- Engine version, mruby version, raylib version
- Schema versions for syntax/api files

These files are pure data (no executable code) and can be consumed by any editor. See the files for the complete JSON schema.

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
- Check console for Ruby syntax errors (errors now show file:line info)

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
