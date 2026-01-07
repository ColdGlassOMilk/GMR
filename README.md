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

**Minimal Example** - Moving circle in 15 lines:

```ruby
include GMR

def init
  Window.set_title("My Game")
  Input.map(:move_left, [:a, :left])
  Input.map(:move_right, [:d, :right])
  @x, @y = 400, 300
end

def update(dt)
  speed = 200 * dt
  @x -= speed if Input.action_down?(:move_left)
  @x += speed if Input.action_down?(:move_right)
end

def draw
  Graphics.clear([20, 20, 40])
  Graphics.draw_circle(@x, @y, 20, [100, 200, 255])
end
```

---

## Engine Showcase

### State Machine + Animator

Wire input directly to state transitions. Animator handles sprite changes automatically.

```ruby
include GMR

class Player
  def initialize
    @sprite = Graphics::Sprite.new(Graphics::Texture.load("assets/player.png"))

    # Animator with transition rules
    @animator = Animation::Animator.new(@sprite, frame_width: 48, frame_height: 48)
    @animator.add(:idle, frames: 0..3, fps: 8)
    @animator.add(:run, frames: 8..13, fps: 12)
    @animator.add(:jump, frames: 16..19, fps: 10, loop: false)
    @animator.add(:fall, frames: 24..27, fps: 10)
    @animator.allow_from_any(:idle)  # Can always return to idle
    @animator.allow_transition(:idle, :run)
    @animator.allow_transition(:run, :jump)

    # State machine with input-triggered transitions
    @fsm = state_machine do
      state :idle do
        animate :idle
        on_input :move_left, :running
        on_input :move_right, :running
        on_input :jump, :jumping, if: -> { on_ground? }
      end

      state :running do
        animate :run
        on_input :jump, :jumping, if: -> { on_ground? }
        transition :idle, unless: -> { moving? }
      end

      state :jumping do
        animate :jump
        on_enter { @vy = -420 }
        transition :falling, if: -> { @vy > 0 }
      end

      state :falling do
        animate :fall
        transition :idle, if: -> { on_ground? }
      end
    end
  end
end
```

### Tweens with Chaining

Smooth animations with 36 easing types and chainable callbacks.

```ruby
include GMR

# Landing squash/stretch effect
def on_land
  Animation::Tween.to(@sprite, :scale_y, 0.7, duration: 0.05, ease: :out_quad)
    .on_complete do
      Animation::Tween.to(@sprite, :scale_y, 1.0, duration: 0.15, ease: :out_elastic)
    end
end

# UI slide-in with bounce
def show_menu
  @menu.x = -200
  Animation::Tween.to(@menu, :x, 50, duration: 0.4, ease: :out_back)
end

# Damage flash: white -> original color
def flash_damage
  @sprite.color = [255, 255, 255]
  Animation::Tween.to(@sprite, :color, @original_color, duration: 0.2, ease: :out_quad)
end
```

### Scene Stack with Overlays

Push pause menus, dialogs, or HUDs without destroying game state.

```ruby
include GMR

class GameScene < Scene
  def update(dt)
    return if Console.open?  # Pause when console is open

    @player.update(dt)
    @camera.target = Mathf::Vec2.new(
      Mathf.lerp(@camera.target.x, @player.x, 0.1),
      Mathf.lerp(@camera.target.y, @player.y, 0.08)
    )

    SceneManager.push(PauseMenu.new) if Input.key_pressed?(:escape)
  end
end

class PauseMenu < Scene
  def update(dt)
    SceneManager.pop if Input.key_pressed?(:escape)
    System.quit if Input.key_pressed?(:q)
  end

  def draw
    Graphics.draw_rect(0, 0, 960, 540, [0, 0, 0, 180])
    Graphics.draw_text("PAUSED", 400, 250, 48, [255, 255, 255])
  end
end

# Overlays render on top but don't pause the game
class MinimapOverlay < Scene
  def draw
    # Always visible, doesn't block game updates
    draw_minimap(@game.level, @game.player)
  end
end

SceneManager.add_overlay(MinimapOverlay.new)
```

### Camera with Smoothing and Shake

```ruby
include GMR

@camera = Graphics::Camera2D.new
@camera.offset = Mathf::Vec2.new(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2)
@camera.zoom = 2.0

def update(dt)
  # Smooth follow with different speeds per axis
  target = Mathf::Vec2.new(@player.x, @player.y - 40)
  @camera.target = Mathf::Vec2.new(
    Mathf.lerp(@camera.target.x, target.x, 0.1),
    Mathf.lerp(@camera.target.y, target.y, 0.08)
  )

  # Clamp to level bounds
  @camera.target = Mathf::Vec2.new(
    Mathf.clamp(@camera.target.x, half_w, @level.width - half_w),
    Mathf.clamp(@camera.target.y, half_h, @level.height - half_h)
  )
end

# Shake on impact
def on_player_land
  @camera.shake(strength: 3, duration: 0.1)
end

# Zoom with mouse wheel
wheel = Input.mouse_wheel
if wheel != 0
  Animation::Tween.to(@camera, :zoom, Mathf.clamp(@camera.zoom + wheel * 0.25, 0.5, 4.0),
           duration: 0.15, ease: :out_quad)
end
```

### Live Console Commands

Register debug commands accessible at runtime via backtick (`).

```ruby
Console.register_command("tp", "Teleport to x,y") do |args|
  if args.length >= 2
    @player.teleport(args[0].to_f, args[1].to_f)
    "Teleported to #{args[0]}, #{args[1]}"
  else
    "Usage: tp <x> <y>"
  end
end

Console.register_command("god", "Toggle invincibility") do
  @player.god_mode = !@player.god_mode
  "God mode: #{@player.god_mode ? 'ON' : 'OFF'}"
end

Console.register_command("spawn", "Spawn enemy at cursor") do
  @enemies << Enemy.new(Input.mouse_x, Input.mouse_y)
  "Spawned enemy at #{Input.mouse_x}, #{Input.mouse_y}"
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

All classes/modules live under the `GMR` namespace. Use `include GMR` at the top of your scripts to access them without the prefix.

### GMR::Graphics
| Module/Class | Key Functions |
|--------------|---------------|
| `Graphics` | `clear`, `draw_rect`, `draw_circle`, `draw_line`, `draw_text`, `measure_text` |
| `Graphics::Texture` | `load`, `draw`, `draw_ex`, `draw_pro`, `width`, `height` |
| `Graphics::Sprite` | `new`, `draw`, `x`, `y`, `rotation`, `scale_x/y`, `source_rect`, `alpha` |
| `Graphics::Tilemap` | `new`, `set`, `fill`, `draw_region`, `solid?`, `tile_property` |
| `Graphics::Camera2D` | `target`, `offset`, `zoom`, `follow`, `shake`, `world_to_screen` |
| `Graphics::Transform2D` | `position`, `rotation`, `scale_x/y`, `parent`, `world_position` |
| `Graphics::Rect` | 2D rectangle with `x`, `y`, `w`, `h` |

### GMR::Animation
| Module/Class | Key Functions |
|--------------|---------------|
| `Animation::Tween` | `to`, `pause`, `resume`, `cancel`, `on_complete`, `on_update` |
| `Animation::Ease` | `linear`, `quad_in/out`, `cubic_in/out`, `elastic_out`, `bounce_out` |
| `Animation::Animator` | `new`, `add`, `play`, `stop`, `current`, `playing?`, `allow_transition`, `allow_from_any` |
| `Animation::SpriteAnimation` | `new`, `play`, `pause`, `stop`, `frame`, `fps`, `loop?`, `on_complete` |

### GMR::Audio
| Module/Class | Key Functions |
|--------------|---------------|
| `Audio` | `master_volume`, `mute`, `unmute` |
| `Audio::Sound` | `load`, `play`, `stop`, `pause`, `volume=`, `playing?` |

### GMR::Input
| Module/Class | Key Functions |
|--------------|---------------|
| `Input` | `key_down?`, `key_pressed?`, `mouse_x/y`, `map`, `action_down?` |

### GMR::Core
| Module/Class | Key Functions |
|--------------|---------------|
| `Core::Node` | `add_child`, `remove_child`, `local_position`, `world_position`, `active?` |
| `Core::StateMachine` | `new`, `attach`, `trigger`, `state`, `active?` |

### GMR::Mathf (math utilities)
| Module/Class | Key Functions |
|--------------|---------------|
| `Mathf` | `lerp`, `clamp`, `smoothstep`, `remap`, `distance`, `sign`, `move_toward`, `wrap`, `random_int`, `random_float`, `deg_to_rad`, `rad_to_deg` |
| `Mathf::Vec2` | 2D vector with `x`, `y` and arithmetic operators (+, -, *, /) |
| `Mathf::Vec3` | 3D vector with `x`, `y`, `z` and arithmetic operators |

### GMR (Top-level modules)
| Module/Class | Key Functions |
|--------------|---------------|
| `Scene` | Base class with `init`, `update`, `draw`, `unload` |
| `SceneManager` | `push`, `pop`, `load`, `current`, `add_overlay` |
| `Console` | `enable`, `register_command`, `open?` |
| `Window` | `set_size`, `set_title`, `fullscreen`, `width`, `height` |
| `Time` | `delta`, `fps`, `elapsed`, `scale` |
| `System` | `platform`, `quit`, `build_type`, `gpu_renderer`, `last_error` |
| `Collision` | `rect_overlap?`, `circle_overlap?`, `point_in_rect?`, `rect_tiles` |

### Color
Colors are specified as arrays: `[r, g, b]` or `[r, g, b, a]` with values 0-255.

## Documentation

📚 **Reference Documentation** (auto-generated from source)

- [CLI Reference](docs/cli/README.md) - All `gmrcli` commands & options
- [API Reference](docs/api/README.md) - Complete Ruby API documentation

Run `gmrcli docs` to regenerate documentation after modifying source files.

## Developer Console

Press **`** (backtick) while running to open an interactive Ruby REPL:

```ruby
@player.x = 400                    # Modify game state live
@player.god_mode = true            # Toggle flags
GMR::Time.fps                      # Check performance
@camera.zoom = 3.0                 # Adjust camera
GMR::Animation::Tween.to(@camera, :zoom, 1.0, duration: 0.5)  # Animate changes
GMR::System.quit                   # Exit game
```

Custom commands registered via `Console.register_command` appear in autocomplete.

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
