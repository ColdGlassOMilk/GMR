# <img src="game/assets/logo.png" height="75" align="middle" /> GMR - Game Middleware for Ruby

**A modern, cross-platform game framework combining the elegance of Ruby with the performance of C++.** Build games with hot-reloading Ruby scripts powered by mruby and raylib, deploy natively to Windows/Linux/macOS or compile to WebAssembly for the browser.

[Check out the live Demo on itch.io!](https://coldglassomilk.itch.io/gmr)

> **⚠️ Early Stage Project:** GMR is in active development and not production-ready. The API bindings are currently MVP-level (graphics, input, audio basics), and advanced features like networking, physics, and extended audio are not yet implemented. That said, it's stable enough for experimentation, game jams, and small projects. Contributions welcome!

## Features

- 🔥 **Instant Hot Reload** - Save your Ruby script, see changes immediately. No compile step, no restart, no lost game state
- 💻 **Live REPL Console** - Press backtick to open a console mid-game. Tweak variables, test functions, debug in real-time
- 🌐 **One Codebase, Every Platform** - Write once, deploy to Windows, Linux, macOS, or the web via WebAssembly
- ⚡ **Ruby Simplicity, Native Speed** - mruby bytecode execution with raylib's hardware-accelerated rendering
- 🎮 **Batteries Included** - Graphics, tilemaps, audio, input mapping, collision detection - no external dependencies to wire up
- 🛡️ **Safe by Design** - Handle-based resource system keeps Ruby away from raw pointers and memory headaches
- 🔧 **Modern CLI Tooling** - `gmrcli setup && gmrcli run` - that's it. JSON output for CI/IDE integration, presets for power users

## Quick Start

```bash
# Clone and setup
git clone https://github.com/ColdGlassOMilk/GMR
cd GMR
./bootstrap.sh

# Build and run
gmrcli dev
```

Edit `game/scripts/main.rb` - changes reload automatically!

```ruby
include GMR

def init
  Window.set_title("My Game")
  $x, $y = 400, 300
end

def update(dt)
  speed = 200
  $x -= speed * dt if Input.key_down?(:left)
  $x += speed * dt if Input.key_down?(:right)
  $y -= speed * dt if Input.key_down?(:up)
  $y += speed * dt if Input.key_down?(:down)
end

def draw
  Graphics.clear([20, 20, 40])
  Graphics.draw_circle($x, $y, 16, [100, 200, 255])
  Graphics.draw_text("Arrow keys to move!", 10, 10, 20, [255, 255, 255])
end
```

## CLI

All GMR operations go through `gmrcli`. It outputs structured JSON by default, making it straightforward to integrate with editors, CI systems, and custom tooling. For terminal use, pass `-o text` for human-readable output, or use the `dev` command which defaults to text mode.

```bash
# Quick development (recommended)
gmrcli dev                # Build and run
gmrcli dev --clean        # Fresh build and run
gmrcli dev web            # Web build and local server

# Fine-grained control
gmrcli build debug        # Debug build only
gmrcli build release      # Optimized release build
gmrcli build web          # WebAssembly build only
gmrcli run                # Run existing build
gmrcli run web            # Start web server

# Project management
gmrcli setup              # Install dependencies
gmrcli new my-game        # Create new project
gmrcli docs               # Generate documentation
```

## API Quick Reference

| Module/Class | Key Functions |
|--------------|---------------|
| Graphics | `clear`, `draw_rect`, `draw_circle`, `draw_line`, `draw_text`, `measure_text` |
| Texture | `load`, `draw`, `draw_ex`, `draw_pro`, `width`, `height` |
| Sprite | `new`, `draw`, `position`, `rotation`, `scale_x/y`, `source_rect`, `alpha` |
| Tilemap | `new`, `set`, `fill`, `draw_region`, `solid?`, `tile_property` |
| Camera2D | `target`, `offset`, `zoom`, `follow`, `shake`, `world_to_screen` |
| Node | `add_child`, `remove_child`, `local_position`, `world_position`, `active?` |
| Scene | `init`, `update`, `draw`, `unload` + `SceneManager.push/pop/load` |
| Transform2D | `position`, `rotation`, `scale_x/y`, `parent`, `world_position` |
| Input | `key_down?`, `key_pressed?`, `mouse_x/y`, `map`, `action_down?` |
| Audio | `Sound.load`, `play`, `stop`, `volume=` |
| Window | `set_size`, `set_title`, `fullscreen`, `width`, `height` |
| System | `random_int`, `random_float`, `platform`, `quit`, `build_type` |
| Collision | `rect_overlap?`, `circle_overlap?`, `point_in_rect?`, `distance` |
| Vec2/Vec3 | `new`, `x`, `y`, `+`, `-`, `*`, `/` |
| Rect | `new`, `x`, `y`, `w`, `h` |

## Documentation

📚 **Reference Documentation** (auto-generated from source)

- [CLI Reference](docs/cli/README.md) - All `gmrcli` commands & options
- [API Reference](docs/api/README.md) - Complete Ruby API documentation

Run `gmrcli docs` to regenerate documentation after modifying source files.

## Developer Console

Press **`** (backtick) while running to open the console:

```ruby
$x = 400              # Modify game state
puts GMR::Time.fps         # Debug info
GMR::System.quit           # Exit game
```

## License

MIT License - completely free and open source, no royalties, no restrictions.

Copyright (c) 2026 GMR Contributors

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
