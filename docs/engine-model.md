# Engine Model

This document describes GMR's execution model, lifecycle guarantees, and architectural invariants. Understanding these rules helps you write predictable, maintainable game code.

## Execution Model

### Frame Order

Each frame executes in this order:

1. **Input polling** - Engine captures keyboard, mouse, gamepad state
2. **`update(dt)`** - Your game logic runs
3. **`draw`** - Your rendering code runs
4. **Present** - Frame buffer is displayed

This order is guaranteed. Input state is always fresh when `update` runs. All rendering happens after game logic.

### The `dt` Parameter

`update(dt)` receives `dt` (delta time) - the elapsed time since the last frame in seconds as a float.

```ruby
def update(dt)
  # dt is typically 0.016 at 60 FPS
  @player.x += @speed * dt  # Frame-rate independent movement
end
```

Multiply velocities and time-based values by `dt` for consistent behavior across different frame rates.

### Global Hooks vs Scene Classes

GMR supports two patterns for structuring games:

**Global Hooks** - Define `init`, `update(dt)`, and `draw` at the top level:

```ruby
include GMR

def init
  @x = 400
end

def update(dt)
  @x += 100 * dt
end

def draw
  Graphics.clear(:black)
end
```

Use global hooks for prototypes, game jams, and small games.

**Scene Classes** - Encapsulate state in Scene subclasses managed by SceneManager:

```ruby
class GameScene < Scene
  def init
    @x = 400
  end

  def update(dt)
    @x += 100 * dt
  end

  def draw
    Graphics.clear(:black)
  end
end

SceneManager.load(GameScene.new)
```

Use scenes for larger games with multiple screens (title, gameplay, pause, game over).

## Lifecycle Guarantees

### Global Hooks

| Hook | When Called |
|------|-------------|
| `init` | Once at startup, and once on each hot-reload |
| `update(dt)` | Every frame, before `draw` |
| `draw` | Every frame, after `update` |

### Scene Lifecycle

| Method | When Called |
|--------|-------------|
| `init` | When the scene becomes active (via `load` or `push`) |
| `update(dt)` | Every frame while this scene is on top of the stack |
| `draw` | Every frame while this scene is on top of the stack |
| `unload` | When the scene is removed from the stack |

### Scene Stack Behavior

- Only the **top scene** receives `update` and `draw` calls
- Scenes below the top are paused but retain their state
- `SceneManager.push` pauses the current scene and activates the new one
- `SceneManager.pop` removes the top scene (calls `unload`) and resumes the one below
- `SceneManager.load` clears the entire stack and starts fresh

### Overlays

Overlays are special scenes that render on top without pausing the scene below:

```ruby
SceneManager.add_overlay(HUD.new)
```

- Overlays receive `draw` calls
- The scene beneath continues receiving `update` and `draw`
- Use overlays for HUDs, minimaps, and notifications

## Resource Management

### Handle-Based Architecture

GMR uses handles to manage native resources. Ruby code never holds raw pointers.

```ruby
@texture = Texture.load("player.png")  # Returns a handle
@sprite = Sprite.new(@texture, @transform)  # Sprite holds texture handle
```

The engine tracks resource lifetimes. When handles are no longer referenced, resources are cleaned up.

### Texture Caching

`Texture.load` caches textures by path. Calling it multiple times with the same path returns the same handle:

```ruby
tex1 = Texture.load("player.png")
tex2 = Texture.load("player.png")
# tex1 and tex2 reference the same underlying texture
```

### Sprite Requirements

Sprites require a Transform2D for spatial properties:

```ruby
@transform = Transform2D.new(x: 100, y: 100)
@sprite = Sprite.new(@texture, @transform)  # Transform is required
```

This separates spatial data (position, rotation, scale) from rendering data (texture, color, flip).

## Hot-Reload Behavior

During development, GMR watches for script changes and reloads automatically.

### What Survives Reload

| Data | Survives? | Notes |
|------|-----------|-------|
| Instance variables (`@x`) | Yes | Preserved across reload |
| Local variables | No | Lost on reload |
| Class definitions | Re-evaluated | Classes are redefined |
| Resource handles | Yes | If stored in instance variables |

### The `init` Hook

`init` is called again on each hot-reload. Use it for re-initialization:

```ruby
def init
  # This runs at startup AND on every reload
  @speed = 200  # Configuration that might change during development

  # Only create new resources if they don't exist
  @texture ||= Texture.load("player.png")
end
```

### Preserving State

Use `||=` to preserve existing state across reloads:

```ruby
def init
  @player ||= Player.new(400, 300)  # Keep existing player
  @score ||= 0                       # Keep existing score
end
```

## Coordinate System

GMR uses a standard 2D screen coordinate system:

- **Origin (0, 0)** is at the top-left corner
- **X** increases to the right
- **Y** increases downward
- **Rotation** is in degrees, clockwise positive
- **Transform2D origin** is the pivot point for rotation and scale

```
(0,0) ────────────► X+
  │
  │
  │
  ▼
  Y+
```

## DSL Conventions

### Input DSL

The `input` block is syntactic sugar for `Input.map`:

```ruby
# Block syntax
input do |i|
  i.jump [:space, :w]
end

# Equivalent to:
Input.map(:jump, [:space, :w])
```

### Input Contexts

`input_context` defines named contexts for mode switching:

```ruby
input_context :menu do |i|
  i.confirm :enter
  i.cancel :escape
end

# Later:
Input.push_context(:menu)
Input.pop_context
```

### State Machine DSL

`state_machine` defines states on the enclosing object:

```ruby
class Player
  def initialize
    state_machine do
      state :idle do
        animate :idle
        on :jump, :jumping
      end
    end
  end
end
```

### Animation Auto-Binding

The `animate` directive in state machines auto-detects your animation system:

1. First checks for `@animator` (Animator instance)
2. Falls back to `@animations` hash

```ruby
# If using Animator:
@animator = Animator.new(@sprite, columns: 8)
@animator.add(:idle, frames: 0..3, fps: 6)

state_machine do
  state :idle do
    animate :idle  # Calls @animator.play(:idle)
  end
end

# If using @animations hash:
@animations = {
  idle: SpriteAnimation.new(@sprite, frames: 0..3, fps: 6, columns: 8)
}

state_machine do
  state :idle do
    animate :idle  # Plays @animations[:idle]
  end
end
```

## Color Normalization

All color parameters accept multiple formats:

| Format | Example |
|--------|---------|
| Symbol | `:red`, `:dark_gray`, `:transparent` |
| Hex string | `"#FF0000"`, `"#F00"`, `"#FF0000AA"` |
| Helper function | `rgb(255, 0, 0)`, `rgb(255, 0, 0, 128)` |
| Array | `[255, 0, 0]`, `[255, 0, 0, 128]` |

All formats are normalized internally. Alpha defaults to 255 (fully opaque) if omitted. RGB values are 0-255.

## Platform Behavior

### Native Builds (Windows/Linux/macOS)

- Files in `:assets` root map to `game/assets/`
- Files in `:data` root map to `game/data/`
- Writes persist immediately to disk
- Hot-reload is enabled

### Web Builds (WebAssembly)

- Files in `:assets` root are preloaded from the WASM package
- Files in `:data` root use IDBFS (IndexedDB file system)
- Writes automatically sync to browser storage
- Hot-reload is not available

## Constraints

These constraints are enforced by the engine:

- **Sprites require Transform2D** - Cannot create a sprite without one
- **File paths are validated** - Directory traversal (`..`) and absolute paths are rejected
- **Writing to `:assets` fails** - Asset root is read-only
- **One music track at a time** - Music streams; playing a new track stops the previous one
- **State machine requires animation system** - `animate` directive needs `@animator` or `@animations`
