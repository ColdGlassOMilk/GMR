# <img src="game/assets/logo.png" height="75" align="middle" /> GMR - Games Made with Ruby

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
gmrcli build -o text
gmrcli run
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

## CLI Quick Reference

```bash
gmrcli setup              # Install dependencies
gmrcli build debug        # Debug build (hot-reload)
gmrcli build release      # Optimized release build
gmrcli build web          # WebAssembly build
gmrcli run                # Run native build
gmrcli run web            # Start local web server
gmrcli new my-game        # Create new project
```

Add `-o text` for colored terminal output (JSON is default).

## API Quick Reference

| Module | Key Functions |
|--------|---------------|
| Graphics | `clear`, `draw_rect`, `draw_circle`, `draw_text` |
| Texture | `load`, `draw`, `draw_ex`, `draw_pro` |
| Tilemap | `new`, `set`, `fill`, `draw_region`, `solid?` |
| Input | `key_down?`, `key_pressed?`, `mouse_x/y`, `map`, `action_down?` |
| Audio | `Sound.load`, `play`, `stop`, `set_volume` |
| Window | `set_size`, `set_title`, `set_fullscreen` |
| Time | `fps`, `elapsed`, `set_target_fps` |
| System | `random_int`, `random`, `platform`, `quit` |
| Collision | `rect_overlap?`, `circle_overlap?`, `point_in_rect?` |

## Documentation

📚 **Reference Documentation** (auto-generated from source)

- [CLI Reference](docs/cli/README.md) - All `gmrcli` commands & options
- [API Reference](docs/api/README.md) - Complete Ruby API documentation

Run `gmrcli docs` to regenerate documentation after modifying source files.

## Developer Console

Press **`** (backtick) while running to open the console:

```ruby
$x = 400              # Modify game state
puts Time.fps         # Debug info
System.quit           # Exit game
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
