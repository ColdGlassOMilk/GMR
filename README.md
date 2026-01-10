# <img src="game/assets/logo.png" height="75" align="middle" /> GMR - Game Middleware for Ruby

**A modern, cross-platform game framework combining the elegance of Ruby with the performance of C++.** Build games with hot-reloading Ruby scripts powered by mruby and raylib, deploy natively to Windows/Linux/macOS or compile to WebAssembly for the browser.

[Live Demo on itch.io](https://coldglassomilk.itch.io/gmr)

> **Early Stage Project:** GMR is in active development and not production-ready. The API bindings are currently MVP-level (graphics, input, audio basics), and advanced features like networking, physics, and extended audio are not yet implemented. Stable enough for experimentation, game jams, and small projects. Contributions welcome!

## Features

- **Instant Hot Reload** - Save your Ruby script, see changes immediately
- **Live REPL Console** - Press backtick to open a console mid-game
- **One Codebase, Every Platform** - Windows, Linux, macOS, or WebAssembly
- **Ruby Simplicity, Native Speed** - mruby bytecode with raylib rendering
- **Resolution Independence** - World-space coordinates, auto-scaling UI, retro and HD modes
- **Batteries Included** - Graphics, tilemaps, audio, input, collision detection
- **Safe by Design** - Handle-based resource system, no raw pointers in Ruby
- **Modern CLI Tooling** - `gmrcli dev` to build and run

---

## Quick Start

```bash
git clone https://github.com/ColdGlassOMilk/GMR
cd GMR
./bootstrap.sh
gmrcli dev
```

Edit `game/scripts/main.rb` - changes reload automatically.

---

## Minimal Example

A moving circle in 15 lines:

```ruby
include GMR

def init
  Window.set_title("My Game")
  input do |i|
    i.move_left [:a, :left]
    i.move_right [:d, :right]
  end
  @x, @y = 400, 300
end

def update(dt)
  speed = 200 * dt
  @x -= speed if Input.action_down?(:move_left)
  @x += speed if Input.action_down?(:move_right)
end

def draw
  Graphics.clear(:dark_gray)
  Graphics.draw_circle(@x, @y, 20, :cyan)
end
```

---

## Animated Sprite Example

Building on the minimal example, add a sprite with animation:

```ruby
include GMR

def init
  Window.set_title("Animated Sprite")

  input do |i|
    i.move_left [:a, :left]
    i.move_right [:d, :right]
  end

  @transform = Transform2D.new(x: 400, y: 300)
  @texture = Texture.load("player.png")
  @sprite = Sprite.new(@texture, @transform)
  @sprite.center_origin

  @anim = SpriteAnimation.new(@sprite,
    frames: 0..3,
    fps: 8,
    columns: 8
  )
  @anim.play

  @facing = 1
end

def update(dt)
  speed = 200 * dt

  if Input.action_down?(:move_left)
    @transform.x -= speed
    @facing = -1
  end

  if Input.action_down?(:move_right)
    @transform.x += speed
    @facing = 1
  end

  @sprite.flip_x = @facing < 0
end

def draw
  Graphics.clear("#141428")
  @sprite.draw
end
```

This example introduces:
- **Transform2D** - Position, rotation, scale, and origin
- **Texture.load** - Load textures from assets
- **Sprite** - Requires a texture and transform
- **SpriteAnimation** - Frame-based spritesheet animation
- **Color formats** - Hex strings like `"#141428"`

---

## Documentation

### Concepts & Guides

| Document | Description |
|----------|-------------|
| [Engine Model](docs/engine-model.md) | Execution model, lifecycle guarantees, invariants |
| [Game Loop](docs/game-loop.md) | Global hooks, frame timing, hot-reload |
| [Scenes](docs/scenes.md) | Scene classes, SceneManager, overlays |
| [Input](docs/input.md) | Action mapping, raw input, contexts |
| [Graphics](docs/graphics.md) | Drawing, textures, sprites, tilemaps |
| [Transforms](docs/transforms.md) | Transform2D, hierarchy, world coordinates |
| [Camera](docs/camera.md) | World-space rendering, resolution independence, following |
| [Animation](docs/animation.md) | SpriteAnimation, Animator, Tweens |
| [State Machine](docs/state-machine.md) | State DSL, transitions, animation binding |
| [Audio](docs/audio.md) | Sound effects, music streaming |
| [Persistence](docs/persistence.md) | File I/O, Storage, save data |
| [Console](docs/console.md) | Developer console, custom commands |
| [Platformer Tutorial](docs/platformer-tutorial.md) | Complete working example |
| [Getting Started](docs/getting-started.md) | Detailed setup guide |

### Reference (Auto-Generated)

| Document | Description |
|----------|-------------|
| [API Reference](docs/api/README.md) | Complete Ruby API documentation |
| [CLI Reference](docs/cli/README.md) | All `gmrcli` commands and options |

Run `gmrcli docs` to regenerate reference documentation.

---

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
